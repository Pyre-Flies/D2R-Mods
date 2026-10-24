#include <D2RLPlugin/api.h>
#include <D2RLPlugin/resource.h>
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void Check(bool value, const char* name) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", name);
        std::exit(1);
    }
}

int wmain(int argc, wchar_t** argv) {
    Check(argc == 2, "DLL path");
    auto module = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    Check(module != nullptr, "resource load");
    const auto resource = FindResourceW(module, MAKEINTRESOURCEW(D2RL_PLUGIN_MANIFEST_RESOURCE_ID), RT_RCDATA);
    Check(resource && SizeofResource(module, resource) == sizeof(DWORD), "manifest DWORD size");
    const auto abi = static_cast<const DWORD*>(LockResource(LoadResource(module, resource)));
    Check(abi && *abi == D2RL_PLUGIN_ABI_VERSION, "manifest ABI");
    const auto configResource = FindResourceW(module, MAKEINTRESOURCEW(D2RL_PLUGIN_CONFIG_RESOURCE_ID), RT_RCDATA);
    Check(configResource && SizeofResource(module, configResource) > 0, "embedded default config resource");
    Check(FindResourceW(module, MAKEINTRESOURCEW(1), RT_VERSION) != nullptr, "VERSIONINFO");
    FreeLibrary(module);

    module = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    Check(module != nullptr, "normal DLL load");
    const auto info = reinterpret_cast<D2RL::GetPluginInfoFn>(GetProcAddress(module, "D2RLoaderGetPluginInfo"));
    const auto load = reinterpret_cast<D2RL::LoadPluginFn>(GetProcAddress(module, "D2RLoaderLoadPlugin"));
    const auto unload = reinterpret_cast<D2RL::UnloadPluginFn>(GetProcAddress(module, "D2RLoaderUnloadPlugin"));
    Check(info && load && unload, "three lifecycle exports");
    const auto value = info();
    Check(value && value->infoSize == D2RL::PluginInfoSize && value->abiVersion == D2RL_PLUGIN_ABI_VERSION &&
        !std::strcmp(value->id, "ultrawide") && !std::strcmp(value->name, "Ultrawide") &&
        !std::strcmp(value->version, "1.3.5+rev.2") &&
        D2RL::HasValidPluginRole(value->flags) && D2RL::PluginRoleValue(value->flags) == D2RL::FlagsValue(D2RL::PluginFlags::Client) &&
        D2RL::HasFlag(value->flags, D2RL::PluginFlags::NativeHooks), "metadata and client-only role");
    Check(!load(nullptr), "null context refusal");
    const D2RL::PluginApi api{.apiSize = D2RL::PluginApiSize};
    D2RL::PluginContext ctx{
        .contextSize = sizeof(D2RL::PluginContext),
        .abiVersion = D2RL_PLUGIN_ABI_VERSION,
        .api = &api,
        .exeBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))
    };
    Check(load(&ctx), "unsupported executable remains inert");
    unload();
    ctx.abiVersion = 0;
    Check(!load(&ctx), "wrong ABI refusal");
    FreeLibrary(module);
    std::puts("Ultrawide manifest, embedded config, exports, role, and unsupported-build checks passed.");
}
