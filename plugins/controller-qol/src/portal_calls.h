#pragma once
#include "glyph_calls.h"
#include "portal_signatures.h"
namespace QolPortalCalls {
inline constexpr uintptr_t Target = 0x34bc90;
struct Call { uintptr_t rva; const unsigned char* expected; };
inline constexpr Call Calls[] = {
    {0x191589, PortalNative::RefreshContactCall + 11},
    {0x1922cd, PortalNative::CandidateContactCall + 6},
    {0x192378, PortalNative::CandidateResultContactCall + 13}
};
// Precompute every displacement before publication. SDK owns each patch and
// cleanup; callers must remain passthrough unless the entire install succeeds.
template<class Patch>
bool Publish(uintptr_t base, uintptr_t relay, Patch patch) {
    unsigned char replacements[3][5]{};
    for (unsigned i=0; i<3; ++i)
        if (!QolGlyphCalls::Encode(base+Calls[i].rva, relay, replacements[i])) return false;
    for (unsigned i=0; i<3; ++i)
        if (!patch(Calls[i].rva, Calls[i].expected, replacements[i])) return false;
    return true;
}
}
