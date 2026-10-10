#pragma once
#include "skill_discovery_policy.h"

namespace Aim::Profiles {
struct Parts {std::string shared,skills;};
inline bool IsSkillSection(std::string_view section) {
    if(section=="aim.whirlwind")return true;
    for(const auto name:{"aim.amazon","aim.sorceress","aim.necromancer","aim.barbarian","aim.paladin","aim.druid","aim.assassin","aim.warlock","aim.custom","aim.targeting","aim.leading","aim.leading_max_ms","aim.leading_max_tiles"})if(section==name)return true;
    return false;
}
inline Parts Split(std::string_view text) {
    Parts out;bool skill=false;std::string pending,section,whirlwind;
    while(!text.empty()) {
        const auto end=text.find('\n');const auto raw=text.substr(0,end==text.npos?text.size():end+1);
        text.remove_prefix(raw.size());const auto line=QolAim::Trim(QolAim::WithoutComment(raw.ends_with('\n')?raw.substr(0,raw.size()-1):raw));
        if(line.empty()){pending.append(raw);continue;}
        if(line.front()=='[' && line.back()==']'){section=line.substr(1,line.size()-2);skill=IsSkillSection(section);}
        const auto equals=line.find('=');const auto key=QolAim::Trim(line.substr(0,equals));
        if(section=="aim" && equals!=line.npos && (key=="whirlwind_pass_through_enabled" || key=="whirlwind_pass_through_distance")) {
            whirlwind+=pending;pending.clear();whirlwind.append(raw);if(!raw.ends_with('\n'))whirlwind+='\n';continue;
        }
        auto& dest=skill?out.skills:out.shared;dest+=pending;pending.clear();dest.append(raw);
    }
    (skill?out.skills:out.shared)+=pending;
    if(!whirlwind.empty())out.skills+="\n[aim.whirlwind]\n"+whirlwind;
    return out;
}
inline std::string Identity(std::string_view section,std::string_view key) {
    key=QolAim::Trim(key);if(key.size()>1 && key.front()=='"' && key.back()=='"')key=key.substr(1,key.size()-2);
    if(section=="aim.whirlwind")return std::string(section)+":"+std::string(key);
    int id{};const auto result=std::from_chars(key.data(),key.data()+key.size(),id);
    if(result.ec!=std::errc{} || result.ptr!=key.data()+key.size()) {
        id=0;for(const auto& skill:SkillCatalog)if(key==skill.name){id=skill.id;break;}
    }
    if(id<=0 || !IsSkillSection(section))return {};
    const bool tuning=section=="aim.targeting" || section.starts_with("aim.leading");
    return std::string(tuning?section:"enable")+":"+std::to_string(id);
}
// Both documents are validated by ParseSkills before use. Preserve the higher
// priority document's bytes and insert only missing keys from the lower layer.
inline std::string FillMissing(std::string upper,std::string_view lower) {
    std::vector<std::string> identities;
    auto collect=[&](std::string_view doc) {
        std::string section;
        while(!doc.empty()) {
            const auto end=doc.find('\n');const auto line=QolAim::Trim(QolAim::WithoutComment(doc.substr(0,end)));
            doc=end==doc.npos?std::string_view{}:doc.substr(end+1);
            if(line.empty())continue;
            if(line.front()=='[' && line.back()==']')section=line.substr(1,line.size()-2);
            else {const auto eq=line.find('=');if(eq!=line.npos)identities.push_back(Identity(section,line.substr(0,eq)));}
        }
    };
    collect(upper);std::string section,pending;
    const std::string newline=upper.find("\r\n")!=upper.npos?"\r\n":"\n";
    while(!lower.empty()) {
        const auto end=lower.find('\n');const auto raw=lower.substr(0,end==lower.npos?lower.size():end+1);lower.remove_prefix(raw.size());
        const auto line=QolAim::Trim(QolAim::WithoutComment(raw.ends_with('\n')?raw.substr(0,raw.size()-1):raw));
        if(line.empty()){pending.append(raw);continue;}
        if(line.front()=='[' && line.back()==']'){section=line.substr(1,line.size()-2);pending.clear();continue;}
        const auto eq=line.find('=');const auto identity=eq==line.npos?std::string{}:Identity(section,line.substr(0,eq));
        if(identity.empty() || std::find(identities.begin(),identities.end(),identity)!=identities.end()){pending.clear();continue;}
        const auto entry=pending+std::string(raw)+(raw.ends_with('\n')?"":newline);pending.clear();
        bool in=false,found=false;std::size_t insert=upper.size();
        for(std::size_t pos=0;pos<upper.size();) {
            auto stop=upper.find('\n',pos);if(stop==upper.npos)stop=upper.size();
            const auto header=QolAim::Trim(QolAim::WithoutComment(std::string_view(upper).substr(pos,stop-pos)));
            if(!header.empty() && header.front()=='[' && header.back()==']') {
                if(in){insert=pos;break;}
                in=header.substr(1,header.size()-2)==section;found=found || in;
            }
            pos=stop==upper.size()?stop:stop+1;
        }
        if(found)upper.insert(insert,(insert && upper[insert-1]!='\n'?newline:"")+entry);
        else upper+=(upper.empty() || upper.back()=='\n'?"":newline)+newline+"["+section+"]"+newline+entry;
        identities.push_back(identity);
    }
    return upper;
}
inline bool Equivalent(const SkillSettings& a,const SkillSettings& b) {
    if(a.whirlwindPassThrough!=b.whirlwindPassThrough || a.whirlwindPassThroughDistance!=b.whirlwindPassThroughDistance)return false;
    for(const auto* settings:{&a,&b})for(const auto& entry:settings->entries) {
        if(!entry.id)break;const auto id=entry.id;
        if(a.Contains(id)!=b.Contains(id) || a.Mode(id)!=b.Mode(id) || a.CanToggle(id)!=b.CanToggle(id) ||
            a.Targeting(id)!=b.Targeting(id) || a.LeadMillisecondsPerTile(id)!=b.LeadMillisecondsPerTile(id) ||
            a.LeadMaxMilliseconds(id)!=b.LeadMaxMilliseconds(id) || a.LeadMaxTiles(id)!=b.LeadMaxTiles(id))return false;
    }
    return true;
}
inline std::string Snapshot(const SkillSettings& settings,const std::vector<Discovery::Skill>& labels={}) {
    std::string out;
    char distance[32];const auto number=std::to_chars(distance,distance+sizeof(distance),settings.whirlwindPassThroughDistance);
    out+="[aim.whirlwind]\nwhirlwind_pass_through_enabled = "+std::string(settings.whirlwindPassThrough?"true":"false")+
        "\nwhirlwind_pass_through_distance = "+std::string(distance,number.ptr)+"\n";
    for(const auto section:{"amazon","sorceress","necromancer","barbarian","paladin","druid","assassin","warlock","custom"}) {
        out+="\n[aim."+std::string(section)+"]\n";
        for(const auto& entry:settings.entries) {
            if(!entry.id)break;const auto* skill=FindCatalogSkill(entry.id);
            if((skill?std::string_view(skill->className):std::string_view("custom"))!=section)continue;
            const auto label=std::find_if(labels.begin(),labels.end(),[&](const auto& item){return item.id==entry.id;});
            out+="# "+(label!=labels.end()?label->name:skill?std::string(skill->name):"Skill "+std::to_string(entry.id))+"\n";
            out+="\""+std::to_string(entry.id)+"\" = "+(entry.locked?"\"disabled\"":entry.mode==TargetMode::Disabled?"false":"true")+"\n";
        }
    }
    for(const auto section:{"aim.targeting","aim.leading","aim.leading_max_ms","aim.leading_max_tiles"}) {
        out+="\n["+std::string(section)+"]\n";
        for(const auto& entry:settings.entries) {
            if(!entry.id)break;
            std::string value;
            if(std::string_view(section)=="aim.targeting")value=entry.targeting==TargetMode::Ground?"\"ground\"":"\"snap\"";
            else value=std::to_string(std::string_view(section)=="aim.leading"?entry.leadMsPerTile:std::string_view(section)=="aim.leading_max_ms"?entry.leadMaxMs:entry.leadMaxTiles);
            out+="\""+std::to_string(entry.id)+"\" = "+value+"\n";
        }
    }
    return out;
}
inline std::string Header(std::string_view mod) {
    return "# Controller QOL skill profile, schema 2. EDIT THIS FILE for skill settings.\n# Mod: "+Discovery::Comment(mod)+
        "\n# R3 saves here immediately; manual edits load after restarting the game.\n# true = enabled; false = native aim, R3 available; \"disabled\" = locked off.\n# [aim.targeting] selects ground/snap; [aim.leading*] tunes prediction.\n# Shared cursor speed, colors, logging and QOL options stay in the main TOML.\n# .catalog.toml is generated reference, not the effective settings.\n# Existing choices are preserved; missing settings are filled at startup.\n\n";
}
}
