#pragma once
#include "item_code_controller_contract.h"
namespace QolUnhoarderDispatchControllerFix {
inline constexpr uintptr_t Rva=0xfabe0,HookRva=0x1dc20,SlotRva=0xd86b8,BaseRva=0xb0418;
inline constexpr uintptr_t TextRva=0x1000;
inline constexpr size_t TextSize=550351;
inline constexpr unsigned char TextHash[]={0x07,0x53,0xcc,0xcf,0xa1,0x48,0x71,0x04,0xaa,0x57,0x03,0x73,0xa9,0xc0,0x4f,0x4f,0xc2,0x73,0xa9,0xef,0x55,0x95,0x6e,0xb4,0xb0,0xd1,0xa3,0x6c,0xf8,0x52,0xfa,0xc3};
inline constexpr unsigned char Wrapper[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x20,0x41,0x8b,0xd9,0x41,0x8b,0xf8,0x48,0x8b,0xea,0x8b,0xf1,0x83,0xf9,0x16,0x75,0x16,0x41,0x83,0xf8,0x04,0x75,0x10,0xe8,0x91,0x76,0x00,0x00,0x84,0xc0,0x74,0x19,0x8b,0xcb,0xe8,0x36,0xe6,0xff,0xff,0x44,0x8b,0xcb,0x44,0x8b,0xc7,0x48,0x8b,0xd5,0x8b,0xce,0xff,0x15,0x4d,0xaa,0x0b,0x00,0x90,0x48,0x8b,0x5c,0x24,0x30,0x48,0x8b,0x6c,0x24,0x38,0x48,0x8b,0x74,0x24,0x40,0x48,0x83,0xc4,0x20,0x5f,0xc3};

inline bool Validate(const QolChatForwarding::Memory& memory,uintptr_t game,uintptr_t companion,
    const unsigned char* original,size_t size) noexcept {
    using namespace QolChatForwarding;
    if(!memory.read || !original || size<32 || size>4096 || !game || !companion ||
       game>UINTPTR_MAX-Rva-size || companion>UINTPTR_MAX-SlotRva-8)return false;
    unsigned char entry[4096]{},relay[14]{},wrapper[sizeof(Wrapper)]{},trampoline[19]{};
    uintptr_t relayAddress{},destination{},previous{},resume{},base{};int32_t displacement{};
    const auto address=game+Rva;
    if(!memory.read(memory.user,address,entry,size,Kind::Trampoline) || entry[0]!=0xe9 ||
       std::memcmp(entry+5,original+5,size-5))return false;
    std::memcpy(&displacement,entry+1,4);
    if(!Relative(address+5,displacement,relayAddress) ||
       !memory.read(memory.user,relayAddress,relay,sizeof(relay),Kind::Trampoline) ||
       std::memcmp(relay,QolItemCodeControllerFix::Jump,6))return false;
    std::memcpy(&destination,relay+6,8);
    if(destination!=companion+HookRva ||
       !memory.read(memory.user,destination,wrapper,sizeof(wrapper),Kind::Code) ||
       std::memcmp(wrapper,Wrapper,sizeof(wrapper)) ||
       !memory.read(memory.user,companion+SlotRva,&previous,8,Kind::Data) || !previous ||
       !memory.read(memory.user,previous,trampoline,sizeof(trampoline),Kind::Trampoline) ||
       std::memcmp(trampoline,original,5) || std::memcmp(trampoline+5,QolItemCodeControllerFix::Jump,6) ||
       !memory.read(memory.user,companion+BaseRva,&base,8,Kind::Data) || base!=game)return false;
    std::memcpy(&resume,trampoline+11,8);
    return resume==address+5 && QolItemCodeControllerFix::Validate(memory,game,companion);
}
}
