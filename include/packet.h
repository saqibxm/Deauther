#pragma once

#include <cstdint>

#include "config.h"

/* FRAME CONTROL SUBTYPES */
#define FT_ASSOC_REQ 0x00
#define FT_ASSIC_RES 0x01
#define FT_REASSOC_REQ 0x02
#define FT_REASSOC_RES 0x03
#define FT_PROBE_REQ 0x04
#define FT_PROBE_RES 0x05
#define FT_BEACON 0x08
#define FT_ATIM 0x09
#define FT_DISASSOC 0x0A
#define FT_AUTH 0x0B
#define FT_DEAUTH 0x0C
#define FT_ACTION 0x0D
#define FT_ACTNOACK 0x0E

/* TAGGED PARAMETERS */
#define TP_SSID 0

#define TP_CHANNEL 3

// Thanks to
// https://research.ivision.com/writing-a-simple-esp8266-based-sniffer.html

struct wifi_pkt_rx_ctrl_t
{
    signed rssi : 8;
    unsigned rate : 4;
    unsigned is_group : 1;
    unsigned : 1;
    unsigned sig_mode : 2;
    unsigned legacy_length : 12;
    unsigned damatch0 : 1;
    unsigned damatch1 : 1;
    unsigned bssidmatch0 : 1;
    unsigned bssidmatch1 : 1;
    unsigned mcs : 7;
    unsigned cwb : 1;
    unsigned ht_length : 16;
    unsigned smoothing : 1;
    unsigned not_sounding : 1;
    unsigned : 1;
    unsigned aggregation : 1;
    unsigned stbc : 2;
    unsigned fec_coding : 1;
    unsigned sgi : 1;
    unsigned rx_state : 8;
    unsigned ampdu_cnt : 8;
    unsigned channel : 4;
    unsigned : 12;
} ATTR_PACKED;

struct wifi_pkt_lenseq_t
{
    uint16_t length;
    uint16_t seq;
    uint8_t address3[6];
} ATTR_PACKED;

struct wifi_pkt_mgmt_t
{
    wifi_pkt_rx_ctrl_t rx_ctrl;
    uint8_t payload[112];
    uint16_t cnt;
    uint16_t len;
} ATTR_PACKED;

struct wifi_promiscuous_pkt_t
{
    wifi_pkt_rx_ctrl_t rx_ctrl;
    uint8_t payload[36];
    uint16_t cnt;
    wifi_pkt_lenseq_t lenseq[1];
} ATTR_PACKED;

struct wifi_header_frame_control_t
{
    unsigned protocol : 2;
    unsigned type : 2;
    unsigned subtype : 4;
    unsigned to_ds : 1;
    unsigned from_ds : 1;
    unsigned more_frag : 1;
    unsigned retry : 1;
    unsigned pwr_mgmt : 1;
    unsigned more_data : 1;
    unsigned wep : 1;
    unsigned strict : 1;
} ATTR_PACKED;

struct wifi_ieee80211_mac_hdr_t
{
    wifi_header_frame_control_t frame_ctrl;
    unsigned duration_id : 16;
    uint8_t addr1[6]; /* receiver address */
    uint8_t addr2[6]; /* sender address */
    uint8_t addr3[6]; /* filtering address */
    unsigned sequence_ctrl : 16;
    uint8_t addr4[0]; /* optional */
} ATTR_PACKED;

struct wifi_mgmt_beacon_t
{
    unsigned long long timestamp : 64;
    unsigned interval : 16;
    unsigned capability : 16;
    unsigned tag_number : 8;
    unsigned tag_length : 8;
    uint8_t tag_data[0]; // gcc extension
    // char ssid[0];
    // uint8 rates[1]; // not interested yet
} ATTR_PACKED;

struct wifi_deauth_pkt_t
{
    wifi_header_frame_control_t frame_ctrl;
    unsigned duration_id : 16;
    uint8_t receiver[6];
    uint8_t sender[6];
    uint8_t bssid[6];
    unsigned fragment_counter : 4;
    unsigned sequence_number : 12;
    unsigned reason : 8;
    unsigned : 8;
} ATTR_PACKED;

enum wifi_promiscuous_pkt_type_t
{
    WIFI_PKT_MGMT = 0x00,
    WIFI_PKT_CTRL = 0x01,
    WIFI_PKT_DATA = 0x02,
    WIFI_PKT_MISC = 0x04,
};

enum wifi_mgmt_subtypes_t
{
    ASSOCIATION_REQ,
    ASSOCIATION_RES,
    REASSOCIATION_REQ,
    REASSOCIATION_RES,
    PROBE_REQ,
    PROBE_RES,
    NU1, /* ......................*/
    NU2, /* 0110, 0111 not used */
    BEACON,
    ATIM,
    DISASSOCIATION,
    AUTHENTICATION,
    DEAUTHENTICATION,
    ACTION,
    ACTION_NACK,
};