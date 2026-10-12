#pragma once
namespace Probe {
inline unsigned char FilterLootTrigger(unsigned char value,bool suppress,bool sharedNavigation) noexcept {
    return suppress && !sharedNavigation && value>30 ? 0 : value;
}
// 0 = original input, -1 = consume in BankPanel, 19/20 = copied page action.
// Ownership survives releasing LB first so repeats cannot change the main tab.
struct SharedPageGesture {
    bool down[2]{}, owned[2]{};
    int Handle(bool active, bool shared, bool lb, unsigned action, bool begin, bool repeat) noexcept {
        if (!active) { *this={}; return 0; }
        if (shared && (action==19 || action==20)) return -1;
        if (action!=7 && action!=8) return 0;
        unsigned index=action-7;
        if (!begin) {
            bool consumed=owned[index]; down[index]=owned[index]=false;
            return consumed?-1:0;
        }
        // Native input may mark the newly pressed opposite trigger as a repeat
        // while the first one is held. Own that press too; never forward it as
        // main-tab navigation or reverse the active chord. Both must release.
        if (owned[1-index]) { down[index]=owned[index]=true; return -1; }
        if (down[index] || repeat) { down[index]=true; return owned[index]?-1:0; }
        down[index]=true;
        if (!shared || !lb) return 0;
        owned[index]=true;
        return action==7?19:20;
    }
};
}
