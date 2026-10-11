#pragma once
#include <charconv>
#include <string_view>
namespace InventoryDeposit {
struct Settings {
    bool enabled{true};
    int x{860},y{1620};
    bool anchorToGold{true};
    int goldGap{85},goldOffsetY{1};
};
inline std::string_view Trim(std::string_view s) noexcept {
    const auto first=s.find_first_not_of(" \t\r");
    if(first==s.npos)return {};
    s.remove_prefix(first);s.remove_suffix(s.size()-s.find_last_not_of(" \t\r")-1);return s;
}
// Exact keys in the plugin's own section; malformed recognized values fail
// closed, rather than accidentally enabling an item action from a comment.
inline bool ParseSettings(std::string_view text,Settings& output) noexcept {
    Settings parsed{};bool section=false,seenSection=false;unsigned seen=0;
    while(!text.empty()) {
        const auto end=text.find('\n');auto line=text.substr(0,end);
        text=end==text.npos?std::string_view{}:text.substr(end+1);
        line=Trim(line.substr(0,line.find('#')));if(line.empty())continue;
        if(line.front()=='[') {
            section=line=="[inventory_deposit]";
            if(section && seenSection)return false;
            seenSection|=section;continue;
        }
        if(!section)continue;
        const auto equal=line.find('=');if(equal==line.npos)return false;
        const auto key=Trim(line.substr(0,equal)),value=Trim(line.substr(equal+1));
        const unsigned bit=key=="enabled"?1:key=="button_x"?2:key=="button_y"?4:
            key=="anchor_to_gold"?8:key=="gold_gap"?16:key=="gold_offset_y"?32:0;
        if(!bit)continue;
        if(seen&bit)return false;seen|=bit;
        if(bit==1 || bit==8) {
            if(value!="true" && value!="false")return false;
            (bit==1?parsed.enabled:parsed.anchorToGold)=value=="true";
        }
        else {
            int n{};const auto [at,error]=std::from_chars(value.data(),value.data()+value.size(),n);
            const int minimum=bit==32?-4096:0;
            if(error!=std::errc{} || at!=value.data()+value.size() || n<minimum || n>4096)return false;
            if(bit==2)parsed.x=n;
            else if(bit==4)parsed.y=n;
            else if(bit==16)parsed.goldGap=n;
            else parsed.goldOffsetY=n;
        }
    }
    output=parsed;return true;
}
}
