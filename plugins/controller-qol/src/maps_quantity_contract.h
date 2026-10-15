#pragma once
#include "maps_quantity_profile.h"
#include "chat_forwarding_contract.h"
#include <initializer_list>

namespace QolMapsQuantity {
using QolChatForwarding::Memory;
using QolChatForwarding::Kind;
using QolChatForwarding::Relative;
inline uintptr_t Plus(uintptr_t address,size_t offset) noexcept {
    return address && address<=UINTPTR_MAX-offset?address+offset:0;
}
template<size_t N> bool Block(const Memory& memory,uintptr_t address,const unsigned char (&expected)[N],
    unsigned char (&actual)[N],std::initializer_list<size_t> references={}) noexcept {
    if(!memory.read || !memory.read(memory.user,address,actual,N,Kind::Code))return false;
    for(size_t i=0;i<N;++i) {
        bool operand=false;
        for(const auto offset:references)operand|=i>=offset && i-offset<4;
        if(!operand && actual[i]!=expected[i])return false;
    }
    return true;
}
inline bool Ref(uintptr_t address,const unsigned char* bytes,size_t operand,size_t end,uintptr_t& out) noexcept {
    int32_t displacement{};std::memcpy(&displacement,bytes+operand,4);
    const auto next=Plus(address,end);
    return next && Relative(next,displacement,out);
}
// Only item quantity (stat 70, layer 0). Every production caller uses those
// exact arguments. This does NOT qualify arbitrary Maps stats or hook chains.
// The installed hook is still called; we prove its reachable quantity path
// preserves all arguments and returns the original Core result unchanged.
inline bool Validate(const Memory& memory,uintptr_t imageBase,uintptr_t hook,uintptr_t original) noexcept {
    if(!memory.read || !imageBase || !hook || !original)return false;
    unsigned char head[sizeof(Profile::Head)]{},gate[sizeof(Profile::Gate)]{},index[sizeof(Profile::Index)]{};
    const auto gateAddress=Plus(hook,0x83);
    if(!Block(memory,hook,Profile::Head,head,{0x19}) ||
       !Block(memory,gateAddress,Profile::Gate,gate,{0x1a,0x27,0x33,0x48}))return false;
    uintptr_t originalSlot{},levelSlot{},continuation{},level{},firstReturn{},secondReturn{},helper{};
    if(!Ref(hook,head,0x19,0x1d,originalSlot) ||
       !Ref(gateAddress,gate,0x1a,0x1f,levelSlot) ||
       !Ref(gateAddress,gate,0x27,0x2b,firstReturn) ||
       !Ref(gateAddress,gate,0x48,0x4c,secondReturn) || firstReturn!=secondReturn ||
       !Ref(gateAddress,gate,0x33,0x37,helper) ||
       (originalSlot&7) || (levelSlot&7) || originalSlot==levelSlot ||
       !memory.read(memory.user,originalSlot,&continuation,8,Kind::Data) || continuation!=original ||
       !memory.read(memory.user,levelSlot,&level,8,Kind::Data))return false;
    unsigned char returnBytes[sizeof(Profile::Return)]{};
    if(!Block(memory,firstReturn,Profile::Return,returnBytes) ||
       !Block(memory,helper,Profile::Index,index,{0x25,0x2d,0x34}))return false;
    uintptr_t tableBase{};
    if(!Ref(helper,index,0x25,0x29,tableBase) || tableBase!=imageBase)return false;
    // The switch normalizes 70 by subtracting 17. Its exact bounds admit 53.
    // Both lookup tables are immutable code-section data owned by Maps.
    int32_t byteTableRva{},jumpTableRva{};
    std::memcpy(&byteTableRva,index+0x2d,4);std::memcpy(&jumpTableRva,index+0x34,4);
    uintptr_t byteTable{},jumpTable{};unsigned char choice{};uint32_t leafRva{};
    if(!Relative(imageBase,byteTableRva,byteTable) || !Relative(imageBase,jumpTableRva,jumpTable) ||
       !memory.read(memory.user,Plus(byteTable,70-17),&choice,1,Kind::Code) || choice>8 ||
       !memory.read(memory.user,Plus(jumpTable,4*choice),&leafRva,4,Kind::Code))return false;
    unsigned char negative[sizeof(Profile::Negative)]{};
    return Block(memory,Plus(imageBase,leafRva),Profile::Negative,negative);
}
}
