#pragma once
#include "compatibility.h"

namespace DayNight {
// Caller serializes commands and shutdown. No native callback is installed here.
class Runtime {
    uintptr_t host_{},core_{};
    bool ready_{},enabled_{};
    OwnedPatch patch_;
public:
    bool Enabled() const noexcept {return enabled_;}
    bool Healthy() const noexcept {
        return ready_ && (!enabled_ || patch_.Owned()) && NativeGuards(host_,core_,enabled_);
    }
    bool Admit(uintptr_t host,uintptr_t core) noexcept {
        if(ready_ || patch_.Installed() || !NativeGuards(host,core)) return false;
        host_=host;core_=core;enabled_=false;ready_=true;return true;
    }
    bool SetEnabled(bool value) noexcept {
        if(!Healthy()) return false;
        if(value==enabled_) return true;
        const auto result=value?patch_.Install(core_+Profile::CoreBlockRva,Profile::OriginalBlock,Profile::EnabledBlock):patch_.Stop();
        if(result!=(value?PatchResult::Applied:PatchResult::Restored)) {
            patch_.Stop();ready_=false;enabled_=false;return false;
        }
        enabled_=value;return true;
    }
    PatchResult Stop() noexcept {
        const auto result=patch_.Stop();ready_=false;enabled_=false;host_=core_=0;return result;
    }
};
}
