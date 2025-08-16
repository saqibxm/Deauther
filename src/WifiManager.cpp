#include "WifiManager.h"

#include "debug.h"

// WiFiManager* WiFiManager::instance_ = nullptr;
WiFiManager wifi;

void enforce_packet_send_delay(uint8_t status)
{
    wifi.packetOptions.canSend = false;
    wifi.packetOptions.lastSentMs = millis();
}

WiFiManager::WiFiManager()
: mode(WManMode::WM_IDLE) // , working(false)
, channel(DEFAULT_NETWORK_CHANNEL)
, paused(false)
, promiscuousModeActive(false), callback(nullptr)
, packetOptions({true, 0, 0})
// , packetSendDelay(0), canSendPacket(true)
// , staConnectedCb(nullptr)
{
    debuglnF("[WiFiManager] Initialized");
    ChangeChannel(channel);
    WiFi.begin();
    WiFi.persistent(false);

    delay(200);
    Mode(WManMode::WM_IDLE);
}

WiFiManager::~WiFiManager()
{
    Stop();
    debuglnF("[WiFiManager] Shutdown");
}
/* void WiFiManager::ConfigureAP(bool hidden, byte channel, IPAddress ip)
{
    WiFiAPSettings settings;
    settings.address = ip;
    settings.hidden = hidden;
    settings.channel = channel;
} */

void WiFiManager::InitializeNetwork(const String &ssid, const String &paswd, byte channel, bool hidden, byte maxConnections)
{
    WiFi.disconnect();
    debuglnF("[WiFiManager] Disconnected");

    Mode(CurrentMode() == WM_CLIENT ? WM_DUAL : WM_SERVER);
    
    WiFi.softAP(ssid, paswd, channel, hidden, maxConnections);

    this->channel = channel;
}

void WiFiManager::DisposeNetwork()
{
    auto current = mode;
    if(current != WManMode::WM_SERVER || current != WManMode::WM_DUAL)
    {
        debuglnF("[WiFiManager] Dispose AP not Active!");
        return;
    }
    if(current == WManMode::WM_DUAL)
    {
        WiFi.softAPdisconnect(false);
        Mode(WManMode::WM_CLIENT);
    }
    else
    {
        WiFi.softAPdisconnect(true);
        mode = WManMode::WM_IDLE;
    }
}

void WiFiManager::InitializeStation(const String &ssid, const String &paswd, bool autoConnect)
{
    WiFi.softAPdisconnect();
    debuglnF("[WiFiManager] Disconnected Stations from AP");
    Mode(CurrentMode() == WManMode::WM_SERVER ? WManMode::WM_DUAL : WManMode::WM_CLIENT);
    
    WiFi.begin(ssid, paswd);
    WiFi.setAutoReconnect(autoConnect);
    WiFi.setAutoConnect(autoConnect);

    debuglnF("[WiFiManager] AP Online");
}

void WiFiManager::SwitchSTConnection(bool connect)
{
    if(CurrentMode() != WManMode::WM_CLIENT || CurrentMode() != WManMode::WM_CLIENT)
        return;
    
    if(connect)
    {
        if(!WiFi.isConnected())
            WiFi.setAutoConnect(true);
    }
    else
    {
        WiFi.disconnect();
        WiFi.setAutoConnect(false);
    }
}

void WiFiManager::DisposeStation()
{
    WiFi.disconnect(true);
    Mode(WManMode::WM_IDLE);
    debuglnF("[WiFiManager] AP Offline");
}

void WiFiManager::Update()
{
    yield();

    /* // Not yet Enabled
    auto currentMs = millis();
    if(packetOptions.sendDelay != 0 && ((currentMs - packetOptions.lastSentMs) >= packetOptions.sendDelay)) // enabled
    {
        packetOptions.canSend = true;
    }
    */
}

void WiFiManager::Pause()
{
    // WiFi.mode(WIFI_SHUTDOWN);
    debuglnF("[WiFiManager] Pause Not Implemented");
}

void WiFiManager::Resume()
{
    // if(WiFi.getMode() != WIFI_SHUTDOWN) return;
    // WiFi.mode(WIFI_RESUME);
    debuglnF("[WiFiManager] Resume Not Implemented");
}

void WiFiManager::Stop()
{
    StopPromiscuous();
    DisposeNetwork();
    DisposeStation();
    WiFi.mode(WIFI_OFF);
    delay(500);
    debuglnF("[WiFiManager] Stopped");
}

bool WiFiManager::StartPromiscuous(PromiscuousCallback cb)
{
    if(promiscuousModeActive || !cb) return false;
    callback = cb;

    WiFi.disconnect();

    // Mode(WManMode::WM_CLIENT); // do not touch current local mode
    WiFi.mode(WIFI_STA);
    delay(250);

    wifi_set_opmode(STATION_MODE);

    wifi_promiscuous_enable(false);
    ChangeChannel(channel);
    wifi_set_promiscuous_rx_cb(callback);
    wifi_promiscuous_enable(true);

    promiscuousModeActive = true;
    debuglnF("[WiFiManager] Promiscuous Mode Enabled");
    return true;
}

void WiFiManager::StopPromiscuous()
{
    if(!promiscuousModeActive) return;
    delay(100);

    wifi_promiscuous_enable(false);
    debuglnF("[WiFiManager] Promiscuous Mode Disabled");
    promiscuousModeActive = false;

    ChangeChannel(channel);
    delay(250); // let it rest a bit
    WiFi.mode(static_cast<WiFiMode_t>(mode)); // restore last mode
    // debuglnF("Promiscuous Disabled");
}

void WiFiManager::Mode(WManMode m)
{
    mode = m;
    WiFi.mode(static_cast<WiFiMode_t>(mode));

    char modeStr[7];
    strcpy_P(modeStr, mode == WManMode::WM_CLIENT ? PSTR("Client") : mode == WManMode::WM_SERVER ? PSTR("Server") : PSTR("OFF"));
    debugfP("[WiFiManager] Mode Changed: %s\r\n", modeStr);
}

/*
void WiFiManager::HandleChannelHop(unsigned long currentMs)
{
    if(promiscuousModeActive && hopInterval != 0)
    {
        if((currentMs - lastHopMs) >= hopInterval)
        {
            lastHopMs = currentMs;
            CycleNextChannel();
        }
    }
}

void WiFiManager::ChannelHopInterval(std::uint16_t interval, ChannelMask cycle)
{
    if(interval != 0 && interval < CHANNEL_HOP_INTERVAL_DEFAULT) interval = CHANNEL_HOP_INTERVAL_DEFAULT;
    if(sys::count_channels(cycle) < 1)
    {
        // immediately change channel and disable hoping
        ChangeChannel(channel = sys::starting_channel(channels));
        interval = 0;
    } // disable hopping

    channels = cycle;
    hopInterval = interval;
}
*/

bool WiFiManager::SendArbitraryPacket(const byte *buffer, std::uint16_t length)
{
    debugf("[WiFiManager] Packet Sent, Length: %d\n", length);
    #ifdef ENABLE_DEBUG
        if(length <= 24 || length >= 1400)
            debuglnF("[Arbitary Packet] Invalid Packet Length");
    #endif // ENABLE_DEBUG
    
    return wifi_send_pkt_freedom(const_cast<byte*>(buffer), length, 0) == 0;
}
