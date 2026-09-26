#include <windows.h>
#include <D2RLPlugin/lifecycle.h>
#include <D2RLPlugin/resource.h>
#include <cstdio>
#include <cstring>
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 1;
    HMODULE module = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    if (!module) return 2;
    auto resource = FindResourceW(module, MAKEINTRESOURCEW(D2RL_PLUGIN_MANIFEST_RESOURCE_ID), RT_RCDATA);
    if (!resource || SizeofResource(module, resource) != sizeof(DWORD)) return 3;
    auto memory = LoadResource(module, resource);
    auto data = memory ? LockResource(memory) : nullptr;
    DWORD abi{};
    if (!data) return 4;
    std::memcpy(&abi, data, sizeof(abi));
    FreeLibrary(module);
    if (abi != D2RL_PLUGIN_ABI_VERSION) return 5;
    module = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!module) return 6;
    using InfoFn = const D2RL::PluginInfo*(__cdecl*)();
    auto getInfo = reinterpret_cast<InfoFn>(GetProcAddress(module, "D2RLoaderGetPluginInfo"));
    if (!getInfo || !GetProcAddress(module, "D2RLoaderLoadPlugin") || !GetProcAddress(module, "D2RLoaderUnloadPlugin")) return 7;
    auto info = getInfo();
    const bool valid = info && info->infoSize == sizeof(*info) && info->abiVersion == abi &&
        std::strcmp(info->id, "item-roll-ranges") == 0 && D2RL::HasValidPluginRole(info->flags);
    FreeLibrary(module);
    if (!valid) return 8;
    std::puts("Built DLL: loader ABI manifest, exports, metadata and role agree.");
    return 0;
}
