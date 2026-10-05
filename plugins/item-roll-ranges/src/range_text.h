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
inline std::string NegatedBounds(std::string text) {
    const auto normalized=Bounds(std::move(text));
    long long low{},high{}; int used{};
    if (sscanf_s(normalized.c_str(),"%lld - %lld%n",&low,&high,&used)==2 &&
        used==static_cast<int>(normalized.size())) {
        const auto negatedLow=-high;
        const auto negatedHigh=-low;
        char out[96]{};
        std::snprintf(out,sizeof(out),"%+lld - %+lld",
            std::min(negatedLow,negatedHigh),std::max(negatedLow,negatedHigh));
        return out;
    }
    // Preserve an unfamiliar native/localized span rather than guessing at it.
    return "-("+normalized+")";
}
inline Analysis Analyze(std::string_view text) {
    Analysis a;
    char externalSign=0;
    bool previousTokenWasNumeric=false;
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
                    a.prefix+=(externalSign=='-' ? NegatedBounds(span) : Bounds(span));
                    externalSign=0;
                    a.key+='#';
                    previousTokenWasNumeric=true;
                }
                i=end; continue;
            }
            i+=token; continue;
        }
        const char c=text[i];
        if (Digit(c) || ((c=='+' || c=='-') && i+1<text.size() && Digit(text[i+1]))) {
            a.key+='#'; ++i;
            while (i<text.size() && (Digit(text[i]) || text[i]=='.' || text[i]==',')) ++i;
            previousTokenWasNumeric=true;
            continue;
        }
        // The sign preceding a native colored range is outside its color span.
        if ((c=='+' || c=='-') && i+1<text.size()) {
            char next{}; if (Color(text,i+1,next) && next=='U') {
                // A dash immediately following a numeric token is a native
                // min/max separator (for example, Adds 1-(6-8) Lightning
                // Damage). Otherwise it is a unary negative sign (for
                // example, -(11-20)% Target Defense).
                externalSign=(c=='-' && !previousTokenWasNumeric) ? '-' : 0;
                ++i; continue;
            }
        }
        if (c!=' ' && c!='\t' && c!='\r') {
            a.key+=c;
            previousTokenWasNumeric=false;
        }
        ++i;
    }
    return a;
}
inline std::string DisplayBounds(const std::string& bounds) {
    long long firstLow{},firstHigh{},secondLow{},secondHigh{}; int used{};
    if (sscanf_s(bounds.c_str(),"%lld - %lld; %lld - %lld%n",
            &firstLow,&firstHigh,&secondLow,&secondHigh,&used)==4 &&
        used==static_cast<int>(bounds.size()) && firstLow<=firstHigh &&
        secondLow<=secondHigh && firstLow<=secondHigh) {
        char result[96]{};
        std::snprintf(result,sizeof(result),"%+lld - %+lld",firstLow,secondHigh);
        return result;
    }
    return bounds;
}
inline std::vector<std::string_view> Lines(std::string_view s) {
    std::vector<std::string_view> lines;
    while (!s.empty()) { auto n=s.find('\n'); if (n==s.npos) { lines.push_back(s); break; } lines.push_back(s.substr(0,n)); s.remove_prefix(n+1); }
    return lines;
}
struct Contribution { int low{},high{}; std::string label; };
struct PairedContribution {
    int minLow{},minHigh{},maxLow{},maxHigh{};
    std::string label;
};
struct Label {
    std::string key, text;
    std::vector<Contribution> parts{};
    bool allowPartialNativeRange{};
    std::string fallbackRange{};
    std::vector<std::string> unknownSources{};
    std::vector<PairedContribution> pairedParts{};
    bool rangeExpected{};
};
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
inline std::string ExpandUnknown(std::string_view original,const Analysis& native,
                                 const std::vector<std::string>& sources) {
    if (sources.size()<2 || sources.size()>6) return {};
    std::string plain;
    for (std::size_t i=0;i<original.size();) {
        char color{}; const auto token=Color(original,i,color);
        if (token) i+=token; else plain+=original[i++];
    }
    std::string unknown;
    for (std::size_t i=0;i<plain.size();) {
        if (Digit(plain[i]) || ((plain[i]=='+' || plain[i]=='-') && i+1<plain.size() && Digit(plain[i+1]))) {
            if (plain[i]=='+' || plain[i]=='-') unknown+=plain[i++];
            unknown+='?';
            while (i<plain.size() && (Digit(plain[i]) || plain[i]=='.' || plain[i]==',')) ++i;
        } else unknown+=plain[i++];
    }
    const std::string marker=native.marker.empty()?"\xee\x81\xbe":native.marker;
    std::string result;
    for (std::size_t i=0;i<sources.size();++i) {
        if (i) result+='\n';
        const auto index=sources.size()-1-i;
        result+=marker+"5"+unknown+' '+marker+"5"+sources[index]+marker+"3";
    }
    result+='\n';
    if (!native.prefix.empty()) result+=marker+"U["+DisplayBounds(native.prefix)+"]"+marker+"3 ";
    else result+=marker+"3";
    result+=plain+" [Combined]";
    return result;
}
inline std::string ExpandPaired(std::string_view original,const Analysis& native,
                                const std::vector<PairedContribution>& parts) {
    if (parts.size()<2 || parts.size()>6) return {};
    std::string plain;
    for (std::size_t i=0;i<original.size();) {
        char color{}; const auto token=Color(original,i,color);
        if (token) i+=token; else plain+=original[i++];
    }
    struct Number { std::size_t start{},finish{}; long long value{}; bool plus{}; };
    std::vector<Number> numbers;
    for (std::size_t i=0;i<plain.size();) {
        if (!Digit(plain[i])) { ++i; continue; }
        auto start=i;
        if (i && (plain[i-1]=='+' || plain[i-1]=='-') &&
            (i<2 || !Digit(plain[i-2]))) --start;
        char* end{}; errno=0;
        const auto value=std::strtoll(plain.c_str()+start,&end,10);
        if (errno || end==plain.c_str()+start) return {};
        const auto finish=static_cast<std::size_t>(end-plain.c_str());
        numbers.push_back({start,finish,value,plain[start]=='+'});
        i=finish;
    }
    if (numbers.size()!=2) return {};
    long long sumMinLow=0,sumMinHigh=0,sumMaxLow=0,sumMaxHigh=0;
    for (const auto& p:parts) {
        if (p.minLow>p.minHigh || p.maxLow>p.maxHigh) return {};
        sumMinLow+=p.minLow; sumMinHigh+=p.minHigh;
        sumMaxLow+=p.maxLow; sumMaxHigh+=p.maxHigh;
    }
    if (numbers[0].value<sumMinLow || numbers[0].value>sumMinHigh ||
        numbers[1].value<sumMaxLow || numbers[1].value>sumMaxHigh) return {};
    std::vector<std::pair<long long,long long>> values;
    for (const auto& p:parts) {
        const auto minLow=std::max<long long>(p.minLow,numbers[0].value-(sumMinHigh-p.minHigh));
        const auto minHigh=std::min<long long>(p.minHigh,numbers[0].value-(sumMinLow-p.minLow));
        const auto maxLow=std::max<long long>(p.maxLow,numbers[1].value-(sumMaxHigh-p.maxHigh));
        const auto maxHigh=std::min<long long>(p.maxHigh,numbers[1].value-(sumMaxLow-p.maxLow));
        if (minLow!=minHigh || maxLow!=maxHigh) return {};
        values.emplace_back(minLow,maxLow);
    }
    const std::string marker=native.marker.empty()?"\xee\x81\xbe":native.marker;
    auto valueText=[](long long value,bool plus) {
        return std::string(plus && value>=0?"+":"")+std::to_string(value);
    };
    std::string result;
    for (std::size_t i=0;i<parts.size();++i) {
        if (i) result+='\n';
        const auto index=parts.size()-1-i;
        result+=marker+"5"+plain.substr(0,numbers[0].start);
        result+=valueText(values[index].first,numbers[0].plus);
        result+=plain.substr(numbers[0].finish,numbers[1].start-numbers[0].finish);
        result+=valueText(values[index].second,numbers[1].plus);
        result+=plain.substr(numbers[1].finish);
        result+=' '; result+=marker+"5"+parts[index].label+marker+"3";
    }
    result+='\n';
    if (!native.prefix.empty()) result+=marker+"U["+DisplayBounds(native.prefix)+"]"+marker+"3 ";
    else result+=marker+"3";
    result+=plain+" [Combined]";
    return result;
}
struct Result { std::string text; unsigned annotated{}, unmatched{}, labeled{}; };
inline Result Merge(std::string_view actual,std::string_view ranged,std::size_t capacity,
                    const std::vector<Label>& labels={}) {
    const auto originals=Lines(actual), ranges=Lines(ranged);
    bool unavailable=false;
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
            prefix=match->marker+"U["+DisplayBounds(match->prefix)+"]"+match->marker+"3 ";
            ++result.annotated;
        } else if (actualCount!=1 || rangeCount!=1) ++result.unmatched;
        const Label* label=nullptr; unsigned labelCount=0;
        for (const auto& entry:labels) if (entry.key==a[i].key) { label=&entry; ++labelCount; }
        if (prefix.empty() && actualCount==1 && labelCount==1 && label && !label->fallbackRange.empty()) {
            prefix="\xee\x81\xbe" "U["+label->fallbackRange+"]\xee\x81\xbe" "3 ";
            ++result.annotated;
        }
        if (actualCount==1 && rangeCount==1 && labelCount==1 && label && match) {
            auto expanded=Expand(originals[i],*match,label->parts,label->allowPartialNativeRange);
            if (expanded.empty()) expanded=ExpandPaired(originals[i],*match,label->pairedParts);
            if (expanded.empty()) expanded=ExpandUnknown(originals[i],*match,label->unknownSources);
            if (!expanded.empty()) {
                result.text+=expanded; ++result.labeled;
                if (i+1<originals.size() || actual.ends_with('\n')) result.text+='\n';
                continue;
            }
        }
        // A stacked source can change only one endpoint, so Core may emit a
        // one-number ranged line for a two-number actual damage line. Its key
        // and partial bounds cannot represent the combined total, but verified
        // sources may still be shown as unknown gray contributions.
        if (actualCount==1 && labelCount==1 && label && label->unknownSources.size()>1) {
            auto expanded=ExpandPaired(originals[i],a[i],label->pairedParts);
            if (expanded.empty()) expanded=ExpandUnknown(originals[i],a[i],label->unknownSources);
            if (!expanded.empty()) {
                result.text+=expanded; ++result.labeled;
                if (i+1<originals.size() || actual.ends_with('\n')) result.text+='\n';
                continue;
            }
        }
        result.text+=prefix;
        result.text+=originals[i];
        if (prefix.empty() && actualCount==1 && labelCount==1 && label && label->rangeExpected) {
            result.text+=" \xee\x81\xbe" "5(?)\xee\x81\xbe" "3";
            unavailable=true;
        }
        if (actualCount==1 && labelCount==1 && label && !label->text.empty()) {
            const std::string marker=match && !match->marker.empty()?match->marker:"\xee\x81\xbe";
            result.text+=' '; result.text+=marker+"U"+label->text+marker+"3";
            ++result.labeled;
        }
        if (i+1<originals.size() || actual.ends_with('\n')) result.text+='\n';
    }
    if (unavailable) {
        // Tooltip property buffers are laid out bottom-to-top: the first
        // emitted line is the footer. Emit it once for the entire block.
        result.text="\xee\x81\xbe" "5(?) Range Unavailable - Please report item affixes\xee\x81\xbe" "3\n"+result.text;
    }
    if (result.text.size()>=capacity) { result.text=actual; result.annotated=0; result.labeled=0; }
    return result;
}
}
