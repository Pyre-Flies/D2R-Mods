#include <D2RLPlugin/api.h>
static D2RL::PluginInfo info{
    .infoSize=D2RL::PluginInfoSize, .abiVersion=D2RL_PLUGIN_ABI_VERSION,
    .id="ruffneckk-potion-auto-pickup", .name="Test fixture only", .version="1.3.3"
};
extern "C" __declspec(dllexport) const D2RL::PluginInfo* D2RLoaderGetPluginInfo() noexcept { return &info; }
extern "C" __declspec(dllexport) void FixtureTarget() {}
extern "C" __declspec(dllexport) void FixtureVersion(int valid) { info.version=valid ? "1.3.3" : "unreviewed"; }
