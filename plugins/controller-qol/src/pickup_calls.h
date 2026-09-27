#pragma once
#include "glyph_calls.h"
#include <cstddef>
#include <iterator>

namespace QolPickupCalls {
inline constexpr uintptr_t Target = 0x471950;

struct Call {
    uintptr_t rva;
    unsigned char expected[5];
};

// Direct game callers of D2Game's pickup entry for build 3.3.93847.
// Patching callers instead of Target preserves any earlier owner of the shared
// entry (for example Auto Deposit) and lets allowed pickups traverse its hook.
inline constexpr Call Calls[] = {
    {0x410005, {0xe8, 0x46, 0x19, 0x06, 0x00}},
    {0x4112f5, {0xe8, 0x56, 0x06, 0x06, 0x00}},
    {0x416126, {0xe8, 0x25, 0xb8, 0x05, 0x00}},
    {0x41738d, {0xe8, 0xbe, 0xa5, 0x05, 0x00}},
    {0x4ba0ef, {0xe8, 0x5c, 0x78, 0xfb, 0xff}},
    {0x4bbad0, {0xe8, 0x7b, 0x5e, 0xfb, 0xff}},
    {0x55496c, {0xe8, 0xdf, 0xcf, 0xf1, 0xff}},
};

template<class Patch>
bool Publish(uintptr_t base, uintptr_t relay, Patch patch) {
    unsigned char replacements[std::size(Calls)][5]{};
    for (size_t i = 0; i < std::size(Calls); ++i)
        if (!QolGlyphCalls::Encode(base + Calls[i].rva, relay, replacements[i])) return false;
    for (size_t i = 0; i < std::size(Calls); ++i)
        if (!patch(Calls[i].rva, Calls[i].expected, replacements[i])) return false;
    return true;
}
}
