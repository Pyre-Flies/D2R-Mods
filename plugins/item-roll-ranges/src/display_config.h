#pragma once
#include <string_view>
namespace RollRanges {
struct DisplaySettings {bool alwaysShowRanges{};bool valid{true};};
inline std::string_view TrimConfig(std::string_view text) noexcept {
    const auto first=text.find_first_not_of(" \t\r");
    if(first==text.npos)return {};
    const auto last=text.find_last_not_of(" \t\r");return text.substr(first,last-first+1);
}
// A bounded single-boolean schema, like the repository's existing lightweight
// plugin settings readers. Unknown keys/sections are ignored. Never rewrite
// user text; malformed or duplicate known options retain ordinary hold mode.
inline DisplaySettings ParseDisplayConfig(std::string_view text) noexcept {
    if(text.size()>65536)return {false,false};
    if(text.starts_with("\xef\xbb\xbf"))text.remove_prefix(3);
    bool display=false,seen=false;DisplaySettings result;
    while(!text.empty()){
        const auto end=text.find('\n');auto line=TrimConfig(text.substr(0,end));
        text=end==text.npos?std::string_view{}:text.substr(end+1);
        line=TrimConfig(line.substr(0,line.find('#')));if(line.empty())continue;
        if(line[0]=='['){display=line=="[display]";continue;}
        if(!display)continue;
        const auto equals=line.find('=');const auto key=TrimConfig(line.substr(0,equals));
        if(key!="always_show_ranges")continue;
        if(seen || equals==line.npos)return {false,false};seen=true;
        const auto value=TrimConfig(line.substr(equals+1));
        if(value=="true")result.alwaysShowRanges=true;
        else if(value!="false")return {false,false};
    }
    return result;
}
}
