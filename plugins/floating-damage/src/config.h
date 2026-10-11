#pragma once
#include "model.h"
#include <charconv>
#include <string_view>
namespace Damage {
inline std::string_view Trim(std::string_view s) noexcept {
    while(!s.empty() && (s.front()==' ' || s.front()=='\t' || s.front()=='\r')) s.remove_prefix(1);
    while(!s.empty() && (s.back()==' ' || s.back()=='\t' || s.back()=='\r')) s.remove_suffix(1);
    return s;
}
inline std::string_view StripComment(std::string_view line) noexcept {
    char quote{};
    for(std::size_t i=0;i<line.size();++i) {
        const char c=line[i];
        if(quote) {if(c==quote) quote=0;}
        else if(c=='\'' || c=='"') quote=c;
        else if(c=='#') return line.substr(0,i);
    }
    return line;
}
inline bool Color(std::string_view value,std::array<float,3>& out) noexcept {
    if(value.size()==9 && (value.front()=='"' || value.front()=='\'') && value.back()==value.front() && value[1]=='#') {
        const auto digit=[](char c)->int {return c>='0' && c<='9'?c-'0':c>='a' && c<='f'?c-'a'+10:c>='A' && c<='F'?c-'A'+10:-1;};
        std::array<float,3> parsed{};
        for(unsigned i=0;i<3;++i) {
            const int high=digit(value[2+i*2]),low=digit(value[3+i*2]);
            if(high<0 || low<0) return false;
            parsed[i]=static_cast<float>(high*16+low)/255.0f;
        }
        out=parsed;return true;
    }
    // Retain beta.4 RGB arrays so upgrading cannot discard custom colors.
    if(value.size()<2 || value.front()!='[' || value.back()!=']') return false;
    value.remove_prefix(1);value.remove_suffix(1);
    std::array<float,3> parsed{};
    for(unsigned i=0;i<3;++i) {
        const auto comma=value.find(',');
        const auto part=Trim(value.substr(0,comma));
        const auto number=std::from_chars(part.data(),part.data()+part.size(),parsed[i]);
        if(part.empty() || number.ec!=std::errc{} || number.ptr!=part.data()+part.size() ||
            !std::isfinite(parsed[i]) || parsed[i]<0 || parsed[i]>1 || (i<2 ? comma==value.npos:comma!=value.npos)) return false;
        if(comma!=value.npos) value.remove_prefix(comma+1);
    }
    out=parsed;return true;
}
inline Settings Parse(std::string_view text,unsigned& rejected) noexcept {
    Settings config; bool section=false; rejected=0;
    while(!text.empty()) {
        const auto end=text.find('\n'); auto line=Trim(text.substr(0,end));
        text=end==text.npos ? std::string_view{}:text.substr(end+1);
        line=Trim(StripComment(line));
        if(line.empty()) continue;
        if(line.front()=='[') { section=line=="[floating-damage]"; continue; }
        if(!section) continue;
        const auto eq=line.find('=');
        if(eq==line.npos) {++rejected; continue;}
        const auto key=Trim(line.substr(0,eq)), value=Trim(line.substr(eq+1));
        if(key=="critical_hits") {
            if(value!="true" && value!="false") {++rejected;continue;}
            config.criticalHits=value=="true";continue;
        }
        if(key=="backing" || key=="backing_poison_only") {
            if(value!="true" && value!="false") {++rejected;continue;}
            (key=="backing"?config.backing:config.backingPoisonOnly)=value=="true";continue;
        }
        if(key=="enabled" || key=="debug" || key=="falling_motion" || key=="shadow" || key=="element_colors") {
            if(value!="true" && value!="false") {++rejected;continue;}
            (key=="enabled"?config.enabled:key=="debug"?config.debug:key=="shadow"?config.shadow:key=="element_colors"?config.elementColors:config.fallingMotion)=value=="true"; continue;
        }
        if(key=="physical_color" || key=="fire_color" || key=="cold_color" || key=="lightning_color" || key=="magic_color" || key=="poison_color" || key=="mixed_color") {
            if(!Color(value,key=="physical_color"?config.physicalColor:key=="fire_color"?config.fireColor:
                key=="cold_color"?config.coldColor:key=="lightning_color"?config.lightningColor:
                key=="magic_color"?config.magicColor:key=="poison_color"?config.poisonColor:config.mixedColor)) ++rejected;
            continue;
        }
        if(key=="text_color" || key=="outline_color" || key=="shadow_color" || key=="backing_color") {
            if(!Color(value,key=="text_color"?config.textColor:key=="outline_color"?config.outlineColor:key=="shadow_color"?config.shadowColor:config.backingColor)) ++rejected;
            continue;
        }
        float number{};
        const auto result=std::from_chars(value.data(),value.data()+value.size(),number);
        if(result.ec!=std::errc{} || result.ptr!=value.data()+value.size() || !std::isfinite(number)) {++rejected;continue;}
        auto setFloat=[&](float& target,float low,float high){if(number<low || number>high) ++rejected; else target=number;};
        auto setInt=[&](unsigned& target,unsigned low,unsigned high){if(number<static_cast<float>(low) || number>static_cast<float>(high) || std::floor(number)!=number) ++rejected; else target=static_cast<unsigned>(number);};
        if(key=="text_size") setFloat(config.textSize,12,64);
        else if(key=="critical_size_scale") setFloat(config.criticalScale,1,2);
        else if(key=="outline_width") setInt(config.outlineWidth,0,3);
        else if(key=="fade_start") setFloat(config.fadeStart,0,0.95f);
        else if(key=="side_pixels") setFloat(config.side,0,200);
        else if(key=="fall_pixels") setFloat(config.fall,0,300);
        else if(key=="shadow_offset_x") setFloat(config.shadowX,-20,20);
        else if(key=="shadow_offset_y") setFloat(config.shadowY,-20,20);
        else if(key=="shadow_opacity") setFloat(config.shadowOpacity,0,1);
        else if(key=="shadow_softness") setInt(config.shadowSoftness,0,2);
        else if(key=="backing_opacity") setFloat(config.backingOpacity,0,1);
        else if(key=="backing_padding_x") setFloat(config.backingPaddingX,0,12);
        else if(key=="backing_padding_y") setFloat(config.backingPaddingY,0,12);
        else if(key=="backing_rounding") setFloat(config.backingRounding,0,12);
        else if(key=="rise_pixels") setFloat(config.rise,0,300);
        else if(key=="anchor_offset_pixels") setFloat(config.offset,0,200);
        else if(key=="lane_spacing_pixels") setFloat(config.laneSpacing,10,80);
        else if(key=="lifetime_ms") setInt(config.lifetime,250,3000);
        else if(key=="tick_merge_ms") setInt(config.mergeMs,0,300);
        else if(key=="decimal_places") setInt(config.precision,0,2);
        else if(key=="max_numbers") setInt(config.maximum,1,128);
        else ++rejected;
    }
    return config;
}
}
