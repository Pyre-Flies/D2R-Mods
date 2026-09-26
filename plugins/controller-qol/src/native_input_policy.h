#pragma once
#include "physical_input.h"
#include "shared_page_policy.h"
namespace QolNativeInput {
// Verified game-normalized keys: XInput-compatible buttons plus digital LT/RT.
inline constexpr unsigned TriggerLeft=0x400, TriggerRight=0x800;
inline constexpr unsigned Provider=0x100;
inline ControllerQoL::PhysicalInput Decode(unsigned keys) noexcept {
    ControllerQoL::PhysicalInput r;
    r.valid=true; r.providers=Provider;
    r.buttons=static_cast<uint16_t>(keys & ~0xc00u);
    r.leftTrigger=(keys&TriggerLeft)?255:0;
    r.rightTrigger=(keys&TriggerRight)?255:0;
    return r;
}
inline unsigned Encode(const ControllerQoL::PhysicalInput& r) noexcept {
    return r.buttons | (r.leftTrigger?TriggerLeft:0) | (r.rightTrigger?TriggerRight:0);
}
inline ControllerQoL::PhysicalInput FilterModified(ControllerQoL::PhysicalInput r,bool modifier,bool shared) noexcept {
    if(!modifier) return r;
    r.buttons &= ~static_cast<uint16_t>(0x2000|0x4000|0x8000|0x200|0x80); // B/X/Y/RB/R3
    r.rightTrigger=Probe::FilterLootTrigger(r.rightTrigger,true,shared);
    r.leftTrigger=Probe::FilterLootTrigger(r.leftTrigger,(r.buttons&0x100)!=0,shared);
    return r;
}
template<class Emit> void Deliver(unsigned& delivered,unsigned desired,unsigned primaryKey,bool primaryPressed,Emit emit) noexcept {
    const unsigned changed=desired^delivered;
    for(unsigned key=1;key<=0x8000;key<<=1) if((changed&key) && !(desired&key)) {
        delivered&=~key;emit(key,false);
    }
    for(unsigned key=1;key<=0x8000;key<<=1) if((changed&key) && (desired&key)) {
        delivered|=key;emit(key,true);
    }
    if(primaryPressed && (desired&primaryKey) && !(changed&primaryKey)) emit(primaryKey,true);
}
inline bool KnownKey(unsigned key) noexcept { return key && key<=0x8000 && !(key&(key-1)); }
inline bool Fresh(uint64_t now,uint64_t last) noexcept {return last && now>=last && now-last<=250;}
}
