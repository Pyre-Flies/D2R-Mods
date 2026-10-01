#include <D2RLPlugin/api.h>
#include <D2RLPlugin/resource.h>
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include "aim/aim_config.h"

static void check(bool ok,const char* message) {
    if(!ok) {std::fprintf(stderr,"FAIL: %s (Win32=%lu)\n",message,GetLastError());std::exit(1);}
}
int main(int argc,char** argv) {
    check(argc==3,"DLL path and expected version required");
    HMODULE dll=LoadLibraryExA(argv[1],nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
    check(dll!=nullptr,"merged DLL loads with Windows dependencies");
    using GetInfo=const D2RL::PluginInfo*(*)() noexcept;
    auto getInfo=reinterpret_cast<GetInfo>(GetProcAddress(dll,"D2RLoaderGetPluginInfo"));
    check(getInfo!=nullptr,"metadata export exists");
    const auto* info=getInfo();
    check(info && info->infoSize==D2RL::PluginInfoSize,"metadata layout matches SDK");
    check(info->abiVersion==D2RL_PLUGIN_ABI_VERSION,"metadata ABI matches SDK");
    check(std::strcmp(info->id,"controller-qol-updates")==0 && std::strcmp(info->name,"Controller QOL Updates")==0,"single QOL identity");
    check(std::strcmp(info->version,argv[2])==0,"merged version");
    check(std::strcmp(info->author,"PyreFly")==0,"release author credit");
    check(GetProcAddress(dll,"D2RLoaderLoadPlugin") && GetProcAddress(dll,"D2RLoaderUnloadPlugin"),"lifecycle exports exist");
    HRSRC manifest=FindResourceA(dll,MAKEINTRESOURCEA(D2RL_PLUGIN_MANIFEST_RESOURCE_ID),MAKEINTRESOURCEA(10));
    check(manifest && SizeofResource(dll,manifest)==sizeof(DWORD),"embedded ABI manifest exists");
    auto abi=static_cast<const DWORD*>(LockResource(LoadResource(dll,manifest)));
    check(abi && *abi==D2RL_PLUGIN_ABI_VERSION,"resource ABI matches metadata");
    HRSRC resource=FindResourceA(dll,MAKEINTRESOURCEA(D2RL_PLUGIN_CONFIG_RESOURCE_ID),MAKEINTRESOURCEA(10));
    check(resource!=nullptr,"one embedded configuration exists");
    auto data=static_cast<const char*>(LockResource(LoadResource(dll,resource)));
    check(data!=nullptr,"configuration is readable");
    std::string_view config(data,SizeofResource(dll,resource));
    check(config.find("[qol]")!=config.npos,"configuration section is QOL");
    check(config.find("modifier = \"bumper\"")!=config.npos,"identify/move default uses L1");
    check(config.find("ground_pickup_button = \"bumper\"")!=config.npos,"ground-loot default uses L1");
    check(config.find("quick_identify = true")!=config.npos && config.find("quick_move = true")!=config.npos,"item actions remain enabled");
    check(config.find("debug_logging = false")!=config.npos,"release debug logging defaults off");
    check(config.find("prioritize_portals = true")!=config.npos,"portal priority defaults on");
    check(config.find("portal_priority_distance = 10")!=config.npos,"portal priority range defaults to ten");
    check(config.find("portal_diagnostics = false")!=config.npos,"portal diagnostics default off");
    Aim::MotionSettings aim{}; bool aimEnabled=true;
    check(Aim::ParseQolSettings(config,aim,aimEnabled) && aimEnabled,"embedded aim settings parse and default on");
    check(aim.whirlwindPassThrough && aim.whirlwindPassThroughDistance==1.5f && !aim.debugOverlay,"merged aim defaults preserve reviewed behavior");
    check(!GetProcAddress(dll,"D2RControllerAimOwnsGuidedArrowV1"),"prototype ownership export replaced by internal interface");
    // Do not call the plugin load export: this process has no game or loader services.
    FreeLibrary(dll);
    std::puts("Merged DLL identity, exports, ABI, dependencies, and embedded defaults passed.");
}
