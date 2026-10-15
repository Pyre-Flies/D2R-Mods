#pragma once
#include "contribution_definitions.h"
#include "contribution_sources.h"
namespace RunewordSources {
// Compiled recipe semantics are qualified independently of source names. All
// seven specifications come from the active Runes row, never a static catalog.
inline std::optional<std::vector<ContributionDefinitions::Spec>> Resolve(
    const ContributionDefinitions::Table& runes,const ContributionDefinitions::Table& items,
    const ContributionDefinitions::Table& types,unsigned host,unsigned quality,unsigned flags,
    std::span<const Contributions::Socket> sockets,std::size_t properties,std::size_t groups) {
    if(runes.stride!=232 || !runes.rows || !runes.count || runes.count>4096 ||
       items.stride!=448 || types.stride!=232 || (quality!=2 && quality!=3) ||
       !(flags&0x04000000u) || (flags&0xc0000000u) || sockets.empty() || sockets.size()>6)return {};
    const auto* item=items.Row(host);if(!item)return {};
    const int primary=Affixes::Read<std::int16_t>(item,0x12e),secondary=Affixes::Read<std::int16_t>(item,0x130);
    const unsigned char* found=nullptr;
    for(std::size_t n=0;n<runes.count;++n){
        const auto* row=runes.Row(n);if(!row[0x44] || !Affixes::Read<std::uint16_t>(row,0x46))continue;
        std::size_t count=0;bool ended=false,valid=true;
        for(unsigned k=0;k<6;++k){
            const int id=Affixes::Read<int>(row,0x5c+k*4);
            if(id==-1 || id==0){ended=true;continue;}
            if(id<0 || ended || count>=sockets.size() || sockets[count].identity.classId!=static_cast<unsigned>(id)){valid=false;break;}
            ++count;
        }
        if(!valid || count!=sockets.size())continue;
        bool allowed=false;
        for(unsigned k=0;k<9;++k){
            const int target=Affixes::Read<std::int16_t>(row,0x4a+k*2);if(!target)continue;
            if(target<0)return {};
            const auto match=ContributionDefinitions::Type(types,primary,secondary,static_cast<unsigned>(target));
            if(!match)return {};
            if(k<6)allowed|=*match;else if(*match){valid=false;break;}
        }
        if(!valid || !allowed)continue;
        if(found)return {};found=row;
    }
    if(!found)return {};
    std::vector<ContributionDefinitions::Spec> result;
    for(unsigned n=0;n<7;++n){
        const auto* spec=found+0x74+n*16;const auto id=Affixes::Read<unsigned>(spec,0);
        if(id==UINT32_MAX)continue;
        if((id>>16)==0 ? id>=properties : ((id>>16)!=1 || (id&0xffff)>=groups))return {};
        result.push_back({id,Affixes::Read<int>(spec,4),Affixes::Read<int>(spec,8),Affixes::Read<int>(spec,12)});
    }
    return result.empty()?std::optional<std::vector<ContributionDefinitions::Spec>>{}:result;
}
}
