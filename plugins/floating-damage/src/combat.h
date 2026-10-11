#pragma once
#include "native_reader.h"

namespace Damage::Combat {
using Apply=std::uintptr_t(__fastcall*)(void*,void*,void*,std::uint32_t,void*);
inline constexpr std::uint64_t ApplyRva=0x44ce80, HpSiteRva=0x44d062;
inline constexpr unsigned char ApplyBytes[]={
    0x40,0x55,0x53,0x56,0x57,0x41,0x56,0x41,0x57,0x48,0x8d,0xac,0x24,0xe8,0xfe,0xff,
    0xff,0x48,0x81,0xec,0x18,0x02,0x00,0x00,0x48,0x8b,0x05,0x29,0xe4,0x57,0x02,0x48,
    0x33,0xc4,0x48,0x89,0x85,0x00,0x01,0x00,0x00,0x48,0x8b,0xbd,0x70,0x01,0x00,0x00
};
inline constexpr unsigned char HpBytes[]={
    0x8b,0x9f,0x34,0x01,0x00,0x00,0x85,0xdb,0x7e,0x2c,0x45,0x33,0xc0,0x48,0x8b,0xce,
    0x41,0x8d,0x50,0x06,0xe8,0xa5,0x7f,0xea,0xff,0x2b,0xc3,0x45,0x33,0xc9,0x45,0x33,
    0xc0,0x48,0x8b,0xce,0x3d,0x00,0x01,0x00,0x00,0x44,0x0f,0x4d,0xc0,0x41,0x8d,0x51,
    0x06,0xe8,0x78,0xac,0xea,0xff
};
struct Observation {Monster target{}; bool valid{};};
inline Observation Before(std::uintptr_t base,void* game,void* defender,bool poisonLayout=false) noexcept {
    Observation result{}; std::uintptr_t current{},owner{}; std::uint32_t type{};
    const auto unit=reinterpret_cast<std::uintptr_t>(defender); Native::Stats stats{};
    if(!Native::Field(base+Native::GameGlobalRva,current) || current!=reinterpret_cast<std::uintptr_t>(game) ||
        !Native::Field(unit+0xd8,owner) || owner!=current || !Native::Field(unit,type) || type!=1 ||
        !Native::Field(unit+4,result.target.classId) || !Native::Field(unit+8,result.target.id) ||
        !Native::ReadStats(unit,stats) || stats.alignment!=0 || stats.hp<=0) return result;
    result.target.identity=unit; result.target.hp=stats.hp; result.target.maximum=stats.maximum;
    std::uint32_t frame{};const bool frameValid=poisonLayout && Native::ReadGameFrame(current,frame);
    result.target.poison=Native::ReadPoison(unit,stats,frame,frameValid);
    result.valid=true; return result;
}
inline bool After(const Observation& before,Native::Stats& stats) noexcept {
    std::uint32_t id{},cls{};
    return Native::Field(before.target.identity+8,id) && id==before.target.id &&
        Native::Field(before.target.identity+4,cls) && cls==before.target.classId && Native::ReadStats(before.target.identity,stats);
}
} // namespace Damage::Combat
