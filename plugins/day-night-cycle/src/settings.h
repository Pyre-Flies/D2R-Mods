#pragma once
#include <ChatConfig/chat_config.h>
#include <array>
#include <cstring>
#include "preview.h"

namespace DayNight {
inline constexpr unsigned ConfigCapacity=16384;
inline constexpr std::string_view ConfigSection="day-night-cycle";
inline std::optional<bool> ParseEnabled(std::string_view text) {
    // Qualify duplicate/malformed keys, headers and multiline input before read.
    if(!ChatConfig::UpdateBoolean(text,ConfigSection,{"enabled",true})) return {};
    bool inside=false;
    for(size_t cursor=0;cursor<text.size();) {
        const auto newline=text.find('\n',cursor),end=newline==text.npos?text.size():newline;
        auto line=text.substr(cursor,end-cursor);
        if(cursor==0 && line.starts_with("\xef\xbb\xbf")) line.remove_prefix(3);
        line=ChatConfig::Trim(line.substr(0,line.find('#')));
        if(line.starts_with('[')) inside=line=="[day-night-cycle]";
        else if(inside) {
            const auto equal=line.find('=');
            if(equal!=line.npos && ChatConfig::Trim(line.substr(0,equal))=="enabled")
                return ChatConfig::Trim(line.substr(equal+1))=="true";
        }
        cursor=newline==text.npos?text.size():newline+1;
    }
    return true;
}

struct Settings {bool enabled=true;Mode mode=Mode::Auto;bool operator==(const Settings&) const = default;};
struct ValueSpan {size_t first{},last{};};
inline std::optional<ValueSpan> ModeSpan(std::string_view text) {
    bool inside=false;
    for(size_t cursor=0;cursor<text.size();) {
        const auto newline=text.find('\n',cursor),end=newline==text.npos?text.size():newline;
        auto line=text.substr(cursor,end-cursor);size_t bom=cursor==0 && line.starts_with("\xef\xbb\xbf")?3:0;
        line.remove_prefix(bom);const auto logical=ChatConfig::Trim(line.substr(0,line.find('#')));
        if(logical.starts_with('[')) inside=logical=="[day-night-cycle]";
        else if(inside) {
            const auto equal=line.find('=');
            if(equal!=line.npos && ChatConfig::Trim(line.substr(0,equal))=="mode") {
                size_t first=equal+1,last=line.find('#');if(last==line.npos)last=line.size();
                while(first<last && (line[first]==' ' || line[first]=='\t'))++first;
                while(last>first && (line[last-1]==' ' || line[last-1]=='\t' || line[last-1]=='\r'))--last;
                return ValueSpan{cursor+bom+first,cursor+bom+last};
            }
        }
        cursor=newline==text.npos?text.size():newline+1;
    }
    return {};
}
inline std::optional<Mode> ParseModeValue(std::string_view text) noexcept {
    if(text=="\"auto\"" || text=="'auto'")return Mode::Auto;
    if(text=="\"day\"" || text=="'day'")return Mode::Day;
    if(text=="\"night\"" || text=="'night'")return Mode::Night;
    return {};
}
inline std::optional<std::string> UpdateMode(std::string_view text,Mode mode) {
    std::string qualified(text);
    if(const auto span=ModeSpan(text)) {
        if(!ParseModeValue(text.substr(span->first,span->last-span->first)))return {};
        qualified.replace(span->first,span->last-span->first,"true");
    }
    // The proven scalar editor validates duplicate/quoted keys, sections and
    // ambiguous syntax using a boolean placeholder. Restore only its value.
    auto updated=ChatConfig::UpdateBoolean(qualified,ConfigSection,{"mode",false});
    if(!updated)return {};
    const auto span=ModeSpan(*updated);if(!span)return {};
    updated->replace(span->first,span->last-span->first,std::string("\"")+ModeName(mode)+"\"");return updated;
}
inline std::optional<Settings> ParseConfig(std::string_view text) {
    const auto enabled=ParseEnabled(text);if(!enabled || !UpdateMode(text,Mode::Auto))return {};
    const auto span=ModeSpan(text);const auto mode=span?ParseModeValue(text.substr(span->first,span->last-span->first)):std::optional{Mode::Auto};
    if(!mode)return {};return Settings{*enabled,*mode};
}
inline std::optional<std::string> UpdateConfig(std::string_view text,Settings value) {
    const auto enabled=ChatConfig::UpdateBoolean(text,ConfigSection,{"enabled",value.enabled});
    return enabled?UpdateMode(*enabled,value.mode):std::nullopt;
}
template<class Context> std::optional<std::string> ReadSettings(const Context* context) {
    if(!context) return {};
    std::array<char,ConfigCapacity> buffer{};uint32_t required{};
    if(!context->ReadConfig(buffer.data(),ConfigCapacity,&required) || required>ConfigCapacity) return {};
    const auto length=strnlen_s(buffer.data(),buffer.size());
    if(length==buffer.size() || (required && required!=length+1)) return {};
    return std::string(buffer.data(),length);
}
}
