#include "Scanner.h"
#include "FrameBuffer.h"
#include "mac.h"

#include "debug.h"

Scanner scanner; // global definition
// static FrameBuffer buffer; // buffer for holding packets, type is ring

struct DebugScanData {
    std::uint32_t packet_recv = 0, packets_proc = 0;
} debug_data;

/* SCANNER */
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

    auto currentMode = wifi.CurrentMode();
    if(currentMode == WManMode::WM_SERVER)
        wifi.Mode(WManMode::WM_DUAL);
    else
        wifi.Mode(WManMode::WM_CLIENT);

    // WiFi.mode(WIFI_STA);
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
            wifi.ChangeChannel(sys::next_channel(settings.channels, wifi.ActiveChannel()));
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
    // debugfP("[Debug] Total Packets Received: %d\r\nTotal Packets Processed: %d\r\n", debug_data.packet_recv, debug_data.packets_proc);
}

void Scanner::packet_callback(byte* buf, uint16_t len) {
    // debuglnF("[Scanner] Packet Callback Triggered");
    // debugf("[Scanner] Packet length is %d\n", len);
    // buffer.Push(buf, len, scanner.elapsedTime);
    scanner.handle_packet(buf, len);
}

void Scanner::handle_packet(byte* buf, std::uint16_t len) {
    if (len < 24) return; // Minimum 802.11 frame size, if not drop!

    auto* pkt = reinterpret_cast<wifi_promiscuous_pkt_t*>(buf);
    auto* frame = pkt->payload;
    auto* frame_ctrl = reinterpret_cast<wifi_header_frame_control_t*>(pkt->payload);
    signed rssi = pkt->rx_ctrl.rssi;
    // unsigned channel = pkt->rx_ctrl.channel;
    
    switch (frame_ctrl->type) {
        case wifi_promiscuous_pkt_type_t::WIFI_PKT_MGMT: // is a management frame
            if (frame_ctrl->subtype == wifi_mgmt_subtypes_t::BEACON
                && (settings.target == ScanTarget::ACCESSPOINT || settings.target == ScanTarget::BOTH))
            { // Beacon frame
                parse_beacon_frame(frame, len, rssi);
            }
            else if (frame_ctrl->subtype == wifi_mgmt_subtypes_t::PROBE_REQ)
            {
                // parse_probe_frame(frame, len, rssi);
            }
            break;
            
        case wifi_promiscuous_pkt_type_t::WIFI_PKT_DATA: // Data frames
            if(settings.target == ScanTarget::STATION || settings.target == ScanTarget::BOTH)
                parse_data_frame(frame, len, rssi);
            break;
        
        default:
            /* UNSUPPORTED FRAME TYPE */
            break;
    }
}

/*
void Scanner::handle_packet(byte* buf, std::uint16_t len) {
    if (len < 24) return; // Minimum 802.11 frame size, if not drop!

    auto* pkt = reinterpret_cast<wifi_promiscuous_pkt_t*>(buf);

    byte* frame = pkt->payload;
    signed rssi = pkt->rx_ctrl.rssi;
    // unsigned channel = pkt->rx_ctrl.channel;
    
    byte frameType = frame[0];
    byte frameSubType = (frameType & 0xF0) >> 4;
    frameType = (frameType & 0x0C) >> 2;
    
    switch (frameType) {
        case wifi_promiscuous_pkt_type_t::WIFI_PKT_MGMT: // is a management frame
            if (frameSubType == wifi_mgmt_subtypes_t::BEACON
                && (settings.target == ScanTarget::ACCESSPOINT || settings.target == ScanTarget::BOTH))
            { // Beacon frame
                parse_beacon_frame(frame, len, rssi);
            } else if (frameSubType == 0x04) {
                // parse_probe_frame(frame, len, rssi);
            }
            break;
            
        case wifi_promiscuous_pkt_type_t::WIFI_PKT_DATA: // Data frames
            if(settings.target == ScanTarget::STATION || settings.target == ScanTarget::BOTH)
                parse_data_frame(frame, len, rssi);
            break;
        
        default:
            break;
    }
}*/

void Scanner::parse_beacon_frame(const byte* buf, std::uint16_t length, std::int8_t rssi)
{
    struct BeaconFrame
    {
        wifi_ieee80211_mac_hdr_t header;
        wifi_mgmt_beacon_t footer;
    } ATTR_PACKED;

    NetworkInfo network;
    auto *beacon = reinterpret_cast<const BeaconFrame*>(buf);
    auto *header = &beacon->header;
    auto *frame = &beacon->footer; // automatic offset by sizeof wifi_iee80211 header
    // auto *header = reinterpret_cast<const wifi_ieee80211_mac_hdr_t*>(buf);
    // auto *frame = reinterpret_cast<const wifi_mgmt_beacon_t*>(buf + sizeof(wifi_ieee80211_mac_hdr_t));

    memcpy(network.bssid, header->addr2, 6);
    network.channel = wifi.ActiveChannel(); // do not parse data for further tags

    network.rssi = rssi;
    network.lastSeen = this->elapsedTime; // misleading name but elapsedTime is actually current time (TODO: Fix)
    
    sizeof(int);
    if(frame->tag_length > 0);
        
    switch (frame->tag_number) {
    case 0: // SSID
        if (frame->tag_length > 0 && frame->tag_length <= 32) {
            char ssid[33];
            memcpy(ssid, frame->tag_data, frame->tag_length);
            ssid[frame->tag_length] = '\0';
            network.ssid = ssid;
        } else {
            network.hidden = true;
            // memcpy_P(network.ssid, HIDDEN_NETWORK_MARKER, strlen_P(HIDDEN_NETWORK_MARKER));
            network.ssid = FPSTR(HIDDEN_NETWORK_MARKER);
        }
        break;
        
    case 3: // DS Parameter set (channel)
        if (frame->tag_length == 1) {
            network.channel = frame->tag_data[0];
        }
        break;
    }
    network.encryption = (frame->capability & 0x10) ? AUTH_WEP : AUTH_OPEN; // WEP bit
    
    if (add_network_overwrite(network)) {
        if (networkFoundCb) {
            networkFoundCb(network);
        }
    }
    debuglnF("[Scanner] Parsed Beacon");
}

/*
void Scanner::parse_beacon_frame(const byte* frame, size_t length, std::int8_t rssi) {
    if (length < 36) return;
    
    NetworkInfo network;
    
    // Extract BSSID (AP MAC address)
    // const byte *receiver = frame + 16, *sender = frame + 22;
    // if(mac::broadcast(receiver) || mac::broadcast(sender) || mac::multicast(receiver) || mac::multicast(sender)) return;
    memcpy(network.bssid, frame + 16, 6);
    
    // Extract channel from DS Parameter Set
    network.channel = wifi.ActiveChannel();
    
    network.rssi = rssi;
    network.lastSeen = millis();
    
    // Parse tagged parameters for SSID and encryption
    const byte* tag = frame + 36; // jump fixed parameters
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
                    network.ssid = ssid;
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
    network.encryption = (capability & 0x10) ? AUTH_WEP : AUTH_OPEN; // WEP bit
    
    if (add_network_overwrite(network)) {
        if (networkFoundCb) {
            networkFoundCb(network);
        }
    }
    debuglnF("[Scanner] Parsed Beacon");
}
*/

void Scanner::parse_data_frame(const uint8_t* buf, std::uint16_t length, std::int8_t rssi) {
    if (length < 24) return;
    
    StationInfo station;

    auto *frame = reinterpret_cast<const wifi_ieee80211_mac_hdr_t*>(buf);
    
    // Extract MAC addresses based on DS bits
    if (!frame->frame_ctrl.to_ds && !frame->frame_ctrl.from_ds) {
        // IBSS
        memcpy(station.mac, frame->addr2, 6);
        memcpy(station.ap, frame->addr1, 6);
    } else if (!frame->frame_ctrl.to_ds && frame->frame_ctrl.from_ds) {
        // From AP to Station
        memcpy(station.mac, frame->addr1, 6);
        memcpy(station.ap, frame->addr2, 6);
    } else if (frame->frame_ctrl.to_ds && !frame->frame_ctrl.from_ds) {
        // From Station to AP
        memcpy(station.mac, frame->addr2, 6);
        memcpy(station.ap, frame->addr1, 6);
    } else {
        // WDS - skip for now, not handled
        return;
    }
    
    station.rssi = rssi;
    station.channel = wifi.ActiveChannel();
    station.lastSeen = elapsedTime;
    station.packets++;
    
    if (add_station_overwrite(station)) {
        if (stationFoundCb) {
            stationFoundCb(station);
        }
    }
}

/*
void Scanner::parse_data_frame(const uint8_t* frame, size_t length, std::int8_t rssi) {
    if (length < 24) return;
    
    StationInfo station;
    
    byte toDS = (frame[1] & 0x01);
    byte fromDS = (frame[1] & 0x02) >> 1;
    
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
    station.channel = wifi.ActiveChannel();
    station.lastSeen = millis();
    station.packets++;
    
    if (add_station_overwrite(station)) {
        if (stationFoundCb) {
            stationFoundCb(station);
        }
    }
}*/

void Scanner::parse_probe_frame(const byte* frame, size_t length, std::int8_t rssi) {
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
