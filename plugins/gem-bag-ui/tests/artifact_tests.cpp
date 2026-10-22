#include <D2RLPlugin/api.h>
#include <D2RLPlugin/resource.h>
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
std::vector<std::string> warnings;
void __cdecl Warn(const D2RL::PluginContext*,const char* text) noexcept {warnings.emplace_back(text);std::puts(text);}
bool HasWarning(const char* text){for(const auto& line:warnings)if(line.find(text)!=std::string::npos)return true;return false;}
void Check(bool b,const char* label){if(!b){std::fprintf(stderr,"FAIL %s\n",label);std::exit(1);}}
int wmain(int argc,wchar_t** argv) {
    Check(argc==2,"DLL path");
    auto m=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);Check(m!=nullptr,"resource load");
    auto r=FindResourceW(m,MAKEINTRESOURCEW(D2RL_PLUGIN_MANIFEST_RESOURCE_ID),RT_RCDATA);
    Check(r&&SizeofResource(m,r)==sizeof(DWORD),"ABI manifest size");
    auto abi=static_cast<const DWORD*>(LockResource(LoadResource(m,r)));Check(abi&&*abi==D2RL_PLUGIN_ABI_VERSION,"ABI manifest value");
    Check(FindResourceW(m,MAKEINTRESOURCEW(1),RT_VERSION)!=nullptr,"VERSIONINFO");FreeLibrary(m);
    m=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);Check(m!=nullptr,"normal load");
    auto info=reinterpret_cast<D2RL::GetPluginInfoFn>(GetProcAddress(m,"D2RLoaderGetPluginInfo"));
    auto load=reinterpret_cast<D2RL::LoadPluginFn>(GetProcAddress(m,"D2RLoaderLoadPlugin"));
    auto unload=reinterpret_cast<D2RL::UnloadPluginFn>(GetProcAddress(m,"D2RLoaderUnloadPlugin"));
    Check(info&&load&&unload,"lifecycle exports");auto p=info();
    Check(p&&p->infoSize==D2RL::PluginInfoSize&&p->abiVersion==D2RL_PLUGIN_ABI_VERSION&&!std::strcmp(p->id,"gem-bag-ui")&&
        !std::strcmp(p->version,"1.3.1+rev.1")&&D2RL::HasValidPluginRole(p->flags)&&D2RL::HasFlag(p->flags,D2RL::PluginFlags::Shared)&&
        D2RL::HasFlag(p->flags,D2RL::PluginFlags::NativeHooks),"plugin identity ABI and role");
    Check(!D2RL::HasFlag(p->flags,D2RL::PluginFlags::ModScopedOnly),"global installation is allowed by plugin metadata");
    Check(!load(nullptr),"null context rejected");
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize,.logWarn=Warn};
    const D2RL::PluginContext c{.contextSize=sizeof(D2RL::PluginContext),.abiVersion=D2RL_PLUGIN_ABI_VERSION,.api=&api,.loadScope=D2RL::LoadScope::Global,.exeBase=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))};
    Check(load(&c),"unknown process stays inert");
    Check(HasWarning("[GemBag/Admission] file=D2RLoader.exe SHA-256 mismatch")&&HasWarning("module=D2RCore.dll unavailable")&&
        !HasWarning("check=active-mod")&&HasWarning("Selector disabled"),"real DLL names native admission failures without requiring a named mod");
    unload();FreeLibrary(m);
    std::puts("Gem Bag UI artifact admitted; unsupported process stayed inert.");
}
