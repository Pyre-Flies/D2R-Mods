#pragma once
#include <algorithm>
#include <initializer_list>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ChatConfig {
struct Boolean {std::string_view key;bool value;};
inline std::string_view Trim(std::string_view text) noexcept {
    const auto first=text.find_first_not_of(" \t\r");
    if(first==text.npos) return {};
    return text.substr(first,text.find_last_not_of(" \t\r")-first+1);
}
// Edit one known scalar without reserializing the document. Refuse ambiguous
// duplicate keys/sections and multiline strings rather than altering user text.
inline std::optional<std::string> UpdateBoolean(std::string_view text,std::string_view section,Boolean setting) {
    if(text.find('\0')!=text.npos || text.find("\"\"\"")!=text.npos || text.find("'''")!=text.npos) return {};
    const std::string header="["+std::string(section)+"]";
    const std::string_view eol=text.find("\r\n")!=text.npos?"\r\n":"\n";
    bool inside=false,foundSection=false,foundKey=false;
    std::size_t insertion=text.size(),firstValue=0,lastValue=0;
    for(std::size_t cursor=0;cursor<text.size();) {
        const auto newline=text.find('\n',cursor),end=newline==text.npos?text.size():newline;
        auto line=text.substr(cursor,end-cursor);
        std::size_t bom=cursor==0 && line.starts_with("\xef\xbb\xbf")?3:0;
        line.remove_prefix(bom);
        const auto comment=line.find('#');
        const auto logical=Trim(line.substr(0,comment));
        if(!logical.empty() && logical.front()=='[') {
            if(logical.back()==']') {
                auto candidate=Trim(logical.substr(1,logical.size()-2));
                if(candidate.size()>=2 && (candidate.front()=='\'' || candidate.front()=='"') && candidate.back()==candidate.front())
                    candidate=candidate.substr(1,candidate.size()-2);
                if(candidate==section && logical!=header) return {};
            }
            if(inside) insertion=cursor;
            inside=logical==header;
            if(inside) {if(foundSection) return {};foundSection=true;insertion=text.size();}
        } else if(inside) {
            if(logical==setting.key) return {};
            const auto equal=line.find('=');
            const auto key=Trim(line.substr(0,equal));
            if(key==setting.key) {
                if(foundKey || equal==line.npos) return {};
                foundKey=true;
                auto first=equal+1,last=comment==line.npos?line.size():comment;
                while(first<last && (line[first]==' ' || line[first]=='\t')) ++first;
                while(last>first && (line[last-1]==' ' || line[last-1]=='\t' || line[last-1]=='\r')) --last;
                const auto previous=Trim(line.substr(first,last-first));
                if(previous!="true" && previous!="false") return {};
                firstValue=cursor+bom+first;lastValue=cursor+bom+last;
            } else if(key.size()>=2 && (key.front()=='\'' || key.front()=='"') && key.back()==key.front() &&
                      key.substr(1,key.size()-2)==setting.key) return {};
        }
        cursor=newline==text.npos?text.size():newline+1;
    }
    const std::string_view value=setting.value?"true":"false";
    std::string output(text);
    if(foundKey) output.replace(firstValue,lastValue-firstValue,value);
    else {
        std::string added;
        if(insertion && text[insertion-1]!='\n') added+=eol;
        if(!foundSection) {added+=header;added+=eol;}
        added+=setting.key;added+=" = ";added+=value;added+=eol;
        output.insert(insertion,added);
    }
    return output;
}
template<class Context>
bool Save(const Context* context,std::string_view section,std::initializer_list<Boolean> changes,std::size_t capacity) noexcept {
    if(!context || capacity<2 || capacity>65537) return false;
    try {
        static std::mutex mutex;
        const std::lock_guard guard(mutex);
        std::vector<char> buffer(capacity,0);unsigned required{};
        if(!context->ReadConfig(buffer.data(),static_cast<unsigned>(buffer.size()),&required) || required>capacity) return false;
        const auto terminator=std::find(buffer.begin(),buffer.end(),'\0');
        if(terminator==buffer.end()) return false;
        std::string updated(buffer.begin(),terminator);
        for(const auto change:changes) {
            auto next=UpdateBoolean(updated,section,change);
            if(!next || next->size()>=capacity) return false;
            updated=std::move(*next);
        }
        return context->WriteConfig(updated.c_str());
    } catch(...) {return false;}
}
}
