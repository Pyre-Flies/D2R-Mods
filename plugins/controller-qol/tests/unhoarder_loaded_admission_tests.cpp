// Runs only in this test process. The game is never opened or modified.
#include "unhoarder_dispatch_compatibility.h"
#include "loot_approach_profile.h"
#include <cstdio>
#include <cstdlib>
static void Check(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL: %s (%lu)\n",message,GetLastError());std::exit(1);}}
static void Branch(unsigned char* at,unsigned char* relay,uintptr_t destination){
    at[0]=0xe9;const auto delta=static_cast<int32_t>(relay-(at+5));std::memcpy(at+1,&delta,4);
    std::memcpy(relay,QolItemCode::Jump,6);std::memcpy(relay+6,&destination,8);
}
static void Trampoline(unsigned char* at,const unsigned char* original,uintptr_t resume){
    std::memcpy(at,original,5);std::memcpy(at+5,QolItemCode::Jump,6);std::memcpy(at+11,&resume,8);
}
int main(int argc,char** argv){
    Check(argc==3,"DLL and old/new argument");
    const bool fixed=std::strcmp(argv[2],"new")==0;
    auto module=LoadLibraryExA(argv[1],nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
    Check(module!=nullptr,"load actual companion without calling plugin initialization");
    const auto companion=reinterpret_cast<uintptr_t>(module);
    auto* storage=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x380000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(storage!=nullptr,"allocate synthetic native image");const auto game=reinterpret_cast<uintptr_t>(storage);
    const auto itemHook=fixed?QolItemCodeBackgroundFix::HookRva:QolItemCode::HookRva;
    const auto dispatchHook=fixed?QolUnhoarderDispatchBackgroundFix::HookRva:QolUnhoarderDispatch::HookRva;
    const auto helper=fixed?uintptr_t(0x24f40):uintptr_t(0x24fa0);
    const auto* original=QolClientLoot::ApproachProfile::Dispatch;
    std::memcpy(storage+QolItemCode::Rva,QolItemCode::Original,sizeof(QolItemCode::Original));
    std::memcpy(storage+QolUnhoarderDispatch::Rva,original,sizeof(QolClientLoot::ApproachProfile::Dispatch));
    Branch(storage+QolItemCode::Rva,storage+0x100,companion+itemHook);
    Branch(storage+QolUnhoarderDispatch::Rva,storage+0x120,companion+dispatchHook);
    Trampoline(storage+0x140,QolItemCode::Original,game+QolItemCode::Rva+5);
    Trampoline(storage+0x160,original,game+QolUnhoarderDispatch::Rva+5);
    const uintptr_t itemOriginal=game+0x140,dispatchOriginal=game+0x160;
    std::memcpy(reinterpret_cast<void*>(companion+0xb0428),&itemOriginal,8);
    std::memcpy(reinterpret_cast<void*>(companion+0xd86b8),&dispatchOriginal,8);
    std::memcpy(reinterpret_cast<void*>(companion+0xb0418),&game,8);
    DWORD old{};Check(VirtualProtect(storage,0x380000,PAGE_EXECUTE_READ,&old)!=0,"synthetic image RX");
    D2RL::PluginApi api{};api.apiSize=sizeof(api);
    api.checkExpectedBytes=+[](const D2RL::PluginContext*,uintptr_t,const void*,uint32_t) noexcept {return false;};
    D2RL::PluginContext context{};context.contextSize=sizeof(context);context.api=&api;context.exeBase=game;
    Check(QolItemCode::Compatible(game,QolItemCode::Original,sizeof(QolItemCode::Original)),"production item-code admission accepts actual artifact and live graph");
    Check(QolUnhoarderDispatch::Check(&context,original,sizeof(QolClientLoot::ApproachProfile::Dispatch)),"production dispatch admission accepts actual artifact, code and graph");
    // Corrupt a reachable filtering helper, outside the wrapper. A disk hash
    // and intact wrapper must never override modified live behavior.
    auto* helperByte=reinterpret_cast<unsigned char*>(companion+helper);const auto before=*helperByte;
    Check(VirtualProtect(helperByte,1,PAGE_EXECUTE_READWRITE,&old)!=0,"test helper writable");
    *helperByte^=1;DWORD previous{};Check(VirtualProtect(helperByte,1,old,&previous)!=0,"test helper RX again");
    const bool changedAccepted=QolUnhoarderDispatch::Check(&context,original,sizeof(QolClientLoot::ApproachProfile::Dispatch));
    Check(VirtualProtect(helperByte,1,PAGE_EXECUTE_READWRITE,&old)!=0,"restore helper writable");*helperByte=before;
    Check(VirtualProtect(helperByte,1,old,&previous)!=0,"restore helper RX");FlushInstructionCache(GetCurrentProcess(),helperByte,1);
    Check(!changedAccepted,"changed live helper refused despite known artifact");
    Check(QolUnhoarderDispatch::Check(&context,original,sizeof(QolClientLoot::ApproachProfile::Dispatch)),"restored complete graph accepted");
    // Restore initial private companion data before unloading the test image.
    const uintptr_t zero=0;
    for(const auto slot:{0xb0428u,0xd86b8u,0xb0418u})std::memcpy(reinterpret_cast<void*>(companion+slot),&zero,8);
    VirtualFree(storage,0,MEM_RELEASE);FreeLibrary(module);
    std::puts("Actual old/new DLL production admission passed; changed loaded helper refused. Game untouched.");
}
