#pragma once
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

namespace Sockets {
constexpr unsigned DefaultScalePercent=65;
constexpr unsigned DefaultOpacityPercent=65;
constexpr unsigned DefaultRimOpacityPercent=80;
constexpr unsigned DefaultCenterOpacityPercent=40;
constexpr unsigned DefaultInsertedLiftPixels=1;
constexpr unsigned CircleSizeMarginPoints=15;
constexpr unsigned MaxCircleScalePercent=100+CircleSizeMarginPoints;
// Keep a ring visible around inserted art, with one shared size control.
constexpr unsigned CircleScalePercent(unsigned insertedPercent) noexcept {
    if(insertedPercent<10 || insertedPercent>100) return 100;
    return insertedPercent+CircleSizeMarginPoints;
}
struct Settings {
    unsigned scalePercent=DefaultScalePercent,opacityPercent=DefaultOpacityPercent;
    unsigned rimOpacityPercent=DefaultRimOpacityPercent,centerOpacityPercent=DefaultCenterOpacityPercent;
    unsigned insertedLiftPixels=DefaultInsertedLiftPixels;
    bool enabled=true,alwaysVisible=false;
};
struct Point {std::int32_t x{},y{};};
struct SpriteBounds {std::int32_t width{},height{},pivotX{},pivotY{};};
struct SpriteCenter {double x{},y{};};
inline bool ValidBounds(const SpriteBounds& b) noexcept {
    return b.width>0 && b.width<=8192 && b.height>0 && b.height<=8192 &&
        b.pivotX>=-16384 && b.pivotX<=16384 && b.pivotY>=-16384 && b.pivotY<=16384;
}
inline bool CenterOf(const Point& anchor,float scale,const SpriteBounds& b,SpriteCenter& out) noexcept {
    if(!ValidBounds(b) || !std::isfinite(scale) || scale<=0 || scale>16) return false;
    out={anchor.x+(b.width*0.5-b.pivotX)*scale,anchor.y+(b.height*0.5-b.pivotY)*scale};
    return true;
}
// Center the following inserted sprite on its preceding circle, using only
// copied geometry from the same synchronous socket draw.
inline bool ScaleAtCenter(Point& anchor,float& scale,const SpriteBounds& b,unsigned percent,
                          const SpriteCenter& center) noexcept {
    if(!ValidBounds(b) || percent<10 || percent>100 || !std::isfinite(scale) || scale<=0 || scale>16 ||
       !std::isfinite(center.x) || !std::isfinite(center.y)) return false;
    const float reduced=scale*(static_cast<float>(percent)/100);
    const double x=std::round(center.x-(b.width*0.5-b.pivotX)*reduced);
    const double y=std::round(center.y-(b.height*0.5-b.pivotY)*reduced);
    constexpr auto low=std::numeric_limits<std::int32_t>::min(),high=std::numeric_limits<std::int32_t>::max();
    if(x<low || x>high || y<low || y>high) return false;
    anchor={static_cast<std::int32_t>(x),static_cast<std::int32_t>(y)};scale=reduced;return true;
}
inline std::string_view Trim(std::string_view text) noexcept {
    while(!text.empty() && (text.front()==' ' || text.front()=='\t' || text.front()=='\r')) text.remove_prefix(1);
    while(!text.empty() && (text.back()==' ' || text.back()=='\t' || text.back()=='\r')) text.remove_suffix(1);
    return text;
}
inline bool ParseBoundedUnsigned(std::string_view text,unsigned& out,unsigned minimum,unsigned maximum) noexcept {
    text=Trim(text);unsigned value{};
    if(text.empty()) return false;
    const auto result=std::from_chars(text.data(),text.data()+text.size(),value);
    if(result.ec!=std::errc{} || result.ptr!=text.data()+text.size() || value<minimum || value>maximum) return false;
    out=value;return true;
}
inline bool ParsePercent(std::string_view text,unsigned& out) noexcept {return ParseBoundedUnsigned(text,out,10,100);}
inline bool ParseRingOpacity(std::string_view text,unsigned& out) noexcept {return ParseBoundedUnsigned(text,out,0,100);}
inline bool ParseLift(std::string_view text,unsigned& out) noexcept {return ParseBoundedUnsigned(text,out,0,8);}
inline Settings ParseConfig(std::string_view text,unsigned& rejected) noexcept {
    Settings settings;bool section=false;rejected=0;
    if(text.starts_with("\xef\xbb\xbf")) text.remove_prefix(3);
    while(!text.empty()) {
        const auto end=text.find('\n');auto line=Trim(text.substr(0,end));
        text=end==text.npos?std::string_view{}:text.substr(end+1);
        line=Trim(line.substr(0,line.find('#')));
        if(line.empty()) continue;
        if(line.front()=='[') {section=line=="[always-show-sockets]";continue;}
        if(!section) continue;
        const auto equal=line.find('=');
        if(equal==line.npos) {++rejected;continue;}
        const auto key=Trim(line.substr(0,equal));
        if(key=="enabled" || key=="always_visible") {
            const auto value=Trim(line.substr(equal+1));
            if(value!="true" && value!="false") ++rejected;
            else (key=="enabled"?settings.enabled:settings.alwaysVisible)=value=="true";
            continue;
        }
        if(key=="inserted_item_scale_percent" && !ParsePercent(line.substr(equal+1),settings.scalePercent)) ++rejected;
        if(key=="inserted_item_opacity_percent" && !ParsePercent(line.substr(equal+1),settings.opacityPercent)) ++rejected;
        if(key=="inserted_item_lift_pixels" && !ParseLift(line.substr(equal+1),settings.insertedLiftPixels)) ++rejected;
        if(key=="ring_opacity_percent" && !ParseRingOpacity(line.substr(equal+1),settings.rimOpacityPercent)) ++rejected;
        if(key=="ring_center_opacity_percent" && !ParseRingOpacity(line.substr(equal+1),settings.centerOpacityPercent)) ++rejected;
    }
    return settings;
}
inline unsigned ParseScaleConfig(std::string_view text,unsigned& rejected) noexcept {
    return ParseConfig(text,rejected).scalePercent;
}
inline bool FadeOpacity(float& opacity,unsigned percent) noexcept {
    if(percent<10 || percent>=100 || !std::isfinite(opacity) || opacity<0 || opacity>1) return false;
    opacity*=static_cast<float>(percent)/100.0f;return true;
}
// HD sprite rectangle is [anchor - pivot*scale, anchor + (size-pivot)*scale].
// Keep that rectangle's center fixed, rounding only the final pixel anchor.
inline bool CenterScale(Point& anchor,float& scale,const SpriteBounds& b,unsigned percent) noexcept {
    if(percent<10 || percent>=100 || !std::isfinite(scale) || scale<=0 || scale>16 ||
       b.width<=0 || b.width>8192 || b.height<=0 || b.height>8192 ||
       b.pivotX< -16384 || b.pivotX>16384 || b.pivotY< -16384 || b.pivotY>16384) return false;
    const float reduced=scale*(static_cast<float>(percent)/100.0f);
    const double delta=static_cast<double>(scale)-reduced;
    const double x=std::round(anchor.x+delta*(b.width*0.5-b.pivotX));
    const double y=std::round(anchor.y+delta*(b.height*0.5-b.pivotY));
    constexpr auto low=std::numeric_limits<std::int32_t>::min(),high=std::numeric_limits<std::int32_t>::max();
    if(x<low || x>high || y<low || y>high) return false;
    anchor={static_cast<std::int32_t>(x),static_cast<std::int32_t>(y)};scale=reduced;return true;
}
}
