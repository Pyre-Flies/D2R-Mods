#pragma once
#include "d2r_defs.h"

namespace D2R {

class IdentifyHandler {
public:
    // Attempts to quick-identify the currently hovered item
    // Returns true if the action was consumed/handled
    static bool HandleIdentifyAction();

private:
    static bool CanIdentify(UnitAny* item);
};

} // namespace D2R
