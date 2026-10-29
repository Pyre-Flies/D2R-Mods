#include <D2RLPlugin/api.h>
#include <D2RLPlugin/resource.h>
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
	if (!condition) {
		std::fprintf(stderr, "FAIL: %s\n", message);
		std::exit(1);
	}
}

} // namespace

int wmain(int argc, wchar_t** argv) {
	Check(argc == 2, "DLL path argument required");

	HMODULE module = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	Check(module != nullptr, "resource-only DLL load");

	const HRSRC manifest = FindResourceW(module, MAKEINTRESOURCEW(D2RL_PLUGIN_MANIFEST_RESOURCE_ID), RT_RCDATA);
	Check(manifest != nullptr && SizeofResource(module, manifest) == sizeof(DWORD), "exact DWORD ABI manifest");
	const auto* abi = static_cast<const DWORD*>(LockResource(LoadResource(module, manifest)));
	Check(abi != nullptr && *abi == D2RL_PLUGIN_ABI_VERSION, "manifest ABI matches SDK");

	const HRSRC configRes = FindResourceW(module, MAKEINTRESOURCEW(D2RL_PLUGIN_CONFIG_RESOURCE_ID), RT_RCDATA);
	Check(configRes != nullptr, "embedded TOML config resource exists");
	const auto* configText = static_cast<const char*>(LockResource(LoadResource(module, configRes)));
	const std::string_view cfg(configText ? configText : "", SizeofResource(module, configRes));
	Check(cfg.find("[pyre-stashsearch]") != std::string_view::npos, "embedded section header");
	Check(cfg.find("tab_flyout_expanded = false") != std::string_view::npos, "embedded tab_flyout_expanded default");

	Check(FindResourceW(module, MAKEINTRESOURCEW(1), RT_VERSION) != nullptr, "VERSIONINFO present");
	FreeLibrary(module);

	module = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	Check(module != nullptr, "normal DLL load");

	const auto info   = reinterpret_cast<D2RL::GetPluginInfoFn>(GetProcAddress(module, "D2RLoaderGetPluginInfo"));
	const auto load   = reinterpret_cast<D2RL::LoadPluginFn>(GetProcAddress(module, "D2RLoaderLoadPlugin"));
	const auto unload = reinterpret_cast<D2RL::UnloadPluginFn>(GetProcAddress(module, "D2RLoaderUnloadPlugin"));
	Check(info != nullptr && load != nullptr && unload != nullptr, "three D2RLoader exports present");

	const D2RL::PluginInfo* value = info();
	Check(
		value != nullptr &&
		value->infoSize == D2RL::PluginInfoSize &&
		value->abiVersion == D2RL_PLUGIN_ABI_VERSION &&
		std::strcmp(value->id, "pyre-stashsearch") == 0 &&
		std::strcmp(value->name, "Pyre Stash Search") == 0 &&
		std::strcmp(value->version, "1.0.0") == 0 &&
		std::strcmp(value->author, "PyreFly") == 0 &&
		D2RL::HasValidPluginRole(value->flags) &&
		D2RL::PluginRoleValue(value->flags) == D2RL::FlagsValue(D2RL::PluginFlags::Client) &&
		D2RL::HasFlag(value->flags, D2RL::PluginFlags::NativeHooks),
		"PluginInfo metadata, ABI version, Client role, and NativeHooks flag match pyre-stashsearch 1.0.0");

	Check(!load(nullptr), "null context refused");
	unload();

	const D2RL::PluginApi api{ .apiSize = D2RL::PluginApiSize };
	D2RL::PluginContext   context{
		.contextSize = sizeof(D2RL::PluginContext),
		.abiVersion  = D2RL_PLUGIN_ABI_VERSION,
		.api         = &api,
		.exeBase     = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),
	};
	Check(!load(&context), "non-D2R executable refused cleanly by signature/D2RCore guard");
	unload();

	context.contextSize = 8;
	Check(!load(&context), "short contextSize refused");
	unload();

	FreeLibrary(module);
	std::puts("PASS: pyre-stashsearch.dll ABI manifest, embedded config, exports, metadata, and signature guard checks succeeded.");
	return 0;
}
