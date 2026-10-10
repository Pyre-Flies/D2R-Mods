#pragma once
#include <D2RLPlugin/api.h>
#include "skill_toggle.h"
namespace QolSkillDiscovery {
// Profiles are required; source discovery is optional. A failed migration keeps
// original files and must prevent aim from loading incomplete settings.
bool Prepare(const D2RL::PluginContext*,const char* mainConfig,Aim::LiveSkills&) noexcept;
void Register(const D2RL::PluginContext*,const D2RL::LifecycleService*,const D2RL::ThreadService*) noexcept;
void Stop() noexcept;
bool Active() noexcept;
bool Save(int id,bool enabled) noexcept;
}
