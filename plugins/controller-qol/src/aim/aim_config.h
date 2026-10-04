#pragma once
#include "aim_policy.h"
#include <charconv>
#include <string_view>
#include "config_sections.h"
#include "skill_settings.h"

namespace Aim {
inline bool ParseReticleColor(std::string_view value,ReticleColor& output) noexcept {
    if((value.size()!=9 && value.size()!=11) || value.front()!='"' || value.back()!='"' || value[1]!='#')return false;
    auto digit=[](char ch) noexcept -> int {if(ch>='0' && ch<='9')return ch-'0';if(ch>='a' && ch<='f')return ch-'a'+10;if(ch>='A' && ch<='F')return ch-'A'+10;return -1;};
    float channels[4]={0,0,0,1};
    for(std::size_t i=0;i<(value.size()==9?3u:4u);++i) {
        const int a=digit(value[2+i*2]),b=digit(value[3+i*2]);if(a<0 || b<0)return false;
        channels[i]=static_cast<float>(a*16+b)/255;
    }
    output={channels[0],channels[1],channels[2],channels[3]};return true;
}
// Deliberately limited to the documented numeric [aim] configuration schema.
inline bool ParseSettings(std::string_view text,MotionSettings& output,bool* enabled=nullptr) noexcept {
    auto trim=[](std::string_view s) {
        const auto first=s.find_first_not_of(" \t\r");
        return first==s.npos ? std::string_view{} : s.substr(first,s.find_last_not_of(" \t\r")-first+1);
    };
    MotionSettings parsed{};
    bool parsedEnabled=true;
    unsigned seen=0;
    bool inAim=false;
    while(!text.empty()) {
        const auto end=text.find('\n');
        auto line=text.substr(0,end);
        text=end==text.npos ? std::string_view{} : text.substr(end+1);
        line=trim(QolAim::WithoutComment(line));
        if(line.empty()) continue;
        if(line=="[aim]") { if(inAim) return false; inAim=true; continue; }
        if(!inAim) return false;
        const auto equals=line.find('=');
        if(equals==line.npos) return false;
        const auto key=trim(line.substr(0,equals)), value=trim(line.substr(equals+1));
        bool* boolean{}; unsigned booleanBit{};
        if(key=="enabled" && enabled) { boolean=&parsedEnabled; booleanBit=8192; }
        else if(key=="snapping_enabled") { boolean=&parsed.snapping; booleanBit=16; }
        else if(key=="overlay_enabled") { boolean=&parsed.overlay; booleanBit=32; }
        else if(key=="whirlwind_pass_through_enabled") { boolean=&parsed.whirlwindPassThrough; booleanBit=2048; }
        else if(key=="debug_overlay") { boolean=&parsed.debugOverlay; booleanBit=64; }
        else if(key=="skill_tree_toggle_enabled") { boolean=&parsed.skillTreeToggle; booleanBit=32768; }
        else if(key=="cast_observer_enabled") { boolean=&parsed.castObserver; booleanBit=16384; }
        if(boolean) {
            if(seen&booleanBit || (value!="true" && value!="false")) return false;
            seen|=booleanBit; *boolean=value=="true"; continue;
        }
        ReticleColor* color=nullptr;unsigned colorBit=0;
        if(key=="ground_reticle_color"){color=&parsed.groundReticleColor;colorBit=65536;}
        else if(key=="lock_reticle_color"){color=&parsed.lockReticleColor;colorBit=131072;}
        if(color){if((seen&colorBit) || !ParseReticleColor(value,*color))return false;seen|=colorBit;continue;}
        float number{};
        const auto result=std::from_chars(value.data(),value.data()+value.size(),number);
        if(result.ec!=std::errc{} || result.ptr!=value.data()+value.size()) return false;
        float* destination{}; unsigned bit{};
        if(key=="deadzone") { destination=&parsed.deadzone; bit=1; }
        else if(key=="initial_speed") { destination=&parsed.initialSpeed; bit=2; }
        else if(key=="maximum_speed") { destination=&parsed.maximumSpeed; bit=4; }
        else if(key=="acceleration_seconds") { destination=&parsed.accelerationSeconds; bit=8; }
        else if(key=="snap_radius") { destination=&parsed.snapRadius; bit=128; }
        else if(key=="switch_advantage") { destination=&parsed.switchAdvantage; bit=256; }
        else if(key=="projection_hz") { destination=&parsed.projectionHz; bit=512; }
        else if(key=="overlay_smoothing_ms") { destination=&parsed.overlaySmoothingMs; bit=1024; }
        else if(key=="reticle_thickness") { destination=&parsed.reticleThickness; bit=262144; }
        else if(key=="whirlwind_pass_through_distance") { destination=&parsed.whirlwindPassThroughDistance; bit=4096; }
        else return false;
        if(seen&bit) return false;
        seen|=bit; *destination=number;
    }
    if(!inAim || !parsed.Valid()) return false;
    output=parsed;
    if(enabled) *enabled=parsedEnabled;
    return true;
}
inline bool ParseQolSettings(std::string_view text,MotionSettings& output,bool& enabled,SkillSettings* skills=nullptr) {
    std::string section;
    if(!QolAim::Section(text,"aim",section)) return false;
    MotionSettings parsed{}; bool parsedEnabled{}; SkillSettings parsedSkills{};
    if(!ParseSettings(std::string("[aim]\n")+section,parsed,&parsedEnabled) || !ParseSkills(text,parsedSkills)) return false;
    output=parsed; enabled=parsedEnabled;
    if(skills) *skills=parsedSkills;
    return true;
}
}
