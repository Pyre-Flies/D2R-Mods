#pragma once
#include "aim_config.h"
#include <memory>
#include <string>
namespace QolAim {
struct Reload {
    std::shared_ptr<const Aim::MotionSettings> motion;
    std::shared_ptr<const Aim::SkillSettings> skills;
    std::uint64_t profileRevision{};
    std::string restart;
};
bool PrepareReload(std::string_view main,Reload&,std::string& error) noexcept;
bool ApplyReload(const Reload&) noexcept;
}
