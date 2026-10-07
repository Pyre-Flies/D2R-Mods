#include "plugin_coexistence_policy.h"
#include "client_request_profile.h"
#include <array>
#include <cstdio>
#include <cstdlib>
void Check(bool ok,const char* message) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main() {
    using namespace QolCoexistence;
    constexpr uintptr_t core=0x10000000,hook=0x200538b0;
    std::array<unsigned char,68> original{},entry{};
    std::array<unsigned char,28> trampoline{};
    for(size_t i=0;i<original.size();++i)original[i]=static_cast<unsigned char>(i+1);
    entry=original;
    auto jump=[](unsigned char* out,uintptr_t target) {
        const unsigned char prefix[]={0xff,0x25,0,0,0,0};
        std::memcpy(out,prefix,6);std::memcpy(out+6,&target,8);
    };
    jump(entry.data(),hook);
    std::memcpy(trampoline.data(),original.data(),14);jump(trampoline.data()+14,core+14);
    auto valid=[&](bool owner=true){return SendChain(entry.data(),original.data(),original.size(),hook,trampoline.data(),core+14,owner);};
    Check(valid(),"known hook forwards through exact relocated original prologue");
    Check(!valid(false),"unknown owner cannot use known hook shape");
    entry[6]^=1;Check(!valid(),"different hook rejected");entry[6]^=1;
    entry[20]^=1;Check(!valid(),"changed Core tail rejected");entry[20]^=1;
    trampoline[1]^=1;Check(!valid(),"changed relocated instruction rejected");trampoline[1]^=1;
    trampoline[20]^=1;Check(!valid(),"wrong trampoline continuation rejected");trampoline[20]^=1;
    Check(!SendChain(entry.data(),original.data(),original.size(),hook,nullptr,core+14,true),"null trampoline rejected");
    Check(StatRoute(core,core,0,0,false),"unhooked offline and non-Ladder reader requires no Maps");
    Check(StatRoute(hook,core,hook,core,true),"known Maps forwarding reader admitted");
    Check(!StatRoute(hook,core,hook,core,false),"unverified Maps refused");
    Check(!StatRoute(hook,core,hook,hook,true),"recursive Maps continuation refused");
    Check(!StatRoute(hook+1,core,hook,core,true),"unrelated stat hook refused");
    Check(!StatRoute(hook,core,hook,0,true),"missing original reader refused");
    // Exercise the production guards in a standalone process with no Ladder
    // plugins loaded. Only the referenced pages of these fake images are used.
    const auto gameImage=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x4000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    const auto coreImage=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    Check(gameImage && coreImage,"allocate isolated image fixtures");
    std::memcpy(reinterpret_cast<void*>(gameImage+0x3e2b73c),QolClientRequest::Thunk,sizeof(QolClientRequest::Thunk));
    *reinterpret_cast<uintptr_t*>(gameImage+0x3e2ae20)=coreImage+0x822dd0;
    std::memcpy(reinterpret_cast<void*>(coreImage+0x822dd0),QolClientRequest::CoreEntry,sizeof(QolClientRequest::CoreEntry));
    *reinterpret_cast<uintptr_t*>(gameImage+0x3e2a218)=coreImage+0x831de0;
    Check(QolClientRequest::Guard(gameImage,coreImage),"production unhooked request guard needs no Ladder plugin");
    Check(StatSlot(gameImage,coreImage),"production unhooked stat slot needs no Ladder plugin");
    *reinterpret_cast<unsigned char*>(coreImage+0x822dd0)=0xff;
    Check(!QolClientRequest::Guard(gameImage,coreImage),"unknown sender replacement still refused");
    *reinterpret_cast<uintptr_t*>(gameImage+0x3e2a218)=hook;
    Check(!StatSlot(gameImage,coreImage),"unknown stat replacement still refused");
    VirtualFree(reinterpret_cast<void*>(gameImage),0,MEM_RELEASE);
    VirtualFree(reinterpret_cast<void*>(coreImage),0,MEM_RELEASE);
    std::puts("Plugin forwarding chains and original-path isolation passed.");
}
