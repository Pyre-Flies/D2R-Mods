#include <D2RLPlugin/api.h>
#include "aim/controller_aim.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
namespace QolPortal { bool OwnsContactDestination(std::uintptr_t) noexcept { return false; } }
static const char* config="[qol]\nenabled=true\n[aim]\nenabled=false";
static unsigned reads{},queries{},hooks{};
static bool __cdecl Read(const D2RL::PluginContext*,char* out,uint32_t size,uint32_t*) noexcept {
    ++reads; if(std::strlen(config)+1>size) return false; std::memcpy(out,config,std::strlen(config)+1); return true;
}
static D2RL::ServiceQueryResult __cdecl Query(const D2RL::PluginContext*,D2RL::ServiceId,uint32_t,const void**) noexcept {
    ++queries; return D2RL::ServiceQueryResult::Unavailable;
}
static bool __cdecl Hook(const D2RL::PluginContext*,const D2RL::InlineHookRegistration*) noexcept { ++hooks; return false; }
static void Check(bool ok,const char* message) { if(!ok) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);} }
int main() {
    D2RL::PluginApi api{};api.apiSize=sizeof(api);api.readConfig=Read;api.queryService=Query;api.installInlineHook=Hook;
    D2RL::PluginContext ctx{};ctx.contextSize=sizeof(ctx);ctx.api=&api;ctx.exeBase=1;
    Check(QolAim::Initialize(&ctx,false) && reads==0,"global QOL disable skips all aim setup");
    Check(QolAim::Initialize(&ctx,true) && reads==1,"aim off avoids native initialization");
    config="[qol]\nenabled=true\n[aim]\nenabled=false";
    Check(QolAim::Initialize(&ctx,true),"explicit aim disable succeeds without services");
    config="[aim]\nenabled=true\nsnap_radius=100";
    Check(!QolAim::Initialize(&ctx,true),"bad aim settings fail closed for aim only");
    Check(!QolAim::OwnsGuidedArrow() && queries==0 && hooks==0,"disabled and malformed aim never register or claim Guided Arrow");
    QolAim::Shutdown();Check(!QolAim::OwnsGuidedArrow(),"shutdown leaves native Guided Arrow ownership");
    std::puts("Aim configuration gates leave loader services untouched.");
}
