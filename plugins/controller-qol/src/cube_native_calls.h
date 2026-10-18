#pragma once
#include "glyph_calls.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace QolCubeNativeCalls {
enum class Kind { Tap, Hold, Prompt, ItemPrompt };
inline constexpr unsigned char Tap[]={0xff,0x90,0x00,0x01,0x00,0x00};
inline constexpr unsigned char Hold[]={0x48,0x8b,0xd0,0x48,0x8b,0xcf,0xff,0xd3};
inline constexpr unsigned char Prompt[]={0xe8,0x44,0xfa,0xdc,0xff};
inline constexpr unsigned char ItemPrompt[]={0xe8,0x4b,0x99,0xb6,0x03};
struct Site {uintptr_t rva;const unsigned char* expected;size_t size;Kind kind;};
inline constexpr Site Sites[]={{0x2aa399,Tap,sizeof(Tap),Kind::Tap},{0x2aa37a,Hold,sizeof(Hold),Kind::Hold},{0x2a8987,Prompt,sizeof(Prompt),Kind::Prompt},{0x2c1de0,ItemPrompt,sizeof(ItemPrompt),Kind::ItemPrompt}};
// CALL supplies the original return address; these tails only reconstruct the
// reviewed arguments, then tail-jump to a normal Windows x64 callback.
inline size_t Relay(Kind kind,uintptr_t destination,unsigned char (&out)[32]) noexcept {
    constexpr unsigned char tap[]={0x4c,0x8b,0x80,0x00,0x01,0x00,0x00}; // r8=[rax+100]: original tap
    constexpr unsigned char hold[]={0x48,0x8b,0xd0,0x48,0x8b,0xcf,0x4c,0x8b,0xc3}; // rdx=rax, rcx=rdi, r8=rbx
    constexpr unsigned char prompt[]={0x4d,0x8b,0xce}; // r9=r14: widget, original first 3 args retained
    constexpr unsigned char itemPrompt[]={0x48,0x8b,0xd3}; // rdx=rbx: item; ecx=original localization id
    const auto prefix=kind==Kind::Tap?tap:kind==Kind::Hold?hold:kind==Kind::Prompt?prompt:itemPrompt;
    const auto count=kind==Kind::Tap?sizeof(tap):kind==Kind::Hold?sizeof(hold):kind==Kind::Prompt?sizeof(prompt):sizeof(itemPrompt);
    if(!destination)return 0;
    std::memset(out,0,sizeof(out));std::memcpy(out,prefix,count);
    out[count]=0xff;out[count+1]=0x25;std::memcpy(out+count+6,&destination,sizeof(destination));
    return count+14;
}
inline bool Patch(uintptr_t site,uintptr_t relay,size_t size,unsigned char (&out)[8]) noexcept {
    if(size<5 || size>sizeof(out))return false;
    unsigned char call[5]{};if(!QolGlyphCalls::Encode(site,relay,call))return false;
    std::memset(out,0x90,sizeof(out));std::memcpy(out,call,sizeof(call));return true;
}
}
