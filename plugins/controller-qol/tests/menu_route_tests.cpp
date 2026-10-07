#include "menu_route_profile.h"
#include "policy.h"
#include <cstdio>
#include <cstdlib>
void Check(bool ok,const char* text) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
int main() {
    Check(!Probe::GroundLootHooksRequired(false,false),"ground hooks remain off when both loot features are disabled");
    Check(Probe::GroundLootHooksRequired(true,false),"direct ground pickup enables ground hooks");
    Check(Probe::GroundLootHooksRequired(false,true),"filtered A-button blocking independently enables ground hooks");
    Check(!Probe::BlockNativePickup(false,true,false,false),"disabled pickup policies do not block native pickup");
    Check(!Probe::BlockNativePickup(false,true,true,true),"visible filtered-policy item remains pickable");
    Check(Probe::BlockNativePickup(false,false,true,false),"hidden item is blocked without enabling shortcuts");
    Check(!Probe::BlockNativePickup(false,true,true,true),"modifier alone cannot activate a disabled shortcut");
    Check(Probe::BlockNativePickup(true,true,false,true),"enabled shortcut suppresses native pickup while held");
    Check(Probe::BatchFeatureEnabled(true,true),"enabled batch feature follows enabled master");
    Check(!Probe::BatchFeatureEnabled(true,false),"feature gate disables batch action");
    Check(!Probe::BatchFeatureEnabled(false,true),"master gate disables batch action");
    Check(Probe::GroundShortcutsEnabled(true,true),"enabled direct ground pickup permits shortcuts");
    Check(!Probe::GroundShortcutsEnabled(true,false),"filtered A-only mode does not permit shortcuts");
    Check(!Probe::GroundShortcutsEnabled(false,true),"disabled plugin does not permit shortcuts");
    Check(Probe::LootFilterTab("ItemListTabs","LootFilterRuleDetailsPanel",true),"exact rule editor tab");
    Check(!Probe::LootFilterTab("OtherTabs","LootFilterRuleDetailsPanel",true) && !Probe::LootFilterTab("ItemListTabs","OtherPanel",true) && !Probe::LootFilterTab("ItemListTabs","LootFilterRuleDetailsPanel",false),"unrelated and hidden tabs excluded");
    for(unsigned mask : {16U,32U,64U}) Check(!Probe::GroundShortcutsAllowed(0,mask),"loot editor excludes world shortcuts");

    Check(Probe::GroundShortcutsAllowed(0,0),"world loot allowed outside menus");
    Check(Probe::WorldGroundInput(true,true,0,0,0),"live HUD owns ground chords");
    Check(!Probe::WorldGroundInput(false,true,0,0,0) && !Probe::WorldGroundInput(true,false,0,0,0),"unknown/left session fails open");
    for(const auto name:{"PlayerInventoryExpansionLayout","PlayerInventoryOriginalLayout","CharacterStatsPanel","NpcDialogPanel","HireMenuPanel"})
        Check(!Probe::WorldGroundInput(true,true,0,0,Probe::WorldBlockingPanelBit(name)),"inventory/dialog A remains native");
    Check(!Probe::WorldGroundInput(true,true,1,0,0) && !Probe::WorldGroundInput(true,true,0,4,0),"stash and settings A remain native");
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
    Check(Probe::EnterFilterItems(true,true,13),"focused filter Down enters lower section");
    Check(!Probe::EnterFilterItems(false,true,13) && !Probe::EnterFilterItems(true,false,13),"disabled or unfocused input passes through");
    for(unsigned action : {0u,7u,8u,12u,16u,19u,20u,22u})
        Check(!Probe::EnterFilterItems(true,true,action),"other actions and nested section event pass through");
    Check(Probe::RecoverFilterTabFocus(0,1,true),"Equipment to Items missing focus recovered");
    Check(Probe::RecoverFilterTabFocus(1,0,true),"Items nested rows to Equipment missing focus recovered");
    Check(!Probe::RecoverFilterTabFocus(0,0,true) && !Probe::RecoverFilterTabFocus(1,1,true) &&
          !Probe::RecoverFilterTabFocus(0,1,false) && !Probe::RecoverFilterTabFocus(1,0,false) &&
          !Probe::RecoverFilterTabFocus(~0u,1,true) && !Probe::RecoverFilterTabFocus(0,2,true),"unchanged, healthy and unknown transitions preserved");
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
