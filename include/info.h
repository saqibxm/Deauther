#pragma once

#include <Arduino.h>
#include <cstdint>

#include "mac.h"

struct NetworkInfo {
    // char *ssid = nullptr;
    String ssid;
    byte bssid[6] = {};
    byte channel = 0;
    std::int16_t rssi = 0;
    byte encryption = 0;
    bool hidden = false;
    std::uint8_t clientCount = 0; // max 255 clients support atm
    unsigned long lastSeen = 0;
    
    /*
    NetworkInfo() : channel(0), rssi(0), encryption(0), hidden(false), 
                   clientCount(0), lastSeen(0) {
        memset(bssid, 0, 6);
    }
    */
   NetworkInfo() = default;
   NetworkInfo(const String& name, const byte* bssid, byte ch, std::int16_t rssi, byte enc, bool hidden, std::uint8_t clients, unsigned long lastseen)
   : ssid(name), channel(ch), rssi(rssi), hidden(hidden), clientCount(clients), lastSeen(lastseen)
   {
        // SetSSID(name);
        if (bssid) memcpy(this->bssid, bssid, sizeof(this->bssid));
   }

   void SetSSID(const char *const name) noexcept
   {
    ssid = String(name);
    /*
        if (name)
        {
            size_t ssidlen = strlen(name);

            if (ssidlen > 0) {
                if (ssidlen > 32) ssidlen = 32;

                this->ssid = new char[ssidlen + 1];
                memcpy(this->ssid, name, ssidlen);
                this->ssid[ssidlen] = '\0';
            }
        }
    */
    }

    String GetSSID() const
    {
        return ssid;
        String name;
        if (!ssid)
            name = F("<NOT SET>");
        else
        {
            if(hidden) name = F("<HIDDEN>");
            else name = ssid;
        }
        return name;
    }
};

// Station information structure
struct StationInfo {
    byte mac[6] = {};
    byte ap[6] = {};
    byte channel = 0;
    std::int8_t rssi = 0; // -127 to 0
    std::uint32_t packets = 0;
    unsigned long lastSeen = 0;
    
    /*
    StationInfo() : rssi(0), channel(0), lastSeen(0), packets(0) {
        memset(mac, 0, 6);
        memset(ap, 0, 6);
    }
    */
};