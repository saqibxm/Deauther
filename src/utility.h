#pragma once

/*
   Copyright (c) 2020 Stefan Kremser (@Spacehuhn)
   This software is licensed under the MIT License. See the license file for details.
   Source: github.com/spacehuhn/esp8266_deauther
*/

#include <Arduino.h> // String
#include <cstddef> // size_t
#include <cstdint> // uint8_t
#include "common.h" // all thing common
#include "debug.h"

extern "C" {
    #include "user_interface.h"
    typedef void (* freedom_outside_cb_t)(uint8 status);
    int wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);
    void wifi_unregister_send_pkt_freedom_cb(void);
    int wifi_send_pkt_freedom(uint8* buf, int len, bool sys_seq);
}

namespace sys {
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    inline void channel(byte ch) {
        debugF("[sys] Set channel ");
        debugln(String(ch));

        if (wifi_get_channel() == ch)
        {
            debuglnF("[sys] Channel Already Set");
            return;
        }

        wifi_set_channel(ch);
    }

    inline bool send(byte ch, byte* buf, std::uint16_t len) {
        sys::channel(ch);
        debuglnF("[sys] Send packet");
        return wifi_send_pkt_freedom(buf, len, 0) == 0;
    }

    inline byte count_channels(ChannelMask channels) noexcept {
        return __builtin_popcount(static_cast<unsigned int>(channels));
        // if(channels & Channels::C_ALL) return MAX_CHANNEL;
        // if(channels | Channels::C_NONE) return MIN_CHANNEL;

        /*
        byte count = 0;
        for (byte i = MIN_CHANNEL; i <= MAX_CHANNEL; ++i) {
            count += ((channels >> i) & C_SET);
        }

        return count;
        */
    }

    // byte next_channel(ChannelMask channels) noexcept {
    //     return next_channel(channels, wifi_get_channel());
    // }

    inline byte next_channel(ChannelMask channels, byte current = wifi_get_channel()) noexcept
    {
        byte next = __builtin_ffs(static_cast<int>(channels >> (current + 1))) + current;

        if(next == current && channels != C_NONE)
            next = __builtin_ctz(static_cast<unsigned int>(channels));

        debugF("[sys] Next Channel "); debugln(String(next));

        return next;

        /*
        if (((channels) == C_NONE) ||
            (((channels >> (current)) & C_SET) && ((channels & ~(ChannelMask{1} << (current))) == C_NONE)))
            return current;

        do {
            if (++current > MAX_CHANNEL) current = MIN_CHANNEL;
        } while (!((channels >> current) & C_SET));

        debugF("[sys] Next Channel "); debugln(String(current));
        return current;
        */
    }

    inline byte starting_channel(ChannelMask channels) noexcept
    {
        if(channels == C_NONE) return C_NONE;
        byte next = __builtin_ctz(static_cast<unsigned int>(channels));
        debugF("[sys] Starting Channel "); debug(String(next));
        return next;
        /*
        for (byte i = MIN_CHANNEL; i <= MAX_CHANNEL; ++i) {
            if((channels >> i) & C_SET)
            {
                debugF("[sys] Starting Channel "); debug(String(i));
                return i;
            }
        }
        return C_NONE;
        */
    }

    inline void channel_hop_next(ChannelMask channels) noexcept {
        sys::channel(next_channel(channels));
    }

#ifdef __cplusplus
}
#endif // __cplusplus
}

namespace str {
    void hide_mac(bool mode);
    String whitespace(int len);
    String left(int len, String str);
    String right(int len, String str);
    String center(int len, String str);
    String mac(const byte* b, unsigned int len = 6);
    String escape(String str);
    String time(unsigned long time);
    String channels(ChannelMask reg);
    String boolean(bool value);
}


/*
// JUNK
void channel(Channel ch) ICACHE_RAM_ATTR;
    bool send(Channel ch, byte* buf, std::uint16_t len) ICACHE_RAM_ATTR;

    byte count_channels(ChannelMask channels) noexcept ICACHE_RAM_ATTR;
    // byte next_channel(ChannelMask channels) noexcept;

    // byte next_channel(ChannelMask channels, byte current = wifi_get_channel()) noexcept;
    byte next_channel(ChannelMask channels, byte current = 0) noexcept ICACHE_RAM_ATTR;
    byte starting_channel(ChannelMask channels) noexcept ICACHE_RAM_ATTR;

    void channel_hop_next(ChannelMask channels) noexcept ICACHE_RAM_ATTR;
*/