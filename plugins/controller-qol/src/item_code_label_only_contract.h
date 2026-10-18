#pragma once
#include "chat_forwarding_contract.h"

namespace QolItemCodeLabelOnly {
inline constexpr uintptr_t Rva=0x36ef50, HookRva=0x1da70, SlotRva=0xb1488;
inline constexpr unsigned char Original[]={0x48,0x89,0x5c,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x85,0xc9,0x75,0x13,0x88,0x4c,0x24,0x30,0x48,0x8d,0x4c,0x24,0x30,0xe8,0x80,0x83,0xff,0xff};
inline constexpr unsigned char Wrapper[]={0x48,0x83,0xec,0x28,0x48,0x8b,0x05,0x0d,0x3a,0x09,0x00,0xff,0xd0,0x90,0x48,0x83,0xc4,0x28,0xc3};
inline constexpr unsigned char Jump[]={0xff,0x25,0,0,0,0};
// Only this complete one-argument, unchanged-return forwarding graph is known.
// Continue calling the installed game entry; this validator never patches it.
inline bool Validate(const QolChatForwarding::Memory& memory,uintptr_t game,uintptr_t companion) noexcept {
    using namespace QolChatForwarding;
    if(!memory.read || !game || game>UINTPTR_MAX-Rva-sizeof(Original) ||
       !companion || companion>UINTPTR_MAX-SlotRva-sizeof(uintptr_t))return false;
    const auto entry=game+Rva;
    unsigned char actual[sizeof(Original)]{},relay[14]{},wrapper[sizeof(Wrapper)]{},trampoline[19]{};
    uintptr_t relayAddress{},destination{},previous{},resume{};int32_t displacement{};
    if(!memory.read(memory.user,entry,actual,sizeof(actual),Kind::Trampoline) || actual[0]!=0xe9 ||
       std::memcmp(actual+5,Original+5,sizeof(Original)-5))return false;
    std::memcpy(&displacement,actual+1,4);
    if(!Relative(entry+5,displacement,relayAddress) ||
       !memory.read(memory.user,relayAddress,relay,sizeof(relay),Kind::Trampoline) ||
       std::memcmp(relay,Jump,sizeof(Jump)))return false;
    std::memcpy(&destination,relay+6,8);
    if(destination!=companion+HookRva ||
       !memory.read(memory.user,destination,wrapper,sizeof(wrapper),Kind::Code) ||
       std::memcmp(wrapper,Wrapper,sizeof(Wrapper)) ||
       !memory.read(memory.user,companion+SlotRva,&previous,sizeof(previous),Kind::Data) ||
       !previous || !memory.read(memory.user,previous,trampoline,sizeof(trampoline),Kind::Trampoline) ||
       std::memcmp(trampoline,Original,5) || std::memcmp(trampoline+5,Jump,sizeof(Jump)))return false;
    std::memcpy(&resume,trampoline+11,8);
    return resume==entry+5;
}
}
