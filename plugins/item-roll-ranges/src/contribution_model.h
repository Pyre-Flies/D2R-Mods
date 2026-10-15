#pragma once
#include "contribution_sources.h"
#include "range_text.h"
#include "affixes.h"
namespace ContributionModel {
struct Group {Contributions::Stats values;std::vector<Affixes::Rolled> definitions;std::string prefix;};
inline bool Supported(unsigned stat) {
    switch(stat){case 0:case 1:case 2:case 3:case 7:case 9:case 16:case 17:case 18:case 19:
    case 21:case 22:case 23:case 24:case 31:case 34:case 35:case 36:case 37:case 39:case 41:case 43:case 45:
    case 48:case 49:case 50:case 51:case 52:case 53:case 54:case 55:case 60:case 62:case 74:
    case 78:case 79:case 80:case 85:case 86:case 89:case 93:case 96:case 99:case 102:case 149:return true;
    default:return false;}
}
inline bool Bounds(const Affixes::Rolled& definition,std::span<const Affixes::Property> properties,int stat,int& low,int& high) {
    if(!definition.unconditionalScalar || (definition.source&&(std::string_view(definition.source)=="[U]"||std::string_view(definition.source)=="[RW]")&&!definition.uniqueScalar))return false;
    auto plain=definition;plain.source=nullptr;
    return Affixes::ScalarBounds(plain,properties,stat,0,low,high);
}
inline std::optional<RangeText::Label> Build(std::string key,unsigned stat,unsigned shift,
    std::span<const Group> groups,std::span<const Affixes::Property> properties,bool paired=false) {
    if(!Supported(stat)||groups.empty()||groups.size()>8)return {};
    if(paired && (stat<48||stat>54||(stat&1)))return {};
    RangeText::Label label;label.key=std::move(key);long long total=0,maxTotal=0;
    for(const auto& group:groups){
        const auto actual=Contributions::Normalize(Contributions::Value(group.values,stat),shift);
        const auto maxActual=paired?Contributions::Normalize(Contributions::Value(group.values,stat+1),shift):std::optional<int>{0};
        if(!actual||!maxActual)return {};
        std::vector<RangeText::OwnedContribution> parts;
        long long lo=0,hi=0,maxLo=0,maxHi=0;
        for(const auto& definition:group.definitions){
            const bool contributes=Affixes::Contributes(definition.row,properties,static_cast<int>(stat),0);
            const bool contributesMax=paired && Affixes::Contributes(definition.row,properties,static_cast<int>(stat+1),0);
            if(!contributes&&!contributesMax)continue;
            if(!definition.unconditionalScalar || (definition.source&&(std::string_view(definition.source)=="[U]"||std::string_view(definition.source)=="[RW]")&&!definition.uniqueScalar))return {};
            RangeText::OwnedContribution part;part.label=group.prefix+Affixes::NamedLabel(definition);
            if(paired){
                auto plain=definition;plain.source=nullptr;
                if(!Affixes::DamageEndpointBounds(plain,properties,static_cast<int>(stat),static_cast<int>(stat+1),
                    part.low,part.high,part.maxLow,part.maxHigh))return {};
            } else if(!Bounds(definition,properties,static_cast<int>(stat),part.low,part.high))return {};
            lo+=part.low;hi+=part.high;maxLo+=part.maxLow;maxHi+=part.maxHigh;parts.push_back(std::move(part));
        }
        if(parts.empty()){if(*actual||*maxActual)return {};continue;}
        if(*actual<lo||*actual>hi||(paired&&(*maxActual<maxLo||*maxActual>maxHi)))return {};
        for(auto& part:parts){
            const auto a=std::max<long long>(part.low,*actual-(hi-part.high));
            const auto b=std::min<long long>(part.high,*actual-(lo-part.low));
            if(a==b)part.value=static_cast<int>(a);
            if(paired){
                const auto ma=std::max<long long>(part.maxLow,*maxActual-(maxHi-part.maxHigh));
                const auto mb=std::min<long long>(part.maxHigh,*maxActual-(maxLo-part.maxLow));
                if(ma==mb)part.maxValue=static_cast<int>(ma);
            }
            label.ownedParts.push_back(std::move(part));
        }
        total+=*actual;maxTotal+=*maxActual;
    }
    if(label.ownedParts.empty()||label.ownedParts.size()>12||total<INT_MIN||total>INT_MAX||maxTotal<INT_MIN||maxTotal>INT_MAX)return {};
    label.ownedTotal=static_cast<int>(total);if(paired)label.ownedMaxTotal=static_cast<int>(maxTotal);
    return label;
}
// Only property-function-proven fixed scalar corruption values may be split
// from merged host state. Variable/unrecognized operations refuse the line.
inline std::optional<int> Exact(std::span<const Affixes::Rolled> definitions,std::span<const Affixes::Property> properties,unsigned stat) {
    long long sum=0;
    for(const auto& definition:definitions){
        if(!Affixes::Contributes(definition.row,properties,static_cast<int>(stat),0))continue;
        int lo{},hi{};if(!Bounds(definition,properties,static_cast<int>(stat),lo,hi)||lo!=hi)return {};
        sum+=lo;if(sum<INT_MIN||sum>INT_MAX)return {};
    }
    return static_cast<int>(sum);
}
}
