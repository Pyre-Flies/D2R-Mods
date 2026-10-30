#include <D2RLPlugin/api.h>
#include <D2RLPlugin/resource.h>
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

namespace {
void Check(bool ok, const char* name) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", name);
        std::exit(1);
    }
}

D2RL::ConsoleCommandRegistration g_registered{};
bool __cdecl RegisterConsole(const D2RL::PluginContext*,
                             const D2RL::ConsoleCommandRegistration* command) noexcept {
    g_registered = *command;
    return true;
}
} // namespace

int wmain(int argc, wchar_t** argv) {
    Check(argc == 2, "DLL path argument");

    auto module = LoadLibraryExW(
        argv[1], nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    Check(module != nullptr, "resource load");

    const auto manifestRes = FindResourceW(
        module, MAKEINTRESOURCEW(D2RL_PLUGIN_MANIFEST_RESOURCE_ID), RT_RCDATA);
    Check(manifestRes && SizeofResource(module, manifestRes) == sizeof(DWORD),
          "one DWORD ABI manifest");
    const auto* abi = static_cast<const DWORD*>(LockResource(LoadResource(module, manifestRes)));
    Check(abi && *abi == D2RL_PLUGIN_ABI_VERSION, "SDK ABI manifest version");

    const auto configRes = FindResourceW(
        module, MAKEINTRESOURCEW(D2RL_PLUGIN_CONFIG_RESOURCE_ID), RT_RCDATA);
    Check(configRes != nullptr, "embedded TOML config resource exists");
    const auto* configText = static_cast<const char*>(LockResource(LoadResource(module, configRes)));
    const std::string_view cfg(configText ? configText : "", SizeofResource(module, configRes));
    Check(cfg.find("[pyre-lootfilter]") != std::string_view::npos, "embedded section header");
    Check(cfg.find("enabled = true") != std::string_view::npos, "embedded enabled default");
    Check(cfg.find("auto_reload = true") != std::string_view::npos, "embedded auto_reload default");
    Check(cfg.find("alt_pickup_override = true") != std::string_view::npos,
          "embedded alt_pickup_override default");
    Check(cfg.find("minimap_icons = true") != std::string_view::npos,
          "embedded minimap_icons default");
    Check(cfg.find("drop_sounds = true") != std::string_view::npos,
          "embedded drop_sounds default");
    FreeLibrary(module);

    module = LoadLibraryExW(
        argv[1], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    Check(module != nullptr, "normal DLL load");

    const auto getInfo = reinterpret_cast<D2RL::GetPluginInfoFn>(
        GetProcAddress(module, "D2RLoaderGetPluginInfo"));
    const auto loadPlugin = reinterpret_cast<D2RL::LoadPluginFn>(
        GetProcAddress(module, "D2RLoaderLoadPlugin"));
    const auto unloadPlugin = reinterpret_cast<D2RL::UnloadPluginFn>(
        GetProcAddress(module, "D2RLoaderUnloadPlugin"));
    Check(getInfo && loadPlugin && unloadPlugin, "three lifecycle exports");

    const auto* info = getInfo();
    Check(info && info->infoSize == D2RL::PluginInfoSize &&
              info->abiVersion == D2RL_PLUGIN_ABI_VERSION &&
              std::strcmp(info->id, "pyre-lootfilter") == 0 &&
              std::strcmp(info->version, "1.3.1+rev.2") == 0 &&
              D2RL::HasValidPluginRole(info->flags) &&
              D2RL::HasFlag(info->flags, D2RL::PluginFlags::NativeHooks),
          "PluginInfo identity, version, role, and native-hook capability");

    const D2RL::PluginApi api{
        .apiSize = D2RL::PluginApiSize,
        .registerConsoleCommand = &RegisterConsole,
    };
    const D2RL::PluginContext context{
        .contextSize = sizeof(D2RL::PluginContext),
        .abiVersion = D2RL_PLUGIN_ABI_VERSION,
        .api = &api,
    };
    Check(loadPlugin(&context) && g_registered.callback &&
              std::strcmp(g_registered.name, "filter") == 0 &&
              g_registered.scope == D2RL::ConsoleCommandScope::LocalOnly,
          "unsupported fixture remains inert and registers /filter console command");

    for (const char* sub : {"status", "reload", "on", "off"}) {
        const D2RL::ConsoleCommandContext cmd{
            .contextSize = D2RL::ConsoleCommandContextSize,
            .argsLength = static_cast<std::uint32_t>(std::strlen(sub)),
            .args = sub,
            .plugin = &context,
        };
        Check(g_registered.callback(nullptr, &cmd, nullptr) == D2RL::ConsoleCommandResult::Failed,
              "unsupported fixture refuses commands while inactive");
    }

    unloadPlugin();
    FreeLibrary(module);
    std::puts("pyre-lootfilter artifact manifest, exports, metadata, and fail-open refusal pass.");
    return 0;
}
