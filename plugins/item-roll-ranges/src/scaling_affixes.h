#pragma once
#include "affixes.h"
#include <optional>
#include <climits>
#include <string_view>
namespace ScalingAffixes {
// Current native Experience rows have eight class columns and an ExpRatio.
// Row zero contains MaxLvl. Unequal class caps need a separate player proof.
inline std::optional<int> LevelCap(std::span<const unsigned char> header,std::size_t rows) {
    if(header.size()!=36 || rows<3 || rows>1024)return {};
    const int cap=Affixes::Read<int>(header.data(),0);
    if(cap<1 || cap>255 || static_cast<std::size_t>(cap)+2>rows)return {};
    for(unsigned n=1;n<8;++n)if(Affixes::Read<int>(header.data(),n*4)!=cap)return {};
    return cap;
}
inline std::optional<int> Coefficient(const Affixes::Rolled& affix,
    std::span<const Affixes::Property> properties,int stat) {
    if(affix.source && std::string_view(affix.source)=="[U]" && !affix.uniqueScalar)return {};
    std::optional<int> result;
    for(unsigned n=0;n<3;++n){
        const int id=affix.row.Number(0x24+n*16);
        if(id<0 || static_cast<std::size_t>(id)>=properties.size())continue;
        const auto* p=properties[static_cast<std::size_t>(id)].bytes.data();
        for(unsigned slot=0;slot<7;++slot){
            if(!p[0x18+slot] || Affixes::Read<std::uint16_t>(p,0x20+slot*2)!=stat)continue;
            if(p[0x18+slot]!=17 || result || affix.row.Number(0x28+n*16)<=0)return {};
            result=affix.row.Number(0x28+n*16);
        }
    }
    return result;
}
inline std::optional<std::pair<int,int>> Bounds(std::span<const unsigned char> row,int coefficient,int cap) {
    if(row.size()!=0x144 || row[0x14] || row[0x32]!=19 ||
       (row[0x50]!=2 && row[0x50]!=4 && row[0x50]!=5) || row[0x51]>15 ||
       Affixes::Read<std::uint16_t>(row.data(),0x52)!=12 || coefficient<=0 || cap<1 || cap>255)return {};
    const auto factor=1ll<<row[0x51];
    const auto low=coefficient/factor,high=static_cast<long long>(coefficient)*cap/factor;
    if(high>INT_MAX)return {};
    return std::pair{static_cast<int>(low),static_cast<int>(high)};
}
// Positive level and chance are separate fixed operands of property function11.
inline std::optional<std::pair<int,int>> Proc(const Affixes::Rolled& affix,
    std::span<const Affixes::Property> properties,int stat,int layer,Affixes::LayerEncoding encoding) {
    if(affix.source || encoding.skillBits<1 || encoding.skillBits>8 ||
       encoding.skillMask!=((1u<<encoding.skillBits)-1))return {};
    std::optional<std::pair<int,int>> result;
    for(unsigned n=0;n<3;++n){
        const int id=affix.row.Number(0x24+n*16);
        if(id<0 || static_cast<std::size_t>(id)>=properties.size())continue;
        const auto* p=properties[static_cast<std::size_t>(id)].bytes.data();
        for(unsigned slot=0;slot<7;++slot){
            if(!p[0x18+slot] || Affixes::Read<std::uint16_t>(p,0x20+slot*2)!=stat)continue;
            const int skill=affix.row.Number(0x28+n*16),chance=affix.row.Number(0x2c+n*16),level=affix.row.Number(0x30+n*16);
            if(p[0x18+slot]!=11 || result || skill<0 || skill>0x7fff || chance<1 || chance>100 ||
               level<1 || static_cast<unsigned>(level)>encoding.skillMask || layer<0 ||
               static_cast<unsigned>(layer)!=(static_cast<unsigned>(skill)<<encoding.skillBits | static_cast<unsigned>(level)))return {};
            result=std::pair{chance,level};
        }
    }
    return result;
}
}
