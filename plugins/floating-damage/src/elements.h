#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace Damage {
enum Element : unsigned { Unclassified=0, Physical=1, Fire=2, Cold=4, Lightning=8, Magic=16, Poison=32, Unsupported=128 };
struct CombatPacket {
    std::array<unsigned char,0x138> bytes{};
    std::int32_t Value(unsigned offset) const noexcept {
        std::int32_t value{}; std::memcpy(&value,bytes.data()+offset,sizeof(value)); return value;
    }
    std::uint64_t Wide(unsigned offset) const noexcept {
        std::uint64_t value{};std::memcpy(&value,bytes.data()+offset,sizeof(value));return value;
    }
    std::uint16_t ResultFlags() const noexcept {
        std::uint16_t value{};std::memcpy(&value,bytes.data()+4,sizeof(value));return value;
    }
    bool CriticalMarker() const noexcept {return (ResultFlags()&0x2000U)!=0;}
    unsigned Elements() const noexcept {
        // Controlled captures identify these six fields on the admitted build.
        // Refuse negative components and totals we cannot account for.
        constexpr unsigned offsets[]={0x18,0x20,0x34,0x2c,0x30,0x38};
        constexpr unsigned types[]={Physical,Fire,Cold,Lightning,Magic,Poison};
        std::int64_t sum{}; unsigned mask{};
        for(unsigned i=0;i<6;++i) {
            const auto value=Value(offsets[i]);
            if(value<0) return Unsupported;
            if(value>0) {sum+=value; mask|=types[i];}
        }
        return sum>0 && sum==Value(0x134)?mask:Unsupported;
    }
};
} // namespace Damage
