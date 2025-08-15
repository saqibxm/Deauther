#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "debug.h"
#include "Scanner.h"

int16_t scanStatus = 0;

unsigned long previousTime, currentTime;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    #ifndef ENABLE_DEBUG
    Serial.begin(115200);
    #endif
    debug_init();

    WiFi.begin();
    delay(5000);

    Serial.println();
    ScanSettings sts;
    sts.mode = ScanMode::DEEP;
    sts.target = ScanTarget::BOTH;
    sts.hopInterval = 5000;
    sts.channels = C_ASIA; // exclude channel 14
    sts.channels = C1 | C2 | C3 | C11;
    sts.timeout = (sts.hopInterval * sys::count_channels(sts.channels)); // give each channel 5 seconds;

    Serial.printf("Total Scan Channels : %d\n", sys::count_channels(sts.channels));

    scanner.StartScan(sts);
    previousTime = millis();
    Serial.println(F("Scan Started!"));
}

void loop() {
  // wifi.Update();
  if(scanner.SearchRunning())
  {
      scanner.Update();
      currentTime = millis();
      if(currentTime - previousTime > 1000)
      {
        Serial.println("Scanning: " + String(scanner.ProgressPercentage()) + '%');
        previousTime = currentTime;
      }
  }
  else if(scanner.Available())
    {
      auto &list = scanner.FoundNetworks();
      Serial.printf("Scan Completed! Found %d Networks\n", list.size());
      yield();

      int entry = 1;
      for(const auto &curr : list) {
        Serial.printf("Entry %d of %d\n", entry++, list.size());
        Serial.println("Network SSID: " + curr.GetSSID());
        Serial.print("Network's RSSI: ");
        Serial.println(curr.rssi);
        Serial.println("Network's Mac: " + str::mac(curr.bssid));
        Serial.print("Network's Channel: ");
        Serial.println(curr.channel);
        Serial.println();
      }

      entry = 1;
      Serial.println("Stations found are:");
      for(const auto &sta : scanner.FoundStations()) {
        Serial.printf("Station %d of %d\n", entry++, scanner.FoundStations().size());
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
