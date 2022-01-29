#pragma once

#include "types.h"
#include "log.h"

#include <stdio.h>
#include <memory.h>

// GAMEINPUT_MAX_BYTES * GAMEINPUT_MAX_PLAYERS * 8 must be less than
// 2^BITVECTOR_NIBBLE_SIZE (see bitvector.h)

#define GAMEINPUT_MAX_BYTES      9
#define GAMEINPUT_MAX_PLAYERS    2

struct GameInput {
    enum Constants {
        NullFrame = -1
    };

    int frame;
    int size; /* size in bytes of the entire input for all players */
    char bits[GAMEINPUT_MAX_BYTES * GAMEINPUT_MAX_PLAYERS];

    bool is_null() { return frame == NullFrame; }

    void init(int iframe, char* ibits, int isize, int offset) {
        ASSERT(isize);
        ASSERT(isize <= GAMEINPUT_MAX_BYTES);
        frame = iframe;
        size = isize;
        memset(bits, 0, sizeof(bits));
        if (ibits) {
            memcpy(bits + (offset * isize), ibits, isize);
        }
    }

    void init(int iframe, char* ibits, int isize) {
        ASSERT(isize);
        ASSERT(isize <= GAMEINPUT_MAX_BYTES * GAMEINPUT_MAX_PLAYERS);
        frame = iframe;
        size = isize;
        memset(bits, 0, sizeof(bits));
        if (ibits) {
            memcpy(bits, ibits, isize);
        }
    }

    bool value(int i) const {
        return (bits[i / 8] & (1 << (i % 8))) != 0;
    }
    void set(int i) {
        bits[i / 8] |= (1 << (i % 8));
    }
    void clear(int i) {
        bits[i / 8] &= ~(1 << (i % 8));
    }
    void erase() {
        memset(bits, 0, sizeof(bits));
    }
    
    void desc(char* buf, size_t buf_size, bool show_frame = true) const {
        ASSERT(size);
        size_t remaining = buf_size;
        if (show_frame) {
            remaining -= sprintf_s(buf, buf_size, "(frame:%d size:%d ", frame, size);
        } else {
            remaining -= sprintf_s(buf, buf_size, "(size:%d ", size);
        }

        for (int i = 0; i < size * 8; i++) {
            char buf2[16];
            if (value(i)) {
                int c = sprintf_s(buf2, ARRAY_SIZE(buf2), "%2d ", i);
                strncat_s(buf, remaining, buf2, ARRAY_SIZE(buf2));
                remaining -= c;
            }
        }
        strncat_s(buf, remaining, ")", 1);
    }
    
    void log(char* prefix, bool show_frame = true) const {
        char buf[1024];
        size_t c = strlen(prefix);
        strcpy_s(buf, prefix);
        desc(buf + c, ARRAY_SIZE(buf) - c, show_frame);
        strncat_s(buf, ARRAY_SIZE(buf) - strlen(buf), "\n", 1);
        Log(buf);
    }
    
    bool equal(GameInput& other, bool bitsonly = false) {
        if (!bitsonly && frame != other.frame) {
            Log("frames don't match: %d, %d\n", frame, other.frame);
        }
        if (size != other.size) {
            Log("sizes don't match: %d, %d\n", size, other.size);
        }
        if (memcmp(bits, other.bits, size)) {
            Log("bits don't match\n");
        }
        ASSERT(size && other.size);
        return (bitsonly || frame == other.frame) &&
               size == other.size &&
               memcmp(bits, other.bits, size) == 0;
    }
};