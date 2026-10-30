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

bool __cdecl ReadConfig(const D2RL::PluginContext*, char* output, std::uint32_t size, std::uint32_t* required) noexcept {
    constexpr char config[] = "[running-defense]\nenabled = true\ndefense_loss_percent = 30\n";
    *required = sizeof(config);
    if (size < sizeof(config)) return false;
    std::memcpy(output, config, sizeof(config));
    return true;
}

bool __cdecl Register(const D2RL::PluginContext*, const D2RL::ConsoleCommandRegistration* command) noexcept {
    if (!command || command->registrationSize != sizeof(*command)) return false;
    registered = *command;
    return true;
}

void __cdecl Message(const D2RL::PluginContext*, const char* message, D2RL::ConsoleMessageKind) noexcept {
    std::snprintf(lastMessage, sizeof(lastMessage), "%s", message ? message : "");
}

void __cdecl LogMessage(const D2RL::PluginContext*, const char*) noexcept {}

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
    const auto versionSize = SizeofResource(resources, version);
    const auto* versionData = static_cast<const unsigned char*>(LockResource(LoadResource(resources, version)));
    Check(versionData != nullptr && versionSize >= sizeof(VS_FIXEDFILEINFO), "Windows version resource is readable");
    bool versionMatches = false;
    for (std::size_t offset = 0; offset + sizeof(VS_FIXEDFILEINFO) <= versionSize; offset += sizeof(DWORD)) {
        VS_FIXEDFILEINFO fixed{};
        std::memcpy(&fixed, versionData + offset, sizeof(fixed));
        if (fixed.dwSignature != 0xFEEF04BD || fixed.dwStrucVersion != 0x10000) continue;
        versionMatches = fixed.dwFileVersionMS == 0x00010003 && fixed.dwFileVersionLS == 0x00010001 &&
                         fixed.dwProductVersionMS == fixed.dwFileVersionMS &&
                         fixed.dwProductVersionLS == fixed.dwFileVersionLS;
        break;
    }
    Check(versionMatches, "Windows file/product resources identify 1.3.1.1");

    HRSRC config = FindResourceW(resources, MAKEINTRESOURCEW(D2RL_PLUGIN_CONFIG_RESOURCE_ID), RT_RCDATA);
    Check(config != nullptr && SizeofResource(resources, config) > 0 && SizeofResource(resources, config) <= 1024 * 1024,
        "embedded default config exists and is within loader limit");
    const auto configSize = SizeofResource(resources, config);
    const auto* configData = static_cast<const char*>(LockResource(LoadResource(resources, config)));
    Check(configData != nullptr, "embedded default config is readable");
    const std::string_view configText(configData, configSize);
    Check(configText.find("[running-defense]") != std::string_view::npos &&
          configText.find("defense_loss_percent") != std::string_view::npos &&
          configText.find("show_running_icon_on_sheet") != std::string_view::npos,
        "embedded default config has expected TOML keys");

    unsigned rejected = 0;
    const auto defaults = RunningDefense::ParseConfig(configText, rejected);
    Check(rejected == 0 && defaults.enabled && defaults.defenseLossPercent == 30 &&
          !defaults.overrideBlockPenalty && defaults.blockLossPercent == 67 &&
          defaults.showRunningIconOnSheet && defaults.highlightDefenseWhenRunning,
        "embedded default config parses cleanly with expected defaults");
    FreeLibrary(resources);

    HMODULE dll = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    Check(dll != nullptr, "DLL loads with Windows dependencies");

    using GetInfo = const D2RL::PluginInfo*(__cdecl*)() noexcept;
    const auto getInfo = reinterpret_cast<GetInfo>(GetProcAddress(dll, "D2RLoaderGetPluginInfo"));
    Check(getInfo != nullptr && GetProcAddress(dll, "D2RLoaderLoadPlugin") != nullptr &&
          GetProcAddress(dll, "D2RLoaderUnloadPlugin") != nullptr,
        "loader exports exist");

    const auto* info = getInfo();
    Check(info != nullptr && info->infoSize == D2RL::PluginInfoSize && info->abiVersion == manifestAbi,
        "metadata layout and ABI match manifest");
    Check(std::strcmp(info->id, "running-defense") == 0 && std::strcmp(info->version, "1.3.1+rev.1") == 0 &&
          D2RL::HasValidPluginRole(info->flags),
        "plugin identity, version, and role are valid");
    Check((info->flags & D2RL::PluginFlags::NativeHooks) != D2RL::PluginFlags::None,
        "plugin declares required native-hook capability");

    const auto load = reinterpret_cast<D2RL::LoadPluginFn>(GetProcAddress(dll, "D2RLoaderLoadPlugin"));
    const auto unload = reinterpret_cast<D2RL::UnloadPluginFn>(GetProcAddress(dll, "D2RLoaderUnloadPlugin"));
    const D2RL::PluginApi api{
        .apiSize = D2RL::PluginApiSize,
        .logInfo = LogMessage,
        .logWarn = LogMessage,
        .logError = LogMessage,
        .readConfig = ReadConfig,
        .registerConsoleCommand = Register,
        .writeConsoleMessage = Message,
    };
    const D2RL::PluginContext context{
        .contextSize = sizeof(D2RL::PluginContext),
        .abiVersion = D2RL_PLUGIN_ABI_VERSION,
        .api = &api,
    };

    Check(load(&context), "unsupported host loads fail-open (inert) with local command registered");
    Check(registered.callback != nullptr && std::strcmp(registered.name, "runningdefense") == 0,
        "runningdefense console command registered");

    const D2RL::ConsoleCommandContext statusCmd{
        .contextSize = D2RL::ConsoleCommandContextSize,
        .argsLength = 6,
        .args = "status",
        .plugin = &context,
    };
    Check(registered.callback(nullptr, &statusCmd, nullptr) == D2RL::ConsoleCommandResult::Failed,
        "inert host reports status as not admitted");
    Check( std::strstr(lastMessage, "native=inert") != nullptr && std::strstr(lastMessage, "defense_loss=30%") != nullptr,
        "status command output includes admission state and defense_loss_percent");

    unload();
    FreeLibrary(dll);
    return 0;
}
