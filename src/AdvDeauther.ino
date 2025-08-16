#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "debug.h"
#include "Scanner.h"

int16_t scanStatus = 0;

unsigned long previousTime, currentTime;
unsigned long startTime;
ScanSettings sts;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    #ifndef ENABLE_DEBUG
    Serial.begin(115200);
    #endif
    debug_init();

    // wifi.InitializeNetwork("X", "deauther");
    // wifi.APConnectCallback([] (const WiFiEventSoftAPModeStationConnected &evt) -> void {
    //   Serial.printf_P("Device Connected, MAC: %s\r\n", str::mac(evt.mac).c_str());
    // });
    delay(5000);

    Serial.println();
    sts.mode = ScanMode::DEEP;
    sts.target = ScanTarget::ACCESSPOINT;
    sts.hopInterval = 5000;
    sts.channels = C_ASIA; // exclude channel 14
    sts.channels = C1 | C2 | C3 | C11;
    sts.timeout = (sts.hopInterval * sys::count_channels(sts.channels)); // give each channel 5 seconds;

    Serial.printf("Total Scan Channels : %d\n", sys::count_channels(sts.channels));
    Serial.println(str::channels(sts.channels));

    scanner.StartScan(sts);
    startTime = previousTime = millis();
}

void loop() {
  // wifi.Update();
  currentTime = millis();
  if(scanner.SearchRunning())
  {
      scanner.Update();
      if(currentTime - previousTime > 1000)
      {
        Serial.println("Scanning: " + String(scanner.ProgressPercentage()) + '%');
        previousTime = currentTime;
      }
  }
  else if(scanner.Available())
    {
      auto &list = scanner.FoundNetworks();
      Serial.printf_P(PSTR("Scan Completed! Found %d Networks\n"), list.size());
      yield();

      int entry = 1;
      for(const auto &net : list) {
        Serial.printf("Entry %d of %d\n", entry++, scanner.FoundNetworks().size());
        Serial.println("Network SSID: " + net.GetSSID());
        Serial.print("Network's RSSI: ");
        Serial.println(net.rssi);
        Serial.println("Network's Mac: " + str::mac(net.bssid));
        Serial.print("Network's Channel: ");
        Serial.println(net.channel);
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
    }
    else
    {
      if(currentTime - startTime >= sts.timeout)
      {
        Serial.printf_P("Free Heap Space = %d\r\n", ESP.getFreeHeap());
        Serial.println("Going to deep sleep, reset to restart");
        ESP.deepSleep(0);
      }
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
