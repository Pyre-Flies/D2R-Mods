#include <D2RLPlugin/api.h>
#include <D2RLPlugin/resource.h>
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include "config.h"

namespace {

void Check(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s (Win32=%lu)\n", message, GetLastError());
    std::exit(1);
}

D2RL::ConsoleCommandRegistration registered{};
char lastMessage[512]{};
bool __cdecl ReadConfig(const D2RL::PluginContext*,char* output,std::uint32_t size,std::uint32_t* required) noexcept {
    constexpr char config[]="[floating-damage]\nenabled = false\ndebug = false\n";
    *required=sizeof(config);
    if(size<sizeof(config)) return false;
    std::memcpy(output,config,sizeof(config)); return true;
}
bool __cdecl Register(const D2RL::PluginContext*,const D2RL::ConsoleCommandRegistration* command) noexcept {
    if(!command || command->registrationSize!=sizeof(*command)) return false;
    registered=*command; return true;
}
void __cdecl Message(const D2RL::PluginContext*,const char* message,D2RL::ConsoleMessageKind) noexcept {
    std::snprintf(lastMessage,sizeof(lastMessage),"%s",message);
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    Check(argc == 2, "DLL path required");

    HMODULE resources = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    Check(resources != nullptr, "DLL resources load");
    HRSRC manifest = FindResourceW(resources, MAKEINTRESOURCEW(D2RL_PLUGIN_MANIFEST_RESOURCE_ID), RT_RCDATA);
    Check(manifest != nullptr && SizeofResource(resources, manifest) == sizeof(DWORD), "embedded loader ABI manifest exists");
    const auto* abi = static_cast<const DWORD*>(LockResource(LoadResource(resources, manifest)));
    Check(abi != nullptr && *abi == D2RL_PLUGIN_ABI_VERSION, "resource ABI matches SDK");
    const DWORD manifestAbi = *abi;
    HRSRC version = FindResourceW(resources, MAKEINTRESOURCEW(1), RT_VERSION);
    Check(version != nullptr, "Windows version resource exists");
    const auto versionSize=SizeofResource(resources,version);
    const auto* versionData=static_cast<const unsigned char*>(LockResource(LoadResource(resources,version)));
    Check(versionData != nullptr && versionSize>=sizeof(VS_FIXEDFILEINFO), "Windows version resource is readable");
    bool versionMatches{};
    for(std::size_t offset=0;offset+sizeof(VS_FIXEDFILEINFO)<=versionSize;offset+=sizeof(DWORD)) {
        VS_FIXEDFILEINFO fixed{};std::memcpy(&fixed,versionData+offset,sizeof(fixed));
        if(fixed.dwSignature!=0xFEEF04BD || fixed.dwStrucVersion!=0x10000) continue;
        versionMatches=fixed.dwFileVersionMS==0x00000001 && fixed.dwFileVersionLS==12 &&
            fixed.dwProductVersionMS==fixed.dwFileVersionMS && fixed.dwProductVersionLS==fixed.dwFileVersionLS;
        break;
    }
    Check(versionMatches,"Windows file/product resources identify beta.12 (0.1.0.12)");
    HRSRC config = FindResourceW(resources, MAKEINTRESOURCEW(D2RL_PLUGIN_CONFIG_RESOURCE_ID), RT_RCDATA);
    Check(config != nullptr && SizeofResource(resources, config) > 0 && SizeofResource(resources, config) <= 1024 * 1024,
        "embedded default config exists and is within loader limit");
    const auto configSize = SizeofResource(resources, config);
    const auto* configData = static_cast<const char*>(LockResource(LoadResource(resources, config)));
    Check(configData != nullptr, "embedded default config is readable");
    const std::string_view configText(configData, configSize);
    Check(configText.find("[floating-damage]") != std::string_view::npos &&
        configText.find("lifetime_ms") != std::string_view::npos, "embedded default config has expected TOML sections");
    unsigned rejected{};
    const auto defaults=Damage::Parse(configText,rejected);
    Check(rejected==0 && defaults.enabled && !defaults.debug && defaults.textColor==Damage::Settings{}.textColor &&
        defaults.outlineWidth==2 && defaults.fadeStart==0.8f && defaults.fallingMotion && defaults.shadow &&
        defaults.backing && defaults.backingPoisonOnly && defaults.poisonColor==Damage::Settings{}.poisonColor &&
        defaults.mixedColor==Damage::Settings{}.mixedColor && defaults.criticalHits && defaults.criticalScale==1.35f &&
        defaults.resolutionScaling && defaults.criticalStyle==Damage::CriticalStyle::Both &&
        configText.find("resolution_scaling")!=std::string_view::npos && configText.find("critical_style")!=std::string_view::npos &&
        configText.find("\"#D5DAE2\"")!=std::string_view::npos,
        "embedded defaults include resolution scaling, combined critical styling, diagnostics off and poison backing");
    FreeLibrary(resources);

    HMODULE dll = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    Check(dll != nullptr, "DLL loads with Windows dependencies");
    using GetInfo = const D2RL::PluginInfo*(__cdecl*)() noexcept;
    const auto getInfo = reinterpret_cast<GetInfo>(GetProcAddress(dll, "D2RLoaderGetPluginInfo"));
    Check(getInfo != nullptr && GetProcAddress(dll, "D2RLoaderLoadPlugin") != nullptr &&
        GetProcAddress(dll, "D2RLoaderUnloadPlugin") != nullptr, "loader exports exist");
    const auto* info = getInfo();
    Check(info != nullptr && info->infoSize == D2RL::PluginInfoSize && info->abiVersion == manifestAbi,
        "metadata layout and ABI match manifest");
    Check(std::strcmp(info->id, "floating-damage") == 0 && std::strcmp(info->version, "0.1.0-beta.12") == 0 &&
        D2RL::HasValidPluginRole(info->flags), "plugin identity, version, and role are valid");
    Check((info->flags&D2RL::PluginFlags::NativeHooks)!=D2RL::PluginFlags::None,"combat observer declares required native-hook capability");
    const auto load=reinterpret_cast<D2RL::LoadPluginFn>(GetProcAddress(dll,"D2RLoaderLoadPlugin"));
    const auto unload=reinterpret_cast<D2RL::UnloadPluginFn>(GetProcAddress(dll,"D2RLoaderUnloadPlugin"));
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize,.readConfig=ReadConfig,.registerConsoleCommand=Register,.writeConsoleMessage=Message};
    const D2RL::PluginContext context{.contextSize=sizeof(D2RL::PluginContext),.abiVersion=D2RL_PLUGIN_ABI_VERSION,.api=&api};
    // This test process has no admitted game/Core. Exercise the real exported
    // registration and callback without bypassing any native compatibility gate.
    Check(load(&context),"unsupported fixture loads inert with local command registered");
    Check(registered.callback && std::strcmp(registered.name,"damagenumbers")==0 &&
        registered.scope==D2RL::ConsoleCommandScope::LocalOnly && registered.usage,"command is registered locally with usage");
    D2RL::ConsoleCommandContext command{.contextSize=D2RL::ConsoleCommandContextSize,.argsLength=2,.args="on",.plugin=&context};
    Check(registered.callback(nullptr,&command,nullptr)==D2RL::ConsoleCommandResult::Failed &&
        std::string_view(lastMessage).find("unavailable")!=std::string_view::npos,"command cannot enable an unsupported native build");
    command.args="on off"; command.argsLength=6;
    Check(registered.callback(nullptr,&command,nullptr)==D2RL::ConsoleCommandResult::InvalidArguments &&
        std::string_view(lastMessage).find("Usage:")!=std::string_view::npos,"real callback rejects malformed arguments and reports usage");
    command.args=nullptr; command.argsLength=2;
    Check(registered.callback(nullptr,&command,nullptr)==D2RL::ConsoleCommandResult::Failed,"null bounded arguments are refused");
    unload();
    FreeLibrary(dll);

    std::puts("DLL resources/metadata, debug-off style defaults, local command callback and unsupported-build refusal pass.");
    return 0;
}
