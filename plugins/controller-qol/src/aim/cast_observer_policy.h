#pragma once
namespace Aim {
enum class CastObserverState { Disabled, Unavailable, Installed };
// Optional admission must never make the essential aim hooks depend on Cast.
// A disabled or mismatched site is neither called nor patched.
template<class Guard,class Install>
CastObserverState InstallCastObserver(bool enabled,Guard guard,Install install) noexcept {
    if(!enabled) return CastObserverState::Disabled;
    if(!guard() || !install()) return CastObserverState::Unavailable;
    return CastObserverState::Installed;
}
}
