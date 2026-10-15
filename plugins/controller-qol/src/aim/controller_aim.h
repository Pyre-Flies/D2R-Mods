#pragma once
namespace D2RL { struct PluginContext; }
namespace QolAim {
bool Initialize(const D2RL::PluginContext*,bool qolEnabled) noexcept;
void Shutdown() noexcept;
bool OwnsGuidedArrow() noexcept;
#ifdef QOL_AIM_TEST_INSTALL
bool TestInstall(const D2RL::PluginContext*,bool castObserver,bool fresh=true) noexcept;
void TestLifecycle(unsigned kind) noexcept;
bool TestInactiveWrappers() noexcept;
bool TestScoreLogging(const D2RL::PluginContext*,bool verbose) noexcept;
#endif
}
