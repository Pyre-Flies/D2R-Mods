#include "menu_route_profile.h"
#include "policy.h"
#include <cstdio>
#include <cstdlib>
void Check(bool ok,const char* text) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
int main() {
    Check(Probe::GroundShortcutsAllowed(0,0),"world loot allowed outside menus");
    for(unsigned panel : {1U,2U,4U,8U}) Check(!Probe::GroundShortcutsAllowed(0,panel),"Quest Skills Options Chronicle bumpers cannot queue ground loot");
    Check(!Probe::GroundShortcutsAllowed(1,0),"dedicated stash panel excludes world loot");
    Check(Probe::SubPanelBit("ChroniclePanel")==8,"Chronicle lifecycle isolated");
    Check(Probe::ChronicleTab("ChronicleTabs","ChroniclePanel",true),"verified Chronicle widget admitted");
    Check(!Probe::ChronicleTab("OtherTabs","ChroniclePanel",true) && !Probe::ChronicleTab("ChronicleTabs","SettingsPanel",true) && !Probe::ChronicleTab("ChronicleTabs","ChroniclePanel",false),"unrelated and hidden Chronicle widgets untouched");
    Check(Probe::SubMenuAction(true,19)==7 && Probe::SubMenuAction(true,20)==8 && Probe::SubMenuAction(true,7)==-1 && Probe::SubMenuAction(true,8)==-1,"Chronicle inner bumpers and outer triggers");
    Check(Probe::SubPanelBit("SettingsPanel")==4,"settings lifecycle classified");
    Check(Probe::OptionsTab("OptionsTabs","SettingsPanel",true),"verified settings widget admitted");
    Check(!Probe::OptionsTab("OtherTabs","SettingsPanel",true) && !Probe::OptionsTab("OptionsTabs","OtherPanel",true) && !Probe::OptionsTab("OptionsTabs","SettingsPanel",false),"unrelated and hidden widgets untouched");
    Check(Probe::SubMenuAction(true,19)==7 && Probe::SubMenuAction(true,20)==8,"Options bumpers route to inner tabs");
    Check(Probe::SubMenuAction(true,7)==-1 && Probe::SubMenuAction(true,8)==-1,"Options triggers released to outer menu");
    Check(Probe::SubMenuAction(true,0)==0 && Probe::SubMenuAction(false,19)==19,"other controls and disabled mode preserved");
    Check(Probe::RecoverControllerLabels(true,true,true,1,0),"stale display recovered");
    Check(!Probe::RecoverControllerLabels(true,true,true,1,1),"healthy display not rebuilt");
    Check(!Probe::RecoverControllerLabels(true,true,false,1,0),"menu suppression retained");
    Check(!Probe::RecoverControllerLabels(true,false,true,1,0),"keyboard mode untouched");
    Check(!Probe::RecoverControllerLabels(false,true,true,1,0),"disabled recovery untouched");
    Check(!Probe::RecoverControllerLabels(true,true,true,-1,0) && !Probe::RecoverControllerLabels(true,true,true,1,2),"unknown state refused");
    Check(Probe::LockFilteredLabels(true,true,true,true,true,0),"controller filtered labels locked");
    Check(!Probe::LockFilteredLabels(true,true,true,true,false,0),"keyboard Alt press/release pass through");
    Check(!Probe::LockFilteredLabels(true,true,true,true,true,1),"unfiltered channel untouched");
    Check(!Probe::LockFilteredLabels(true,true,true,false,true,0),"out of game native labels preserved");
    for(bool controller : {true,false,true,false})
        Check(Probe::LockFilteredLabels(true,true,true,true,controller,0)==controller,"input mode transitions update ownership");
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
