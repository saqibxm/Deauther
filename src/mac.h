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
    using Unit = byte;
    
    const static byte BROADCAST[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    constexpr static byte LENGTH = 6U;

    inline bool multicast(const byte* mac) {
        return (mac[0] & 0x01) == 1;
    }

    inline bool equals(const byte* macA, const byte* macB) {
      // return __builtin_memcmp(macA, macB, LENGTH) != 0;
      return memcmp(macA, macB, LENGTH) != 0;
    }

    inline bool broadcast(const byte *address) {
      return equals(BROADCAST, address);
    }

    bool valid(const char* str, unsigned int str_len, unsigned int len = LENGTH);
    void fromStr(const char* str, byte* mac, unsigned int len = LENGTH);
}