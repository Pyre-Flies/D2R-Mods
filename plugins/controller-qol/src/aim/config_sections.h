#pragma once
#include <string>
#include <string_view>
namespace QolAim {
inline std::string_view Trim(std::string_view s) {
    const auto first=s.find_first_not_of(" \t\r");
    return first==s.npos?std::string_view{}:s.substr(first,s.find_last_not_of(" \t\r")-first+1);
}
// Section boundaries and comments cannot leak aim.enabled into qol.enabled.
inline bool Section(std::string_view text,std::string_view name,std::string& out) {
    out.clear(); bool active=false,seen=false;
    while(!text.empty()) {
        const auto end=text.find('\n'); auto line=Trim(text.substr(0,end));
        text=end==text.npos?std::string_view{}:text.substr(end+1);
        line=Trim(line.substr(0,line.find('#')));
        if(line.empty()) continue;
        if(line.front()=='[') {
            active=line.size()==name.size()+2 && line.back()==']' && line.substr(1,name.size())==name;
            if(active) { if(seen) return false; seen=true; }
        } else if(active) { out.append(line); out.push_back('\n'); }
    }
    return true;
}
}
