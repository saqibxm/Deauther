#ifndef SCAN_HEADER
#define SCAN_HEADER

#include <ESP8266WiFi.h>
#include <vector>
#include "LinkedList.h"

#include "WifiManager.h"
#include "Singleton.h"
#include "AccessPoint.h"

#include "common.h"
#include "config.h"
// #include <ArduinoJson.h>

enum class ScanMode : byte
{
    QUICK, // Supports only AccessPoint Scan
    DEEP, // Both AP and ST modes supported
    NONE
};

enum class ScanTarget : byte
{
    ACCESSPOINT,
    STATION,
    BOTH
};

struct ScanSettings
{
    /*
    ScanSettings(const ScanTarget &scanTarget = ScanTarget::ACCESSPOINT, const ScanMode &scanMode, std::uint32_t scanTimeout = 15000,
    ChannelMask filterChannels = C_ALL, bool cleanResult = true, std::uint16_t chHopInteral = CHANNEL_HOP_INTERVAL_DEFAULT)
        : target(scanTarget), mode(scanMode), timeout(scanTimeout), channels(filterChannels), clearList(cleanResult), hopInterval(chHopInteral)
    {}
    */

    ScanMode mode = ScanMode::QUICK;
    ScanTarget target = ScanTarget::ACCESSPOINT; // only ap mode in quick scan
    bool clearList = true;
    std::uint32_t timeout = DEFAULT_SCAN_TIMEOUT; // ineffective in quick scan
    ChannelMask channels = C_ALL;
    std::uint16_t hopInterval = CHANNEL_HOP_INTERVAL_DEFAULT;
    // std::uint16_t continueAfter; // restart scan after specified time
};

enum class ScanConclusion : byte
{
    NONE,
    SUCCESSFUL,
    FAILED
};

class Scanner// : public Singleton<Scanner>
{
    friend class Singleton<Scanner>;

public:
    template <typename T>
    using ListType = std::vector<T>; // none
    using NetworkListType = ListType<NetworkInfo>;
    using StationListType = ListType<StationInfo>;
    using NetworkFoundCallback = void(*)(NetworkInfo&);
    using StationFoundCallback = void(*)(StationInfo&);
    // using SearchProgressCallback = void(*)(???);

public:
    Scanner(); // = default; // only allow Singleton to instantiate
    void StartScan(const ScanSettings &settings);

    void ScanAPs();
    void ScanSTs();

    void StartQuickScan();
    void StartDeepScan();

    void Update();

    // void StopAPScan();
    // void StopSTScan();
    // void StopDeepScan();
    void Stop();

    void CallOnNetworkFound(NetworkFoundCallback cb);
    void CallOnStationFound(NetworkFoundCallback cb);

    bool Available() const; // if the results are avaialable and the search completed
    bool SearchRunning() const { return scanRunning; }

    NetworkListType& FoundNetworks() { return networks; }
    StationListType& FoundStations() { return stations; }

    static Scanner& instance() { return *instance_ptr(); }
    static Scanner* instance_ptr() { if(!instance_) instance_ = new Scanner; return instance_; }

private:
    static Scanner* instance_;

    WiFiManager &wifi = WiFiManager::instance();
    bool scanRunning = false;
    NetworkListType networks;
    StationListType stations;

    // ScanTarget mode = ScanTarget::NONE;
    ScanSettings settings;
    std::uint16_t found = 0;

    std::uint32_t startTime = 0;
    std::uint32_t elapsedTime = 0;
    std::uint32_t scanTimeout = 0;

    NetworkFoundCallback networkFoundCb = nullptr;
    StationFoundCallback stationFoundCb = nullptr;
    // ListType<Station> sts;

private:
    static void packet_callback(byte* buf, std::uint16_t len);
    void handle_packet(byte* buf, std::uint16_t len);
    void parse_beacon_frame(const byte* frame, size_t length, int16_t rssi);
    void parse_data_frame(const byte* frame, size_t length, int16_t rssi);
    void parse_probe_frame(const byte* frame, size_t length, int16_t rssi);
    bool add_network_overwrite(const NetworkInfo& network);
    bool add_station_overwrite(const StationInfo& station);
};

#endif // SCAN_HEADER

