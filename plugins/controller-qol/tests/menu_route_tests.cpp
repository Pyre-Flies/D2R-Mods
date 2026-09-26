#include "menu_route_profile.h"
#include "policy.h"
#include <cstdio>
#include <cstdlib>
void Check(bool ok,const char* text) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
int main() {
    using namespace QolMenuRoute;
    constexpr uintptr_t game=0x140000000,core=0xc0de5000000;
    unsigned char thunk[6]={0xff,0x25};
    const int32_t delta=static_cast<int32_t>(Import-Thunk-6);
    std::memcpy(thunk+2,&delta,4);
    Check(ValidLinks(game,core,game+Thunk,thunk,core+Dispatcher,game+0x27e620),"qualified route chain");
    Check(!ValidLinks(game,core,game+Thunk+1,thunk,core+Dispatcher,game+0x27e620),"foreign slot rejected");
    Check(!ValidLinks(game,core,game+Thunk,thunk,core+Dispatcher+1,game+0x27e620),"foreign dispatcher rejected");
    Check(!ValidLinks(game,core,game+Thunk,thunk,core+Dispatcher,game+0x27df80),"wrong forwarding ABI/path rejected");
    thunk[2]^=1;
    Check(!ValidLinks(game,core,game+Thunk,thunk,core+Dispatcher,game+0x27e620),"changed thunk rejected");
    // Model native versus loader-owned menu navigation. Both must receive the
    // translated trigger exactly once; bumpers must never reach either route.
    for(bool registered : {false,true}) {
        for(unsigned action : {7u,8u,19u,20u,0u,9u}) {
            int routeCalls=0,nativeCalls=0,selection=0;
            const int mapped=Probe::MenuAction(true,action);
            if(mapped>=0) {
                ++routeCalls;
                if(registered && (mapped==19 || mapped==20)) selection=mapped;
                else {++nativeCalls;selection=mapped;}
            }
            Check(routeCalls==((action==19 || action==20)?0:1),"bumpers blocked before loader route");
            if(action==7 || action==8) {
                Check(selection==static_cast<int>(action+12),"trigger reaches route as bumper navigation");
                Check(nativeCalls==(registered?0:1),"custom route and native fallback each execute once");
            }
        }
    }
    Check(Probe::MenuAction(false,19)==19,"disabled remap passes through");
    Check(Probe::SubMenuAction(true,19)==7 && Probe::SubMenuAction(true,7)==-1,"submenu exception preserved");
    std::puts("Menu route admission and upstream remap policy passed.");
}
