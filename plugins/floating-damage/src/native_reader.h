#pragma once
#include "native_profile.h"
#include "model.h"
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <bit>
#include <cstdio>
#include <cstring>
#include <vector>

namespace Damage::Native {
inline bool Bytes(std::uintptr_t address,void* buffer,std::size_t size) noexcept {
    if(address<65536 || size>65536 || address>0x7fffffffffffULL-size) return false;
    SIZE_T got{};
    return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),buffer,size,&got) && got==size;
}
template<class T> bool Field(std::uintptr_t address,T& value) noexcept { return Bytes(address,&value,sizeof(value)); }
inline bool Hash(HMODULE module,const char* expected) noexcept {
    wchar_t path[32768]{};
    const auto length=GetModuleFileNameW(module,path,32768);
    if(!length || length>=32768) return false;
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE alg{}; BCRYPT_HASH_HANDLE hash{};
    bool match=false;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0) {
        DWORD objectSize{},got{};
        if(BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&objectSize),sizeof(objectSize),&got,0)>=0) {
            try {
                std::vector<unsigned char> object(objectSize),block(65536);
                if(BCryptCreateHash(alg,&hash,object.data(),objectSize,nullptr,0,0)>=0) {
                    DWORD count{}; bool valid=true;
                    for(;;) {
                        if(!ReadFile(file,block.data(),static_cast<DWORD>(block.size()),&count,nullptr)) {valid=false;break;}
                        if(!count) break;
                        if(BCryptHashData(hash,block.data(),count,0)<0) {valid=false;break;}
                    }
                    unsigned char digest[32]{};
                    if(valid && BCryptFinishHash(hash,digest,32,0)>=0) {
                        char text[65]{};
                        for(unsigned i=0;i<32;++i) std::snprintf(text+i*2,3,"%02X",digest[i]);
                        match=std::strcmp(text,expected)==0;
                    }
                    BCryptDestroyHash(hash); hash=nullptr;
                }
            } catch(...) { if(hash) BCryptDestroyHash(hash); }
        }
        BCryptCloseAlgorithmProvider(alg,0);
    }
    CloseHandle(file); return match;
}
struct Stats {std::int64_t hp{},maximum{},alignment{},regen{};bool hasHp{},hasMax{},hasRegen{};};
inline bool ReadStats(std::uintptr_t unit,Stats& result) noexcept {
    std::uintptr_t list{}; std::int32_t flags{};
    if(!Field(unit+0x88,list) || !Field(list+0x1c,flags)) return false;
    struct Array {std::uintptr_t first; std::uint64_t count,capacity;} array{};
    if(!Field(list+(flags<0?0xa8:0x30),array) || !array.first || !array.count || array.count>2048 || array.capacity<array.count) return false;
    struct Stat {std::uint32_t layer,id;std::int64_t value;};
    static_assert(sizeof(Stat)==16);
    std::array<Stat,2048> entries{};
    if(!Bytes(array.first,entries.data(),static_cast<std::size_t>(array.count)*sizeof(Stat))) return false;
    result={};
    for(std::uint64_t i=0;i<array.count;++i) {
        const auto& s=entries[static_cast<std::size_t>(i)];
        if(s.layer) continue;
        if(s.id==6) {result.hp=s.value;result.hasHp=true;}
        if(s.id==7) {result.maximum=s.value;result.hasMax=true;}
        if(s.id==172) result.alignment=s.value;
        if(s.id==74) {result.regen=s.value;result.hasRegen=true;}
    }
    // Health remaining has a zero floor; this reports lost HP rather than overkill.
    result.hp=std::max<std::int64_t>(0,result.hp);
    return result.hasHp && result.hasMax && result.maximum>0 && result.maximum<(1LL<<50) && result.hp<=result.maximum;
}
// Stat 74 alone has live witnesses with a zero-extended negative int32 cell.
// Do not apply this conversion to widened HP or arbitrary stats.
inline bool SignedRegen(std::int64_t raw,std::int32_t& out) noexcept {
    if(raw<INT32_MIN || raw>UINT32_MAX) return false;
    out=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(raw));return true;
}
inline bool ReadBaseRegen(std::uintptr_t unit,std::int32_t& value) noexcept {
    std::uintptr_t root{};
    struct Array {std::uintptr_t first;std::uint64_t count,capacity;} array{};
    if(!Field(unit+0x88,root) || !Field(root+0x30,array) || array.count>2048 || array.capacity<array.count) return false;
    struct Stat {std::uint32_t layer,id;std::int64_t value;};std::array<Stat,2048> entries{};
    if(array.count && !Bytes(array.first,entries.data(),static_cast<std::size_t>(array.count)*sizeof(Stat))) return false;
    value=0;bool found{};
    for(std::uint64_t i=0;i<array.count;++i) {
        const auto& stat=entries[static_cast<std::size_t>(i)];
        if(stat.layer==0 && stat.id==74) {
            if(found || !SignedRegen(stat.value,value)) return false;
            found=true;
        }
    }
    return true;
}
struct PoisonState {bool readable{},found{},hasRate{};std::int64_t rate{};float expiration{};};
inline PoisonState ReadPoisonState(std::uintptr_t unit) noexcept {
    PoisonState result{};std::uintptr_t root{};std::int32_t flags{};
    if(!Field(unit+0x88,root) || !Field(root+0x1c,flags)) return result;
    result.readable=true;if(flags>=0) return result;
    std::array<std::uintptr_t,64> seen{};std::size_t visited{};
    for(unsigned head:{0x90U,0x98U}) {
        std::uintptr_t list{};if(!Field(root+head,list)) {result.readable=false;return result;}
        while(list && visited<seen.size()) {
            if(std::find(seen.begin(),seen.begin()+static_cast<std::ptrdiff_t>(visited),list)!=seen.begin()+static_cast<std::ptrdiff_t>(visited)) {
                result.readable=false;return result;
            }
            seen[visited++]=list;std::uint32_t state{};
            if(!Field(list+0x20,state)) {result.readable=false;return result;}
            if(state==2) {
                struct Array {std::uintptr_t first;std::uint64_t count,capacity;} array{};
                if(!Field(list+0x30,array) || array.count>2048 || array.capacity<array.count ||
                    !Field(list+0x24,result.expiration)) {result.readable=false;return result;}
                struct Stat {std::uint32_t layer,id;std::int64_t value;};std::array<Stat,2048> entries{};
                if(array.count && !Bytes(array.first,entries.data(),static_cast<std::size_t>(array.count)*sizeof(Stat))) {result.readable=false;return result;}
                result.found=true;
                for(std::uint64_t i=0;i<array.count;++i) if(entries[static_cast<std::size_t>(i)].layer==0 && entries[static_cast<std::size_t>(i)].id==74) {
                    if(result.hasRate) {result.readable=false;return result;}
                    result.rate=entries[static_cast<std::size_t>(i)].value;result.hasRate=true;
                }
                return result;
            }
            if(!Field(list+0x68,list)) {result.readable=false;return result;}
        }
        if(list) {result.readable=false;return result;}
    }
    return result;
}
inline bool ReadGameFrame(std::uintptr_t game,std::uint32_t& frame) noexcept {
    return Field(game+0x170,frame) && frame<0x1000000U;
}
inline bool PoisonLayoutMatches(std::uintptr_t base) noexcept {
    constexpr unsigned char expiration[]={0x48,0x85,0xc9,0x74,0x14,0x85,0xd2,0x7e,0x04,0x83,0x49,0x1c,0x02,
        0x66,0x0f,0x6e,0xc2,0x0f,0x5b,0xc0,0xf3,0x0f,0x11,0x41,0x24,0xc3};
    constexpr unsigned char frame[]={0x44,0x03,0xb7,0x70,0x01,0x00,0x00};
    std::array<unsigned char,sizeof(expiration)> actual{};std::array<unsigned char,sizeof(frame)> clock{};
    return Bytes(base+0x2f7ab0,actual.data(),actual.size()) && std::memcmp(actual.data(),expiration,actual.size())==0 &&
        Bytes(base+0x451a25,clock.data(),clock.size()) && std::memcmp(clock.data(),frame,clock.size())==0;
}
inline PoisonStamp ReadPoison(std::uintptr_t unit,const Stats& stats,std::uint32_t frame,bool frameValid) noexcept {
    PoisonStamp result{};if(!frameValid) return result;
    result.frame=frame;
    const auto state=ReadPoisonState(unit);std::int32_t rate{},combined{},base{};
    // Surviving captures lose the full state rate each frame even with positive
    // base regen. Account for that base contribution; never use combined regen
    // as the displayed amount or accept an unexplained extra contribution.
    if(!state.readable || !state.found || !state.hasRate || !stats.hasRegen ||
        !SignedRegen(state.rate,rate) || !SignedRegen(stats.regen,combined) || rate>=0 || rate==INT32_MIN || combined>=0 ||
        !ReadBaseRegen(unit,base) || base<0 || static_cast<std::int64_t>(rate)+base!=combined ||
        !std::isfinite(state.expiration) || state.expiration<=static_cast<float>(frame) || state.expiration>=16777216.0f ||
        std::floor(state.expiration)!=state.expiration) return result;
    result.rate=-static_cast<std::int64_t>(rate);result.expires=static_cast<std::uint32_t>(state.expiration);
    result.valid=result.expires-frame<=15000;return result;
}
inline bool Position(std::uintptr_t unit,Point& p) noexcept {
    std::uintptr_t path{}; std::uint32_t xy[2]{};
    if(!Field(unit+0x38,path) || !Bytes(path,xy,sizeof(xy))) return false;
    p={static_cast<float>(xy[0])/65536.0f,static_cast<float>(xy[1])/65536.0f};
    return Valid(p);
}
struct Snapshot {
    std::array<Monster,1024> monsters{};
    std::size_t count{};
    std::uintptr_t game{};
    Point player{};
    bool playerValid{};
};
inline bool SnapshotGame(std::uintptr_t base,std::uint32_t playerId,Snapshot& result,bool poisonLayout=false) noexcept {
    result={};
    if(!Field(base+GameGlobalRva,result.game) || result.game<65536) return false;
    std::uint32_t frame{};const bool frameValid=poisonLayout && ReadGameFrame(result.game,frame);
    std::array<std::uintptr_t,128> buckets{};
    if(!Bytes(result.game+0x2630,buckets.data(),sizeof(buckets))) return false;
    std::array<std::uintptr_t,2048> seen{}; std::size_t seenCount{};
    for(unsigned bucket=0;bucket<128;++bucket) {
        auto unit=buckets[bucket];
        for(unsigned chain=0;unit && chain<128;++chain) {
            if(seenCount==seen.size() || std::find(seen.begin(),seen.begin()+static_cast<std::ptrdiff_t>(seenCount),unit)!=seen.begin()+static_cast<std::ptrdiff_t>(seenCount)) return false;
            seen[seenCount++]=unit;
            struct Header {std::uint32_t type,cls,id,mode;} header{};
            std::uintptr_t owner{},next{};
            if(!Field(unit,header) || !Field(unit+0xd8,owner) || !Field(unit+0x158,next) ||
                header.type!=1 || (header.id&127)!=bucket || owner!=result.game) return false;
            Stats stats{}; Point point{};
            if(ReadStats(unit,stats) && stats.alignment==0 && Position(unit,point)) {
                if(result.count==result.monsters.size()) return false;
                result.monsters[result.count++]={unit,header.id,header.cls,stats.hp,stats.maximum,point,ReadPoison(unit,stats,frame,frameValid)};
            }
            unit=next;
        }
        if(unit) return false;
    }
    if(playerId) {
        std::uintptr_t player{};
        if(Field(result.game+0x2230+(playerId&127)*8,player)) {
            for(unsigned i=0;player && i<128;++i) {
                std::uint32_t type{},id{}; std::uintptr_t owner{};
                if(!Field(player,type) || !Field(player+8,id) || !Field(player+0xd8,owner)) break;
                if(type==0 && id==playerId && owner==result.game) {result.playerValid=Position(player,result.player);break;}
                if(!Field(player+0x158,player)) break;
            }
        }
    }
    return true;
}
} // namespace Damage::Native
