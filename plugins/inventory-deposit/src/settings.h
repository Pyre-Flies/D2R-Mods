#pragma once
#include <charconv>
#include <cstdint>
#include <string_view>
namespace InventoryDeposit {
struct Settings {
    bool enabled{true};
    bool depositRejuvenationPotions{true};
    int x{860},y{1620};
    bool anchorToGold{true};
    int goldGap{85},goldOffsetY{1};
};
enum class CommandAction {
    RejuvToggle,
    RejuvOn,
    RejuvOff,
    Status,
    Help,
    Invalid,
};
inline constexpr char CommandHelp[]="/autodeposit rejuv [on|off|toggle|status] | status | help";
static_assert(sizeof(CommandHelp)<=64);
constexpr uint32_t TrimItemCode(uint32_t code) noexcept {
    return (code&0xFF000000u)==0x20000000u?(code&0x00FFFFFFu):code;
}
constexpr uint32_t ThreeCharCode(char a,char b,char c) noexcept {
    return static_cast<uint8_t>(a) |
        (static_cast<uint32_t>(static_cast<uint8_t>(b))<<8) |
        (static_cast<uint32_t>(static_cast<uint8_t>(c))<<16);
}
constexpr bool IsRejuvenationPotionCode(uint32_t code) noexcept {
    const uint32_t trimmed=TrimItemCode(code);
    return trimmed==ThreeCharCode('r','v','s') || trimmed==ThreeCharCode('r','v','l');
}
inline std::string_view Trim(std::string_view s) noexcept {
    const auto first=s.find_first_not_of(" \t\r");
    if(first==s.npos)return {};
    s.remove_prefix(first);s.remove_suffix(s.size()-s.find_last_not_of(" \t\r")-1);return s;
}
inline bool EqualsIgnoreCase(std::string_view a,std::string_view b) noexcept {
    if(a.size()!=b.size())return false;
    for(std::size_t i=0;i<a.size();++i) {
        const auto ca=(a[i]>='A' && a[i]<='Z')?static_cast<char>(a[i]-'A'+'a'):a[i];
        const auto cb=(b[i]>='A' && b[i]<='Z')?static_cast<char>(b[i]-'A'+'a'):b[i];
        if(ca!=cb)return false;
    }
    return true;
}
inline std::string_view NextToken(std::string_view& s) noexcept {
    s=Trim(s);
    if(s.empty())return {};
    const auto split=s.find_first_of(" \t\r");
    if(split==s.npos) {
        const auto token=s;
        s={};
        return token;
    }
    const auto token=s.substr(0,split);
    s.remove_prefix(split);
    s=Trim(s);
    return token;
}
inline CommandAction ParseCommand(std::string_view arguments) noexcept {
    auto rest=Trim(arguments);
    if(rest.empty())return CommandAction::Status;
    auto first=NextToken(rest);
    if(EqualsIgnoreCase(first,"/autodeposit") || EqualsIgnoreCase(first,"autodeposit")) {
        first=NextToken(rest);
        if(first.empty())return CommandAction::Status;
    }
    if(rest.empty()) {
        if(EqualsIgnoreCase(first,"status"))return CommandAction::Status;
        if(EqualsIgnoreCase(first,"help") || first=="?")return CommandAction::Help;
    }
    if(EqualsIgnoreCase(first,"rejuv") || EqualsIgnoreCase(first,"rejuvs") ||
       EqualsIgnoreCase(first,"rejuvenation")) {
        const auto second=NextToken(rest);
        if(!rest.empty())return CommandAction::Invalid;
        if(second.empty() || EqualsIgnoreCase(second,"toggle"))return CommandAction::RejuvToggle;
        if(EqualsIgnoreCase(second,"on"))return CommandAction::RejuvOn;
        if(EqualsIgnoreCase(second,"off"))return CommandAction::RejuvOff;
        if(EqualsIgnoreCase(second,"status"))return CommandAction::Status;
        if(EqualsIgnoreCase(second,"help") || second=="?")return CommandAction::Help;
    }
    return CommandAction::Invalid;
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
            key=="anchor_to_gold"?8:key=="gold_gap"?16:key=="gold_offset_y"?32:
            key=="deposit_rejuvenation_potions"?64:0;
        if(!bit)continue;
        if(seen&bit)return false;seen|=bit;
        if(bit==1 || bit==8 || bit==64) {
            if(value!="true" && value!="false")return false;
            const bool flag=value=="true";
            if(bit==1)parsed.enabled=flag;
            else if(bit==8)parsed.anchorToGold=flag;
            else parsed.depositRejuvenationPotions=flag;
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
