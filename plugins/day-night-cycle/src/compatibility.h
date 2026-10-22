#pragma once
#include "native_profile.h"
#include "owned_patch.h"

namespace DayNight {
inline bool NativeGuards(uintptr_t host,uintptr_t core,bool enabled=false) noexcept {
    using namespace Profile;
    auto wrapper=CoreWrapper;
    if(enabled) std::memcpy(wrapper.data()+CoreBlockRva-CoreWrapperRva,EnabledBlock.data(),EnabledBlock.size());
    if(!host || !core || !Bytes(core+CoreWrapperRva,wrapper) ||
       !Bytes(core+NoDebugDescriptorRva,NoDebugDescriptor) ||
       !Bytes(core+SetterDescriptorRva,SetterDescriptor) ||
       !Bytes(host+GameCallerRva,GameCaller) || !Bytes(host+GameThunkRva,GameThunk) ||
       !Bytes(host+NativeSetterRva,NativeSetter)) return false;
    uintptr_t noDebug{},setter{},thunk{};
    return Read(core+CoreNoDebugPointerRva,&noDebug,sizeof(noDebug)) && noDebug==host+NativeNoDebugRva &&
        Read(core+CoreSetterPointerRva,&setter,sizeof(setter)) && setter==host+NativeSetterRva &&
        Read(host+GameThunkSlotRva,&thunk,sizeof(thunk)) && thunk==core+CoreWrapperRva;
}
}
