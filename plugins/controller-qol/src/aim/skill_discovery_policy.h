#pragma once
#include "skill_toggle.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace Aim::Discovery {
struct Skill { int id{}; std::string name; bool passive{}, ground{}; };
inline constexpr std::size_t MaxText=65535;
inline bool EligibleId(int id) noexcept {return id>5 && id<65535 && !(id>=357 && id<=364) && id!=370;}
inline std::string Fingerprint(std::string_view text) {
    std::uint64_t hash=14695981039346656037ull;
    for(const unsigned char ch:text){hash^=ch;hash*=1099511628211ull;}
    char out[17]{};std::snprintf(out,sizeof(out),"%016llx",static_cast<unsigned long long>(hash));return out;
}
inline std::string Comment(std::string_view text) {
    std::string result;
    for(const unsigned char ch:text)if(ch>=32 && ch!=127)result+=static_cast<char>(ch);
    return result.substr(0,160);
}
inline std::vector<std::string_view> Cells(std::string_view line) {
    std::vector<std::string_view> cells;
    while(true){const auto end=line.find('\t');cells.push_back(line.substr(0,end));if(end==line.npos)break;line.remove_prefix(end+1);}
    return cells;
}
// Source labels are advisory. Runtime IDs are separately confirmed by the SDK;
// no compiled row offsets, localized names or perfect semantic inference assumed.
inline bool ReadSource(std::string_view text,std::vector<Skill>& output) {
    if(text.starts_with("\xef\xbb\xbf"))text.remove_prefix(3);
    const auto end=text.find('\n');if(end==text.npos)return false;
    auto header=Cells(QolAim::Trim(text.substr(0,end)));text.remove_prefix(end+1);
    const auto column=[&](std::string_view name){const auto it=std::find(header.begin(),header.end(),name);return static_cast<std::size_t>(it-header.begin());};
    const auto name=column("skill"),id=column("*Id"),desc=column("skilldesc"),passive=column("passive"),warp=column("warp"),server=column("srvdofunc");
    for(const auto n:{name,id,desc,passive,warp,server})if(n==header.size())return false;
    std::vector<Skill> result;
    std::vector<int> seen;
    while(!text.empty()) {
        const auto newline=text.find('\n');auto line=text.substr(0,newline);if(line.ends_with('\r'))line.remove_suffix(1);
        text=newline==text.npos?std::string_view{}:text.substr(newline+1);
        if(QolAim::Trim(line).empty())continue;
        const auto cells=Cells(line);
        if(cells.size()!=header.size())return false;
        if(cells[id].empty())continue; // Expansion separators.
        int number{};const auto parsed=std::from_chars(cells[id].data(),cells[id].data()+cells[id].size(),number);
        if(parsed.ec!=std::errc{} || parsed.ptr!=cells[id].data()+cells[id].size() || number<0 || number>=65535)return false;
        if(std::find(seen.begin(),seen.end(),number)!=seen.end())return false;
        seen.push_back(number);
        if(!EligibleId(number) || cells[name].empty() || cells[desc].empty())continue;
        if(!cells[passive].empty() && cells[passive]!="0" && cells[passive]!="1")return false;
        // Only the already reviewed Reimagined Warp signature gets a new mode.
        const bool ground=number==429 && cells[name]=="Warp" && cells[warp]=="1" && cells[server]=="27";
        result.push_back({number,Comment(cells[name]),cells[passive]=="1",ground});
        if(result.size()>SkillSettings::Capacity-SkillCatalog.size()) {
            // Total capacity is checked against the union below, not raw IDs.
            std::size_t extra=0;for(const auto& skill:result)if(!FindCatalogSkill(skill.id))++extra;
            if(extra>SkillSettings::Capacity-SkillCatalog.size())return false;
        }
    }
    if(result.empty())return false;
    output=std::move(result);return true;
}
inline bool Defaults(const std::vector<Skill>& source,SkillSettings& output) {
    SkillSettings result;
    for(const auto& skill:source) {
        if(result.Contains(skill.id))continue; // Existing reviewed policy stays intact.
        auto empty=std::find_if(result.entries.begin(),result.entries.end(),[](const auto& entry){return entry.id==0;});
        if(empty==result.entries.end())return false;
        *empty={skill.id,TargetMode::Disabled,skill.passive,skill.ground?TargetMode::Ground:TargetMode::Snap};
    }
    output=result;return true;
}
inline std::string Catalog(const std::vector<Skill>& source,std::string_view mod,std::string_view fingerprint) {
    std::string out="# Generated skill catalog, schema 1. REFERENCE ONLY; do not edit.\n# Mod: "+Comment(mod)+"\n# Source fingerprint: "+std::string(fingerprint)+
        "\n# This file is not loaded as settings and does not show your current choices.\n"
        "# Edit the matching .overrides.toml skill profile; R3 saves to that file.\n"
        "# A false value here does not mean the skill is off in your profile.\n"
        "# Labels come from source data and may differ from displayed/localized names.\n"
        "# Listed values summarize built-in/discovery defaults, not all shipped tuning.\n"
        "# Runtime confirmation is required for new automatic IDs; discovery alone\n"
        "# does not establish that their targeting behavior is appropriate.\n";
    for(const auto section:{"amazon","sorceress","necromancer","barbarian","paladin","druid","assassin","warlock","custom"}) {
        out+="\n[aim."+std::string(section)+"]\n";
        for(const auto& skill:source) {
            const auto* known=FindCatalogSkill(skill.id);
            if((known?std::string_view(known->className):std::string_view("custom"))!=section)continue;
            out+="# "+skill.name+"\n\""+std::to_string(skill.id)+"\" = "+
                (known?(known->enabledByDefault?"true":"false"):(skill.passive?"\"disabled\"":"false"))+"\n";
        }
    }
    out+="\n[aim.targeting]\n";
    for(const auto& skill:source)if(skill.ground)out+="\""+std::to_string(skill.id)+"\" = \"ground\" # "+skill.name+"\n";
    return out;
}
inline bool ProfileOnly(std::string_view text) {
    bool inSection=false;
    while(!text.empty()) {
        const auto end=text.find('\n');const auto line=QolAim::Trim(QolAim::WithoutComment(text.substr(0,end)));
        text=end==text.npos?std::string_view{}:text.substr(end+1);
        if(line.empty())continue;
        if(line.front()=='['){if(!line.starts_with("[aim."))return false;inSection=true;}
        else if(!inSection)return false;
    }
    return true; // ParseSkills validates known sections and values next.
}
// Missing entries are added only on an explicit R3 action. No rewrite of the
// main config and no guessing whether a value equal to an old default is owned.
inline bool RewriteOverride(const std::string& text,const SkillSettings& base,int id,bool enabled,std::string& output) {
    return ProfileOnly(text) && RewriteSkillToggle(text,id,enabled,output,&base);
}
}
