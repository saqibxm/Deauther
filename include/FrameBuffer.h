#pragma once

#include <Arduino.h>
#include "packet.h"

#define FRAME_BUFFER_BYTES 128
#define FRAME_BUFFER_ENTRIES 64

struct Frame {
    Frame() = default;
    Frame(const byte* buf, std::uint16_t len, std::uint32_t ts = millis())
        : length(len), timeStamp(ts)
    {
        memcpy(reinterpret_cast<byte*>(&packet), buf, len);
    }

    std::uint16_t length;
    std::uint32_t timeStamp;
    byte packet[FRAME_BUFFER_BYTES];
};

class FrameBuffer
{
    using Index = byte;
public:
    FrameBuffer() : head(0), tail(0)
    {
        debuglnF("[Frame Buffer] Initialized");
    }

    void Push(const byte *buf, std::uint16_t len, std::uint32_t ts = millis())
    {
        Index next = (tail + 1) % (FRAME_BUFFER_ENTRIES);
        if(next == head) // reaches back to head
        {
            // do nothing for now
            return;
        }

        decltype(Frame::length) copyBytes = len > FRAME_BUFFER_BYTES ? FRAME_BUFFER_BYTES : len;
        noInterrupts();
        Frame &frame = ring[next];
        frame.length = copyBytes;
        frame.timeStamp = ts;
        memcpy(reinterpret_cast<byte*>(&frame.packet), buf, copyBytes);
        tail = next;
        interrupts();

        // debugfP("[Frame Buffer] Pushed, Packet Length: %d\r\n", copyBytes);
    }

    bool Pop(Frame &to)
    {
        if(Empty()) return false;

        noInterrupts();
        Frame &requested = ring[head];
        to.length = requested.length;
        to.timeStamp = requested.timeStamp;
        memcpy(reinterpret_cast<byte*>(&to.packet), reinterpret_cast<const byte*>(&requested.packet), requested.length);
        
        head = (head + 1) % FRAME_BUFFER_ENTRIES;
        interrupts();

        // debugfP("[Frame Buffer] Popped, Packet Length: %d\r\n", to.length);

        return true;
    }

    void Clear()
    {
        head = tail = 0;
    }

private:
    volatile Index head, tail; // beware only supports upto 255
    Frame ring[FRAME_BUFFER_ENTRIES];

    bool Empty() {
        return head == tail;
    }
    // Frame& Slot(Index at) {
    //     // if(at >= tail || at < head)
    //     return 
    // }
    void CheckResetTail() {
        if(tail >= FRAME_BUFFER_ENTRIES) tail = 0;
    }
};