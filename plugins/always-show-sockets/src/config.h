#pragma once
#include "scale.h"
#include <string>

namespace Sockets {
constexpr unsigned ConfigCapacity=4096;
// Replace only the selected numeric key, retaining comments, line endings and
// other settings/sections. A missing key is inserted inside the plugin section.
inline std::string UpdateConfig(std::string_view text,std::string_view key,unsigned value) {
    const std::string_view eol=text.find("\r\n")!=text.npos?"\r\n":"\n";
    const auto number=std::to_string(value);
    std::string output;
    bool section=false,foundSection=false,replaced=false;
    std::size_t insertion=std::string::npos;
    for(std::size_t cursor=0;cursor<text.size();) {
        const auto newline=text.find('\n',cursor);
        const auto end=newline==text.npos?text.size():newline;
        const auto line=text.substr(cursor,end-cursor);
        const auto comment=line.find('#');
        auto logical=Trim(line.substr(0,comment));
        if(cursor==0 && logical.starts_with("\xef\xbb\xbf")) logical=Trim(logical.substr(3));
        if(!logical.empty() && logical.front()=='[') {
            if(section && insertion==std::string::npos) insertion=output.size();
            section=logical=="[always-show-sockets]";foundSection|=section;
        }
        const auto equal=line.find('=');
        if(section && equal!=line.npos && Trim(line.substr(0,equal))==key) {
            auto first=equal+1,last=comment==line.npos?line.size():comment;
            while(first<last && (line[first]==' ' || line[first]=='\t')) ++first;
            while(last>first && (line[last-1]==' ' || line[last-1]=='\t' || line[last-1]=='\r')) --last;
            output.append(line.substr(0,first));output+=number;output.append(line.substr(last));
            replaced=true;
        } else output.append(line);
        if(newline!=text.npos) output+='\n';
        cursor=newline==text.npos?text.size():newline+1;
    }
    if(!replaced) {
        if(insertion==std::string::npos) insertion=output.size();
        std::string added;
        if(insertion && output[insertion-1]!='\n') added+=eol;
        if(!foundSection) {added+="[always-show-sockets]";added+=eol;}
        added+=key;added+=" = ";added+=number;added+=eol;
        output.insert(insertion,added);
    }
    return output;
}
}
