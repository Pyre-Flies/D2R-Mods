#pragma once
#include <cstdint>

// D2R 3.3.93787 / reviewed D2RCore 1.3.1. No callable native entry points.
// See docs/GENERAL-SKILLS-OSKILLS.md for live IDs, ancestry and witnesses.
namespace GeneralSkillNative {
inline constexpr std::uintptr_t ClassButton=0x1ce4148, GeneralButton=0x1ce4210;
struct Guard {std::uintptr_t rva;const unsigned char* bytes;unsigned size;};
inline constexpr unsigned char Kind[]={0x41,0x83,0xbe,0x88,0x0b,0,0,0};
inline constexpr unsigned char Id[]={0x41,0x8b,0x96,0x08,0x0c,0,0,0x48,0x8b,0xc8,0xe8,0x57,0x1b,0xf1,0xff};
inline constexpr unsigned char Pair[]={0x45,0x8b,0x8e,0x0c,0x0c,0,0,0xba,1,0,0,0,0x45,0x8b,0x86,0x08,0x0c,0,0};
inline constexpr Guard Guards[]={{0x238e0c,Kind,sizeof(Kind)},{0x238e8a,Id,sizeof(Id)},{0x238e99,Pair,sizeof(Pair)}};
inline bool Admit(auto check,auto slot) noexcept {
    for(const auto& guard:Guards)if(!check(guard))return false;
    return slot(GeneralButton+0x20)==0x238c20 && slot(GeneralButton+0x58)==0x2382d0;
}
// The caller validates the visible tree ancestry and catches invalidated native
// reads on the UI thread. No widget or skill record pointer leaves that callback.
inline int ReadId(std::uintptr_t widget,std::uintptr_t base,bool generalAdmitted,auto pointer,auto integer) noexcept {
    const auto type=pointer(widget)-base;
    int id=-1;
    if(type==ClassButton) {
        const auto record=pointer(widget+0x668);
        if(record)id=integer(record);
    } else if(type==GeneralButton && generalAdmitted) {
        // Only the observed uncharged skill representation is supported.
        if(integer(widget+0xb88)!=0 || integer(widget+0xc0c)!=-1)return -1;
        id=integer(widget+0xc08);
        // General Skills also contains native actions. Never expose an aim
        // toggle for those, even if a legacy example declared one as custom.
        if(id<=5 || (id>=357 && id<=364) || id==370)return -1;
    }
    return id>0 && id<65535?id:-1;
}
}
