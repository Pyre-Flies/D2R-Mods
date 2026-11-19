#pragma once
#include <algorithm>
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
struct Property { std::array<unsigned char,0x30> bytes{}; };
struct Row {
    std::array<unsigned char,0x8c> bytes{};
    int Number(std::size_t offset) const { return Read<int>(bytes.data(),offset); }
    std::string Name() const {
        const auto* p=reinterpret_cast<const char*>(bytes.data());
        return std::string(p,strnlen(p,32));
    }
    std::uint16_t NameStringIndex() const { return Read<std::uint16_t>(bytes.data(),0x20); }
    unsigned Version() const { return Read<std::uint16_t>(bytes.data(),0x22); }
    bool Spawnable() const { return bytes[0x54]!=0; }
    int Level() const { return Number(0x58); }
    int Group() const { return Number(0x5c); }
    int MaxLevel() const { return Number(0x60); }
    bool RareAllowed() const { return bytes[0x64]!=0; }
    int LevelReq() const { return static_cast<int>(bytes[0x65]); }
    unsigned ClassSpecific() const { return bytes[0x66]; }
    unsigned Frequency() const { return bytes[0x82]; }
    bool SameTypeRestrictions(const Row& other) const {
        return std::memcmp(bytes.data()+0x6a,other.bytes.data()+0x6a,0x18)==0;
    }
    bool SameFamily(const Row& other,std::span<const Property> properties={}) const {
        if (Number(0x5c)!=other.Number(0x5c) || bytes[0x66]!=other.bytes[0x66]) return false;
        for (unsigned n=0;n<3;++n) {
            const auto at=0x24+16*n;
            if (Number(at)!=other.Number(at)) return false;
            const int id=Number(at);
            if(id>=0 && static_cast<std::size_t>(id)<properties.size()) {
                bool random=false,onlyRandom=true;
                for(unsigned slot=0;slot<7;++slot) {
                    const auto fn=properties[static_cast<std::size_t>(id)].bytes[0x18+slot];
                    random|=fn==12;onlyRandom&=!fn || fn==12;
                }
                if(random && onlyRandom) {
                    // Function12's parameter is bonus strength; min/max select
                    // a stat layer. Keep class/skill pools distinct across tiers.
                    if(Number(at+4)<=0 || other.Number(at+4)<=0 ||
                       Number(at+8)!=other.Number(at+8) || Number(at+12)!=other.Number(at+12))return false;
                    continue;
                }
            }
            if (Number(at+4)!=other.Number(at+4)) {
                // Function17's nonzero parameter is a coefficient, not a skill,
                // class or layer selector. Keep different coefficients in one
                // family; different property IDs and mixed functions stay apart.
                if (id<0 || static_cast<std::size_t>(id)>=properties.size() ||
                    Number(at+4)<=0 || other.Number(at+4)<=0) return false;
                bool coefficient=false;
                for (unsigned slot=0;slot<7;++slot) {
                    const auto fn=properties[static_cast<std::size_t>(id)].bytes[0x18+slot];
                    if (fn && fn!=17) return false;
                    coefficient |= fn==17;
                }
                if (!coefficient) return false;
            }
        }
        return true;
    }
};
struct PropertyGroup { std::array<unsigned char,0xc8> bytes{}; };
static_assert(sizeof(Row)==0x8c && sizeof(Property)==0x30 && sizeof(PropertyGroup)==0xc8);

// Native D2RCore.dll+0x813FE0 affix level (alvl) calculation.
inline int CalculateAffixLevel(int ilvl,int qlvl,int magicLvl=0) {
    const int clampedIlvl=ilvl<1?1:(ilvl>99?99:ilvl);
    const int base=qlvl>clampedIlvl?qlvl:clampedIlvl;
    if (magicLvl>0) {
        const int alvl=base+magicLvl;
        return alvl<1?1:(alvl>99?99:alvl);
    }
    const int threshold=99-(qlvl/2);
    const int alvl=(base<threshold)?(base-(qlvl/2)):(2*base-99);
    return alvl<1?1:(alvl>99?99:alvl);
}

// Native D2RCore.dll+0x815400 affix pool weight at a given alvl.
inline unsigned AffixSpawnWeight(const Row& row,int alvl,int magicLvl=0) {
    if (!row.Spawnable() || !row.Frequency() || alvl<=0) return 0;
    if (row.Level()>alvl) return 0;
    if (row.MaxLevel()>0 && alvl>row.MaxLevel()) return 0;
    if (magicLvl>0) {
        const unsigned lvl=row.Level()>0?static_cast<unsigned>(row.Level()):1u;
        return row.Frequency()*lvl;
    }
    return row.Frequency();
}

struct TierSummary {
    unsigned tier{};
    unsigned totalTiers{};
    unsigned bestTierAtAlvl{};
    unsigned spawnWeight{};
};

// Distinct affix levels within the applicable family; duplicate levels share
// a tier. Current item level does not hide stronger tiers.
template<class Eligible> unsigned Tier(const Row& rolled,std::span<const Row> family,
                                     bool rare,unsigned itemVersion,Eligible eligible,
                                     std::span<const Property> properties={}) {
    std::set<int> higher;
    const auto rolledLevel=rolled.Number(0x58);
    for (const auto& row:family) {
        const auto rowLevel=row.Number(0x58);
        if (rowLevel<=rolledLevel || !row.bytes[0x54] || !row.bytes[0x82] || (rare && !row.bytes[0x64]) ||
            (itemVersion<100 && row.Version()>=100) || higher.contains(rowLevel) ||
            !rolled.SameFamily(row,properties) || !eligible(row)) continue;
        higher.insert(rowLevel);
    }
    return 1+static_cast<unsigned>(higher.size());
}

template<class Eligible>
inline TierSummary EvaluateTier(const Row& rolled,std::span<const Row> family,
                                bool rare,unsigned itemVersion,int alvl,int magicLvl,
                                Eligible eligible,std::span<const Property> properties={}) {
    std::set<int> allLevels;
    std::set<int> higherLevels;
    int bestAlvlLevel=-1;
    allLevels.insert(rolled.Level());
    for (const auto& row:family) {
        if (!row.Spawnable() || !row.Frequency() || (rare && !row.RareAllowed()) ||
            (itemVersion<100 && row.Version()>=100) || !rolled.SameFamily(row,properties) ||
            !eligible(row)) continue;
        const int lvl=row.Level();
        allLevels.insert(lvl);
        if (lvl>rolled.Level()) higherLevels.insert(lvl);
        if (alvl>0 && lvl<=alvl && (row.MaxLevel()<=0 || alvl<=row.MaxLevel())) {
            if (lvl>bestAlvlLevel) bestAlvlLevel=lvl;
        }
    }
    if (alvl>0 && rolled.Spawnable() && rolled.Frequency() && (!rare || rolled.RareAllowed()) &&
        !(itemVersion<100 && rolled.Version()>=100) && eligible(rolled) &&
        rolled.Level()<=alvl && (rolled.MaxLevel()<=0 || alvl<=rolled.MaxLevel())) {
        if (rolled.Level()>bestAlvlLevel) bestAlvlLevel=rolled.Level();
    }
    unsigned bestTierAtAlvl=0;
    if (bestAlvlLevel>=0) {
        unsigned higherThanBest=0;
        for (int lvl:allLevels) if (lvl>bestAlvlLevel) ++higherThanBest;
        bestTierAtAlvl=1+higherThanBest;
    }
    return {
        1+static_cast<unsigned>(higherLevels.size()),
        static_cast<unsigned>(allLevels.size()),
        bestTierAtAlvl,
        alvl>0?AffixSpawnWeight(rolled,alvl,magicLvl):0
    };
}

struct LayerEncoding { unsigned skillBits{6}, skillMask{63}; bool randomSkills{}; bool chargeSourcesOnly{}; };
inline bool RandomLayerSpec(int param,int low,int high,unsigned layer,int stat=0) {
    if (param<=0 || param>1000000 || low<0 || low>high || high>65535) return false;
    if (stat==188 && (layer%8)>=3) return false;
    if (layer>=static_cast<unsigned>(low) && layer<=static_cast<unsigned>(high) &&
        !(stat==188 && high<24 && (low%3)==0 && !(low>=8 && (low%8)==0))) return true;
    // item_addskill_tab (stat 188) encodes runtime layer as 8*class + tree,
    // while properties fn=12 may use either 8*class + tree or linear 3*class + tree (0..23) in low..high.
    if (stat==188 && high<24 && (layer%8)<3 && !(low>=8 && (low%8)==0 && high==low+2)) {
        const unsigned linear=(layer/8)*3+(layer%8);
        return linear>=static_cast<unsigned>(low) && linear<=static_cast<unsigned>(high);
    }
    return false;
}
#if defined(_MSC_VER)
__declspec(noinline)
#endif
inline bool PropertyContributes(const Property& property,int param,int low,int level,int stat,int layer,
                                LayerEncoding encoding={}) {
    const auto* p=property.bytes.data();
    for (unsigned slot=0;slot<7;++slot) {
        const auto fn=p[0x18+slot];
        if (!fn) continue;
        if(encoding.chargeSourcesOnly && stat==204 && (fn!=19 || p[2+slot]))continue;
        const int target=Read<std::uint16_t>(p,0x20+slot*2);
        // Physical damage functions have implicit stats, not statN fields.
        if (!layer && ((fn==5 && (stat==21 || stat==23)) ||
                      (fn==6 && (stat==22 || stat==24)) ||
                      (fn==7 && (stat==17 || stat==18)) ||
                      (!p[2+slot] && ((fn==15 && target==21 && stat==23) || (fn==16 && target==22 && stat==24))))) return true;
        if (target!=stat) continue;
        // Mirror native property functions, not localized tooltip text.
        if (fn==21) { // fixed class/element selector stored in Properties.valN
            if (Read<std::uint16_t>(p,0x0a+slot*2)==layer) return true;
        } else if (fn==10) { // tab = 3*class+tree, encoded as 8*class+tree
            if (param>=0 && param<24 && (param/3*8+param%3)==layer) return true;
        } else if (fn==9 || fn==22 || fn==24) { // single skill / oskill / state / monster param
            if (param>=0 && param==layer) return true;
        } else if(fn==12) {
            if(encoding.randomSkills && layer>=0 && RandomLayerSpec(param,low,level,static_cast<unsigned>(layer),stat))return true;
        } else if (fn==19 && encoding.chargeSourcesOnly && stat==204 && level<0) {
            // Source identity only: negative level operands are calculated by
            // the game. The saved affix/property still identifies its skill.
            // Do not expose this match to any numeric-bound calculation.
            if(!p[2+slot] && param>=0 && encoding.skillBits>0 && encoding.skillBits<16 &&
               encoding.skillMask==((1u<<encoding.skillBits)-1) && layer>=0 &&
               (static_cast<unsigned>(layer)>>encoding.skillBits)==static_cast<unsigned>(param) &&
               (static_cast<unsigned>(layer)&encoding.skillMask)>0)return true;
        } else if (fn==11 || fn==19) { // triggered/charged skill + fixed level
            if (param>=0 && level>0 && encoding.skillBits>0 && encoding.skillBits<16 &&
                encoding.skillMask==((1u<<encoding.skillBits)-1) &&
                static_cast<unsigned>(level)<=encoding.skillMask && layer>=0 &&
                (static_cast<unsigned>(layer)>>encoding.skillBits)==static_cast<unsigned>(param) &&
                (static_cast<unsigned>(layer)&encoding.skillMask)==static_cast<unsigned>(level)) return true;
            // Automatically calculated levels need source item level; no guessed match.
        } else if (!layer && fn>=1 && fn<=25) return true;
    }
    return false;
}
inline bool SpecContributes(std::uint32_t id,int param,int low,int level,
                            std::span<const Property> properties,std::span<const PropertyGroup> groups,
                            int stat,int layer,LayerEncoding encoding,unsigned depth=0) {
    if (depth>4 || id==0xffffffffu) return false;
    if ((id>>16)==0)
        return id<properties.size() && PropertyContributes(properties[id],param,low,level,stat,layer,encoding);
    if ((id>>16)!=1 || (id&0xffff)>=groups.size()) return false;
    const auto* group=groups[id&0xffff].bytes.data();
    if (group[4]>2) return false;
    // A weighted group is attributable only when selection is deterministic.
    // Multiple viable entries are intentionally omitted rather than guessed.
    const unsigned char* selected=nullptr;
    for (unsigned n=0;n<8;++n) {
        const auto* entry=group+8+n*24;
        if (Read<std::uint32_t>(entry,0)==0xffffffffu || std::all_of(entry,entry+24,[](unsigned char b){return b==0;})) continue;
        if (selected || Read<int>(entry,4)!=Read<int>(entry,8)) return false;
        selected=entry;
    }
    return selected && SpecContributes(Read<std::uint32_t>(selected,0),Read<int>(selected,4),
        Read<int>(selected,12),Read<int>(selected,16),properties,groups,stat,layer,encoding,depth+1);
}
inline bool Contributes(const Row& row,std::span<const Property> properties,int stat,int layer,
                        LayerEncoding encoding={},std::span<const PropertyGroup> groups={}) {
    for (unsigned n=0;n<3;++n) {
        const auto id=static_cast<std::uint32_t>(row.Number(0x24+16*n));
        if (SpecContributes(id,row.Number(0x28+16*n),row.Number(0x2c+16*n),row.Number(0x30+16*n),
                            properties,groups,stat,layer,encoding)) return true;
    }
    return false;
}
struct Rolled { std::uint32_t id{}; Row row; bool prefix{}; unsigned tier{}; const char* source{}; bool uniqueScalar{}; bool unconditionalScalar{true}; unsigned choicePool{},choiceEntry{}; };
inline bool UniqueScalarBounds(const Rolled& source,std::span<const Property> properties,int stat,int layer,int& low,int& high) {
    if (!source.uniqueScalar || !source.source || layer || source.row.Number(0x34)!=-1 || source.row.Number(0x44)!=-1) return false;
    const int id=source.row.Number(0x24);
    if (id<0 || static_cast<std::size_t>(id)>=properties.size()) return false;
    const auto* p=properties[static_cast<std::size_t>(id)].bytes.data();
    if(stat>=21&&stat<=24){
        bool minimum=false,maximum=false;
        for(unsigned slot=0;slot<7;++slot){minimum|=p[0x18+slot]==5;maximum|=p[0x18+slot]==6;}
        if(minimum&&maximum)return false;
    }
    unsigned found=0;
    for (unsigned slot=0;slot<7;++slot) {
        const auto fn=p[0x18+slot];
        if (!fn) continue;
        if ((fn==5 && (stat==21 || stat==23)) || (fn==6 && (stat==22 || stat==24)) ||
            (fn==7 && (stat==17 || stat==18))) {
            ++found; continue;
        }
        if (Read<std::uint16_t>(p,0x20+slot*2)!=stat) continue;
        if (fn!=1 && fn!=3 && fn!=8 && !(fn==2 && stat==16)) return false;
        ++found;
    }
    low=source.row.Number(0x2c); high=source.row.Number(0x30);
    return found==1 && low<high && low>=-1000000 && high<=1000000;
}
inline std::string NamedLabel(const Rolled& affix) {
    std::string result=affix.source?affix.source:(affix.prefix?"[P]":"[S]");
    if (!affix.source) {
        const auto* name=reinterpret_cast<const char*>(affix.row.bytes.data());
        const auto length=strnlen(name,32);
        bool safe=length>0 && length<32;
        for (std::size_t n=0;n<length;++n)
            safe &= static_cast<unsigned char>(name[n])>=32 && name[n]!='[' && name[n]!=']';
        if (safe) result+=" ["+std::string(name,length)+"]";
    }
    if (affix.tier) result+=" [T"+std::to_string(affix.tier)+"]";
    return result;
}
// Only additive scalar properties. More complex functions require a separate
// verified transformation; Contributes alone does not establish numeric bounds.
inline bool ScalarBounds(const Rolled& affix,std::span<const Property> properties,
                         int stat,int layer,int& low,int& high) {
    if (affix.source) return false;
    bool found=false; low=high=0;
    for (unsigned n=0;n<3;++n) {
        const int id=affix.row.Number(0x24+16*n);
        if (id<0 || static_cast<std::size_t>(id)>=properties.size()) continue;
        Row one=affix.row;
        for (unsigned j=0;j<3;++j) if (j!=n) { const int absent=-1; std::memcpy(one.bytes.data()+0x24+16*j,&absent,4); }
        if (!Contributes(one,properties,stat,layer)) continue;
        const auto* p=properties[static_cast<std::size_t>(id)].bytes.data();
        // A paired physical property (functions 5 and 6 together) uses its
        // min/max as endpoint operands, not the scalar roll of either alone.
        if(stat>=21&&stat<=24){
            bool minimum=false,maximum=false;
            for(unsigned slot=0;slot<7;++slot){minimum|=p[0x18+slot]==5;maximum|=p[0x18+slot]==6;}
            if(minimum&&maximum)return false;
        }
        const int param=one.Number(0x28+16*n);
        unsigned matches=0;
        for (unsigned slot=0;slot<7;++slot) {
            const auto fn=p[0x18+slot];
            const int target=Read<std::uint16_t>(p,0x20+slot*2);
            if ((fn==5 && (stat==21 || stat==23)) || (fn==6 && (stat==22 || stat==24)) ||
                (fn==7 && (stat==17 || stat==18)) ||
                (!layer && (fn==1 || fn==3 || fn==4 || fn==8 || (fn==2 && stat==16) || (fn==13 && stat==75 && !p[2+slot])) && target==stat) ||
                (fn==21 && target==stat && layer>=0 && Read<std::uint16_t>(p,0x0a+slot*2)==layer) ||
                (fn==10 && target==stat && param>=0 && param<24 && (param/3*8+param%3)==layer) ||
                ((fn==9 || fn==22 || fn==24) && target==stat && param>=0 && param==layer)) ++matches;
        }
        const int lo=one.Number(0x2c+16*n),hi=one.Number(0x30+16*n);
        if (matches!=1 || found || lo>hi || lo< -1000000 || hi>1000000) return false;
        low=lo; high=hi; found=true;
    }
    return found;
}
template<class Eligible>
inline bool FamilyScalarBounds(const Rolled& rolled,std::span<const Row> family,
                               std::span<const Property> properties,bool rare,
                               unsigned itemVersion,int stat,int layer,
                               int& familyLow,int& familyHigh,Eligible eligible) {
    int low=0,high=0;
    if (!ScalarBounds(rolled,properties,stat,layer,low,high)) return false;
    familyLow=low;
    familyHigh=high;
    for (const auto& row:family) {
        if (!row.Spawnable() || !row.Frequency() || (rare && !row.RareAllowed()) ||
            (itemVersion<100 && row.Version()>=100) || !rolled.row.SameFamily(row,properties) ||
            !eligible(row)) continue;
        Rolled candidate{0,row,rolled.prefix,0,rolled.source};
        int cLow=0,cHigh=0;
        if (!ScalarBounds(candidate,properties,stat,layer,cLow,cHigh)) continue;
        if (cLow<familyLow) familyLow=cLow;
        if (cHigh>familyHigh) familyHigh=cHigh;
    }
    return true;
}
// Resolve the two independently displayed endpoints of an elemental damage
// family. Function 1 rolls the named endpoint within mod min/max; paired
// functions 15/16 take the mod minimum and maximum as fixed damage endpoints.
inline bool DamageEndpointBounds(const Rolled& affix,std::span<const Property> properties,
                                 int minStat,int maxStat,int& minLow,int& minHigh,
                                 int& maxLow,int& maxHigh,bool allowMissing=false) {
    if (affix.source || minStat<0 || maxStat<0) return false;
    minLow=minHigh=maxLow=maxHigh=0;
    bool foundMin=false,foundMax=false;
    const bool physical=(minStat==21 && maxStat==22) || (minStat==23 && maxStat==24);
    for (unsigned n=0;n<3;++n) {
        const int id=affix.row.Number(0x24+16*n);
        if (id<0 || static_cast<std::size_t>(id)>=properties.size()) continue;
        const auto* p=properties[static_cast<std::size_t>(id)].bytes.data();
        const int low=affix.row.Number(0x2c+16*n),high=affix.row.Number(0x30+16*n);
        if (low>high || low< -1000000 || high>1000000) return false;
        for (unsigned slot=0;slot<7;++slot) {
            const auto fn=p[0x18+slot];
            if (!fn) continue;
            if (physical && (fn==5 || fn==6)) {
                if (p[2+slot]) return false;
                if (fn==5) {
                    if (foundMin) return false;
                    minLow=low; minHigh=high; foundMin=true;
                } else {
                    if (foundMax) return false;
                    maxLow=low; maxHigh=high; foundMax=true;
                }
                continue;
            }
            const int rawTarget=Read<std::uint16_t>(p,0x20+slot*2);
            const int target=(minStat==23 && maxStat==24 && ((fn==15 && rawTarget==21) || (fn==16 && rawTarget==22)))
                ? (rawTarget+2) : rawTarget;
            if(fn && (target==minStat || target==maxStat) &&
               ((target==minStat && fn!=1 && fn!=15) || (target==maxStat && fn!=1 && fn!=16) || p[2+slot]))return false;
            if (target==minStat && (fn==1 || fn==15)) {
                if (foundMin) return false;
                minLow=minHigh=(fn==15?low:0);
                if (fn==1) { minLow=low; minHigh=high; }
                foundMin=true;
            } else if (target==maxStat && (fn==1 || fn==16)) {
                if (foundMax) return false;
                maxLow=maxHigh=(fn==16?high:0);
                if (fn==1) { maxLow=low; maxHigh=high; }
                foundMax=true;
            }
        }
    }
    return allowMissing?(foundMin || foundMax):(foundMin && foundMax);
}
// Require one source candidate; matching a skill alone cannot disambiguate
// multiple calculated-level sources. Ordinary numeric matching stays strict.
inline std::string ChargeSourceLabel(std::span<const Rolled> rolled,std::span<const Property> properties,
                                     int layer,LayerEncoding encoding,std::span<const PropertyGroup> groups={}) {
    encoding.chargeSourcesOnly=true;
    const Rolled* found=nullptr;
    for(const auto& affix:rolled){
        if(!affix.unconditionalScalar || !Contributes(affix.row,properties,204,layer,encoding,groups))continue;
        if(found)return {};found=&affix;
    }
    return found?NamedLabel(*found):std::string{};
}
inline std::string Label(std::span<const Rolled> rolled,std::span<const Property> properties,int stat,int layer,
                         LayerEncoding encoding={},std::span<const PropertyGroup> groups={}) {
    std::string result;
    for (const auto& affix:rolled) {
        if (!Contributes(affix.row,properties,stat,layer,encoding,groups)) continue;
        if (affix.source && result.find(affix.source)!=std::string::npos) continue;
        if (!result.empty()) result+=' ';
        result+=NamedLabel(affix);
    }
    return result;
}
// A combined display line is labeled only when every member has verified
// contributors. Emit each contributing affix once across the entire group.
inline std::string GroupLabel(std::span<const Rolled> rolled,std::span<const Property> properties,
                              std::span<const int> stats,int layer,LayerEncoding encoding={},
                              std::span<const PropertyGroup> groups={}) {
    if (stats.empty()) return {};
    for (int stat:stats) if (Label(rolled,properties,stat,layer,encoding,groups).empty()) return {};
    std::string result;
    for (const auto& affix:rolled) {
        bool contributes=false;
        for (int stat:stats) contributes |= Contributes(affix.row,properties,stat,layer,encoding,groups);
        if (!contributes) continue;
        if (affix.source && result.find(affix.source)!=std::string::npos) continue;
        if (!result.empty()) result+=' ';
        result+=NamedLabel(affix);
    }
    return result;
}
}
