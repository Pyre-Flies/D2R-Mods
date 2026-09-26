#include <D2RLPlugin/api.h>
#include <D2RLPlugin/resource.h>
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
    if (condition) return;
    std::fprintf(stderr, "FAIL: %s (Win32=%lu)\n", message, GetLastError());
    std::exit(1);
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
    HRSRC config = FindResourceW(resources, MAKEINTRESOURCEW(D2RL_PLUGIN_CONFIG_RESOURCE_ID), RT_RCDATA);
    Check(config != nullptr && SizeofResource(resources, config) > 0 && SizeofResource(resources, config) <= 1024 * 1024,
        "embedded default config exists and is within loader limit");
    const auto configSize = SizeofResource(resources, config);
    const auto* configData = static_cast<const char*>(LockResource(LoadResource(resources, config)));
    Check(configData != nullptr, "embedded default config is readable");
    const std::string_view configText(configData, configSize);
    Check(configText.find("[map-assistance]") != std::string_view::npos &&
        configText.find("[[zones]]") != std::string_view::npos, "embedded default config has expected TOML sections");
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
    Check(std::strcmp(info->id, "map-assistance") == 0 && std::strcmp(info->version, "1.3.1+rev.1") == 0 &&
        D2RL::HasValidPluginRole(info->flags), "plugin identity, version, and role are valid");
    FreeLibrary(dll);

    std::puts("Built DLL: loader ABI manifest, embedded TOML, exports, metadata, version, and role agree.");
    return 0;
}
