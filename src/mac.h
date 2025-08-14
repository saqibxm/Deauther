/*
   Copyright (c) 2020 Stefan Kremser (@Spacehuhn)
   This software is licensed under the MIT License. See the license file for details.
   Source: github.com/spacehuhn/esp8266_deauther
 */

#pragma once

#include <stdint.h>  // uint8_t
#include <stdbool.h> // bool

#include "common.h"

namespace mac {
    const static byte BROADCAST[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    bool multicast(const byte* mac);
    bool equals(const byte* macA, const byte* macB);

    bool valid(const char* str, unsigned int str_len, unsigned int len = 6);
    void fromStr(const char* str, byte* mac, unsigned int len       = 6);
}