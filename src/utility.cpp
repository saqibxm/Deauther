/*
   Copyright (c) 2020 Stefan Kremser (@Spacehuhn)
   This software is licensed under the MIT License. See the license file for details.
   Source: github.com/spacehuhn/esp8266_deauther
 */

extern "C" {
    #include "user_interface.h"
    typedef void (* freedom_outside_cb_t)(uint8 status);
    int wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);
    void wifi_unregister_send_pkt_freedom_cb(void);
    int wifi_send_pkt_freedom(uint8* buf, int len, bool sys_seq);
}

#include "config.h"

#include "utility.h"
#include "debug.h"

namespace str {
    bool hidden { false };

    void hide_mac(bool mode) {
        hidden = mode;
    }

    String whitespace(int len) {
        String res;

        res.reserve(len);

        while (len > 0) {
            res += ' ';
            --len;
        }

        return res;
    }

    String left(int len, String str) {
        int spaces = len - str.length();

        while (spaces > 0) {
            str = str + ' ';
            --spaces;
        }

        str = str.substring(0, len);

        return str;
    }

    String right(int len, String str) {
        int spaces = len - str.length();

        while (spaces > 0) {
            str = ' ' + str;
            --spaces;
        }

        str = str.substring(0, len);

        return str;
    }

    String center(int len, String str) {
        int spaces = len - str.length();

        for (int i = 0; i < spaces; i += 2) {
            str = ' ' + str + ' ';
        }

        str = str.substring(0, len);

        return str;
    }

    String mac(const byte* b, unsigned int len) {
        len = len - (len % 3);
        String str;

        for (decltype(len) i = 0; i < len; i++) {
            if (i>0) str += ':';
            if (hidden && (i>=2) && (i<=4)) {
                str += '*';
                str += '*';
            } else {
                if (b[i] < 0x10) str += '0';
                str += String(b[i], HEX);
            }
        }

        return str;
    }

    String escape(String str) {
        if ((str == String('"')) || !str.startsWith("\"") || !str.endsWith("\"")) {
            str.replace("\\", "\\\\");
            str.replace("\"", "\\\"");
            str = '"' + str + '"';
        }
        return str;
    }

    String time(unsigned long time) {
        if (time == 0) return "0ms";

        String str;

        unsigned long second_ms = 1000;
        unsigned long minute_ms = 60*second_ms;
        unsigned long hour_ms   = 60*minute_ms;

        if (time >= hour_ms) {
            unsigned long hours = time/hour_ms;
            time -= hours*hour_ms;
            str  += String(hours)+"h+";
        }

        if (time >= minute_ms) {
            unsigned long minutes = time/minute_ms;
            time -= minutes*minute_ms;
            str  += String(minutes)+"min+";
        }

        if (time >= second_ms) {
            unsigned long seconds = time/second_ms;
            time -= seconds*second_ms;
            str  += String(seconds)+"s+";
        }

        if (time > 0) {
            str += String(time)+"ms+";
        }

        str.remove(str.length()-1);

        return str;
    }

    String channels(ChannelMask reg) {
        String str;

        for (uint8_t i = MIN_CHANNEL; i <= MAX_CHANNEL; ++i) {
            if ((reg >> (i)) & C_SET) {
                str += String(i+1);
                str += String(',');
            }
        }
        return str;
    }

    String boolean(bool value) {
        if (value) {
            return String(F("True"));
        } else {
            return String(F("False"));
        }
    }
}

namespace sys {
    void channel(byte ch) {
        if (wifi_get_channel() != ch) {
            wifi_set_channel(ch);
            debugF("[sys] Set channel ");
            debugln(String(ch));
        }
    }

    bool send(byte ch, byte* buf, std::uint16_t len) {
        sys::channel(ch);
        debuglnF("[sys] Send packet");
        return wifi_send_pkt_freedom(buf, len, 0) == 0;
    }

    byte count_channels(ChannelMask channels) {
        // if(channels & Channels::C_ALL) return MAX_CHANNEL;
        // if(channels | Channels::C_NONE) return MIN_CHANNEL;

        byte count = 0;
        for (byte i = MIN_CHANNEL; i <= MAX_CHANNEL; ++i) {
            count += ((channels >> i) & C_SET);
        }

        return count;
    }

    byte next_channel(ChannelMask channels) {
        byte current = wifi_get_channel();

        // If no channels in register
        // Or the only channel that is, is already set

        if (((channels) == C_NONE) ||
            (((channels >> (current)) & C_SET) && ((channels & ~(ChannelMask{1}<< (current))) == C_NONE)))
            return current;

        do {
            if (++current > MAX_CHANNEL) current = MIN_CHANNEL;
        } while (!((channels >> current) & C_SET));

        // debugF("[sys] Get next channel ");
        // debugln(String(ch));

        return current;
    }

    void channel_hop_next(ChannelMask channels) {
        sys::channel(next_channel(channels));
    }
}