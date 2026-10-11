#pragma once
#include "config.h"

namespace Damage {
enum class Control { Toggle, On, Off, Status, Invalid };
inline bool Matches(std::string_view args,std::string_view word) noexcept {
    if(args.size()!=word.size()) return false;
    for(std::size_t i=0;i<args.size();++i) {
        const char c=args[i]>='A' && args[i]<='Z' ? static_cast<char>(args[i]-'A'+'a'):args[i];
        if(c!=word[i]) return false;
    }
    return true;
}
inline Control ParseControl(std::string_view args) noexcept {
    args=Trim(args);
    if(args.empty() || Matches(args,"toggle")) return Control::Toggle;
    if(Matches(args,"on")) return Control::On;
    if(Matches(args,"off")) return Control::Off;
    if(Matches(args,"status")) return Control::Status;
    return Control::Invalid;
}
struct CommandRequest { bool motion{}; Control action=Control::Invalid; bool capture{}; };
inline CommandRequest ParseRequest(std::string_view args) noexcept {
    args=Trim(args);
    const auto end=args.find_first_of(" \t\r");
    if(Matches(args.substr(0,end),"motion")) return {true,ParseControl(end==args.npos?std::string_view{}:args.substr(end+1))};
    if(Matches(args.substr(0,end),"capture")) return {false,ParseControl(end==args.npos?std::string_view{}:args.substr(end+1)),true};
    return {false,ParseControl(args)};
}
inline bool UpdateFlag(Control action,bool& value) noexcept {
    const bool next=action==Control::Toggle ? !value:action==Control::On ? true:action==Control::Off ? false:value;
    if(next==value) return false;
    value=next;return true;
}
// Called under the plugin's state mutex. Clear HP baselines and visible numbers
// only on a real transition, so damage taken while disabled cannot reappear.
inline bool ApplyControl(Control action,bool& enabled,Model& model) noexcept {
    if(!UpdateFlag(action,enabled)) return false;
    model.Clear();return true;
}
}
