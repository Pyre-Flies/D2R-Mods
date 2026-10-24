#pragma once
#include "range_text.h"
#include <optional>
namespace HeaderText {
inline constexpr std::string_view Footer="(?) Range Unavailable - Please report item affixes";
// The native builder returns the whole tooltip in bottom-to-top line order,
// including modifiers. Capacity belongs just before Required Level in that
// buffer, not at its beginning. For items without a level requirement, use
// the first (lowest displayed) recognized native header line.
inline std::string SocketCapacity(std::string_view text,std::string_view capacityKey,
    std::string_view label,const std::vector<std::string>& anchors) {
    const auto lines=RangeText::Lines(text);
    for(const auto line:lines)if(RangeText::Analyze(line).key==capacityKey)return std::string(text);
    if(anchors.empty() || anchors[0].empty())return std::string(text);
    const auto matches=[](std::string_view line,const std::string& key) {
        if(key.empty())return false;
        const auto shape=RangeText::Analyze(line).key;
        // Existing Base or unknown annotations can extend a Defense/damage
        // key; accept only the suffix shapes produced by this header adapter.
        return shape==key || shape==key+"#" || shape==key+"(?)";
    };
    std::size_t position=lines.size();unsigned count=0;
    for(std::size_t n=0;n<lines.size();++n)if(matches(lines[n],anchors[0])){position=n;++count;}
    if(count>1)return std::string(text);
    if(!count)for(std::size_t a=1;a<anchors.size();++a){
        std::size_t at=lines.size();count=0;
        for(std::size_t n=0;n<lines.size();++n)if(matches(lines[n],anchors[a])){at=n;++count;}
        if(count>1)return std::string(text);
        if(count)position=std::min(position,at);
    }
    if(position==lines.size())return std::string(text);
    std::string result;
    for(std::size_t n=0;n<lines.size();++n){
        if(n==position)result+="\xee\x81\xbe" "0"+std::string(label)+'\n';
        result+=lines[n];result+='\n';
    }
    if(!text.ends_with('\n') && !result.empty())result.pop_back();
    return result;
}
inline std::string ReplaceSocketCapacity(std::string_view text,std::string_view capacityKey,std::string_view label) {
    if(capacityKey.empty() || label.empty())return std::string(text);
    const auto lines=RangeText::Lines(text);
    unsigned matches=0;
    for(const auto line:lines)if(RangeText::Analyze(line).key==capacityKey)++matches;
    if(matches!=1)return std::string(text);
    std::string result;
    for(std::size_t n=0;n<lines.size();++n){
        if(RangeText::Analyze(lines[n]).key==capacityKey)result+="\xee\x81\xbe" "0"+std::string(label);
        else result+=lines[n];
        result+='\n';
    }
    if(!text.ends_with('\n') && !result.empty())result.pop_back();
    return result;
}
// Use the same localized requirement/header anchors as capacity, after capacity
// has been inserted. This displays Required Level / levels / Max Sockets / speed.
// Jewelry and other items can have no native numeric header at all; use the
// caller-proven title boundary in that case; otherwise omit the line.
inline std::string LevelInfo(std::string_view text,std::string_view label,
    const std::vector<std::string>& anchors,std::string_view title={}) {
    const auto key=RangeText::Analyze(label).key;
    if(key.empty() || anchors.empty() || anchors[0].empty())return std::string(text);
    const auto lines=RangeText::Lines(text);
    for(const auto line:lines)if(RangeText::Analyze(line).key==key)return std::string(text);
    auto result=SocketCapacity(text,key,label,anchors);
    if(result!=text)return result;
    // A recognized or ambiguous header that could not be safely placed must
    // not fall through to the no-header case.
    for(const auto line:lines){
        const auto shape=RangeText::Analyze(line).key;
        for(const auto& anchor:anchors)if(!anchor.empty() &&
            (shape==anchor || shape==anchor+"#" || shape==anchor+"(?)"))return std::string(text);
    }
    // A full tooltip already contains its title. Appending here displays the
    // metadata above it. Only insert at a caller-proven single-line title.
    if(title.empty() || lines.empty() || lines.back()!=title)return std::string(text);
    unsigned titles=0;for(const auto line:lines)titles+=line==title;
    if(titles!=1)return std::string(text);
    result.clear();
    for(std::size_t n=0;n<lines.size();++n){
        if(n+1==lines.size())result+="\xee\x81\xbe" "0"+std::string(label)+'\n';
        result+=lines[n];result+='\n';
    }
    if(!text.ends_with('\n'))result.pop_back();
    return result;
}
inline std::vector<long long> Numbers(std::string_view line) {
    std::string plain;
    for (std::size_t n=0;n<line.size();) {
        char color{}; const auto token=RangeText::Color(line,n,color);
        if (token) n+=token; else plain+=line[n++];
    }
    std::vector<long long> result;
    for (std::size_t n=0;n<plain.size();++n) if (RangeText::Digit(plain[n])) {
        char* end{}; result.push_back(std::strtoll(plain.c_str()+n,&end,10));
        n=static_cast<std::size_t>(end-plain.c_str())-1;
    }
    return result;
}
inline std::string Damage(std::string_view text,std::string_view actual,std::string_view key,
    std::optional<std::pair<int,int>> bounds) {
    unsigned count=0; std::string_view original;
    for (const auto line:RangeText::Lines(actual)) if (RangeText::Analyze(line).key==key) { original=line; ++count; }
    const auto values=Numbers(original);
    if (count!=1 || values.size()!=2 || values[0]<0 || values[0]>values[1]) return std::string(text);
    // Core can wrap a whole min/max fragment in one range color span. Its
    // normalized numeric shape then differs from the original two placeholders.
    // Match the unique localized literal header prefix witnessed in actual text.
    const auto number=key.find('#');
    const auto prefix=key.substr(0,number);
    const auto matches=[&](std::string_view line) {
        const auto candidate=RangeText::Analyze(line).key;
        return candidate==key || (number!=key.npos && prefix.size()>=3 && candidate.starts_with(prefix) &&
            candidate.size()>prefix.size() && candidate[prefix.size()]=='#');
    };
    count=0; for (const auto line:RangeText::Lines(text)) count+=matches(line);
    if (count!=1) return std::string(text);
    if (bounds && (bounds->first<0 || bounds->first>bounds->second)) bounds.reset();
    std::string result;
    for (const auto line:RangeText::Lines(text)) {
        if (!matches(line)) result+=line;
        else {
            result+=original;
            if (bounds) result+=" \xee\x81\xbe" "U(Base: "+std::to_string(bounds->first)+" - "+
                std::to_string(bounds->second)+")\xee\x81\xbe" "3";
            else result+=" \xee\x81\xbe" "5(?)\xee\x81\xbe" "3";
        }
        result+='\n';
    }
    if (!text.ends_with('\n') && !result.empty()) result.pop_back();
    if (!bounds && result.find(Footer)==result.npos)
        result="\xee\x81\xbe" "5"+std::string(Footer)+"\xee\x81\xbe" "3\n"+result;
    return result;
}
inline std::optional<long long> Value(std::string_view text,std::string_view key) {
    std::optional<long long> result;
    for (const auto line:RangeText::Lines(text)) if (RangeText::Analyze(line).key==key) {
        if (result) return {};
        std::string plain;
        for (std::size_t n=0;n<line.size();) {
            char color{}; const auto token=RangeText::Color(line,n,color);
            if (token) n+=token; else plain+=line[n++];
        }
        for (std::size_t n=0;n<plain.size();++n) if (RangeText::Digit(plain[n])) {
            if (result) return {};
            char* end{}; result=std::strtoll(plain.c_str()+n,&end,10);
            n=static_cast<std::size_t>(end-plain.c_str())-1;
        }
    }
    return result;
}
inline std::string Fixed(std::string_view text,std::string_view key,bool verified,
    std::optional<std::pair<int,int>> bounds={}) {
    const auto lines=RangeText::Lines(text);
    unsigned count=0; for (const auto line:lines) count+=RangeText::Analyze(line).key==key;
    if (count!=1) return std::string(text);
    std::string result;
    for (const auto line:lines) {
        result+=line;
        if (RangeText::Analyze(line).key==key) {
            std::string plain;
            for (std::size_t n=0;n<line.size();) {
                char color{}; const auto token=RangeText::Color(line,n,color);
                if (token) n+=token; else plain+=line[n++];
            }
            long long value=-1; unsigned numbers=0;
            for (std::size_t n=0;n<plain.size();++n) if (RangeText::Digit(plain[n])) {
                char* end{}; value=std::strtoll(plain.c_str()+n,&end,10);
                n=static_cast<std::size_t>(end-plain.c_str())-1; ++numbers;
            }
            if (numbers!=1 || value<0) return std::string(text);
            if (bounds && (bounds->first<0 || bounds->first>bounds->second)) return std::string(text);
            if (verified) result+=" \xee\x81\xbe" "U("+std::string("Base: ")+std::to_string(bounds?bounds->first:value)+" - "+std::to_string(bounds?bounds->second:value)+")\xee\x81\xbe" "3";
            else result+=" \xee\x81\xbe" "5(?)\xee\x81\xbe" "3";
        }
        result+='\n';
    }
    if (!text.ends_with('\n') && !result.empty()) result.pop_back();
    if (!verified && text.find(Footer)==text.npos)
        result="\xee\x81\xbe" "5"+std::string(Footer)+"\xee\x81\xbe" "3\n"+result;
    return result;
}
// Only the caller qualified as Core's Defense header publication supplies
// these blocks. No English label matching or guessed item arithmetic.
inline std::string Merge(std::string_view actual,std::string_view ranged) {
    const auto originals=RangeText::Lines(actual), replacements=RangeText::Lines(ranged);
    std::string result;
    for (const auto line:replacements) {
        const auto analysis=RangeText::Analyze(line);
        const std::string_view* matched=nullptr;
        unsigned count=0;
        for (const auto& original:originals)
            if (RangeText::Analyze(original).key==analysis.key) { matched=&original; ++count; }
        unsigned replacementCount=0;
        for (const auto other:replacements)
            replacementCount+=RangeText::Analyze(other).key==analysis.key;
        if (count==1 && replacementCount==1 && matched && !analysis.prefix.empty() &&
            RangeText::Analyze(*matched).prefix.empty()) {
            auto bounds=analysis.prefix;
            // Defense bounds are nonnegative native integers. Preserve complex
            // or unrecognized templates instead of constructing a false range.
            long long low{},high{}; int used{};
            if (sscanf_s(bounds.c_str(),"%lld - %lld%n",&low,&high,&used)==2 &&
                used==static_cast<int>(bounds.size()) && low>=0 && low<=high) {
                result+=*matched;
                result+=" \xee\x81\xbe" "U("+std::to_string(low)+"-"+std::to_string(high)+")\xee\x81\xbe" "3";
            } else result+=line;
        } else result+=line;
        result+='\n';
    }
    if (!ranged.ends_with('\n') && !result.empty()) result.pop_back();
    return result;
}
}
