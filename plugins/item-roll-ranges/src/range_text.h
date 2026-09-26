#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <cerrno>
namespace RangeText {
struct Analysis { std::string key, prefix, marker; };
inline std::size_t Color(std::string_view s,std::size_t i,char& color) {
    // The runtime localization service converts the source ÿc introducer
    // to U+E07E (UTF-8 EE 81 BE), followed directly by the color selector.
    if (i+3<s.size() && static_cast<unsigned char>(s[i])==0xee &&
        static_cast<unsigned char>(s[i+1])==0x81 && static_cast<unsigned char>(s[i+2])==0xbe) { color=s[i+3]; return 4; }
    if (i+2<s.size() && static_cast<unsigned char>(s[i])==255 && s[i+1]=='c') { color=s[i+2]; return 3; }
    if (i+3<s.size() && static_cast<unsigned char>(s[i])==195 && static_cast<unsigned char>(s[i+1])==191 && s[i+2]=='c') { color=s[i+3]; return 4; }
    return 0;
}
inline bool Digit(char c) { return c>='0' && c<='9'; }
inline std::string Bounds(std::string text) {
    // Native range spans are authoritative. Only normalize a complete simple
    // signed integer pair; leave complex/localized spans intact.
    if (text.size()>1 && text.front()=='(' && text.back()==')') text=text.substr(1,text.size()-2);
    if (text.size()>=6 && text.starts_with("\xef\xbc\x88") && text.ends_with("\xef\xbc\x89")) text=text.substr(3,text.size()-6);
    char* end{}; const char* begin=text.c_str();
    const long long lo=std::strtoll(begin,&end,10);
    if (end!=begin && *end=='-') {
        const char* second=end+1; const long long hi=std::strtoll(second,&end,10);
        if (end!=second && !*end) {
            char out[96]{}; std::snprintf(out,sizeof(out),"%+lld - %+lld",lo,hi); return out;
        }
    }
    return text;
}
inline Analysis Analyze(std::string_view text) {
    Analysis a;
    char externalSign=0;
    for (std::size_t i=0;i<text.size();) {
        char color{}; const auto token=Color(text,i,color);
        if (token) {
            if (color=='U') {
                if (a.marker.empty()) a.marker=std::string(text.substr(i,token-1));
                const auto start=i+token; auto end=start;
                for (;end<text.size();++end) { char next{}; if (Color(text,end,next)) break; }
                auto span=std::string(text.substr(start,end-start));
                if (!span.empty()) {
                    if (!a.prefix.empty()) a.prefix+="; ";
                    a.prefix+=(externalSign=='-' ? "-("+Bounds(span)+")" : Bounds(span));
                    externalSign=0;
                    a.key+='#';
                }
                i=end; continue;
            }
            i+=token; continue;
        }
        const char c=text[i];
        if (Digit(c) || ((c=='+' || c=='-') && i+1<text.size() && Digit(text[i+1]))) {
            a.key+='#'; ++i;
            while (i<text.size() && (Digit(text[i]) || text[i]=='.' || text[i]==',')) ++i;
            continue;
        }
        // The sign preceding a native colored range is outside its color span.
        if ((c=='+' || c=='-') && i+1<text.size()) {
            char next{}; if (Color(text,i+1,next) && next=='U') { externalSign=c; ++i; continue; }
        }
        if (c!=' ' && c!='\t' && c!='\r') a.key+=c;
        ++i;
    }
    return a;
}
inline std::vector<std::string_view> Lines(std::string_view s) {
    std::vector<std::string_view> lines;
    while (!s.empty()) { auto n=s.find('\n'); if (n==s.npos) { lines.push_back(s); break; } lines.push_back(s.substr(0,n)); s.remove_prefix(n+1); }
    return lines;
}
struct Contribution { int low{},high{}; std::string label; };
struct Label { std::string key, text; std::vector<Contribution> parts{}; bool allowPartialNativeRange{}; std::string fallbackRange{}; };
// Compare two native scalar renders; preserve localized wording and signs.
inline std::string EndpointRange(std::string_view actual,std::string_view low,std::string_view high) {
    if (Analyze(actual).key!=Analyze(low).key || Analyze(actual).key!=Analyze(high).key) return {};
    auto number=[](std::string_view input,long long& value,bool& percent) {
        std::string plain;
        for (std::size_t i=0;i<input.size();) {
            char color{}; const auto token=Color(input,i,color);
            if (token) i+=token; else plain+=input[i++];
        }
        unsigned count=0;
        for (std::size_t i=0;i<plain.size();++i) {
            if (!Digit(plain[i])) continue;
            if (++count!=1) return false;
            std::size_t start=i;
            if (i && (plain[i-1]=='-' || plain[i-1]=='+')) --start;
            char* end{}; errno=0; value=std::strtoll(plain.c_str()+start,&end,10);
            if (errno) return false;
            const auto finish=static_cast<std::size_t>(end-plain.c_str());
            if (finish<plain.size() && (plain[finish]=='.' || plain[finish]==',')) return false;
            percent=finish<plain.size() && plain[finish]=='%'; i=finish-1;
        }
        return count==1;
    };
    long long value{},lo{},hi{}; bool vp{},lp{},hp{};
    if (!number(actual,value,vp) || !number(low,lo,lp) || !number(high,hi,hp) || vp!=lp || lp!=hp || lo==hi) return {};
    if (lo>hi) std::swap(lo,hi);
    if (value<lo || value>hi) return {};
    const std::string unit=lp?"%":"";
    return std::to_string(lo)+unit+" - "+std::to_string(hi)+unit;
}
// A numeric split is valid only if the complete native interval agrees with
// the sum of source intervals. Never choose an arbitrary solution to a sum.
inline std::string Expand(std::string_view original,const Analysis& native,
                          const std::vector<Contribution>& parts,bool allowPartialNativeRange=false) {
    if (parts.empty() || parts.size()>6 || native.prefix.empty()) return {};
    char* end{}; errno=0;
    const char* start=native.prefix.c_str();
    const auto nativeLow=std::strtoll(start,&end,10);
    if (end==start || errno) return {};
    while (*end==' ') ++end;
    if (*end++!='-') return {};
    const char* second=end;
    const auto nativeHigh=std::strtoll(second,&end,10);
    if (end==second || *end || errno) return {};
    long long sumLow=0,sumHigh=0;
    for (const auto& p:parts) {
        if (p.low>p.high) return {};
        sumLow+=p.low; sumHigh+=p.high;
    }
    const bool completeNativeRange=sumLow==nativeLow && sumHigh==nativeHigh;
    if (!completeNativeRange) {
        // Core's combined ED provider can retain only one of two dmg% specs.
        // Show verified per-affix bounds, but do not infer exact rolls from a
        // native interval that does not account for all of those sources.
        bool matchesOne=false;
        for (const auto& p:parts) matchesOne |= nativeLow==p.low && nativeHigh==p.high;
        if (!allowPartialNativeRange || parts.size()<2 || !matchesOne) return {};
    }
    std::string plain;
    for (std::size_t i=0;i<original.size();) {
        char color{}; const auto token=Color(original,i,color);
        if (token) i+=token; else plain+=original[i++];
    }
    std::size_t number=plain.npos,finish=0;
    long long total=0;
    for (std::size_t i=0;i<plain.size();++i) {
        if (!Digit(plain[i])) continue;
        if (number!=plain.npos) return {}; // Paired damage, proc/skill levels etc.
        number=i;
        if (i && (plain[i-1]=='+' || plain[i-1]=='-')) --number;
        errno=0; total=std::strtoll(plain.c_str()+number,&end,10);
        if (errno) return {};
        finish=static_cast<std::size_t>(end-plain.c_str());
        if (finish<plain.size() && (plain[finish]=='.' || plain[finish]==',')) return {};
        i=finish-1;
    }
    if (number==plain.npos || total<sumLow || total>sumHigh) return {};
    std::vector<long long> values;
    bool exact=completeNativeRange;
    for (const auto& p:parts) {
        const auto lo=std::max<long long>(p.low,total-(sumHigh-p.high));
        const auto hi=std::min<long long>(p.high,total-(sumLow-p.low));
        exact &= lo==hi; values.push_back(lo);
    }
    const std::string marker=native.marker.empty()?"\xee\x81\xbe":native.marker;
    const bool percent=finish<plain.size() && plain[finish]=='%';
    const bool plus=plain[number]=='+';
    std::string result;
    for (std::size_t i=0;i<parts.size();++i) {
        if (i) result+='\n';
        // D2R lays property lines out bottom-to-top. Emit child rows in reverse
        // order and put the total last so it appears above them on screen.
        const auto index=parts.size()-1-i;
        const auto& p=parts[index];
        const std::string tint=parts.size()>1?"5":"U";
        result+=marker+tint+"["+std::to_string(p.low)+(percent?"%":"")+"-"+
            std::to_string(p.high)+(percent?"%":"")+"]"+(parts.size()>1?" ":marker+"3 ");
        result+=plain.substr(0,number);
        if (exact) {
            if (plus && values[index]>=0) result+='+';
            result+=std::to_string(values[index]);
        } else result+='?';
        result+=plain.substr(finish);
        result+=' '; result+=marker+tint+p.label;
        result+=marker+"3";
    }
    if (!exact) result+='\n'+marker+"3"+std::string(original)+" [Combined]";
    return result;
}
struct Result { std::string text; unsigned annotated{}, unmatched{}, labeled{}; };
inline Result Merge(std::string_view actual,std::string_view ranged,std::size_t capacity,
                    const std::vector<Label>& labels={}) {
    const auto originals=Lines(actual), ranges=Lines(ranged);
    std::vector<Analysis> a,r;
    for (auto line: originals) a.push_back(Analyze(line));
    for (auto line: ranges) r.push_back(Analyze(line));
    Result result;
    for (std::size_t i=0;i<originals.size();++i) {
        std::string prefix;
        const Analysis* match=nullptr;
        unsigned actualCount=0,rangeCount=0;
        for (const auto& entry:a) if (entry.key==a[i].key) ++actualCount;
        for (const auto& entry:r) if (entry.key==a[i].key) { ++rangeCount; match=&entry; }
        // Never attach one line's bounds to an ambiguous repeated label.
        if (actualCount==1 && rangeCount==1 && match && !match->prefix.empty()) {
            prefix=match->marker+"U["+match->prefix+"]"+match->marker+"3 ";
            ++result.annotated;
        } else if (actualCount!=1 || rangeCount!=1) ++result.unmatched;
        const Label* label=nullptr; unsigned labelCount=0;
        for (const auto& entry:labels) if (entry.key==a[i].key) { label=&entry; ++labelCount; }
        if (prefix.empty() && actualCount==1 && labelCount==1 && label && !label->fallbackRange.empty()) {
            prefix="\xee\x81\xbe" "U["+label->fallbackRange+"]\xee\x81\xbe" "3 ";
            ++result.annotated;
        }
        if (actualCount==1 && rangeCount==1 && labelCount==1 && label && match) {
            const auto expanded=Expand(originals[i],*match,label->parts,label->allowPartialNativeRange);
            if (!expanded.empty()) {
                result.text+=expanded; ++result.labeled;
                if (i+1<originals.size() || actual.ends_with('\n')) result.text+='\n';
                continue;
            }
        }
        result.text+=prefix;
        result.text+=originals[i];
        if (actualCount==1 && labelCount==1 && label && !label->text.empty()) {
            const std::string marker=match && !match->marker.empty()?match->marker:"\xee\x81\xbe";
            result.text+=' '; result.text+=marker+"U"+label->text+marker+"3";
            ++result.labeled;
        }
        if (i+1<originals.size() || actual.ends_with('\n')) result.text+='\n';
    }
    if (result.text.size()>=capacity) { result.text=actual; result.annotated=0; result.labeled=0; }
    return result;
}
}
