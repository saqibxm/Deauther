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

namespace sys {
    void channel(Channel ch);
    bool send(Channel ch, byte* buf, std::uint16_t len);

    byte count_channels(ChannelMask channels);
    byte next_channel(ChannelMask channels);

    void channel_hop_next(ChannelMask channels);
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