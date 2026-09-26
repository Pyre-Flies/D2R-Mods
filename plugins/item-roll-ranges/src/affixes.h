#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <set>
#include <span>
#include <string>
#include <vector>
namespace Affixes {
template<class T> inline T Read(const unsigned char* p,std::size_t offset) {
    T value{}; std::memcpy(&value,p+offset,sizeof(value)); return value;
}
struct Row {
    std::array<unsigned char,0x8c> bytes{};
    int Number(std::size_t offset) const { return Read<int>(bytes.data(),offset); }
    unsigned Version() const { return Read<std::uint16_t>(bytes.data(),0x22); }
    bool SameFamily(const Row& other) const {
        if (Number(0x5c)!=other.Number(0x5c) || bytes[0x66]!=other.bytes[0x66]) return false;
        for (unsigned n=0;n<3;++n) {
            const auto at=0x24+16*n;
            if (Number(at)!=other.Number(at) || Number(at+4)!=other.Number(at+4)) return false;
        }
        return true;
    }
};
struct Property { std::array<unsigned char,0x30> bytes{}; };
static_assert(sizeof(Row)==0x8c && sizeof(Property)==0x30);
// Distinct affix levels within the applicable family; duplicate levels share
// a tier. Current item level does not hide stronger tiers.
template<class Eligible> unsigned Tier(const Row& rolled,std::span<const Row> family,
                                     bool rare,unsigned itemVersion,Eligible eligible) {
    std::set<int> higher;
    for (const auto& row:family) {
        if (!row.bytes[0x54] || !row.bytes[0x82] || (rare && !row.bytes[0x64]) ||
            (itemVersion<100 && row.Version()>=100) || !rolled.SameFamily(row) ||
            row.Number(0x58)<=rolled.Number(0x58) || !eligible(row)) continue;
        higher.insert(row.Number(0x58));
    }
    return 1+static_cast<unsigned>(higher.size());
}
struct LayerEncoding { unsigned skillBits{6}, skillMask{63}; };
inline bool Contributes(const Row& row,std::span<const Property> properties,int stat,int layer,
                        LayerEncoding encoding={}) {
    for (unsigned n=0;n<3;++n) {
        const int id=row.Number(0x24+16*n),param=row.Number(0x28+16*n);
        if (id<0 || static_cast<std::size_t>(id)>=properties.size()) continue;
        const auto* p=properties[static_cast<std::size_t>(id)].bytes.data();
        for (unsigned slot=0;slot<7;++slot) {
            const auto fn=p[0x18+slot];
            if (!fn) continue;
            const int target=Read<std::uint16_t>(p,0x20+slot*2);
            // Physical damage functions have implicit stats, not statN fields.
            if (!layer && ((fn==5 && (stat==21 || stat==23)) ||
                          (fn==6 && (stat==22 || stat==24)) ||
                          (fn==7 && (stat==17 || stat==18)))) return true;
            if (target!=stat) continue;
            // Mirror native property functions, not localized tooltip text.
            if (fn==21) { // fixed class/element selector stored in Properties.valN
                if (Read<std::uint16_t>(p,0x0a+slot*2)==layer) return true;
            } else if (fn==10) { // tab = 3*class+tree, encoded as 8*class+tree
                if (param>=0 && param<24 && (param/3*8+param%3)==layer) return true;
            } else if (fn==22 || fn==24) {
                if (param>=0 && param==layer) return true;
            } else if (fn==11 || fn==19) { // triggered/charged skill + fixed level
                const int level=row.Number(0x30+16*n);
                if (param>=0 && level>0 && encoding.skillBits>0 && encoding.skillBits<16 &&
                    encoding.skillMask==((1u<<encoding.skillBits)-1) &&
                    static_cast<unsigned>(level)<=encoding.skillMask && layer>=0 &&
                    (static_cast<unsigned>(layer)>>encoding.skillBits)==static_cast<unsigned>(param) &&
                    (static_cast<unsigned>(layer)&encoding.skillMask)==static_cast<unsigned>(level)) return true;
                // Automatically calculated levels need source item level; no guessed match.
            } else if (!layer && fn>=1 && fn<=25) return true;
        }
    }
    return false;
}
struct Rolled { std::uint32_t id{}; Row row; bool prefix{}; unsigned tier{}; const char* source{}; bool uniqueScalar{}; };
inline bool UniqueScalarBounds(const Rolled& source,std::span<const Property> properties,int stat,int layer,int& low,int& high) {
    if (!source.uniqueScalar || !source.source || layer || source.row.Number(0x34)!=-1 || source.row.Number(0x44)!=-1) return false;
    const int id=source.row.Number(0x24);
    if (id<0 || static_cast<std::size_t>(id)>=properties.size()) return false;
    const auto* p=properties[static_cast<std::size_t>(id)].bytes.data();
    unsigned found=0;
    for (unsigned slot=0;slot<7;++slot) {
        if (Read<std::uint16_t>(p,0x20+slot*2)!=stat || !p[0x18+slot]) continue;
        if (p[0x18+slot]!=1 && p[0x18+slot]!=8) return false;
        ++found;
    }
    low=source.row.Number(0x2c); high=source.row.Number(0x30);
    return found==1 && low<high && low>=-1000000 && high<=1000000;
}
inline std::string NamedLabel(const Rolled& affix) {
    std::string result=affix.source?affix.source:(affix.prefix?"[Prefix]":"[Suffix]");
    if (affix.tier) result+=" [T"+std::to_string(affix.tier)+"]";
    return result;
}
// Only additive scalar properties. More complex functions require a separate
// verified transformation; Contributes alone does not establish numeric bounds.
inline bool ScalarBounds(const Rolled& affix,std::span<const Property> properties,
                         int stat,int layer,int& low,int& high) {
    if (affix.source || layer) return false;
    bool found=false; low=high=0;
    for (unsigned n=0;n<3;++n) {
        const int id=affix.row.Number(0x24+16*n);
        if (id<0 || static_cast<std::size_t>(id)>=properties.size()) continue;
        Row one=affix.row;
        for (unsigned j=0;j<3;++j) if (j!=n) { const int absent=-1; std::memcpy(one.bytes.data()+0x24+16*j,&absent,4); }
        if (!Contributes(one,properties,stat,layer)) continue;
        const auto* p=properties[static_cast<std::size_t>(id)].bytes.data();
        unsigned matches=0;
        for (unsigned slot=0;slot<7;++slot) {
            const auto fn=p[0x18+slot];
            if ((fn==7 && (stat==17 || stat==18)) ||
                (fn==1 && Read<std::uint16_t>(p,0x20+slot*2)==stat)) ++matches;
        }
        const int lo=one.Number(0x2c+16*n),hi=one.Number(0x30+16*n);
        if (matches!=1 || found || lo>hi || lo< -1000000 || hi>1000000) return false;
        low=lo; high=hi; found=true;
    }
    return found;
}
inline std::string Label(std::span<const Rolled> rolled,std::span<const Property> properties,int stat,int layer,LayerEncoding encoding={}) {
    std::string result;
    for (const auto& affix:rolled) {
        if (!Contributes(affix.row,properties,stat,layer,encoding)) continue;
        if (affix.source && result.find(affix.source)!=std::string::npos) continue;
        if (!result.empty()) result+=' ';
        result+=NamedLabel(affix);
    }
    return result;
}
// A combined display line is labeled only when every member has verified
// contributors. Emit each contributing affix once across the entire group.
inline std::string GroupLabel(std::span<const Rolled> rolled,std::span<const Property> properties,
                              std::span<const int> stats,int layer,LayerEncoding encoding={}) {
    if (stats.empty()) return {};
    for (int stat:stats) if (Label(rolled,properties,stat,layer,encoding).empty()) return {};
    std::string result;
    for (const auto& affix:rolled) {
        bool contributes=false;
        for (int stat:stats) contributes |= Contributes(affix.row,properties,stat,layer,encoding);
        if (!contributes) continue;
        if (affix.source && result.find(affix.source)!=std::string::npos) continue;
        if (!result.empty()) result+=' ';
        result+=NamedLabel(affix);
    }
    return result;
}
}
