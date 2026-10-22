#pragma once
#include <D2RLPlugin/api.h>
#include "skill_toggle.h"
#include <filesystem>
namespace QolSkillDiscovery {
// Profiles are required; source discovery is optional. A failed migration keeps
// original files and must prevent aim from loading incomplete settings.
bool Prepare(const D2RL::PluginContext*,const char* mainConfig,Aim::LiveSkills&) noexcept;
void Register(const D2RL::PluginContext*,const D2RL::LifecycleService*,const D2RL::ThreadService*) noexcept;
void Stop() noexcept;
bool Active() noexcept;
bool Save(int id,bool enabled) noexcept;
std::filesystem::path ProfilePath();
// Read-only preparation never migrates or writes a profile. A revision check
// prevents a concurrent R3 save from being replaced by a stale file snapshot.
bool ReadReload(Aim::SkillSettings&,std::uint64_t& revision,std::string& error) noexcept;
bool CommitReload(std::shared_ptr<const Aim::SkillSettings>,std::uint64_t revision) noexcept;
}
