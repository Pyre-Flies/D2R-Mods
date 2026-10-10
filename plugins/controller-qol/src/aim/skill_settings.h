#pragma once
#include "aim_policy.h"
#include "config_sections.h"
#include "skill_catalog.h"
#include <array>
#include <charconv>

namespace Aim {
enum class TargetMode { Disabled, Ground, Snap };
// Telekinesis operates on the selected unit (including objects/items). A point
// cannot replace that identity as it can for projectile/area spells.
inline bool UsesNativeUnitTarget(int skill) noexcept { return skill==43; }
struct SkillSetting { int id{}; TargetMode mode{}; bool locked{}; TargetMode targeting{TargetMode::Snap}; unsigned leadMsPerTile{}; unsigned leadMaxMs{600}, leadMaxTiles{3}; };
struct SkillSettings {
    static constexpr std::size_t Capacity=1024;
    std::array<SkillSetting,Capacity> entries{};
    bool whirlwindPassThrough=true;
    float whirlwindPassThroughDistance=1.5f;
    SkillSettings() noexcept {
        for(std::size_t i=0;i<SkillCatalog.size();++i) {
            const auto& skill=SkillCatalog[i];
            entries[i]={skill.id,skill.enabledByDefault?(skill.snap?TargetMode::Snap:TargetMode::Ground):TargetMode::Disabled,false,skill.snap?TargetMode::Snap:TargetMode::Ground};
        }
    }
    TargetMode Mode(int id) const noexcept {
        if(id<=0) return TargetMode::Disabled;
        for(const auto& entry:entries) {if(!entry.id)break;if(entry.id==id)return entry.mode;}
        return TargetMode::Disabled;
    }
    TargetMode Targeting(int id) const noexcept {
        for(const auto& entry:entries) {if(!entry.id)break;if(entry.id==id)return entry.targeting;}
        return TargetMode::Snap;
    }
    bool CanToggle(int id) const noexcept {
        if(id<=0 || id>=65535) return false;
        for(const auto& entry:entries) {if(!entry.id)break;if(entry.id==id)return !entry.locked;}
        return false;
    }
    bool Contains(int id) const noexcept {
        if(id<=0 || id>=65535) return false;
        for(const auto& entry:entries) {if(!entry.id)break;if(entry.id==id)return true;}
        return false;
    }
    bool Enabled(int id) const noexcept { return Mode(id)!=TargetMode::Disabled; }
    bool Snaps(int id) const noexcept { return Mode(id)==TargetMode::Snap; }
    unsigned LeadMaxMilliseconds(int id) const noexcept {
        for(const auto& entry:entries) {if(!entry.id)break;if(entry.id==id)return entry.leadMaxMs;}
        return 600;
    }
    unsigned LeadMaxTiles(int id) const noexcept {
        for(const auto& entry:entries) {if(!entry.id)break;if(entry.id==id)return entry.leadMaxTiles;}
        return 3;
    }
    unsigned LeadMillisecondsPerTile(int id) const noexcept {
        for(const auto& entry:entries) {if(!entry.id)break;if(entry.id==id)return entry.leadMsPerTile;}
        return 0;
    }
};
// Class headings organize IDs only. Legacy name keys remain readable for migration.
inline bool ParseSkills(std::string_view text,SkillSettings& output,const SkillSettings* defaults=nullptr) {
    SkillSettings parsed=defaults?*defaults:SkillSettings{};
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
                targeting=parsed.Targeting(id);
                if(value=="\"ground\"") mode=targeting=TargetMode::Ground;
                else if(value=="true") mode=targeting;
                else if(value=="\"snap\"") mode=targeting=TargetMode::Snap;
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
                targeting=parsed.Targeting(id);
                locked=value=="\"disabled\"";
                mode=(value=="false" || locked)?TargetMode::Disabled:targeting;
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
    // Independent integer maps: estimates never enable a skill; limits retain
    // the original 600 ms / 3-tile defaults unless explicitly overridden.
    struct LeadOption { const char* section; unsigned SkillSetting::*member; unsigned minimum, maximum; };
    for(const auto option : {LeadOption{"aim.leading", &SkillSetting::leadMsPerTile, 0, 200},
            LeadOption{"aim.leading_max_ms", &SkillSetting::leadMaxMs, 100, 1500},
            LeadOption{"aim.leading_max_tiles", &SkillSetting::leadMaxTiles, 1, 8}}) {
        std::string leading;
        if(!QolAim::Section(text,option.section,leading)) return false;
        remaining=leading; targeted={}; targetCount=0;
        while(!remaining.empty()) {
            const auto end=remaining.find('\n'); auto line=QolAim::Trim(remaining.substr(0,end));
            remaining=end==remaining.npos?std::string_view{}:remaining.substr(end+1);
            if(line.empty()) continue;
            const auto eq=line.find('='); if(eq==line.npos) return false;
            auto key=QolAim::Trim(line.substr(0,eq)); const auto value=QolAim::Trim(line.substr(eq+1));
            if(key.size()>=2 && key.front()=='"' && key.back()=='"') key=key.substr(1,key.size()-2);
            int id{}; unsigned valueNumber{};
            const auto idResult=std::from_chars(key.data(),key.data()+key.size(),id);
            const auto timeResult=std::from_chars(value.data(),value.data()+value.size(),valueNumber);
            if(idResult.ec!=std::errc{} || idResult.ptr!=key.data()+key.size() || id<=0 || id>65534 ||
                timeResult.ec!=std::errc{} || timeResult.ptr!=value.data()+value.size() || valueNumber<option.minimum || valueNumber>option.maximum || targetCount==targeted.size()) return false;
            for(std::size_t i=0;i<targetCount;++i) if(targeted[i]==id) return false;
            targeted[targetCount++]=id;
            bool found=false;
            for(auto& entry:parsed.entries) if(entry.id==id) { entry.*(option.member)=valueNumber; found=true; break; }
            if(!found) return false;
        }
    }
    std::string whirlwind;
    if(!QolAim::Section(text,"aim.whirlwind",whirlwind))return false;
    std::string_view options=whirlwind;unsigned whirlwindSeen=0;
    while(!options.empty()) {
        const auto end=options.find('\n');const auto line=QolAim::Trim(options.substr(0,end));
        options=end==options.npos?std::string_view{}:options.substr(end+1);
        if(line.empty())continue;
        const auto eq=line.find('=');if(eq==line.npos)return false;
        const auto key=QolAim::Trim(line.substr(0,eq)),value=QolAim::Trim(line.substr(eq+1));
        if(key=="whirlwind_pass_through_enabled") {
            if((whirlwindSeen&1) || (value!="true" && value!="false"))return false;
            whirlwindSeen|=1;parsed.whirlwindPassThrough=value=="true";
        } else if(key=="whirlwind_pass_through_distance") {
            if(whirlwindSeen&2)return false;whirlwindSeen|=2;
            float distance{};const auto number=std::from_chars(value.data(),value.data()+value.size(),distance);
            if(number.ec!=std::errc{} || number.ptr!=value.data()+value.size() || !std::isfinite(distance) || distance<0 || distance>15)return false;
            parsed.whirlwindPassThroughDistance=distance;
        } else return false;
    }
    // Catch misspelled aim sections instead of silently enabling their defaults.
    while(!text.empty()) {
        const auto end=text.find('\n'); auto line=QolAim::Trim(text.substr(0,end));
        text=end==text.npos?std::string_view{}:text.substr(end+1);
        line=QolAim::Trim(QolAim::WithoutComment(line));
        if(line.size()>=6 && line.substr(0,5)=="[aim.") {
            bool known=line=="[aim.whirlwind]";
            for(const auto name:{"[aim.amazon]","[aim.sorceress]","[aim.necromancer]","[aim.barbarian]","[aim.paladin]","[aim.druid]","[aim.assassin]","[aim.warlock]","[aim.custom]","[aim.targeting]","[aim.leading]","[aim.leading_max_ms]","[aim.leading_max_tiles]"}) if(line==name) known=true;
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
    return armed && requestedSkill==activeSkill && settings.Enabled(activeSkill) && !UsesNativeUnitTarget(activeSkill);
}
inline bool NativeUnitScoring(bool armed,int activeSkill,int previewSkill,unsigned unitType,const auto& settings) noexcept {
    if(!armed || !settings.Snaps(43)) return false;
    // Objects/items must already be ranked before the first Telekinesis query.
    // As with the shared monster preview, the previous enabled skill may be
    // Teleport or another spell. Never extend this to a different active cast.
    if(activeSkill==-1 && settings.Enabled(previewSkill) && (unitType==2 || unitType==4)) return true;
    const int skill=activeSkill==-1?previewSkill:activeSkill;
    return UsesNativeUnitTarget(skill) &&
        (unitType==1 || unitType==2 || unitType==4);
}
inline bool ReleaseForSkill(int activeSkill,const auto& settings) noexcept {
    // Selected is also queried while idle; its request is not evidence of a cast.
    return activeSkill>=0 && !settings.Enabled(activeSkill);
}
}
