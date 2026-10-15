#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>
namespace Contributions {
struct Stat {std::uint64_t key{}; std::int64_t value{};};
using Stats=std::vector<Stat>;
inline std::int64_t Value(const Stats& values,unsigned stat,unsigned layer=0) {
    const auto key=(std::uint64_t{stat}<<32)|layer;
    const auto found=std::lower_bound(values.begin(),values.end(),key,[](const Stat& a,std::uint64_t b){return a.key<b;});
    return found!=values.end() && found->key==key?found->value:0;
}
inline bool Add(Stats& values,const Stats& extra) {
    for (const auto& entry:extra) {
        if(entry.value<INT32_MIN||entry.value>INT32_MAX)return false;
        auto found=std::lower_bound(values.begin(),values.end(),entry.key,[](const Stat& a,std::uint64_t b){return a.key<b;});
        if (found==values.end() || found->key!=entry.key) values.insert(found,entry);
        else {found->value+=entry.value;if(found->value<INT32_MIN || found->value>INT32_MAX)return false;}
    }
    return true;
}
struct Identity {
    unsigned id{},classId{},quality{},version{},bank{},flags{},autoId{};
    std::array<unsigned,6> affixes{};
    int qualityRecord{-1};
};
struct Socket {Identity identity;Stats values;};
struct Snapshot {Identity identity;Stats own,primary,total;std::vector<Socket> sockets;Stats word{};};
inline bool ExtendedFlags(unsigned flags) {return flags==0x80000000u || flags==0xa0000000u;}
// No pointer escapes: source metadata and values are owned copies. Nested socket
// original modifier children duplicate the attached aggregate and are not walked.
template<class Read> std::optional<Snapshot> Capture(std::uintptr_t unit,Read read,unsigned wordState=0) {
    auto load=[&](std::uintptr_t at,auto& out){return at>=0x10000 && at<=0x7fffffffffffULL-sizeof(out) && read(at,&out,sizeof(out));};
    auto vector=[&](std::uintptr_t list,unsigned offset)->std::optional<Stats>{
        std::array<std::uint64_t,3> v{};
        if(!load(list+offset,v)||v[1]>2048||v[1]>v[2]||
           (v[1] && (v[0]<0x10000||v[0]>0x7fffffffffffULL-v[1]*16)))return {};
        Stats out;
        for(std::uint64_t n=0;n<v[1];++n){
            std::array<std::uint64_t,2> entry{};
            if(!load(static_cast<std::uintptr_t>(v[0])+n*16,entry)||
               (n && entry[0]<=out.back().key))return {};
            // Native scalar accessor reads the signed low DWORD, not an int64
            // sign inferred from the unused high DWORD of a stat entry.
            out.push_back({entry[0],std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(entry[1]))});
        }
        return out;
    };
    auto identity=[&](std::uintptr_t at)->std::optional<Identity>{
        std::array<unsigned,3> ids{};std::uintptr_t data{};unsigned char bank{};
        Identity out;std::uint16_t version{};
        if(!load(at,ids)||ids[0]!=4||!load(at+0x10,data)||data<0x1001c||
           !load(at+0x1bd,bank)||bank<1||bank>3||!load(data,out.quality)||out.quality<1||out.quality>8||
           !load(data+0x18,out.flags)||!load(data+0x40,version)||
           !load(data-28,out.autoId)||!load(data-24,out.affixes)||!load(data+0x34,out.qualityRecord))return {};
        out.id=ids[2];out.classId=ids[1];out.bank=bank;out.version=version;return out;
    };
    const auto host=identity(unit);if(!host)return {};
    std::uintptr_t root{},child{};unsigned flags{};
    if(!load(unit+0x88,root)||!load(root+0x1c,flags)||!ExtendedFlags(flags)||!load(root+0x90,child))return {};
    const auto primary=vector(root,0x30),total=vector(root,0xa8);if(!primary||!total)return {};
    Snapshot out{*host,{},*primary,*total,{}};
    struct Attached {std::uintptr_t unit;Socket socket;};
    std::vector<Attached> attached;std::vector<std::uintptr_t> seen;unsigned wordLists=0;
    while(child){
        if(seen.size()>=64||std::find(seen.begin(),seen.end(),child)!=seen.end())return {};
        seen.push_back(child);std::uintptr_t parent{},owner{};unsigned state{};int expiry{};
        if(!load(child+0x78,parent)||parent!=root||!load(child+0x1c,flags)||
           !load(child+0x20,state)||!load(child+0x18,expiry))return {};
        const bool word=wordState && state==wordState && (host->flags&0x04000000u);
        if(state && !word)return {};
        if(ExtendedFlags(flags)){
            std::uintptr_t ownerRoot{};
            if(state||expiry||attached.size()>=6||!load(child+0xa0,owner)||owner==unit||!load(owner+0x88,ownerRoot)||ownerRoot!=child)return {};
            const auto id=identity(owner);const auto values=vector(child,0xa8);
            if(!id||!values||id->bank!=host->bank||std::any_of(attached.begin(),attached.end(),[&](const Attached& a){return a.unit==owner||a.socket.identity.id==id->id;}))return {};
            attached.push_back({owner,{*id,*values}});
        } else {
            if(flags!=0x40||expiry!=-1||!load(child,owner)||owner!=unit)return {};
            const auto values=vector(child,0x30);if(!values||!Add(out.own,*values))return {};
            if(word && (++wordLists!=1 || !Add(out.word,*values)))return {};
        }
        if(!load(child+0x68,child))return {};
    }
    if(wordState && (host->flags&0x04000000u) && wordLists!=1)return {};
    if(attached.empty())return out;
    std::uintptr_t control{},inventory{};unsigned refs{},magic{};
    if(!load(unit+0xa0,control)||(control&&!load(control+8,refs))||
       !load(unit+(control&&refs?0x98:0x90),inventory)||!load(inventory,magic)||magic!=0x01020304||!load(inventory+0x10,child))return {};
    seen.clear();
    while(child){
        if(child==unit||seen.size()>=6||std::find(seen.begin(),seen.end(),child)!=seen.end())return {};
        seen.push_back(child);const auto id=identity(child);std::uintptr_t data{};
        if(!id||!load(child+0x10,data))return {};
        const auto found=std::find_if(attached.begin(),attached.end(),[&](const Attached& a){return a.unit==child&&a.socket.identity.id==id->id&&a.socket.identity.classId==id->classId;});
        if(found==attached.end())return {};
        out.sockets.push_back(found->socket);if(!load(data+0xb0,child))return {};
    }
    if(out.sockets.size()!=attached.size())return {};
    return out;
}
inline std::optional<int> Normalize(std::int64_t raw,unsigned shift) {
    if(shift>16)return {};
    const auto divisor=std::int64_t{1}<<shift;
    if(raw%divisor || raw/divisor<INT_MIN || raw/divisor>INT_MAX)return {};
    return static_cast<int>(raw/divisor);
}
}
