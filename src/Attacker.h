#pragma once

#include <vector>

#include "common.h"
#include "mac.h"

#include "WifiManager.h"
#include "Scanner.h"

// ========== DEAUTH PACKET ========== //
#define DEAUTH_PACKET_SIZE 26
#define DEAUTH_PACKET_RECEIVER_OFFSET 4
#define DEAUTH_PACKET_SENDER_OFFSET (DEAUTH_PACKET_RECEIVER_OFFSET + 6)
#define DEAUTH_PACKET_BSSID_OFFSET (DEAUTH_PACKET_SENDER_OFFSET + 6)

#define DEAUTH_PACKET_BYTES \
        0xC0, 0x00, \
        0x00, 0x00, \
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, \
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, \
        0x00, 0x00, \
        0x01, 0x00 \

// Thanks to SpaceHuhn //
const static byte deauth_pkt[DEAUTH_PACKET_SIZE] PROGMEM = {
    /*  0 - 1  */ 0xC0, 0x00,                         // Type, subtype: c0 => deauth, a0 => disassociate
    /*  2 - 3  */ 0x00, 0x00,                         // Duration (handled by the SDK)
    /*  4 - 9  */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Reciever MAC (To)
    /* 10 - 15 */ 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, // Source MAC (From)
    /* 16 - 21 */ 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, // BSSID MAC (From)
    /* 22 - 23 */ 0x00, 0x00,                         // Fragment & squence number
    /* 24 - 25 */ 0x01, 0x00                          // Reason code (1 = unspecified reason)
};

struct Target
{
    Target(const NetworkInfo &network);
    Target(const StationInfo &station);

    mac::Array sender;
    mac::Array receiver;
    byte channel;
};

struct AttackSettings
{
    byte type; // attack type placeholder
    std::uint32_t timeout;
    std::vector<Target> targets;

    ChannelMask AllTargetChannels()
    {
        ChannelMask ret = C_NONE;
        for(const auto &target : targets)
            ret |= static_cast<ChannelMask>(target.channel); // 1 to 1 correspondence

        /*
        for(const auto &target : targets)
        {
            ret = C_SET << target.channel;
        }
        */
        return ret;
    }
};

class Attacker
{
public:
    Attacker() = default;
    void Start(const AttackSettings&);
    void Update();
    void Stop();

    bool Running() const { return active; }

private:
    bool active = false;
    AttackSettings settings;

    std::uint32_t startTime = 0, elapsedTime = 0;

    byte deauthpkt[DEAUTH_PACKET_SIZE] = {
        DEAUTH_PACKET_BYTES
    };

private:
    struct SeqCtrl
    {
        std::uint16_t /* frame_control */ : 4;
        std::uint16_t sequence_number : 12;
    } /* UNUSED */;

private:
    void DeauthTarget(const Target &target);
    static void PacketCallback(uint8_t status)
    {
        debugln(String(F("[Attacker] Packet Send ")) + String(status == 0 ? F("Succeded") : F("Failed")));
    }
};

extern Attacker attacker;