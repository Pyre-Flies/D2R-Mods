#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <cerrno>
#include <optional>
#include <climits>
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
inline std::string_view StripCompanionMarker(std::string_view text) {
    // d2rl-corruption-marker prepends a red '*' ("\xee\x81\xbe" "1*\xee\x81\xbe" "3 ")
    // inside the native property formatter before Item Roll Ranges merges lines.
    char c1{}, c2{};
    const auto t1 = Color(text, 0, c1);
    if (t1 && c1 == '1' && t1 < text.size() && text[t1] == '*') {
        const auto t2 = Color(text, t1 + 1, c2);
        if (t2 && c2 == '3') {
            auto pos = t1 + 1 + t2;
            if (pos < text.size() && text[pos] == ' ') ++pos;
            return text.substr(pos);
        }
    }
    return text;
}
inline Analysis Analyze(std::string_view text) {
    text = StripCompanionMarker(text);
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
        const auto endpoint=[](long long low,long long high){
            char result[96]{};
            if(low==high)std::snprintf(result,sizeof(result),"%+lld",high);
            else std::snprintf(result,sizeof(result),"%+lld - %+lld",low,high);
            return std::string(result);
        };
        return endpoint(firstLow,firstHigh)+" / "+endpoint(secondLow,secondHigh);
    }
    used=0;
    if(sscanf_s(bounds.c_str(),"%lld - %lld%n",&firstLow,&firstHigh,&used)==2 &&
       used==static_cast<int>(bounds.size()) && firstLow==firstHigh){
        char result[96]{};std::snprintf(result,sizeof(result),"%+lld",firstHigh);return result;
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
struct OwnedContribution {
    int low{},high{};std::optional<int> value;std::string label;
    int maxLow{},maxHigh{};std::optional<int> maxValue;
};
struct Label {
    std::string key, text;
    std::vector<Contribution> parts{};
    bool allowPartialNativeRange{};
    std::string fallbackRange{};
    std::vector<std::string> unknownSources{};
    std::vector<PairedContribution> pairedParts{};
    bool rangeExpected{};
    bool fixedSocketParts{};
    std::vector<OwnedContribution> ownedParts{};
    std::optional<int> ownedTotal{},ownedMaxTotal{};
    bool replaceNativeRange{};
    bool quantityRate{};
    bool sourceOnly{};
    std::optional<int> ownedDurationSeconds{};
};
inline std::string QuantitySeconds(int rate) {
    if(rate<=0)return "?";
    // Qualified recurring event cadence: floor(2500 / rate) + 1 frames,
    // clamped to at least 125 frames at 25 simulation frames per second.
    const int frames=std::max(125,2500/rate+1);
    if(frames%25==0)return std::to_string(frames/25);
    char text[48]{};std::snprintf(text,sizeof(text),"%.2f",frames/25.0);
    std::string result=text;while(result.ends_with('0'))result.pop_back();if(result.ends_with('.'))result.pop_back();
    return result;
}
inline std::string ExpandQuantity(std::string_view original,const Label& label) {
    if(!label.quantityRate || !label.ownedTotal || *label.ownedTotal<=0 || label.ownedParts.empty() || label.ownedParts.size()>12)return {};
    long long low=0,high=0,sum=0;bool exact=true;
    for(const auto& p:label.ownedParts){
        if(p.low<=0 || p.high<p.low || (p.value&&(*p.value<p.low || *p.value>p.high)))return {};
        low+=p.low;high+=p.high;if(p.value)sum+=*p.value;else exact=false;
    }
    if(low>INT_MAX || high>INT_MAX || *label.ownedTotal<low || *label.ownedTotal>high || (exact&&sum!=*label.ownedTotal))return {};
    const auto interval=[](int lo,int hi){return "1 per "+QuantitySeconds(hi)+(lo==hi?"":" - "+QuantitySeconds(lo))+"s";};
    const auto rate=[](std::optional<int> value){return value?"1 per "+QuantitySeconds(*value)+"s":std::string("?");};
    const std::string marker="\xee\x81\xbe";
    std::string result;
    if(label.ownedParts.size()>1)for(auto at=label.ownedParts.rbegin();at!=label.ownedParts.rend();++at)
        result+=marker+"5["+interval(at->low,at->high)+"] "+std::string(original)+" ("+rate(at->value)+") "+at->label+marker+"3\n";
    result+=marker+"U["+interval(static_cast<int>(low),static_cast<int>(high))+"]"+marker+"3 "+std::string(original);
    if(low!=high || label.ownedParts.size()>1)result+=" ("+rate(label.ownedTotal)+")";
    result+=" "+marker+"U"+(label.ownedParts.size()==1?label.ownedParts[0].label:"[Combined]")+marker+"3";
    return result;
}
// An owned source has independently captured actuals and agreeing definitions.
// Native partial intervals are replaced only when every source is accounted for.
inline std::string ExpandOwned(std::string_view original,const Label& label) {
    if(label.quantityRate)return ExpandQuantity(original,label);
    if(label.ownedParts.empty()||label.ownedParts.size()>12||!label.ownedTotal)return {};
    const bool paired=label.ownedMaxTotal.has_value();
    const auto cleanOriginal=StripCompanionMarker(original);
    std::string plain;
    for(std::size_t i=0;i<cleanOriginal.size();){char color{};const auto n=Color(cleanOriginal,i,color);if(n)i+=n;else plain+=cleanOriginal[i++];}
    struct Number {std::size_t start,finish;long long value;bool plus,percent;};
    std::vector<Number> numbers;
    for(std::size_t i=0;i<plain.size();){
        if(!Digit(plain[i])){++i;continue;}
        auto at=i;if(i && (plain[i-1]=='+'||plain[i-1]=='-') && (i<2||!Digit(plain[i-2])))--at;
        char* end{};errno=0;const auto value=std::strtoll(plain.c_str()+at,&end,10);
        const auto finish=static_cast<std::size_t>(end-plain.c_str());
        if(errno||finish<=at||(finish<plain.size()&&(plain[finish]=='.'||plain[finish]==',')))return {};
        numbers.push_back({at,finish,value,plain[at]=='+',finish<plain.size()&&plain[finish]=='%'});i=finish;
    }
    const bool duration=label.ownedDurationSeconds.has_value();
    const bool collapsed=duration && paired && label.ownedTotal==label.ownedMaxTotal && numbers.size()==2;
    const auto damageNumbers=paired&&!collapsed?2u:1u;
    const bool inverted=!paired && !duration && numbers.size()==1 && *label.ownedTotal>0 &&
        numbers[0].value==-*label.ownedTotal && plain[numbers[0].start]=='-';
    if(numbers.size()!=damageNumbers+(duration?1u:0u)||
       numbers[0].value!=(inverted?-*label.ownedTotal:*label.ownedTotal)||
       (paired && !collapsed && (numbers[1].value!=*label.ownedMaxTotal||numbers[0].percent||numbers[1].percent)) ||
       (duration && (label.ownedParts.size()!=1 || numbers.back().value!=*label.ownedDurationSeconds || numbers.back().percent)))return {};
    long long lo=0,hi=0,maxLo=0,maxHi=0,valueSum=0,maxValueSum=0;bool allValues=true,allMaxValues=true;
    for(const auto& p:label.ownedParts){
        if(p.low>p.high||(p.value&&(*p.value<p.low||*p.value>p.high))||
           (paired && (p.maxLow>p.maxHigh||(p.maxValue&&(*p.maxValue<p.maxLow||*p.maxValue>p.maxHigh)))))return {};
        lo+=p.low;hi+=p.high;maxLo+=p.maxLow;maxHi+=p.maxHigh;
        if(p.value)valueSum+=*p.value;else allValues=false;
        if(p.maxValue)maxValueSum+=*p.maxValue;else allMaxValues=false;
    }
    if(*label.ownedTotal<lo||*label.ownedTotal>hi||
       (paired&&(*label.ownedMaxTotal<maxLo||*label.ownedMaxTotal>maxHi))||
       (allValues&&valueSum!=*label.ownedTotal)||(paired&&allMaxValues&&maxValueSum!=*label.ownedMaxTotal))return {};
    auto valueText=[](std::optional<int> value,bool plus,bool inv){
        if(!value)return std::string("?");
        const long long v=inv?-static_cast<long long>(*value):static_cast<long long>(*value);
        return std::string(plus&&v>=0?"+":"")+std::to_string(v);
    };
    auto bounds=[](long long a,long long b,bool plus,bool percent,bool inv){
        const long long da=inv?-a:a,db=inv?-b:b;
        const auto text=[&](long long v){return std::string(plus&&v>=0?"+":"")+std::to_string(v)+(percent?"%":"");};
        return da==db?text(db):text(da)+" - "+text(db);
    };
    const std::string marker="\xee\x81\xbe";
    std::string combinedBounds=bounds(lo,hi,numbers[0].plus,numbers[0].percent,inverted);
    if(paired)combinedBounds+=" / "+bounds(maxLo,maxHi,numbers[1].plus,false,false);
    // A socket-only property has a single complete source and needs no duplicate
    // total/child row. Fixed modifier bounds use a single signed value.
    if(label.ownedParts.size()==1)return marker+"U["+combinedBounds+"]"+marker+"3 "+std::string(original)+" "+marker+"U"+label.ownedParts[0].label+marker+"3";
    std::string result;
    for(auto it=label.ownedParts.rbegin();it!=label.ownedParts.rend();++it){
        std::string sourceBounds=bounds(it->low,it->high,numbers[0].plus,numbers[0].percent,inverted);
        if(paired)sourceBounds+=" / "+bounds(it->maxLow,it->maxHigh,numbers[1].plus,false,false);
        result+=marker+"5["+sourceBounds+"] "+plain.substr(0,numbers[0].start)+valueText(it->value,numbers[0].plus,inverted);
        if(paired){result+=plain.substr(numbers[0].finish,numbers[1].start-numbers[0].finish);result+=valueText(it->maxValue,numbers[1].plus,false);result+=plain.substr(numbers[1].finish);}
        else result+=plain.substr(numbers[0].finish);
        result+=" "+it->label+marker+"3\n";
    }
    result+=marker+"U["+combinedBounds+"]"+marker+"3 "+std::string(original)+" [Combined]";
    return result;
}
// Native range clones usually omit fixed affixes. Show their proven bound
// only when the actual scalar equals the complete sum of fixed definitions.
inline std::string ExpandFixedAffixes(std::string_view original,const std::vector<Contribution>& parts) {
    if(parts.empty()||parts.size()>6)return {};
    Label label;long long total=0;
    for(const auto& part:parts){
        if(part.low!=part.high)return {};
        total+=part.high;label.ownedParts.push_back({part.low,part.high,part.high,part.label});
    }
    if(total<INT_MIN||total>INT_MAX)return {};
    label.ownedTotal=static_cast<int>(total);return ExpandOwned(original,label);
}
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
inline std::string FixedProcChance(std::string_view original,int chance,int level) {
    std::string plain;
    for(std::size_t n=0;n<original.size();){char color{};const auto token=Color(original,n,color);
        if(token)n+=token;else plain+=original[n++];}
    std::vector<std::pair<long long,bool>> values;
    for(std::size_t n=0;n<plain.size();++n)if(Digit(plain[n])){
        char* end{};errno=0;const auto value=std::strtoll(plain.c_str()+n,&end,10);if(errno)return {};
        const auto finish=static_cast<std::size_t>(end-plain.c_str());
        if(n && (plain[n-1]=='-' || plain[n-1]=='+'))return {};
        if(finish<plain.size() && (plain[finish]=='.' || plain[finish]==','))return {};
        values.push_back({value,finish<plain.size() && plain[finish]=='%'});n=finish-1;
    }
    if(values.size()!=2 || values[0]!=std::pair<long long,bool>{chance,true} ||
       values[1]!=std::pair<long long,bool>{level,false})return {};
    return std::to_string(chance)+"%";
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
    const auto cleanOriginal=StripCompanionMarker(original);
    std::string plain;
    for (std::size_t i=0;i<cleanOriginal.size();) {
        char color{}; const auto token=Color(cleanOriginal,i,color);
        if (token) i+=token; else plain+=cleanOriginal[i++];
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
        result+=marker+tint+"["+std::to_string(p.low)+(percent?"%":"");
        if(p.low!=p.high)result+="-"+std::to_string(p.high)+(percent?"%":"");
        result+="]"+(parts.size()>1?" ":marker+"3 ");
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
// The first socket control is IAS: complete fixed host definitions plus owned
// socket values. Keep the actual total above gray source rows, even with no
// native range (fixed modifiers normally have none).
inline std::string ExpandFixedSockets(std::string_view original,const std::vector<Contribution>& parts) {
    if (parts.size()<2 || parts.size()>6) return {};
    long long sum=0;
    for (const auto& part:parts) {if (part.low!=part.high || part.low<0) return {};sum+=part.low;}
    const auto cleanOriginal=StripCompanionMarker(original);
    std::string plain;
    for (std::size_t i=0;i<cleanOriginal.size();) {
        char color{};const auto token=Color(cleanOriginal,i,color);
        if (token) i+=token;else plain+=cleanOriginal[i++];
    }
    std::size_t start=plain.npos,finish=0;long long total=0;
    for (std::size_t i=0;i<plain.size();++i) {
        if (!Digit(plain[i])) continue;
        if (start!=plain.npos) return {};
        start=i;if (i && (plain[i-1]=='+' || plain[i-1]=='-')) --start;
        char* end{};errno=0;total=std::strtoll(plain.c_str()+start,&end,10);
        if (errno) return {};
        finish=static_cast<std::size_t>(end-plain.c_str());
        if (finish>=plain.size() || plain[finish]!='%') return {};
        i=finish-1;
    }
    if (start==plain.npos || total!=sum) return {};
    const std::string marker="\xee\x81\xbe";
    std::string result;
    for (auto p=parts.rbegin();p!=parts.rend();++p) {
        result+=marker+"5"+plain.substr(0,start);
        if (plain[start]=='+') result+='+';
        result+=std::to_string(p->low)+plain.substr(finish)+" "+p->label+marker+"3\n";
    }
    result+=original;
    return result;
}
inline std::string ExpandUnknown(std::string_view original,const Analysis& native,
                                 const std::vector<std::string>& sources) {
    if (sources.size()<2 || sources.size()>6) return {};
    const auto cleanOriginal=StripCompanionMarker(original);
    std::string plain;
    for (std::size_t i=0;i<cleanOriginal.size();) {
        char color{}; const auto token=Color(cleanOriginal,i,color);
        if (token) i+=token; else plain+=cleanOriginal[i++];
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
    const auto cleanOriginal=StripCompanionMarker(original);
    std::string plain;
    for (std::size_t i=0;i<cleanOriginal.size();) {
        char color{}; const auto token=Color(cleanOriginal,i,color);
        if (token) i+=token; else plain+=cleanOriginal[i++];
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
                    const std::vector<Label>& labels={},bool compact=false) {
    const auto originals=Lines(actual), ranges=Lines(ranged);
    const auto expansion=[&](const std::string& text){
        const auto at=compact?text.rfind('\n'):std::string::npos;
        return at==std::string::npos?text:text.substr(at+1);
    };
    bool unavailable=false;
    std::vector<Analysis> a,r;
    for (auto line: originals) a.push_back(Analyze(line));
    for (auto line: ranges) r.push_back(Analyze(line));
    struct LineFallback {
        std::string compactLine;
        std::string rangePrefix;
        unsigned annotated{};
        unsigned labeled{};
    };
    std::vector<LineFallback> fallbacks(originals.size());
    Result result;
    for (std::size_t i=0;i<originals.size();++i) {
        std::string prefix;
        unsigned lineAnnotated=0;
        const Analysis* match=nullptr;
        unsigned actualCount=0,rangeCount=0;
        for (const auto& entry:a) if (entry.key==a[i].key) ++actualCount;
        for (const auto& entry:r) if (entry.key==a[i].key) { ++rangeCount; match=&entry; }
        // Never attach one line's bounds to an ambiguous repeated label.
        if (actualCount==1 && rangeCount==1 && match && !match->prefix.empty()) {
            prefix=match->marker+"U["+DisplayBounds(match->prefix)+"]"+match->marker+"3 ";
            ++result.annotated;
            lineAnnotated=1;
        } else if (actualCount!=1 || rangeCount!=1) ++result.unmatched;
        const Label* label=nullptr; unsigned labelCount=0;
        for (const auto& entry:labels) if (entry.key==a[i].key) { label=&entry; ++labelCount; }
        auto recordFallback=[&](std::string compactCandidate,const std::string& fallbackPrefix,unsigned ann,unsigned lab){
            std::string extractedPrefix=fallbackPrefix;
            if(extractedPrefix.empty()){
                const auto pos=compactCandidate.find(originals[i]);
                if(pos!=std::string::npos && pos>0 && compactCandidate.substr(0,pos).find('[')!=std::string::npos){
                    extractedPrefix=compactCandidate.substr(0,pos);
                    if(!ann)ann=1;
                }
            }
            fallbacks[i]={std::move(compactCandidate),std::move(extractedPrefix),ann,lab};
        };
        if(actualCount==1&&labelCount==1&&label&&label->sourceOnly){
            if(!prefix.empty()){--result.annotated;lineAnnotated=0;}
            std::string lineText(originals[i]);
            unsigned lineLabeled=0;
            if(!label->text.empty()){
                lineText+=" \xee\x81\xbe" "U"+label->text+"\xee\x81\xbe" "3";++result.labeled;lineLabeled=1;
            }
            result.text+=lineText;
            recordFallback( std::move(lineText),{},0,lineLabeled);
            if(i+1<originals.size()||actual.ends_with('\n'))result.text+='\n';
            continue;
        }
        if(actualCount==1&&labelCount==1&&label&&!label->ownedParts.empty()){
            const auto expanded=ExpandOwned(originals[i],*label);
            if(!expanded.empty()){
                auto compactCandidate=expansion(expanded);
                result.text+=compact?compactCandidate:expanded;++result.labeled;
                recordFallback(std::move(compactCandidate),prefix,lineAnnotated,1);
                if(i+1<originals.size()||actual.ends_with('\n'))result.text+='\n';
                continue;
            }
        }
        if (actualCount==1 && labelCount==1 && label && label->fixedSocketParts) {
            const auto expanded=ExpandFixedSockets(originals[i],label->parts);
            if (!expanded.empty()) {
                auto compactCandidate=expansion(expanded);
                result.text+=compact?compactCandidate:expanded;++result.labeled;
                recordFallback(std::move(compactCandidate),prefix,lineAnnotated,1);
                if (i+1<originals.size() || actual.ends_with('\n')) result.text+='\n';
                continue;
            }
        }
        if(actualCount==1&&labelCount==1&&label&&prefix.empty()){
            const auto expanded=ExpandFixedAffixes(originals[i],label->parts);
            if(!expanded.empty()){
                auto compactCandidate=expansion(expanded);
                result.text+=compact?compactCandidate:expanded;++result.labeled;
                recordFallback(std::move(compactCandidate),prefix,lineAnnotated,1);
                if(i+1<originals.size()||actual.ends_with('\n'))result.text+='\n';
                continue;
            }
        }
        if (actualCount==1 && labelCount==1 && label && !label->fallbackRange.empty() &&
            (prefix.empty() || label->replaceNativeRange)) {
            if(prefix.empty()){++result.annotated;lineAnnotated=1;}
            prefix="\xee\x81\xbe" "U["+label->fallbackRange+"]\xee\x81\xbe" "3 ";
        }
        if (actualCount==1 && rangeCount==1 && labelCount==1 && label && match) {
            auto expanded=Expand(originals[i],*match,label->parts,label->allowPartialNativeRange);
            if (expanded.empty()) expanded=ExpandPaired(originals[i],*match,label->pairedParts);
            if (expanded.empty()) expanded=ExpandUnknown(originals[i],*match,label->unknownSources);
            if (!expanded.empty()) {
                auto compactCandidate=expansion(expanded);
                result.text+=compact?compactCandidate:expanded; ++result.labeled;
                recordFallback(std::move(compactCandidate),prefix,lineAnnotated,1);
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
                auto compactCandidate=expansion(expanded);
                result.text+=compact?compactCandidate:expanded; ++result.labeled;
                recordFallback(std::move(compactCandidate),prefix,lineAnnotated,1);
                if (i+1<originals.size() || actual.ends_with('\n')) result.text+='\n';
                continue;
            }
        }
        std::string lineText=prefix;
        lineText+=originals[i];
        if (prefix.empty() && actualCount==1 && labelCount==1 && label && label->rangeExpected) {
            lineText+=" \xee\x81\xbe" "5(?)\xee\x81\xbe" "3";
            unavailable=true;
        }
        unsigned lineLabeled=0;
        if (actualCount==1 && labelCount==1 && label && !label->text.empty()) {
            const std::string marker=match && !match->marker.empty()?match->marker:"\xee\x81\xbe";
            lineText+=' '; lineText+=marker+"U"+label->text+marker+"3";
            ++result.labeled;
            lineLabeled=1;
        }
        result.text+=lineText;
        recordFallback(std::move(lineText),prefix,lineAnnotated,lineLabeled);
        if (i+1<originals.size() || actual.ends_with('\n')) result.text+='\n';
    }
    if (unavailable) {
        // Tooltip property buffers are laid out bottom-to-top: the first
        // emitted line is the footer. Emit it once for the entire block.
        result.text="\xee\x81\xbe" "5(?) Range Unavailable - Please report item affixes\xee\x81\xbe" "3\n"+result.text;
    }
    if (result.text.size()>=capacity) {
        // Native property buffers are fixed-size. Prefer totals over duplicated
        // child breakdowns, then retain as many compact range+label lines,
        // source labels, or numeric range prefixes as fit within capacity.
        if(!compact)return Merge(actual,ranged,capacity,labels,true);
        Result bounded;
        if(actual.size()>=capacity){bounded.text=actual;return bounded;}
        auto available=capacity-actual.size()-1;
        for(std::size_t i=0;i<originals.size();++i){
            const auto& fb=fallbacks[i];
            if(!fb.compactLine.empty() && fb.compactLine!=originals[i] &&
               fb.compactLine.size()>=originals[i].size() &&
               (fb.compactLine.size()-originals[i].size())<=available){
                bounded.text+=fb.compactLine;
                available-=fb.compactLine.size()-originals[i].size();
                bounded.annotated+=fb.annotated;
                bounded.labeled+=fb.labeled;
                if(i+1<originals.size() || actual.ends_with('\n'))bounded.text+='\n';
                continue;
            }
            const auto count=std::count_if(a.begin(),a.end(),[&](const auto& entry){return entry.key==a[i].key;});
            const Label* found=nullptr;unsigned matches=0;
            for(const auto& label:labels)if(label.key==a[i].key){found=&label;++matches;}
            bool fittedSuffix=false;
            if(count==1 && matches==1 && found){
                std::string text=found->text;
                if(!found->ownedParts.empty())text=found->ownedParts.size()==1?found->ownedParts.front().label:"[Combined]";
                if(!text.empty()){
                    const auto suffix=" \xee\x81\xbe" "U"+text+"\xee\x81\xbe" "3";
                    if(suffix.size()<=available){
                        bounded.text+=originals[i];
                        bounded.text+=suffix;
                        available-=suffix.size();
                        ++bounded.labeled;
                        fittedSuffix=true;
                    }
                }
            }
            if(!fittedSuffix){
                if(!fb.rangePrefix.empty() && fb.rangePrefix.size()<=available){
                    bounded.text+=fb.rangePrefix;
                    bounded.text+=originals[i];
                    available-=fb.rangePrefix.size();
                    ++bounded.annotated;
                }else{
                    bounded.text+=originals[i];
                }
            }
            if(i+1<originals.size() || actual.ends_with('\n'))bounded.text+='\n';
        }
        return bounded;
    }
    return result;
}
}
