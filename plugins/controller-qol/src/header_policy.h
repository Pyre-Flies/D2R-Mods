#pragma once
#include <string_view>
namespace QolHeaderPolicy {
inline bool Scope(const std::string_view* names,size_t count) noexcept {
    return count>=5 && names[0]=="Text" && names[1]=="Legend" &&
        names[2]=="legendBG" && names[3]=="Anchor" && names[4]=="ControllerOverlay";
}
inline bool RangeLabel(std::string_view text) noexcept {
    return text=="\xEE\x80\x8E" "Show Ranges";
}
inline int Slot(std::string_view text) noexcept {
    if(text.starts_with("\xEE\x80\x8F")) return 0;
    if(text.starts_with("\xEE\x80\x91")) return 1;
    if(text.starts_with("\xEE\x80\x92")) return 2;
    if(text.starts_with("\xEE\x80\x8A")) return 3;
    return -1;
}
inline const char* Modifier(std::string_view name) noexcept {
    if(name=="LB")return "\xEE\x80\xA7";
    if(name=="RB")return "\xEE\x80\xA8";
    if(name=="LT")return "\xEE\x80\x9B";
    if(name=="RT")return "\xEE\x80\x9C";
    if(name=="L3")return "\xEE\x80\x88";
    return name=="["?"[":"";
}
inline constexpr const char* Buttons[]={"\xEE\x80\x8F","\xEE\x80\x91","\xEE\x80\x92","\xEE\x80\x8A"};
inline bool Fresh(unsigned long long now,unsigned long long tick) noexcept {
    return tick!=0 && now>=tick && now-tick<=250;
}
}
