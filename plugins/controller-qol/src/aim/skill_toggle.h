#pragma once
#include "skill_settings.h"
#include <atomic>
#include <string>

namespace Aim {
// Baseline stays immutable after load; UI changes publish one atomic mode.
struct LiveSkills {
    SkillSettings baseline{};
    std::array<std::atomic<unsigned char>,65535> overrides{};
    TargetMode Mode(int id) const noexcept {
        if(id<=0 || id>=65535) return TargetMode::Disabled;
        if(!baseline.CanToggle(id) && FindCatalogSkill(id)) return TargetMode::Disabled;
        const auto value=overrides[id].load();
        return value?static_cast<TargetMode>(value-1):baseline.Mode(id);
    }
    TargetMode Targeting(int id) const noexcept { return baseline.Targeting(id); }
    bool CanToggle(int id) const noexcept { return baseline.CanToggle(id); }
    bool Enabled(int id) const noexcept { return Mode(id)!=TargetMode::Disabled; }
    bool Snaps(int id) const noexcept { return Mode(id)==TargetMode::Snap; }
    void Publish(int id,TargetMode mode) noexcept { if(id>0 && id<65535) overrides[id].store(static_cast<unsigned char>(mode)+1); }
};
struct TreePress {
    bool held{},observed{};
    bool Update(bool valid,bool pressed,bool inputValid=true) noexcept {
        if(!inputValid)return false; // A stale sample is not a release.
        const bool rising=observed && pressed && !held;
        held=pressed;observed=true;
        return valid && rising;
    }
};
struct TreeSequence {
    std::uint64_t seen{};bool primed{};
    bool Update(std::uint64_t sequence,bool valid) noexcept {
        const bool changed=primed && sequence!=seen;
        seen=sequence;primed=true;return valid && changed;
    }
};
// Rewrite just the selected catalog setting, preserving comments and other bytes.
inline bool RewriteSkillToggle(const std::string& text,int id,bool enabled,std::string& output) {
    const auto* skill=FindCatalogSkill(id);
    SkillSettings parsed;
    if(!skill || !ParseSkills(text,parsed) || !parsed.CanToggle(id)) return false;
    std::string section;
    std::size_t sectionEnd=text.size(),match=std::string::npos,valueStart=0,valueEnd=0;
    const std::string desired="aim."+std::string(skill->className);
    bool foundSection=false;
    for(std::size_t pos=0;pos<text.size();) {
        auto end=text.find('\n',pos);if(end==text.npos)end=text.size();
        const auto raw=std::string_view(text).substr(pos,end-pos);
        auto line=QolAim::Trim(QolAim::WithoutComment(raw));
        if(!line.empty() && line.front()=='[' && line.back()==']') {
            if(section==desired) sectionEnd=pos;
            section=std::string(line.substr(1,line.size()-2));
            if(section==desired) foundSection=true;
        } else if(section.starts_with("aim.") && section!="aim.custom" && section!="aim.targeting") {
            const auto eq=line.find('=');
            if(eq!=line.npos) {
                auto key=QolAim::Trim(line.substr(0,eq));
                if(key.size()>1 && key.front()=='"' && key.back()=='"')key=key.substr(1,key.size()-2);
                int number=0;const auto converted=std::from_chars(key.data(),key.data()+key.size(),number);
                const bool numeric=converted.ec==std::errc{} && converted.ptr==key.data()+key.size();
                if((numeric && number==id) || (!numeric && SkillName(id) && key==SkillName(id))) {
                    auto value=QolAim::Trim(line.substr(eq+1));
                    match=pos;valueStart=static_cast<std::size_t>(value.data()-text.data());valueEnd=valueStart+value.size();
                }
            }
        }
        pos=end==text.size()?end:end+1;
    }
    output=text;
    if(match!=text.npos) output.replace(valueStart,valueEnd-valueStart,enabled?"true":"false");
    else {
        const std::string newline=text.find("\r\n")!=text.npos?"\r\n":"\n";
        const std::string entry="# "+std::string(skill->name)+newline+"\""+std::to_string(id)+"\" = "+(enabled?"true":"false")+newline;
        if(foundSection) {
            const std::string prefix=sectionEnd && text[sectionEnd-1]!='\n'?newline:"";
            output.insert(sectionEnd,prefix+entry);
        } else output+=(output.empty() || output.back()=='\n'?"":newline)+newline+"["+desired+"]"+newline+entry;
    }
    SkillSettings verify;
    return output.size()<65535 && ParseSkills(output,verify) && verify.Enabled(id)==enabled;
}
}
