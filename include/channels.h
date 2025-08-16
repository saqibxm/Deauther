#pragma once

#include <Arduino.h>
#include <cstdint>

using Channel = byte;
using ChannelMask = std::uint16_t;

enum Channels : ChannelMask
{
    C_NONE = 0,
    C_SET = (ChannelMask{1} << 0),
    C1 = (ChannelMask{1} << 1),
    C2 = (ChannelMask{1} << 2),
    C3 = (ChannelMask{1} << 3),
    C4 = (ChannelMask{1} << 4),
    C5 = (ChannelMask{1} << 5),
    C6 = (ChannelMask{1} << 6),
    C7 = (ChannelMask{1} << 7),
    C8 = (ChannelMask{1} << 8),
    C9 = (ChannelMask{1} << 9),
    C10 = (ChannelMask{1} << 10),
    C11 = (ChannelMask{1} << 11),
    C12 = (ChannelMask{1} << 12),
    C13 = (ChannelMask{1} << 13),
    C14 = (ChannelMask{1} << 14),
    C_ALL = C1 | C2 | C3 | C4 | C5 | C6 | C7 | C8 | C9 | C10 | C11 | C12 | C13 | C14,
    C_ASIA = C_ALL & ~C14,
};
