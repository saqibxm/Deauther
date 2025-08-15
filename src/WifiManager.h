#pragma once

#include <ESP8266WiFi.h>
#include <functional>

#include "common.h"
#include "utility.h"

#include "Singleton.h"

enum WManMode : int8_t
{
    WM_IDLE = WIFI_OFF,
    WM_CLIENT = WIFI_STA,
    WM_SERVER = WIFI_AP,
    WM_DUAL = WIFI_AP_STA
}; // just wtf did I do again

struct WiFiAPSettings
{
    IPAddress address = DEFAULT_IP;
    String ssid = "";
    String pswd = "";
    bool hidden = false;
    byte channel = 1;
};

struct WiFiSTSettings
{
    String ssid;
    String pswd;
};

void enforce_packet_send_delay(uint8_t);

class WiFiManager // : public Singleton<WiFiManager>
{
    friend void enforce_packet_send_delay(uint8_t);
public:
    using PromiscuousCallback = wifi_promiscuous_cb_t;
    
    template <typename Event>
    using EventCallback = std::function<void(const Event&)>;

public:
    WiFiManager();
    ~WiFiManager();
    // void Initialize();

    // void ConfigureAP(bool hidden, byte channel = 1/*uncommon_channel()*/, IPAddress ip = DEFAULT_IP);
    void CreateAP(const String& ssid, const String& pass = String(), byte channel = 1, bool hidden = false, byte maxConnections = 4);
    void APConnectCallback(const EventCallback<WiFiEventSoftAPModeStationConnected> &cb) {
        WiFi.onSoftAPModeStationConnected(cb);
    }
    void DisposeAP();

    void CreateST(const String &st_ssid, const String &st_pswd = String(), bool autoConnect = true);
    void SwitchSTConnection(bool connect = true);
    void STConnectCallback(const EventCallback<WiFiEventStationModeConnected> &cb) {
        WiFi.onStationModeConnected(cb);
    }
    void DisposeST();

    void Update(); // ?
    void Stop();

    void Pause(); // should save the state of the config
    void Resume(); // resume with the same config

    WManMode CurrentMode() const { return mode; }
    void Mode(WManMode mode); // { this->mode = mode; WiFi.mode(static_cast<WiFiMode_t>(mode)); }

    bool StartPromiscuous(PromiscuousCallback cb);
    void StopPromiscuous();

    // void SwitchChannelHop(bool enable) { if(hopInterval == 0) enable = false; hopingChannels = enable; }
    //void ChannelHopInterval(std::uint16_t interval = CHANNEL_HOP_INTERVAL_DEFAULT) noexcept { if(interval < CHANNEL_HOP_INTERVAL_MIN) return; hopInterval = interval; }
    // void ChannelHopInterval(std::uint16_t interval = CHANNEL_HOP_INTERVAL_DEFAULT, ChannelMask cycle = C_ALL) noexcept;
    // void HandleChannelHop(unsigned long currentMs = millis()) noexcept;

    bool SendArbitraryPacket(const byte *buffer, std::uint16_t length);
    void SwitchPacketSendDelay(std::uint16_t delayMs = 0) {
        if(delayMs != 0) // if 0 then disables
        {
            wifi_register_send_pkt_freedom_cb(enforce_packet_send_delay);
        }
    }

    void ChangeChannel(byte ch) { sys::channel(ch);/* yield(); delay(100); */ }
    byte CurrentChannel() const { return channel; }
    byte CurrentNetworkChannel() const { return wifi_get_channel(); }
    void CycleNextChannel(ChannelMask channels) { sys::channel_hop_next(channels); }

    ESP8266WiFiClass& LLManager();

private:
    // ESP8266WiFiClass &wifi = WiFi; // wtf did I do
    // bool working;
    WManMode mode;
    byte channel;
    bool paused;

    bool promiscuousModeActive;
    PromiscuousCallback callback;

    struct PacketOptionFields {
        // to control rate at which packets are sent
        std::uint64_t canSend : 1;
        std::uint64_t sendDelay : 15;
        std::uint64_t lastSentMs : 48;
    } packetOptions;

    // static freedom_outside_cb_t sPacketSentCb;

    // std::uint16_t packetSendDelay; 
    // bool canSendPacket;
    // WiFiEventSoftAPModeStationConnected staConnectedCb;
};

extern WiFiManager wifi;

// std::function<typename std::remove_pointer<PromiscuousCallback>::type> callback;