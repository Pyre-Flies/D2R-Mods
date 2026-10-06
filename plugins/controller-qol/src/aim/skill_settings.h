#pragma once
#include "aim_policy.h"
#include "config_sections.h"
#include "skill_catalog.h"
#include <array>
#include <charconv>

namespace Aim {
enum class TargetMode { Disabled, Ground, Snap };
struct SkillSetting { int id{}; TargetMode mode{}; bool locked{}; TargetMode targeting{TargetMode::Snap}; };
struct SkillSettings {
    static constexpr std::size_t Capacity=SkillCatalog.size()+32;
    std::array<SkillSetting,Capacity> entries{};
    SkillSettings() noexcept {
        for(std::size_t i=0;i<SkillCatalog.size();++i) {
            const auto& skill=SkillCatalog[i];
            entries[i]={skill.id,skill.enabledByDefault?(skill.snap?TargetMode::Snap:TargetMode::Ground):TargetMode::Disabled,false,skill.snap?TargetMode::Snap:TargetMode::Ground};
        }
    }
    TargetMode Mode(int id) const noexcept {
        if(id<=0) return TargetMode::Disabled;
        for(const auto& entry:entries) if(entry.id==id) return entry.mode;
        return TargetMode::Disabled;
    }
    TargetMode Targeting(int id) const noexcept {
        for(const auto& entry:entries) if(entry.id==id) return entry.targeting;
        return TargetMode::Snap;
    }
    bool CanToggle(int id) const noexcept {
        if(!FindCatalogSkill(id)) return false;
        for(const auto& entry:entries) if(entry.id==id) return !entry.locked;
        return false;
    }
    bool Enabled(int id) const noexcept { return Mode(id)!=TargetMode::Disabled; }
    bool Snaps(int id) const noexcept { return Mode(id)==TargetMode::Snap; }
};
// Class headings organize IDs only. Legacy name keys remain readable for migration.
inline bool ParseSkills(std::string_view text,SkillSettings& output) {
    SkillSettings parsed{};
    std::array<int,SkillSettings::Capacity> seen{}; std::size_t count=0;
    for(const auto sectionName:{"aim.amazon","aim.sorceress","aim.necromancer","aim.barbarian","aim.paladin","aim.druid","aim.assassin","aim.warlock","aim.custom"}) {
        std::string section;
        if(!QolAim::Section(text,sectionName,section)) return false;
        std::string_view remaining=section;
        while(!remaining.empty()) {
            const auto end=remaining.find('\n'); auto line=QolAim::Trim(remaining.substr(0,end));
            remaining=end==remaining.npos?std::string_view{}:remaining.substr(end+1);
            if(line.empty()) continue;
            const auto equals=line.find('='); if(equals==line.npos) return false;
            auto key=QolAim::Trim(line.substr(0,equals)); const auto value=QolAim::Trim(line.substr(equals+1));
            if(key.size()>=2 && key.front()=='"' && key.back()=='"') key=key.substr(1,key.size()-2);
            int id=0; TargetMode mode=TargetMode::Disabled; bool locked=false; TargetMode targeting=TargetMode::Snap;
            if(std::string_view(sectionName)=="aim.custom") {
                const auto result=std::from_chars(key.data(),key.data()+key.size(),id);
                if(result.ec!=std::errc{} || result.ptr!=key.data()+key.size() || id<=0 || id>65534 || FindCatalogSkill(id)) return false;
                if(value=="\"ground\"") mode=targeting=TargetMode::Ground;
                else if(value=="true" || value=="\"snap\"") mode=TargetMode::Snap;
                else if(value=="\"disabled\"") locked=true;
                else if(value!="false") return false;
            } else {
                const auto result=std::from_chars(key.data(),key.data()+key.size(),id);
                if(result.ec!=std::errc{} || result.ptr!=key.data()+key.size()) {
                    id=0;
                    for(const auto& entry:parsed.entries) if(entry.id && SkillName(entry.id) && key==SkillName(entry.id)) { id=entry.id; break; }
                }
                const auto* catalog=FindCatalogSkill(id);
                if(!catalog || (value!="true" && value!="false" && value!="\"disabled\"")) return false;
                targeting=catalog->snap?TargetMode::Snap:TargetMode::Ground;
                locked=value=="\"disabled\"";
                mode=(value=="false" || locked)?TargetMode::Disabled:(catalog->snap?TargetMode::Snap:TargetMode::Ground);
            }
            if(count==seen.size()) return false;
            for(std::size_t i=0;i<count;++i) if(seen[i]==id) return false;
            seen[count++]=id;
            bool assigned=false;
            for(auto& entry:parsed.entries) if(entry.id==id) { entry.mode=mode; entry.locked=locked; entry.targeting=targeting; assigned=true; break; }
            if(!assigned) {
                for(auto& entry:parsed.entries) if(!entry.id) { entry={id,mode,locked,targeting}; assigned=true; break; }
            }
            if(!assigned) return false;
        }
    }
    // Overrides refine targeting only; they never authorize an absent/disabled ID.
    std::string overrides;
    if(!QolAim::Section(text,"aim.targeting",overrides)) return false;
    std::string_view remaining=overrides;
    std::array<int,SkillSettings::Capacity> targeted{}; std::size_t targetCount=0;
    while(!remaining.empty()) {
        const auto end=remaining.find('\n'); auto line=QolAim::Trim(remaining.substr(0,end));
        remaining=end==remaining.npos?std::string_view{}:remaining.substr(end+1);
        if(line.empty()) continue;
        const auto eq=line.find('='); if(eq==line.npos) return false;
        auto key=QolAim::Trim(line.substr(0,eq)); const auto value=QolAim::Trim(line.substr(eq+1));
        if(key.size()>=2 && key.front()=='"' && key.back()=='"') key=key.substr(1,key.size()-2);
        int id=0;const auto result=std::from_chars(key.data(),key.data()+key.size(),id);
        if(result.ec!=std::errc{} || result.ptr!=key.data()+key.size() || id<=0 || id>65534 || targetCount==targeted.size()) return false;
        for(std::size_t i=0;i<targetCount;++i) if(targeted[i]==id) return false;
        targeted[targetCount++]=id;
        if(value!="\"ground\"" && value!="\"snap\"") return false;
        bool found=false;
        for(auto& entry:parsed.entries) if(entry.id==id) {
            entry.targeting=value=="\"ground\""?TargetMode::Ground:TargetMode::Snap;
            if(entry.mode!=TargetMode::Disabled) entry.mode=entry.targeting;
            found=true;break;
        }
        if(!found) return false;
    }
    // Catch misspelled aim sections instead of silently enabling their defaults.
    while(!text.empty()) {
        const auto end=text.find('\n'); auto line=QolAim::Trim(text.substr(0,end));
        text=end==text.npos?std::string_view{}:text.substr(end+1);
        line=QolAim::Trim(QolAim::WithoutComment(line));
        if(line.size()>=6 && line.substr(0,5)=="[aim.") {
            bool known=false;
            for(const auto name:{"[aim.amazon]","[aim.sorceress]","[aim.necromancer]","[aim.barbarian]","[aim.paladin]","[aim.druid]","[aim.assassin]","[aim.warlock]","[aim.custom]","[aim.targeting]"}) if(line==name) known=true;
            if(!known) return false;
        }
    }
    output=parsed; return true;
}
inline bool DeliberateStick(Point stick,float deadzone) noexcept {
    return Finite(stick) && std::hypot(stick.x,stick.y)>deadzone;
}
inline bool OverrideScoring(bool armed,int activeSkill,const auto& settings,int previewSkill=-1) noexcept {
    // Prepare the shared reticle before the first cast. Idle has no requested
    // cast skill; preview authorization must never authorize coordinate routing.
    return armed && (settings.Snaps(activeSkill) ||
        (activeSkill==-1 && settings.Enabled(previewSkill)));
}
inline bool CoordinateRoute(bool armed,int requestedSkill,int activeSkill,const auto& settings) noexcept {
    return armed && requestedSkill==activeSkill && settings.Enabled(activeSkill);
}
inline bool ReleaseForSkill(int activeSkill,const auto& settings) noexcept {
    // Selected is also queried while idle; its request is not evidence of a cast.
    return activeSkill>=0 && !settings.Enabled(activeSkill);
}
}
