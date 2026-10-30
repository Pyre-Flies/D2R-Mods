#pragma once
#include "affixes.h"
#include <optional>
namespace ContributionDefinitions {
struct Table {
    const unsigned char* rows{};std::size_t count{},stride{};
    const unsigned char* Row(std::size_t index) const {return rows && index<count?rows+index*stride:nullptr;}
};
struct Spec {unsigned property{};int parameter{},low{},high{};};
inline std::optional<std::vector<Spec>> Gem(const Table& gems,unsigned classId,unsigned context) {
    if(gems.stride!=192||context>2)return {};
    const unsigned char* found=nullptr;
    for(std::size_t n=0;n<gems.count;++n)if(Affixes::Read<unsigned>(gems.Row(n),0x28)==classId){if(found)return {};found=gems.Row(n);}
    if(!found)return {};
    std::vector<Spec> out;
    for(unsigned n=0;n<3;++n){
        const auto* p=found+0x30+context*0x30+n*16;
        const auto id=Affixes::Read<unsigned>(p,0);if(id==0xffffffffu)continue;
        out.push_back({id,Affixes::Read<int>(p,4),Affixes::Read<int>(p,8),Affixes::Read<int>(p,12)});
    }
    return out;
}
inline std::optional<bool> Type(const Table& types,int first,int second,unsigned target) {
    if(types.stride!=232||target>=types.count)return {};
    // A repeated ancestor in a DAG is valid; a parent on the current path is
    // a cycle. Bound both the number of unique nodes and recursion depth.
    std::vector<int> active,complete;
    bool matches=false;
    auto visit=[&](auto&& self,int id)->bool {
        if(id<0)return true;
        if(static_cast<std::size_t>(id)>=types.count)return false;
        if(std::find(active.begin(),active.end(),id)!=active.end())return false;
        if(std::find(complete.begin(),complete.end(),id)!=complete.end())return true;
        if(active.size()+complete.size()>=64)return false;
        active.push_back(id);matches|=static_cast<unsigned>(id)==target;
        const auto* row=types.Row(static_cast<std::size_t>(id));
        for(unsigned at:{4u,6u}){const int parent=Affixes::Read<std::int16_t>(row,at);if(parent>0&&!self(self,parent))return false;}
        active.pop_back();complete.push_back(id);return true;
    };
    if(!visit(visit,first)||!visit(visit,second))return {};
    return matches;
}
inline std::optional<unsigned> Code(const Table& table,unsigned code,unsigned offset) {
    std::optional<unsigned> found;
    if(offset+4>table.stride)return {};
    for(std::size_t n=0;n<table.count;++n)if(Affixes::Read<unsigned>(table.Row(n),offset)==code){if(found)return {};found=static_cast<unsigned>(n);}
    return found;
}
constexpr unsigned Code(const char* s) {return static_cast<unsigned char>(s[0])|(static_cast<unsigned>(static_cast<unsigned char>(s[1]))<<8)|(static_cast<unsigned>(static_cast<unsigned char>(s[2]))<<16)|(static_cast<unsigned>(static_cast<unsigned char>(s[3]))<<24);}
inline std::optional<unsigned> Context(const Table& items,const Table& types,unsigned classId) {
    const auto* item=items.Row(classId);if(!item||items.stride!=448)return {};
    const int first=Affixes::Read<std::int16_t>(item,0x12e),second=Affixes::Read<std::int16_t>(item,0x130);
    const auto weapon=Code(types,Code("weap"),0),shield=Code(types,Code("shld"),0),armor=Code(types,Code("armo"),0);
    if(!weapon||!shield||!armor)return {};
    const auto w=Type(types,first,second,*weapon),s=Type(types,first,second,*shield),a=Type(types,first,second,*armor);
    if(!w||!s||!a)return {};
    if(*w&&!s.value()&&!a.value())return 0;
    if(*s&&!w.value())return 2;
    if(*a&&!w.value()&&!s.value())return 1;
    return {};
}
struct Corruption {bool recognized{};bool conversion{};std::vector<Spec> specs;};
inline std::optional<Corruption> ResolveCorruption(const Table& cube,const Table& items,const Table& types,
    unsigned classId,unsigned quality,std::int64_t marker,std::int64_t dummy,unsigned flagProperty,unsigned dummyProperty) {
    if(!marker && !dummy)return Corruption{};
    if(marker<=0||cube.stride!=364||items.stride!=448||types.stride!=232)return {};
    if(dummy==101)return Corruption{true,true,{}};
    if(marker!=2||dummy<102||dummy>201)return {}; // Only the observed completed preserved-item state.
    const auto orb=Code(items,Code("ka3 "),0x80);const auto* host=items.Row(classId);if(!orb||!host)return {};
    const auto roll=dummy-101;
    const int first=Affixes::Read<std::int16_t>(host,0x12e),second=Affixes::Read<std::int16_t>(host,0x130);
    for(std::size_t n=0;n<cube.count;++n){
        const auto* row=cube.Row(n);
        if(row[0]!=1||row[3]!=15||Affixes::Read<unsigned>(row,4)!=362||row[12]!=2||
           Affixes::Read<int>(row,8)<1||Affixes::Read<int>(row,8)>roll)continue;
        // Reject restrictions we have not reconstructed rather than skipping
        // to a later positive recipe that might not be the native winner.
        if(row[1]||row[2]!=0xff||Affixes::Read<unsigned>(row,0x10))return {};
        bool applicable=false,hasOrb=false;
        for(unsigned input=0;input<2;++input){
            const auto* p=row+0x14+input*10;const unsigned flags=Affixes::Read<std::uint16_t>(p,0),id=Affixes::Read<unsigned>(p,2);
            if(p[7]>1||p[8]||p[9])return {};
            if(flags==1 && id==*orb){if(p[6])return {};hasOrb=true;continue;}
            if(p[6] && p[6]!=quality)continue;
            if(flags==1)applicable|=id==classId;
            else if(flags==2){const auto match=Type(types,first,second,id);if(!match)return {};applicable|=*match;}
            else return {};
        }
        if(!hasOrb||!applicable)continue;
        if(row[0x67]!=0xfe || row[0x64])return Corruption{true,true,{}};
        bool flag=false,offset=false;std::vector<Spec> specs;
        for(unsigned mod=0;mod<5;++mod){
            const auto* p=row+0x77+mod*12;const unsigned id=Affixes::Read<unsigned>(p,0);
            if(id==0xffffffffu)continue;
            const int param=Affixes::Read<std::int16_t>(p,4),lo=Affixes::Read<std::int16_t>(p,6),hi=Affixes::Read<std::int16_t>(p,8);
            if(id==flagProperty){flag=param==0 && lo==1 && hi==1;continue;}
            if(id==dummyProperty){offset=param==0 && lo==101 && hi==101;continue;}
            if(p[10]!=100||p[11])return {};
            specs.push_back({id,param,lo,hi});
        }
        if(!flag||!offset)return {};
        return Corruption{true,false,std::move(specs)};
    }
    return {};
}
struct EnchantmentCandidate {unsigned upgradeStat{};std::vector<Spec> specs;};
inline std::optional<std::vector<EnchantmentCandidate>> ResolveEnchantmentCandidates(
    const Table& cube,const Table& items,const Table& types,std::span<const Affixes::Property> properties,
    unsigned classId,unsigned quality,unsigned upgradeStat,int qualityRecord=-1,long long upgradeCount=1) {
    if(cube.stride!=364||items.stride!=448||types.stride!=232||upgradeCount<1)return {};
    const bool multiStep=upgradeCount>1;
    if(multiStep && (upgradeCount>5 || (upgradeStat!=419 && upgradeStat!=420)))return {};
    const auto* host=items.Row(classId);if(!host)return {};
    const int first=Affixes::Read<std::int16_t>(host,0x12e),second=Affixes::Read<std::int16_t>(host,0x130);
    std::vector<EnchantmentCandidate> out;
    std::array<std::vector<std::vector<Spec>>,5> stepSpecs;
    for(std::size_t n=0;n<cube.count;++n){
        const auto* row=cube.Row(n);
        if(row[0]!=1||row[1]||(row[2]!=0xff&&row[2]>=8)||row[0x67]!=0xfe||row[0x64]||row[12]<2)continue;
        const auto isUpgradeStat=[&](unsigned s){return s==upgradeStat || (upgradeStat>=392 && upgradeStat<=395 && s>=392 && s<=395);};
        if((row[3]==16||row[3]==18) && !isUpgradeStat(Affixes::Read<unsigned>(row,4)))continue;
        const int stepVal=Affixes::Read<int>(row,8);
        if(multiStep && (row[3]!=18 || stepVal<0 || stepVal>=upgradeCount))continue;
        if(!multiStep && row[3]==18 && (upgradeStat==419 || upgradeStat==420) && stepVal!=0)continue;
        bool applicable=false,otherInput=false;
        for(unsigned input=0;input<row[12]&&input<7;++input){
            const auto* p=row+0x14+input*10;
            const unsigned flags=Affixes::Read<std::uint16_t>(p,0);
            const unsigned id=Affixes::Read<std::uint16_t>(p,2);
            const unsigned specificId=Affixes::Read<std::uint16_t>(p,4);
            if(p[8]||p[9])continue;
            bool matchesHost=false;
            if(flags==1 && specificId==0 && id==classId && (!p[6]||p[6]==quality))matchesHost=true;
            else if((flags&~0x40u)==1 && (flags&0x40u)!=0 && quality==7 && qualityRecord>=0 &&
                    id==classId && (!p[6]||p[6]==7) &&
                    (specificId==static_cast<unsigned>(qualityRecord) || specificId==static_cast<unsigned>(qualityRecord+1)))matchesHost=true;
            else if(flags==2 && specificId==0 && (!p[6]||p[6]==quality)){
                const auto match=Type(types,first,second,id);if(!match)return {};
                matchesHost=*match;
            }
            if(matchesHost && !applicable)applicable=true;
            else otherInput=true;
        }
        if(!applicable||!otherInput)continue;
        bool markerFound=false,valid=true;std::vector<Spec> specs;
        for(unsigned mod=0;mod<5;++mod){
            const auto* p=row+0x77+mod*12;const unsigned id=Affixes::Read<unsigned>(p,0);
            if(id==0xffffffffu)continue;
            if(id>=properties.size()){valid=false;break;}
            const int param=Affixes::Read<std::int16_t>(p,4),lo=Affixes::Read<std::int16_t>(p,6),hi=Affixes::Read<std::int16_t>(p,8);
            const auto& prop=properties[id].bytes;
            if(prop[0x18]==17 && !prop[0x19] && isUpgradeStat(Affixes::Read<std::uint16_t>(prop.data(),0x20))){
                if(!((param==1&&!lo&&!hi)||(!param&&lo==1&&hi==1))){valid=false;break;}
                markerFound=true;continue;
            }
            if(p[10]!=100||p[11]){valid=false;break;}
            specs.push_back({id,param,lo,hi});
        }
        if(!valid||!markerFound||specs.empty()||specs.size()>3)continue;
        if(multiStep){
            stepSpecs[static_cast<std::size_t>(stepVal)].push_back(std::move(specs));
            continue;
        }
        const bool duplicate=std::any_of(out.begin(),out.end(),[&](const EnchantmentCandidate& c){
            if(c.upgradeStat!=upgradeStat||c.specs.size()!=specs.size())return false;
            for(std::size_t k=0;k<specs.size();++k)
                if(c.specs[k].property!=specs[k].property||c.specs[k].parameter!=specs[k].parameter||
                   c.specs[k].low!=specs[k].low||c.specs[k].high!=specs[k].high)return false;
            return true;
        });
        if(!duplicate)out.push_back({upgradeStat,std::move(specs)});
    }
    if(multiStep){
        std::vector<Spec> cumulative;
        for(long long s=0;s<upgradeCount;++s){
            const auto& list=stepSpecs[static_cast<std::size_t>(s)];
            if(list.size()!=1)return out;
            for(const auto& sp:list[0]){
                auto it=std::find_if(cumulative.begin(),cumulative.end(),[&](const Spec& existing){
                    return existing.property==sp.property && existing.parameter==sp.parameter;
                });
                if(it!=cumulative.end()){it->low+=sp.low;it->high+=sp.high;}
                else cumulative.push_back(sp);
            }
        }
        if(!cumulative.empty() && cumulative.size()<=3)out.push_back({upgradeStat,std::move(cumulative)});
    }
    return out;
}
struct CraftCandidate {std::vector<Spec> specs;};
inline std::optional<std::vector<CraftCandidate>> ResolveCraftCandidates(
    const Table& cube,const Table& items,const Table& types,std::span<const Affixes::Property> properties,
    unsigned classId,unsigned quality) {
    if(cube.stride!=364||items.stride!=448||types.stride!=232||(quality!=4&&quality!=6&&quality!=8))return {};
    const auto* host=items.Row(classId);if(!host)return {};
    const int first=Affixes::Read<std::int16_t>(host,0x12e),second=Affixes::Read<std::int16_t>(host,0x130);
    std::vector<CraftCandidate> out;
    for(std::size_t n=0;n<cube.count;++n){
        const auto* row=cube.Row(n);
        if(row[0]!=1||row[1]||(row[2]!=0xff&&row[2]>=8)||row[3]||row[12]<2)continue;
        if(row[0x67]!=0xff||row[0x64]!=quality||Affixes::Read<std::uint16_t>(row,0x5e)!=0)continue;
        const auto* p0=row+0x14;const unsigned flags=Affixes::Read<std::uint16_t>(p0,0),id=Affixes::Read<unsigned>(p0,2);
        if(p0[8]||p0[9])continue;
        bool applicable=false;
        if(flags==1)applicable=id==classId;
        else if(flags==2){
            const auto match=Type(types,first,second,id);if(!match)return {};
            applicable=*match;
        }
        if(!applicable)continue;
        bool valid=true;std::vector<Spec> specs;
        for(unsigned mod=0;mod<5;++mod){
            const auto* p=row+0x77+mod*12;const unsigned modId=Affixes::Read<unsigned>(p,0);
            if(modId==0xffffffffu)continue;
            if(modId>=properties.size()||p[10]!=100||p[11]){valid=false;break;}
            const int param=Affixes::Read<std::int16_t>(p,4),lo=Affixes::Read<std::int16_t>(p,6),hi=Affixes::Read<std::int16_t>(p,8);
            specs.push_back({modId,param,lo,hi});
        }
        if(!valid||specs.empty())continue;
        const bool duplicate=std::any_of(out.begin(),out.end(),[&](const CraftCandidate& c){
            if(c.specs.size()!=specs.size())return false;
            for(std::size_t k=0;k<specs.size();++k)
                if(c.specs[k].property!=specs[k].property||c.specs[k].parameter!=specs[k].parameter||
                   c.specs[k].low!=specs[k].low||c.specs[k].high!=specs[k].high)return false;
            return true;
        });
        if(!duplicate)out.push_back({std::move(specs)});
    }
    return out;
}
inline Affixes::Rolled Rolled(const Spec& spec,const char* label) {
    Affixes::Rolled out{};out.source=label;
    std::memcpy(out.row.bytes.data()+0x24,&spec.property,4);
    std::memcpy(out.row.bytes.data()+0x28,&spec.parameter,4);
    std::memcpy(out.row.bytes.data()+0x2c,&spec.low,4);
    std::memcpy(out.row.bytes.data()+0x30,&spec.high,4);
    const int absent=-1;std::memcpy(out.row.bytes.data()+0x34,&absent,4);std::memcpy(out.row.bytes.data()+0x44,&absent,4);
    return out;
}
}
