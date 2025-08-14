#ifndef COMMON_HEADER
#define COMMON_HEADER

#include <cstdint>
#include <Arduino.h>
#include "config.h"

using Channel = byte;
using ChannelMask = std::uint16_t;

using MacUnit = byte;

struct NetworkInfo {
    char *ssid = nullptr;
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
   NetworkInfo(const char* name, const byte* bssid, byte ch, std::int16_t rssi, byte enc, bool hidden, std::uint8_t clients, unsigned long lastseen)
   {
        this->hidden = hidden;

        SetSSID(name);

        if (bssid) memcpy(this->bssid, bssid, sizeof(this->bssid));
        this->rssi = rssi;
        this->encryption = enc;
        this->channel = ch;
   }

   void SetSSID(const char *const name) noexcept
   {
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
    }

    String GetSSID() const
    {
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
    std::int16_t rssi = 0;
    std::uint32_t packets = 0;
    unsigned long lastSeen = 0;
    
    /*
    StationInfo() : rssi(0), channel(0), lastSeen(0), packets(0) {
        memset(mac, 0, 6);
        memset(ap, 0, 6);
    }
    */
};

enum Channels : ChannelMask
{
    C_NONE = 0,
    C_SET = (ChannelMask{1} << 0),
    C1 = (ChannelMask{1} << 1),
    C2 = (ChannelMask{1} << 2),
    C3 = (ChannelMask{1} << 3),
    C4 = (ChannelMask{1} << 4),
    C5 = (ChannelMask{1} << 5),
    C6 = (ChannelMask{1} << 6),
    C7 = (ChannelMask{1} << 7),
    C8 = (ChannelMask{1} << 8),
    C9 = (ChannelMask{1} << 9),
    C10 = (ChannelMask{1} << 10),
    C11 = (ChannelMask{1} << 11),
    C12 = (ChannelMask{1} << 12),
    C13 = (ChannelMask{1} << 13),
    C14 = (ChannelMask{1} << 14),
    C_ALL = C1 | C2 | C3 | C4 | C5 | C6 | C7 | C8 | C9 | C10 | C11 | C12 | C13 | C14,
    C_ASIA = C_ALL & ~C14,
};

struct wifi_pkt_rx_ctrl_t {
    signed   rssi          : 8;
    unsigned rate          : 4;
    unsigned is_group      : 1;
    unsigned               : 1;
    unsigned sig_mode      : 2;
    unsigned legacy_length : 12;
    unsigned damatch0      : 1;
    unsigned damatch1      : 1;
    unsigned bssidmatch0   : 1;
    unsigned bssidmatch1   : 1;
    unsigned mcs           : 7;
    unsigned cwb           : 1;
    unsigned ht_length     : 16;
    unsigned smoothing     : 1;
    unsigned not_sounding  : 1;
    unsigned               : 1;
    unsigned aggregation   : 1;
    unsigned stbc          : 2;
    unsigned fec_coding    : 1;
    unsigned sgi           : 1;
    unsigned rx_state      : 8;
    unsigned ampdu_cnt     : 8;
    unsigned channel       : 4;
    unsigned               : 12;
};

struct wifi_pkt_lenseq_t {
    uint16_t length;
    uint16_t seq;
    uint8_t  address3[6];
};

struct wifi_pkt_mgmt_t {
    wifi_pkt_rx_ctrl_t rx_ctrl;
    uint8_t            payload[112];
    uint16_t           cnt;
    uint16_t           len;
};

struct wifi_promiscuous_pkt_t {
    wifi_pkt_rx_ctrl_t rx_ctrl;
    uint8_t            payload[36];
    uint16_t           cnt;
    wifi_pkt_lenseq_t  lenseq[1];
};

#endif // COMMON_HEADER