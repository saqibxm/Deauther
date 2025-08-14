#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "debug.h"
#include "Scanner.h"

Scanner &scanner = Scanner::instance();
WiFiManager& manager = WiFiManager::instance();

int16_t scanStatus = 0;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    WiFi.begin();
    delay(5000);

    Serial.println();
    ScanSettings sts;
    sts.mode = ScanMode::DEEP;
    sts.target = ScanTarget::BOTH;
    sts.hopInterval = 5000;
    sts.channels = C_ASIA; // exclude channel 14
    sts.timeout = 60 * 1000;

    scanner.StartScan(sts);
    Serial.println(F("Scan Started!"));
}

void loop() {
  manager.Update();
  if(scanner.SearchRunning())
      scanner.Update();
  else if(scanner.Available())
    {
      auto &list = scanner.FoundNetworks();
      Serial.printf("Scan Completed! Found %d Networks\n", list.size());
      yield();

      int entry = 1;
      for(const auto &curr : list) {
        Serial.printf("Entry %d of %d\n", entry++, list.size());
        Serial.println("Network SSID" + curr.GetSSID());
        Serial.print("Network's RSSI: ");
        Serial.println(curr.rssi);
        Serial.println("Network's Mac: " + str::mac(curr.bssid));
        Serial.println();
      }

      entry = 0;
      Serial.println("Stations found are:");
      for(const auto &sta : scanner.FoundStations()) {
        Serial.printf("Station %d of %d\n", entry++, list.size());
        Serial.println("Mac address: " + str::mac(sta.mac));
        Serial.print("Station's RSSI: ");
        Serial.println(sta.rssi);
        Serial.print("Station's Channel: ");
        Serial.println(sta.channel);
        Serial.println("Associated to: " + str::mac(sta.ap));
        Serial.println();
      }

      Serial.printf("Free Heap Space %d\n", ESP.getFreeHeap());
      Serial.println(F("Going into Deep Sleep, Reset!"));
      ESP.deepSleep(0);
    }
  else
  {
  }

    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
}

/*
list.begin();
      int entry = 1;
      while(list.available()) {
        AccessPoint *curr = list.iterate();
        Serial.printf("Entry %d of %d\n", entry++, list.size());
        Serial.println(curr->getSSIDString());
        Serial.println(curr->getRSSI());
        Serial.println();
      }
*/