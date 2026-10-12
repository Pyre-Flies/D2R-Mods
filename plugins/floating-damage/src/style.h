#pragma once
#include "model.h"
#include <cstdio>

namespace Damage {
// Overlay dimensions and pixel geometry share the callback's coordinate space.
// Fit a 1080p reference uniformly so wider aspect ratios do not enlarge text.
inline float ResolutionScale(float width,float height,const Settings& config) noexcept {
    if(!config.resolutionScaling || !std::isfinite(width) || !std::isfinite(height) || width<=0 || height<=0) return 1.0f;
    return std::clamp(std::min(width/1920.0f,height/1080.0f),0.25f,4.0f);
}
inline unsigned OutlineRadius(const Settings& config,float scale=1.0f) noexcept {
    return static_cast<unsigned>(std::ceil(std::clamp(static_cast<float>(config.outlineWidth)*scale,0.0f,12.0f)));
}
inline bool Emphasized(const Number& number,const Settings& config) noexcept {return number.critical && config.criticalHits;}
inline float TextSize(const Number& number,const Settings& config,float scale=1.0f) noexcept {
    const bool enlarge=Emphasized(number,config) && config.criticalStyle!=CriticalStyle::Suffix;
    return config.textSize*scale*(enlarge?config.criticalScale:1.0f);
}
template<std::size_t N> void FormatNumber(const Number& number,const Settings& config,char (&text)[N]) noexcept {
    const double unit=config.precision==0?1.0:config.precision==1?0.1:0.01;
    const double value=std::round((static_cast<double>(number.raw)/256.0)/unit)*unit;
    const bool suffix=Emphasized(number,config) && config.criticalStyle!=CriticalStyle::Size;
    std::snprintf(text,N,value>0?"%.*f%s":"<%.*f%s",static_cast<int>(config.precision),value>0?value:unit,suffix?"!!":"");
}
inline bool ShowBacking(unsigned elements,const Settings& config) noexcept {
    return config.backing && config.backingOpacity>0 && (!config.backingPoisonOnly ||
        (config.elementColors && (elements&Poison) && !(elements&Unsupported)));
}
inline std::array<float,3> TextColor(unsigned elements,const Settings& config) noexcept {
    if(!config.elementColors || !elements || (elements&Unsupported)) return config.textColor;
    if(elements==Physical) return config.physicalColor;
    if(elements==Fire) return config.fireColor;
    if(elements==Cold) return config.coldColor;
    if(elements==Lightning) return config.lightningColor;
    if(elements==Magic) return config.magicColor;
    if(elements==Poison) return config.poisonColor;
    return config.mixedColor;
}
inline float Opacity(float age,const Settings& config) noexcept {
    return age<config.fadeStart ? 1.0f:std::clamp((1-age)/(1-config.fadeStart),0.0f,1.0f);
}
inline Point Motion(const Number& number,float age,const Settings& config,bool falling,float scale=1.0f) noexcept {
    age=std::clamp(age,0.0f,1.0f);
    Point offset{0,-config.offset-static_cast<float>(number.lane)*config.laneSpacing};
    if(!falling) {offset.y-=config.rise*age;return {offset.x*scale,offset.y*scale};}
    constexpr float peak=0.42f;
    if(age<=peak) {
        const float progress=age/peak;
        offset.y-=config.rise*progress*(2-progress);
    } else {
        const float progress=(age-peak)/(1-peak);
        offset.y+=-config.rise+config.fall*progress*progress;
    }
    // Adjacent hit lanes alternate sides; an existing number keeps its side
    // when its small ticks merge or the target moves.
    const float direction=((number.target.id+number.lane)&1U)?1.0f:-1.0f;
    offset.x=direction*config.side*age*age;
    return {offset.x*scale,offset.y*scale};
}
inline float ShadowWeight(int x,int y,unsigned radius) noexcept {
    const auto extent=static_cast<int>(radius)+1;
    const float weight=static_cast<float>((extent-std::abs(x))*(extent-std::abs(y)));
    const float total=static_cast<float>(extent*extent*extent*extent);
    return weight/total;
}
}
