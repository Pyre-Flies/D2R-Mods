#include <D2RLPlugin/api.h>
#include <D2RLPlugin/resource.h>
#include <windows.h>
#include "test.h"
#include "hash.h"
#include "settings.h"
#include <cstring>

int wmain(int argc,wchar_t** argv) {
    Check(argc==2 || argc==3,"DLL path and optional expected SHA-256");
    auto module=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    Check(module!=nullptr,"resource-only DLL load");
    const auto manifest=FindResourceW(module,MAKEINTRESOURCEW(D2RL_PLUGIN_MANIFEST_RESOURCE_ID),RT_RCDATA);
    Check(manifest && SizeofResource(module,manifest)==sizeof(DWORD),"exact DWORD ABI manifest");
    const auto abi=static_cast<const DWORD*>(LockResource(LoadResource(module,manifest)));
    Check(abi && *abi==D2RL_PLUGIN_ABI_VERSION,"manifest ABI matches SDK");
    Check(FindResourceW(module,MAKEINTRESOURCEW(1),RT_VERSION)!=nullptr,"VERSIONINFO");
    const auto config=FindResourceW(module,MAKEINTRESOURCEW(D2RL_PLUGIN_CONFIG_RESOURCE_ID),RT_RCDATA);
    Check(config!=nullptr,"embedded default configuration");
    const auto defaults=static_cast<const char*>(LockResource(LoadResource(module,config)));
    const auto defaultSize=SizeofResource(module,config);
    Check(defaults && defaultSize && DayNight::ParseConfig(std::string_view(defaults,defaultSize))==DayNight::Settings{},
        "embedded default starts enabled and parses safely");
    FreeLibrary(module);
    module=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    Check(module!=nullptr,"normal DLL load");
    if(argc==3) {
        char expected[65]{};
        Check(wcslen(argv[2])==64,"expected SHA-256 length");
        for(unsigned i=0;i<64;++i) expected[i]=static_cast<char>(argv[2][i]);
        Check(DayNight::FileHash(module,expected),"runtime BCrypt file hash matches independent SHA-256");
        expected[0]=expected[0]=='0'?'1':'0';
        Check(!DayNight::FileHash(module,expected),"wrong file hash refuses");
    }
    const auto info=reinterpret_cast<D2RL::GetPluginInfoFn>(GetProcAddress(module,"D2RLoaderGetPluginInfo"));
    const auto load=reinterpret_cast<D2RL::LoadPluginFn>(GetProcAddress(module,"D2RLoaderLoadPlugin"));
    const auto unload=reinterpret_cast<D2RL::UnloadPluginFn>(GetProcAddress(module,"D2RLoaderUnloadPlugin"));
    Check(info && load && unload,"three loader exports");
    const auto value=info();
    Check(value && value->infoSize==D2RL::PluginInfoSize && value->abiVersion==D2RL_PLUGIN_ABI_VERSION &&
        !std::strcmp(value->id,"day-night-cycle") && !std::strcmp(value->name,"Day Night Cycle") &&
        !std::strcmp(value->version,"1.3.1+rev.1") && D2RL::HasValidPluginRole(value->flags) &&
        D2RL::PluginRoleValue(value->flags)==D2RL::FlagsValue(D2RL::PluginFlags::Client) &&
        D2RL::HasFlag(value->flags,D2RL::PluginFlags::NativeHooks),"metadata, ABI and client role");
    Check(!load(nullptr),"null context refusal");unload();
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize};
    D2RL::PluginContext context{.contextSize=sizeof(D2RL::PluginContext),.abiVersion=D2RL_PLUGIN_ABI_VERSION,
        .api=&api,.exeBase=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))};
    Check(load(&context),"unsupported executable remains inert");unload();
    context.abiVersion=0;Check(!load(&context),"wrong ABI refusal");
    context.abiVersion=D2RL_PLUGIN_ABI_VERSION;context.contextSize=8;
    Check(!load(&context),"short context refusal");unload();
    FreeLibrary(module);
    std::puts("PASS: ABI resource, exports, metadata, client role, context guards and inert unsupported build.");
}
