#include <D2RLPlugin/api.h>
#include <D2RLPlugin/resource.h>
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <initializer_list>
void Check(bool ok,const char* name) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",name);std::exit(1);}}
D2RL::ConsoleCommandRegistration registered{};
bool __cdecl Register(const D2RL::PluginContext*,const D2RL::ConsoleCommandRegistration* command) noexcept {registered=*command;return true;}
int wmain(int argc,wchar_t** argv) {
    Check(argc==2,"DLL path");
    auto module=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    Check(module!=nullptr,"resource load");
    const auto resource=FindResourceW(module,MAKEINTRESOURCEW(D2RL_PLUGIN_MANIFEST_RESOURCE_ID),RT_RCDATA);
    Check(resource && SizeofResource(module,resource)==sizeof(DWORD),"one DWORD manifest");
    const auto abi=static_cast<const DWORD*>(LockResource(LoadResource(module,resource)));
    Check(abi && *abi==D2RL_PLUGIN_ABI_VERSION,"SDK ABI manifest");
    const auto config=FindResourceW(module,MAKEINTRESOURCEW(D2RL_PLUGIN_CONFIG_RESOURCE_ID),RT_RCDATA);
    Check(config!=nullptr,"embedded configuration exists");
    const auto configText=static_cast<const char*>(LockResource(LoadResource(module,config)));
    Check(configText && std::string_view(configText,SizeofResource(module,config)).find("inserted_item_scale_percent = 65")!=std::string_view::npos,"65 percent embedded default");
    Check(std::string_view(configText,SizeofResource(module,config)).find("inserted_item_opacity_percent = 65")!=std::string_view::npos,"65 percent opacity embedded default");
    Check(std::string_view(configText,SizeofResource(module,config)).find("inserted_item_lift_pixels = 1")!=std::string_view::npos,"one-pixel inserted artwork lift embedded default");
    Check(std::string_view(configText,SizeofResource(module,config)).find("ring_opacity_percent = 80")!=std::string_view::npos,"80 percent ring embedded default");
    Check(std::string_view(configText,SizeofResource(module,config)).find("ring_center_opacity_percent = 40")!=std::string_view::npos,"40 percent center embedded default");
    FreeLibrary(module);
    module=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    Check(module!=nullptr,"normal load");
    const auto info=reinterpret_cast<D2RL::GetPluginInfoFn>(GetProcAddress(module,"D2RLoaderGetPluginInfo"));
    const auto load=reinterpret_cast<D2RL::LoadPluginFn>(GetProcAddress(module,"D2RLoaderLoadPlugin"));
    const auto unload=reinterpret_cast<D2RL::UnloadPluginFn>(GetProcAddress(module,"D2RLoaderUnloadPlugin"));
    Check(info && load && unload,"three lifecycle exports");
    const auto value=info();
    Check(value && value->infoSize==D2RL::PluginInfoSize && value->abiVersion==D2RL_PLUGIN_ABI_VERSION &&
        std::strcmp(value->id,"always-show-sockets")==0 && std::strcmp(value->version,"1.3.1+rev.2")==0 &&
        D2RL::HasValidPluginRole(value->flags) && D2RL::HasFlag(value->flags,D2RL::PluginFlags::NativeHooks),"identity/layout/version/role and native-hook capability");
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize,.registerConsoleCommand=Register};
    const D2RL::PluginContext context{.contextSize=sizeof(D2RL::PluginContext),.abiVersion=D2RL_PLUGIN_ABI_VERSION,.api=&api};
    Check(load(&context) && registered.callback && registered.scope==D2RL::ConsoleCommandScope::LocalOnly,"unsupported fixture remains inert with local command");
    for(const char* mode:{"on","always","default"}) {
        const D2RL::ConsoleCommandContext command{.contextSize=D2RL::ConsoleCommandContextSize,.argsLength=static_cast<std::uint32_t>(std::strlen(mode)),.args=mode,.plugin=&context};
        Check(registered.callback(nullptr,&command,nullptr)==D2RL::ConsoleCommandResult::Failed,"unsupported fixture cannot enable any socket display mode");
    }
    unload();FreeLibrary(module);std::puts("Socket probe manifest/exports/metadata and unsupported-build refusal pass.");
}
