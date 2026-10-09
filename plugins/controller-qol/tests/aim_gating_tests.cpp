#include <D2RLPlugin/api.h>
#include "aim/controller_aim.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <windows.h>
#include <array>
#include "aim/native_profile.h"
#include "physical_input.h"
namespace ControllerQoL { bool NativeRightStickPresses(uint64_t&) noexcept {return false;} bool IsControllerUiActive() noexcept {return false;} PhysicalInput ReadControllerInput() noexcept {return {};} }
namespace QolPortal { bool OwnsContactDestination(std::uintptr_t) noexcept { return false; } }
static const char* config="[qol]\nenabled=true\n[aim]\nenabled=false";
static unsigned reads{},queries{},hooks{};
static unsigned infoLogs{},warningLogs{};
static void __cdecl Info(const D2RL::PluginContext*,const char*) noexcept { ++infoLogs; }
static void __cdecl Warn(const D2RL::PluginContext*,const char*) noexcept { ++warningLogs; }
static bool __cdecl Read(const D2RL::PluginContext*,char* out,uint32_t size,uint32_t*) noexcept {
    ++reads; if(std::strlen(config)+1>size) return false; std::memcpy(out,config,std::strlen(config)+1); return true;
}
static D2RL::ServiceQueryResult __cdecl Query(const D2RL::PluginContext*,D2RL::ServiceId,uint32_t,const void**) noexcept {
    ++queries; return D2RL::ServiceQueryResult::Unavailable;
}
static bool __cdecl Hook(const D2RL::PluginContext*,const D2RL::InlineHookRegistration*) noexcept { ++hooks; return false; }
static void Check(bool ok,const char* message) { if(!ok) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);} }
static std::array<uint64_t,8> installedRvas{};
static unsigned installCount{};
static bool rejectCast{};
static bool __cdecl Match(const D2RL::PluginContext* context,uint64_t rva,const void* expected,uint32_t size) noexcept {
    return std::memcmp(reinterpret_cast<const void*>(context->exeBase+rva),expected,size)==0;
}
static bool __cdecl RecordHook(const D2RL::PluginContext*,const D2RL::InlineHookRegistration* registration) noexcept {
    if(installCount>=installedRvas.size()) return false;
    installedRvas[installCount++]=registration->rva;
    if(rejectCast && registration->rva==Native::CastRva) return false;
    if(registration->original) *registration->original=registration->target;
    return true;
}
static void TestInstallation() {
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x2000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    Check(image!=nullptr,"fixture allocation");
    for(const auto& site:Native::Sites) std::memcpy(image+site.rva,site.bytes,site.size);
    for(const auto rva:{0x1922cdu,0x192378u}) {
        image[rva]=0xe8;
        const int32_t displacement=static_cast<int32_t>(0x34bc90u-rva-5u);
        std::memcpy(image+rva+1,&displacement,sizeof(displacement));
    }
    D2RL::PluginApi api{}; api.apiSize=sizeof(api); api.checkExpectedBytes=Match; api.installInlineHook=RecordHook;
    api.logInfo=Info; api.logWarn=Warn;
    D2RL::PluginContext context{}; context.contextSize=sizeof(context); context.api=&api;
    context.exeBase=reinterpret_cast<std::uintptr_t>(image);
    installCount=0;
    Check(QolAim::TestInstall(&context,true) && installCount==5 && installedRvas[4]==Native::CastRva,"clean installation retains all four aim hooks and full cast observer");
    image[Native::CastRva]=0xe9; // Simulate another owner's entry detour.
    installCount=0;
    Check(QolAim::TestInstall(&context,true) && installCount==4 && image[Native::CastRva]==0xe9,"cast collision preserves essential aim and leaves foreign prefix untouched");
    installCount=0;
    Check(QolAim::TestInstall(&context,false) && installCount==4,"configured opt-out installs essential hooks only");
    std::memcpy(image+Native::CastRva,Native::CastBytes,sizeof(Native::CastBytes));
    rejectCast=true; installCount=0;
    Check(QolAim::TestInstall(&context,true) && installCount==5,"SDK cast ownership rejection retains essential aim");
    rejectCast=false; image[Native::LookupRva]^=1; installCount=0;
    Check(!QolAim::TestInstall(&context,true) && installCount==0,"essential lookup guard mismatch still disables aim before hooks");
    Check(warningLogs>0,"quiet mode retains compatibility warnings");
    QolAim::Shutdown();
    VirtualFree(image,0,MEM_RELEASE);
}
int main() {
    D2RL::PluginApi api{};api.apiSize=sizeof(api);api.readConfig=Read;api.queryService=Query;api.installInlineHook=Hook;
    D2RL::PluginContext ctx{};ctx.contextSize=sizeof(ctx);ctx.api=&api;ctx.exeBase=1;
    Check(QolAim::Initialize(&ctx,false) && reads==0,"global QOL disable skips all aim setup");
    Check(QolAim::Initialize(&ctx,true) && reads==1,"aim off avoids native initialization");
    config="[qol]\nenabled=true\n[aim]\nenabled=false";
    Check(QolAim::Initialize(&ctx,true),"explicit aim disable succeeds without services");
    const std::string largeConfig="[aim]\nenabled=false\n#"+std::string(20000,'x')+"\n[aim.necromancer]\n\"74\"=false";
    config=largeConfig.c_str();
    Check(QolAim::Initialize(&ctx,true),"expanded configuration beyond 16 KiB remains readable without native setup");
    config="[aim]\nenabled=true\nsnap_radius=100";
    Check(!QolAim::Initialize(&ctx,true),"bad aim settings fail closed for aim only");
    config="[aim]\nenabled=true\n[aim.custom]\n\"New Spell\"=\"snap\"";
    Check(!QolAim::Initialize(&ctx,true),"malformed custom skill never reaches native initialization");
    Check(!QolAim::OwnsGuidedArrow() && queries==0 && hooks==0,"disabled and malformed aim never register or claim Guided Arrow");
    QolAim::Shutdown();Check(!QolAim::OwnsGuidedArrow(),"shutdown leaves native Guided Arrow ownership");
    TestInstallation();
    api.logInfo=Info; api.logWarn=Warn;
    infoLogs=0;
    Check(QolAim::TestScoreLogging(&ctx,false) && infoLogs==0,"quiet native scoring preserves enemy/NPC behavior without trace counters or log writes");
    Check(QolAim::TestScoreLogging(&ctx,true) && infoLogs==8,"verbose scoring restores bounded diagnostics without changing targeting");
    std::puts("Aim configuration gates leave loader services untouched.");
}
