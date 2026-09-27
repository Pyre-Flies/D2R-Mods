#include "core_compatibility.h"
#include "controller_input.h"
#include <thread>
#include <atomic>
#include <cstdio>
int main(int argc,char** argv) {
    // Optional real unsupported core: map only, never execute its entry point.
    HMODULE mapped=nullptr;
    if(argc==2) {
        mapped=LoadLibraryExA(argv[1],nullptr,DONT_RESOLVE_DLL_REFERENCES);
        if(!mapped || QolCore::VerifyCore(mapped)) return 3;
    }
    // No game/core executes in this process. Reject absent and foreign modules
    // before touching any private offset; exercise concurrent first access.
    if (QolCore::VerifyCore(nullptr) || QolCore::VerifyCore(GetModuleHandleW(nullptr))) return 1;
    unsigned char zero[32]{},digest[32]{};bool readable=true;
    if(QolCore::VerifyFileHash(nullptr,zero,digest,&readable) || readable)return 4;
    const auto self=GetModuleHandleW(nullptr);
    if(QolCore::VerifyFileHash(self,zero,digest,&readable) || !readable)return 5;
    if(!QolCore::VerifyFileHash(self,digest,nullptr,&readable) || !readable)return 6;
    std::atomic<bool> unexpected{false};
    auto poll=[&] { for(int i=0;i<1000;++i) if(ControllerQoL::IsNativeAltModifierActive()) unexpected=true; };
    std::thread a(poll),b(poll); a.join();b.join();
    if(unexpected) return 2;
    if(mapped) FreeLibrary(mapped);
    std::puts("Unknown/absent core rejected; concurrent bridge fallback passed.");
}
