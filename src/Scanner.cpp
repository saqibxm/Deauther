#include "Scanner.h"

#include "debug.h"

Scanner scanner; // global definition

Scanner::Scanner()
{
    networks.reserve(10);
    stations.reserve(20);
}

void Scanner::StartScan(const ScanSettings &s)
{
    this->settings = s;
    if(settings.mode == ScanMode::NONE) return;

    scanTimeout = (settings.timeout <= 0 /* || settings.timeout >= MAX_SCAN_TIMEOUT */ ) ? DEFAULT_SCAN_TIMEOUT : settings.timeout;

    scanRunning = true;

    if(settings.clearList)
    {
        switch (settings.target)
        {
        case ScanTarget::BOTH:
            stations.clear();
            networks.clear();
            break;

        case ScanTarget::ACCESSPOINT:
            networks.clear();
            break;

        case ScanTarget::STATION:
            stations.clear();
            break;
        
        default:
            break;
        }
    }

    if(settings.mode == ScanMode::QUICK)
        StartQuickScan();
    else
    {
        scanTimeout = settings.timeout;
        StartDeepScan();
    }

    debuglnF("[Scanner] Scan Initiated");
}

void Scanner::StartQuickScan()
{
    // WiFiMode_t wifiMode = WiFi.getMode(); // for later restoration

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    WiFi.scanNetworks(/* async */ true, /* scan hidden */ true); // initiate asynchronous scan

    debuglnF("[Scanner] Quick Scan Started");
    startTime = millis();
}

void Scanner::StartDeepScan()
{
    if(!scanRunning) return;

    // wifi.Mode(WM_IDLE);
    
    if(sys::count_channels(channelList = settings.channels) > 1)
        hopInterval = settings.hopInterval < CHANNEL_HOP_INTERVAL_MIN ? CHANNEL_HOP_INTERVAL_DEFAULT : settings.hopInterval;
    else
        wifi.ChangeChannel(sys::starting_channel(settings.channels));

    // wifi.ChannelHopInterval(settings.hopInterval, settings.channels); // will automatically determine whether to enable hop or not

    if(!wifi.StartPromiscuous(packet_callback))
    {
        scanRunning = false;
        debuglnF("[Scanner] Deep Scan Couldn't Start");
    }
    else debuglnF("[Scanner] Deep Scan Started");

    startTime = lastHopTime = millis();
}

void Scanner::ScanSTs()
{
    debuglnF("[Scanner] Unimplemented Function Called");
    std::terminate(); // not implemented
}

void Scanner::Update()
{
    if(!scanRunning) return;

    yield();
    ESP.wdtFeed();

    elapsedTime = millis();

    if (settings.mode == ScanMode::QUICK)
    {
        debuglnF("[Scanner] Updating Quick Scan");

        std::int16_t sc = WiFi.scanComplete(); // return status of whether the scan completed
        if(sc == WIFI_SCAN_RUNNING) return;

        if(sc <= WIFI_SCAN_FAILED)
        {
            // nothing found on the radar
            debuglnF("[Scanner] Quick Scan Failed");
            Stop();
            return;
        }

        auto actual = static_cast<std::uint16_t>(sc);
        if(found != actual)
        {
            found = actual;
        }

        for(std::uint16_t i = 0; i < found; ++i)
        {
            byte channel = WiFi.channel(i);
            if((settings.channels & channel) == 0) continue; // exclude channel not listed

            bool hidden = WiFi.isHidden(i);
            String ssid = WiFi.SSID(i);          // copy the string
            byte* bssid = WiFi.BSSID(i);
            byte enc = WiFi.encryptionType(i);
            std::int32_t rssi = WiFi.RSSI(i);

            // WiFi.getNetworkInfo(...)
            networks.emplace_back(ssid, bssid, channel, rssi, enc, hidden, 0, 0);
        }

        Stop();
    }
    else if(settings.mode == ScanMode::DEEP)
    {
        debuglnF("[Scanner] Updating Deep Scan");
        if((elapsedTime - startTime) >= scanTimeout)
        {
            // delay(100);
            Stop();
            return;
        }
        if(hopInterval != 0 && (elapsedTime - lastHopTime) >= hopInterval)
        {
            wifi.ChangeChannel(sys::next_channel(settings.channels, wifi.CurrentNetworkChannel()));
            lastHopTime = elapsedTime;
        }
    }
    else return;
}

void Scanner::Stop()
{
    if(settings.mode == ScanMode::QUICK)
        WiFi.scanDelete();
    else
    {
        wifi.StopPromiscuous();
    }
    scanRunning = false;
    debuglnF("[Scanner] Stopped Scanning");

    elapsedTime = 0;
}

void Scanner::packet_callback(byte* buf, uint16_t len) {
    // debuglnF("[Scanner] Packet Callback Triggered");
    // debugf("[Scanner] Packet length is %d\n", len);
    scanner.handle_packet(buf, len);

    // if (instance_ != nullptr) {
    //     instance_ptr()->handle_packet(buf, len);
    // }
}

void Scanner::handle_packet(byte* buf, uint16_t len) {
    if (len < 24) return; // Minimum 802.11 frame size

    auto* pkt = reinterpret_cast<wifi_promiscuous_pkt_t*>(buf);
    byte* frame = pkt->payload;
    int rssi = pkt->rx_ctrl.rssi;
    
    byte frameType = frame[0];
    byte frameSubType = (frameType & 0xF0) >> 4;
    frameType = (frameType & 0x0C) >> 2;
    
    switch (frameType) {
        case 0x00: // Management frames
            if (frameSubType == 0x08) { // Beacon frame
                parse_beacon_frame(frame, len, rssi);
            } else if (frameSubType == 0x04) { // Probe request
                // parse_probe_frame(frame, len, rssi);
            }
            break;
            
        case 0x02: // Data frames
            parse_data_frame(frame, len, rssi);
            break;
    }
    // debuglnF("[Scanner] Packet Handled");
}

void Scanner::parse_beacon_frame(const byte* frame, size_t length, int16_t rssi) {
    if (length < 36) return;
    
    NetworkInfo network;
    
    // Extract BSSID (AP MAC address)
    memcpy(network.bssid, frame + 16, 6);
    
    // Extract channel from DS Parameter Set
    network.channel = wifi.CurrentNetworkChannel();
    
    network.rssi = rssi;
    network.lastSeen = millis();
    
    // Parse tagged parameters for SSID and encryption
    const byte* tag = frame + 36; // Skip fixed parameters
    size_t remaining = length - 36;
    
    while (remaining >= 2) {
        byte tagType = tag[0];
        byte tagLength = tag[1];
        
        if (tagLength > remaining - 2) break;
        
        switch (tagType) {
            case 0: // SSID
                if (tagLength > 0 && tagLength <= 32) {
                    // char ssid[33] = {0};
                    char ssid[33];
                    memcpy(ssid, tag + 2, tagLength);
                    ssid[tagLength] = '\0';
                    network.SetSSID(ssid); // needs optimisations
                } else {
                    network.hidden = true;
                    // memcpy_P(network.ssid, HIDDEN_NETWORK_MARKER, strlen_P(HIDDEN_NETWORK_MARKER));
                    network.ssid = FPSTR(HIDDEN_NETWORK_MARKER);
                }
                break;
                
            case 3: // DS Parameter set (channel)
                if (tagLength == 1) {
                    network.channel = tag[2];
                }
                break;
        }
        
        tag += tagLength + 2;
        remaining -= tagLength + 2;
    }
    
    // Determine encryption from capability info
    auto capability = *reinterpret_cast<const std::uint16_t*>(frame + 34);
    network.encryption = (capability & 0x10) ? 1 : 0; // WEP bit
    
    if (add_network_overwrite(network)) {
        if (networkFoundCb) {
            networkFoundCb(network);
        }
    }
    debuglnF("[Scanner] Parsed Beacon");
}

void Scanner::parse_data_frame(const uint8_t* frame, size_t length, int16_t rssi) {
    if (length < 24) return;
    
    StationInfo station;
    
    uint8_t toDS = (frame[1] & 0x01);
    uint8_t fromDS = (frame[1] & 0x02) >> 1;
    
    // Extract MAC addresses based on DS bits
    if (!toDS && !fromDS) {
        // IBSS
        memcpy(station.mac, frame + 10, 6);
        memcpy(station.ap, frame + 4, 6);
    } else if (!toDS && fromDS) {
        // From AP to Station
        memcpy(station.mac, frame + 4, 6);
        memcpy(station.ap, frame + 10, 6);
    } else if (toDS && !fromDS) {
        // From Station to AP
        memcpy(station.mac, frame + 10, 6);
        memcpy(station.ap, frame + 4, 6);
    } else {
        // WDS - skip for now
        return;
    }
    
    station.rssi = rssi;
    station.channel = wifi.CurrentNetworkChannel();
    station.lastSeen = millis();
    station.packets++;
    
    if (add_station_overwrite(station)) {
        if (stationFoundCb) {
            stationFoundCb(station);
        }
    }
}

void Scanner::parse_probe_frame(const byte* frame, size_t length, int16_t rssi) {
    // Similar to parseDataFrame but for probe requests
    // Implementation would extract station MAC from probe requests
}

bool Scanner::add_network_overwrite(const NetworkInfo& network) {
    // Find existing network
    for (auto& existing : networks) {
        if (memcmp(existing.bssid, network.bssid, 6) == 0) {
            // Update existing
            existing.rssi = network.rssi;
            existing.lastSeen = network.lastSeen;
            if (!network.hidden && existing.hidden) {
                // existing.SetSSID(network.ssid);
                existing.ssid = network.ssid;
                existing.hidden = false;
            }
            return false; // Updated existing
        }
    }
    
    // Add new network if we have space
    if (networks.size() < MAX_NETWORKS) {
        networks.push_back(network);
        debuglnF("[Scanner] Network Seen");
        return true; // Added new
    }
    
    return false;
}

bool Scanner::add_station_overwrite(const StationInfo& station) {
    // Find existing station
    for (auto& existing : stations) {
        if (memcmp(existing.mac, station.mac, 6) == 0) {
            // Update existing
            existing.rssi = station.rssi;
            existing.lastSeen = station.lastSeen;
            existing.packets++;
            return false; // Updated existing
        }
    }
    
    // Add new station if we have space
    if (stations.size() < MAX_STATIONS) {
        stations.push_back(station);
        debuglnF("[Scanner] Station Seen");
        return true; // Added new
    }
    
    return false;
}


/*
switch (settings.target)
        {
        case ScanTarget::STATION:
        {
            // if(clearFirst) sts.clear();
            ScanSTs();
        } break;
        case ScanTarget::ACCESSPOINT:
        {
            if(settings.clearList) networks.clear();
            ScanAPs();
        } break;
        case ScanTarget::BOTH:
        {
            if(settings.clearList)
            {
                networks.clear();
                // sts.clear();
            }
            ScanAPs();
            ScanSTs();
        } break;
        default:
            scanRunning = false;
            break;
        }
*/
