#include <windows.h>
#include <xinput.h>
#include <intrin.h>
#include <bcrypt.h>
#include <D2RLPlugin/context.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include "policy.h"
#include "provider_profile.h"
#include "range_text.h"
#include "affixes.h"
#include "affix_profile.h"
#include "source_profile.h"
#include "special_profile.h"
#include "unique_range_profile.h"
#include <vector>

namespace {
using KeyFn = std::uint64_t(__fastcall*)();
using PadFn = std::uint64_t(__fastcall*)(void*, unsigned, unsigned);
using PanelFn = bool(__fastcall*)(int);
using XInputFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
KeyFn originalKey{};
PadFn originalPad{};
PanelFn testPanel{};
XInputFn readPad{};
void** keySlot{};
void** padSlot{};
std::uintptr_t keyReturn{}, padReturn{};
std::atomic<bool> active{false};
std::atomic<bool> loaded{false};
using PropertiesFn=std::uint64_t(__fastcall*)(void*,char*,int,int,int,int,int,int,const void*,void*);
PropertiesFn originalProperties{};
void** propertiesSlot{};
std::uintptr_t propertiesReturn{};
const unsigned char* coreBase{};
using SingleFn=bool(__fastcall*)(void*,const void*,int,int,bool,char*,int);
using TablesFn=const unsigned char*(__fastcall*)(unsigned char);
using EligibleFn=int(__fastcall*)(void*,const void*);
using RangeHelperFn=int(__fastcall*)(void*,void*,int,int,int,int,char*,int);
RangeHelperFn originalRangeHelper{};
void** rangeHelperSlot{};
std::uintptr_t rangeHelperReturn{}, rangeHelperActualReturn{};
using SpecialFn=int(__fastcall*)(void*,int,char*,std::size_t,unsigned char);
SpecialFn originalSpecial{};
void** specialSlot{};
std::uintptr_t specialReturn{};
SingleFn originalSingle{};
TablesFn getTables{};
EligibleFn eligibleAffix{};
void** singleSlot{};
std::uintptr_t singleReturn{};
struct CapturedLine { std::string key; int stat{},layer{}; bool grouped{}; unsigned group{}; };
thread_local std::vector<CapturedLine>* captureLines{};
thread_local bool rangeRequested{};
thread_local unsigned rangePad=XUSER_MAX_COUNT;
thread_local bool rendering{};
wchar_t diagnosticPath[MAX_PATH]{};
std::atomic<unsigned> diagnostics{0};
bool Readable(const void*,std::size_t) noexcept;
bool AddUnobservedCompositeIdentities(std::vector<CapturedLine>& lines,std::string_view actual,
    std::span<const Affixes::Rolled> rolled,std::span<const Affixes::Property> properties,
    Affixes::LayerEncoding encoding,std::span<const Affixes::PropertyGroup> groups,
    const unsigned char* itemStats,std::size_t itemStatCount) {
    std::vector<std::string> candidates;
    for (const auto text:RangeText::Lines(actual)) {
        const auto key=RangeText::Analyze(text).key;
        if (key.empty()) continue;
        bool observed=false;
        for (const auto& line:lines) if (line.key==key) { observed=true; break; }
        if (!observed) candidates.push_back(key);
    }
    if (candidates.empty()) return false;
    constexpr std::array families{
        std::array{21,22,23,24}, std::array{48,49,-1,-1},
        std::array{50,51,-1,-1}, std::array{52,53,-1,-1},
        std::array{54,55,56,-1}, std::array{57,58,59,-1}};
    std::vector<std::string> observedSources;
    for (const auto& line:lines) {
        bool composite=false;
        for (const auto& family:families)
            for (const int member:family) composite |= member>=0 && line.stat==member;
        if (!composite) continue;
        const auto label=Affixes::Label(rolled,properties,line.stat,line.layer,encoding,groups);
        if (!label.empty() && std::find(observedSources.begin(),observedSources.end(),label)==observedSources.end())
            observedSources.push_back(label);
    }
    struct FamilySource { int stat; std::uint16_t priority; std::string label; };
    std::vector<FamilySource> sources;
    for (const auto& family:families) {
        bool observed=false;
        for (const auto& line:lines)
            for (const int member:family) observed |= member>=0 && line.stat==member;
        if (observed) continue;
        std::string familyLabel;
        int familyStat=-1;
        for (const int stat:family) {
            if (stat<0) continue;
            const auto label=Affixes::Label(rolled,properties,stat,0,encoding,groups);
            if (label.empty()) continue;
            if (std::find(observedSources.begin(),observedSources.end(),label)!=observedSources.end()) continue;
            if (familyLabel.empty()) { familyLabel=label; familyStat=stat; }
            else if (familyLabel!=label) return false;
        }
        if (familyStat<0) continue;
        if (!itemStats || familyStat>=static_cast<int>(itemStatCount)) return false;
        sources.push_back({familyStat,Affixes::Read<std::uint16_t>(
            itemStats+static_cast<std::size_t>(familyStat)*0x144,0x30),std::move(familyLabel)});
    }
    if (sources.size()!=candidates.size()) return false;
    std::sort(sources.begin(),sources.end(),[](const FamilySource& a,const FamilySource& b) {
        return a.priority<b.priority;
    });
    for (std::size_t n=1;n<sources.size();++n)
        if (sources[n-1].priority==sources[n].priority) return false;
    // Core appends lower display priorities first; D2R lays those buffer lines
    // out bottom-to-top. Exact cardinality plus distinct priorities provides a
    // locale-independent map.
    for (std::size_t n=0;n<candidates.size();++n)
        lines.push_back({std::move(candidates[n]),sources[n].stat,0,false,0});
    return true;
}
__declspec(noinline) bool __fastcall SingleAdapter(void* unit,const void* row,int value,int layer,bool grouped,char* output,int mode) noexcept {
    const bool scoped=reinterpret_cast<std::uintptr_t>(_ReturnAddress())==singleReturn && captureLines;
    const bool result=originalSingle(unit,row,value,layer,grouped,output,mode);
    if (scoped && result && Readable(row,0x44) && Readable(output,256)) {
        try {
            const auto n=strnlen(output,256);
            if (n && n<256 && captureLines->size()<256)
                captureLines->push_back({RangeText::Analyze({output,n}).key,
                    Affixes::Read<std::uint16_t>(static_cast<const unsigned char*>(row),0),layer,grouped,
                    Affixes::Read<std::uint16_t>(static_cast<const unsigned char*>(row),0x3a)});
        } catch (...) { /* Observation failure cannot change native output. */ }
    }
    return result;
}
// Core formats paired damage families and some equal-bound actual values
// through this helper, bypassing SingleAdapter. Capture identity generically;
// active-table source reconciliation below decides whether it is attributable.
void CaptureRangeIdentity(int stat,int layer,const char* output) noexcept {
    if (!captureLines || !Readable(output,256)) return;
    try {
        const auto n=strnlen(output,256);
        if (n && n<256 && captureLines->size()<256)
            captureLines->push_back({RangeText::Analyze({output,n}).key,stat,layer,false,0});
    } catch (...) { }
}
__declspec(noinline) int __fastcall RangeHelperAdapter(void* unit,void* stats,int stat,int layer,
    int low,int high,char* output,int mode) noexcept {
    const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const bool scoped=captureLines && (caller==rangeHelperReturn || caller==rangeHelperActualReturn);
    const int result=originalRangeHelper(unit,stats,stat,layer,low,high,output,mode);
    if (scoped && result) CaptureRangeIdentity(stat,layer,output);
    return result;
}
void CaptureSpecialDamage(int stat,std::string_view appended) noexcept {
    if (!captureLines || (stat!=17 && stat!=18)) return;
    while (!appended.empty() && (appended.front()=='\n' || appended.front()=='\r')) appended.remove_prefix(1);
    while (!appended.empty() && (appended.back()=='\n' || appended.back()=='\r')) appended.remove_suffix(1);
    if (appended.empty() || appended.size()>=256 || appended.find('\n')!=appended.npos || appended.find('\r')!=appended.npos) return;
    try {
        if (captureLines->size()<256)
            captureLines->push_back({RangeText::Analyze(appended).key,stat,0,false,0});
    } catch (...) { }
}
int ObserveSpecial(bool callerMatches,void* context,int stat,char* output,std::size_t capacity,unsigned char flags) noexcept {
    const bool scoped=callerMatches && captureLines &&
        (stat==17 || stat==18) && capacity>0 && capacity<=65536 && Readable(output,capacity);
    const auto before=scoped?strnlen(output,capacity):capacity;
    const int result=originalSpecial(context,stat,output,capacity,flags);
    if (scoped && result && before<capacity) {
        const auto after=strnlen(output,capacity);
        if (after>before && after<capacity) CaptureSpecialDamage(stat,{output+before,after-before});
    }
    return result;
}
__declspec(noinline) int __fastcall SpecialAdapter(void* context,int stat,char* output,
                                                  std::size_t capacity,unsigned char flags) noexcept {
    return ObserveSpecial(reinterpret_cast<std::uintptr_t>(_ReturnAddress())==specialReturn,context,stat,output,capacity,flags);
}
// Read-only property definition expansion. Group IDs use high WORD 1, not
// Properties-table indices. Bounds and depth prevent malformed/cyclic groups.
void AddUniqueSource(const unsigned char* spec,const unsigned char* groups,std::size_t groupCount,
                     std::size_t propertyCount,std::vector<Affixes::Rolled>& out,unsigned depth=0,bool scalarAllowed=true) {
    if (depth>4 || out.size()>=128) return;
    const auto id=Affixes::Read<std::uint32_t>(spec,0);
    if (id==0xffffffffu) return;
    if ((id>>16)==1) {
        const auto index=id&0xffff;
        if (index>=groupCount) return;
        const auto* row=groups+index*0xc8;
        if (row[4]>2) return;
        for (unsigned n=0;n<8;++n) {
            const auto* entry=row+8+n*24;
            if (Affixes::Read<int>(entry,20)<=0 || Affixes::Read<int>(entry,4)!=Affixes::Read<int>(entry,8)) continue;
            unsigned char child[16]{};
            std::memcpy(child,entry,4); std::memcpy(child+4,entry+4,4);
            std::memcpy(child+8,entry+12,8);
            AddUniqueSource(child,groups,groupCount,propertyCount,out,depth+1,
                scalarAllowed && row[4]==2 && Affixes::Read<int>(spec,8)==1 && Affixes::Read<int>(spec,12)==1);
        }
    } else if ((id>>16)==0 && id<propertyCount) {
        Affixes::Row row{};
        std::memcpy(row.bytes.data()+0x24,spec,16);
        const int absent=-1;
        std::memcpy(row.bytes.data()+0x34,&absent,4);
        std::memcpy(row.bytes.data()+0x44,&absent,4);
        out.push_back({id,row,false,0,"[Unique]",scalarAllowed});
    }
}
std::string UniqueRange(void* source,const CapturedLine& line,const std::vector<Affixes::Rolled>& sources,
                        std::span<const Affixes::Property> properties,const unsigned char* tables,
                        std::string_view actual) {
    if (line.grouped || line.layer || !originalSingle || actual.empty()) return {};
    const Affixes::Rolled* match=nullptr; unsigned matches=0;
    for (const auto& candidate:sources) if (Affixes::Contributes(candidate.row,properties,line.stat,line.layer)) { match=&candidate; ++matches; }
    int low{},high{};
    if (matches!=1 || !match || !Affixes::UniqueScalarBounds(*match,properties,line.stat,line.layer,low,high)) return {};
    const auto* stats=Affixes::Read<const unsigned char*>(tables,0x1258);
    const auto count=Affixes::Read<std::uint64_t>(tables,0x1260);
    if (line.stat<0 || count>4096 || static_cast<std::uint64_t>(line.stat)>=count || !Readable(stats,count*0x144)) return {};
    const auto* row=stats+static_cast<std::size_t>(line.stat)*0x144;
    // Reviewed descfunc19 formats the supplied display-unit integer through
    // its native template (including inverted signs such as enemy resistance).
    if (row[0x32]!=19) return {};
    char lower[256]{},upper[256]{};
    if (!originalSingle(source,row,low,0,false,lower,1) || !originalSingle(source,row,high,0,false,upper,1)) return {};
    const auto ln=strnlen(lower,256),hn=strnlen(upper,256);
    if (ln==256 || hn==256) return {};
    std::string found; unsigned actualMatches=0;
    for (auto text:RangeText::Lines(actual)) if (RangeText::Analyze(text).key==line.key) {
        ++actualMatches; found=RangeText::EndpointRange(text,{lower,ln},{upper,hn});
    }
    return actualMatches==1?found:std::string{};
}
std::vector<RangeText::Label> AffixLabels(void* source,const std::vector<CapturedLine>& lines,std::string_view actual={}) noexcept {
    try {
        if (!getTables || !eligibleAffix || (lines.empty() && actual.empty()) || !Readable(source,0x1be)) return {};
        const auto* unit=static_cast<const unsigned char*>(source);
        if (Affixes::Read<unsigned>(unit,0)!=4 || unit[0x1bd]>=4) return {};
        const auto* data=Affixes::Read<const unsigned char*>(unit,0x10);
        if (!data || reinterpret_cast<std::uintptr_t>(data)<28 || !Readable(data-28,0x64)) return {};
        const int quality=Affixes::Read<int>(data,0);
        if (quality<2 || quality>9) return {};
        const auto* tables=getTables(unit[0x1bd]);
        if (!Readable(tables,0x1618)) return {};
        const auto* rows=Affixes::Read<const Affixes::Row*>(tables,0x15e8);
        const auto rowCount=Affixes::Read<std::uint64_t>(tables,0x15f0);
        const auto start= reinterpret_cast<std::uintptr_t>(rows);
        const auto suffixStart=Affixes::Read<std::uintptr_t>(tables,0x1600);
        const auto prefixStart=Affixes::Read<std::uintptr_t>(tables,0x1608);
        const auto autoStart=Affixes::Read<std::uintptr_t>(tables,0x1610);
        if (!rowCount || rowCount>32768 || suffixStart!=start || prefixStart<start || autoStart<prefixStart ||
            autoStart-start>rowCount*sizeof(Affixes::Row) || (prefixStart-start)%sizeof(Affixes::Row) ||
            (autoStart-start)%sizeof(Affixes::Row) || !Readable(rows,static_cast<std::size_t>(rowCount)*sizeof(Affixes::Row))) return {};
        const auto prefixIndex=(prefixStart-start)/sizeof(Affixes::Row),autoIndex=(autoStart-start)/sizeof(Affixes::Row);
        const auto* props=Affixes::Read<const Affixes::Property*>(tables,0x240);
        const auto propCount=Affixes::Read<std::uint64_t>(tables,0x248);
        if (!propCount || propCount>32768 || !Readable(props,static_cast<std::size_t>(propCount)*sizeof(Affixes::Property))) return {};
        const std::span<const Affixes::Property> properties(props,static_cast<std::size_t>(propCount));
        std::span<const Affixes::PropertyGroup> groups;
        const auto* groupRows=Affixes::Read<const Affixes::PropertyGroup*>(tables,0x258);
        const auto groupCount=Affixes::Read<std::uint64_t>(tables,0x260);
        if (groupCount && groupCount<32768 &&
            Readable(groupRows,static_cast<std::size_t>(groupCount)*sizeof(Affixes::PropertyGroup)))
            groups={groupRows,static_cast<std::size_t>(groupCount)};
        std::vector<Affixes::Rolled> rolled;
        for (unsigned n=0;n<6 && (quality==4 || quality==6 || quality==8);++n) {
            const auto id=Affixes::Read<std::uint32_t>(data-24,n*4);
            if (!id || id>autoIndex) continue;
            bool duplicate=false; for (const auto& a:rolled) if (a.id==id) duplicate=true;
            if (duplicate) continue;
            const auto index=static_cast<std::size_t>(id-1);
            const bool prefix=index>=prefixIndex;
            const auto first=prefix?prefixIndex:0, last=prefix?autoIndex:prefixIndex;
            const auto& row=rows[index];
            const auto tier=Affixes::Tier(row,{rows+first,last-first},quality!=4,
                Affixes::Read<std::uint16_t>(data,0x40),[&](const Affixes::Row& candidate) { return eligibleAffix(source,&candidate)!=0; });
            rolled.push_back({id,row,prefix,tier});
        }
        const auto autoId=Affixes::Read<std::uint32_t>(data-28,0);
        if (autoId>autoIndex && autoId<=rowCount) {
            const auto& row=rows[autoId-1];
            // Automagic is an inherent base-item modifier, not a rolled prefix
            // or suffix. Keep its internal table term out of the player UI.
            rolled.push_back({autoId,row,false,0,"[Base]"});
        }
        if (quality==7) {
            const int record=Affixes::Read<int>(data,0x34);
            const auto* unique=Affixes::Read<const unsigned char*>(tables,0x13c8);
            const auto count=Affixes::Read<std::uint64_t>(tables,0x13d0);
            const auto* uniqueGroups=reinterpret_cast<const unsigned char*>(groupRows);
            if (record>=0 && count<32768 && static_cast<std::uint64_t>(record)<count &&
                Readable(unique,count*0x15c) && !groups.empty()) {
                const auto* recordRow=unique+record*0x15c;
                // Flag8 suppresses Core's range clone for Renewed Sunders.
                // It does not erase the original item's unique provenance.
                for (unsigned n=0;n<12;++n)
                    AddUniqueSource(recordRow+0x98+n*16,uniqueGroups,groups.size(),properties.size(),rolled);
            }
        }
        const Affixes::LayerEncoding encoding{Affixes::Read<unsigned>(tables,0x14d0),
                                              Affixes::Read<unsigned>(tables,0x14d4)};
        const auto* itemStats=Affixes::Read<const unsigned char*>(tables,0x1258);
        const auto itemStatCount=Affixes::Read<std::uint64_t>(tables,0x1260);
        const bool itemStatsValid=itemStatCount && itemStatCount<=4096 &&
            Readable(itemStats,static_cast<std::size_t>(itemStatCount)*0x144);
        std::vector<CapturedLine> observed=lines;
        // Composite damage lines can bypass both native identity observers.
        // Require an exact line/family count; loaded native display priorities
        // provide the locale-independent mapping for different source labels.
        if (itemStatsValid) AddUnobservedCompositeIdentities(observed,actual,rolled,properties,
            encoding,groups,itemStats,static_cast<std::size_t>(itemStatCount));
        std::vector<RangeText::Label> labels;
        for (const auto& line:observed) {
            std::string text;
            if (line.grouped) {
                if (!line.group || !itemStatsValid) continue;
                std::vector<int> members;
                for (std::size_t n=0;n<itemStatCount;++n) {
                    const auto* row=itemStats+n*0x144;
                    if (Affixes::Read<std::uint16_t>(row,0x3a)==line.group)
                        members.push_back(Affixes::Read<std::uint16_t>(row,0));
                }
                text=Affixes::GroupLabel(rolled,properties,members,line.layer,encoding,groups);
            } else text=Affixes::Label(rolled,properties,line.stat,line.layer,encoding,groups);
            if (!text.empty()) {
                RangeText::Label label{line.key,std::move(text)};
                bool complete=!line.grouped && (quality==4 || quality==6);
                const bool compositeDamage=!line.layer &&
                    ((line.stat>=21 && line.stat<=24) || (line.stat>=48 && line.stat<=59));
                int endpointMin=-1,endpointMax=-1;
                if (line.stat>=48 && line.stat<=55) {
                    endpointMin=line.stat&~1;
                    endpointMax=endpointMin+1;
                }
                bool completePaired=endpointMin>=0 && (quality==4 || quality==6);
                for (const auto& affix:rolled) {
                    if (!Affixes::Contributes(affix.row,properties,line.stat,line.layer,encoding,groups)) continue;
                    if (compositeDamage) {
                        const auto named=Affixes::NamedLabel(affix);
                        if (std::find(label.unknownSources.begin(),label.unknownSources.end(),named)==label.unknownSources.end())
                            label.unknownSources.push_back(named);
                    }
                    int low{},high{};
                    if (!Affixes::ScalarBounds(affix,properties,line.stat,line.layer,low,high)) complete=false;
                    else label.parts.push_back({low,high,Affixes::NamedLabel(affix)});
                    int minLow{},minHigh{},maxLow{},maxHigh{};
                    if (!completePaired || !Affixes::DamageEndpointBounds(affix,properties,
                            endpointMin,endpointMax,minLow,minHigh,maxLow,maxHigh)) completePaired=false;
                    else label.pairedParts.push_back({minLow,minHigh,maxLow,maxHigh,
                        Affixes::NamedLabel(affix)});
                }
                if (!complete) label.parts.clear();
                if (!completePaired || label.pairedParts.size()<2) label.pairedParts.clear();
                label.allowPartialNativeRange=complete && label.parts.size()>1 &&
                    (line.stat==17 || line.stat==18) && line.layer==0;
                if (quality==7) label.fallbackRange=UniqueRange(source,line,rolled,properties,tables,actual);
                bool duplicate=false;
                for (const auto& previous:labels) if (previous.key==label.key) {
                    // The same line can be observed in both actual/range passes,
                    // including the two physical damage stats17/18.
                    if (previous.text==label.text) duplicate=true;
                }
                if (!duplicate) labels.push_back(std::move(label));
            }
        }
        return labels;
    } catch (...) { return {}; }
}
void* SourceFromOverlay(const void* overlay,void* item) noexcept {
    if (!Readable(overlay,16)) return nullptr;
    void* source{}; void* clone{};
    std::memcpy(&source,overlay,8);
    std::memcpy(&clone,static_cast<const unsigned char*>(overlay)+8,8);
    if (!source) return nullptr;
    // The provider installs an overlay even when no range clone was created.
    if (!clone && source==item) return Readable(source,0x90)?source:nullptr;
    if (source==clone || clone!=item) return nullptr;
    return Readable(source,0x90)?source:nullptr;
}
void* SourceItem(void* item) noexcept {
    // Mirrors Core's own FormatItemPropertiesWithTooltipOverlay TLS lookup.
    if (!coreBase || !Readable(coreBase+0x7df224,4)) return nullptr;
    std::uint32_t index{}; std::memcpy(&index,coreBase+0x7df224,4);
    if (index>=1088) return nullptr;
    auto slots=reinterpret_cast<void**>(__readgsqword(0x58));
    if (!Readable(slots+index,8)) return nullptr;
    const auto block=static_cast<const unsigned char*>(slots[index]);
    if (!block || !Readable(block+0x1840,8)) return nullptr;
    void* overlay{}; std::memcpy(&overlay,block+0x1840,8);
    return overlay?SourceFromOverlay(overlay,item):item;
}
void TraceProperties(unsigned annotated,unsigned unmatched,const char* actual,const char* result,const char* ranged="",
                     unsigned labeled=0,unsigned captured=0,const std::vector<CapturedLine>* identities=nullptr,
                     const std::vector<RangeText::Label>* sourceLabels=nullptr) noexcept {
    // Production builds keep formatter dumps disabled. A bounded, explicit
    // coverage build can enable them for deterministic Item Spawner audits.
#ifdef ITEM_ROLL_RANGES_COVERAGE_DIAGNOSTICS
    constexpr bool formatDiagnostics = true;
#else
    constexpr bool formatDiagnostics = false;
#endif
    if constexpr (!formatDiagnostics) return;
    if (!diagnosticPath[0]) return;
    // Comparison tooltips alternate every frame; deduplicate the complete set.
    thread_local std::vector<std::string> seen;
    for (const auto& previous:seen) if (previous==actual) return;
    if (seen.size()>=32 || diagnostics.fetch_add(1)>=32) return;
    try { seen.emplace_back(actual); } catch (...) { return; }
    char message[14000]{};
    const int n=std::snprintf(message,sizeof(message),
        "v1.3.1+rev.13 properties annotated=%u unmatched=%u labeled=%u captured=%u\r\nACTUAL: %.4095s\r\nRANGED: %.4095s\r\nRESULT: %.4095s\r\n",annotated,unmatched,labeled,captured,actual,ranged,result);
    if (n<=0 || n>=static_cast<int>(sizeof(message))) return;
    HANDLE file=CreateFileW(diagnosticPath,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (file==INVALID_HANDLE_VALUE) return;
    DWORD written{}; WriteFile(file,message,static_cast<DWORD>(n),&written,nullptr); CloseHandle(file);
    if (identities) {
        HANDLE details=CreateFileW(diagnosticPath,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if (details!=INVALID_HANDLE_VALUE) {
            for (const auto& line:*identities) {
                char entry[256]{};
                const int length=std::snprintf(entry,sizeof(entry),"IDENTITY stat=%d layer=%d grouped=%u key=%.160s\r\n",line.stat,line.layer,line.grouped?1u:0u,line.key.c_str());
                if (length>0 && length<static_cast<int>(sizeof(entry))) WriteFile(details,entry,static_cast<DWORD>(length),&written,nullptr);
            }
            CloseHandle(details);
        }
    }
    if (sourceLabels) {
        HANDLE details=CreateFileW(diagnosticPath,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if (details!=INVALID_HANDLE_VALUE) {
            for (const auto& label:*sourceLabels) {
                char entry[512]{};
                const int length=std::snprintf(entry,sizeof(entry),
                    "LABEL sources=%zu parts=%zu key=%.160s text=%.220s\r\n",
                    label.unknownSources.size(),label.parts.size(),label.key.c_str(),label.text.c_str());
                if (length>0 && length<static_cast<int>(sizeof(entry))) WriteFile(details,entry,static_cast<DWORD>(length),&written,nullptr);
            }
            CloseHandle(details);
        }
    }
}

bool Foreground() noexcept {
    DWORD pid{};
    auto window = GetForegroundWindow();
    return window && GetWindowThreadProcessId(window, &pid) && pid == GetCurrentProcessId();
}
bool Allowed() noexcept {
    if (!active.load(std::memory_order_acquire) || !Foreground()) return false;
    return RollRanges::Allowed(true, {testPanel(0x1), testPanel(0x18), testPanel(0x19), testPanel(0xb)});
}
bool Held() noexcept {
    if (!rangeRequested || !Allowed()) return false;
    if (rangePad==XUSER_MAX_COUNT) return (GetAsyncKeyState(VK_CONTROL)&0x8000)!=0;
    XINPUT_STATE state{};
    return readPad && readPad(rangePad,&state)==ERROR_SUCCESS &&
        (state.Gamepad.wButtons&XINPUT_GAMEPAD_RIGHT_SHOULDER)!=0;
}
std::uint64_t RenderProperties(void* source,void* item,char* output,int capacity,int mode,
    int state1,int state2,int flags,int extra,const void* definitions,void* count) noexcept {
    // Render the actual item through the same native formatter, with no range
    // definitions. The second pass keeps Core's range calculations untouched.
    struct RenderingGuard { RenderingGuard(){rendering=true;} ~RenderingGuard(){rendering=false;} } guard;
    try {
        const auto cap=static_cast<std::size_t>(capacity);
        const auto initial=strnlen(output,cap);
        if (initial==cap) return originalProperties(item,output,capacity,mode,state1,state2,flags,extra,definitions,count);
        std::vector<char> actual(cap,0), ranged(cap,0);
        std::memcpy(actual.data(),output,initial+1);
        std::memcpy(ranged.data(),output,initial+1);
        struct OptionalCount { std::uint64_t value{}; bool present{}; unsigned char pad[7]{}; } none;
        std::vector<CapturedLine> captured;
        std::uint64_t actualResult{};
        {
            struct CaptureGuard {
                std::vector<CapturedLine>* previous;
                explicit CaptureGuard(std::vector<CapturedLine>& lines):previous(captureLines) { captureLines=&lines; }
                ~CaptureGuard() { captureLines=previous; }
            } capture(captured);
            actualResult=originalProperties(source,actual.data(),capacity,mode,state1,state2,flags,extra,
                source==item?definitions:nullptr,source==item?count:&none);
        }
        // No clone means the native actual text is already the complete result.
        // Preserve its arguments/return and avoid a duplicate formatter pass.
        if (source==item) ranged=actual;
        std::uint64_t result=actualResult;
        if (source!=item) {
            // Some native actual-value paths bypass Core's property observer.
            // Capture identities from the range pass too; use only the source
            // item for affix metadata and the actual pass for displayed values.
            struct RangeCaptureGuard {
                std::vector<CapturedLine>* previous;
                explicit RangeCaptureGuard(std::vector<CapturedLine>& lines):previous(captureLines) { captureLines=&lines; }
                ~RangeCaptureGuard() { captureLines=previous; }
            } capture(captured);
            result=originalProperties(item,ranged.data(),capacity,mode,state1,state2,flags,extra,definitions,count);
        }
        const auto actualLength=strnlen(actual.data(),cap), rangedLength=strnlen(ranged.data(),cap);
        if (actualLength==cap || rangedLength==cap) {
            std::memcpy(output,actualLength<cap?actual.data():ranged.data(),cap); output[cap-1]=0; return result;
        }
        const auto propertyActual=std::string_view(actual.data()+initial,actualLength-initial);
        const auto sourceLabels=AffixLabels(source,captured,propertyActual);
        auto merged=RangeText::Merge({actual.data(),actualLength},{ranged.data(),rangedLength},cap,sourceLabels);
        std::memcpy(output,merged.text.c_str(),merged.text.size()+1);
        TraceProperties(merged.annotated,merged.unmatched,actual.data(),output,ranged.data(),merged.labeled,
            static_cast<unsigned>(captured.size()),&captured,&sourceLabels);
        return result;
    } catch (...) {
        return originalProperties(item,output,capacity,mode,state1,state2,flags,extra,definitions,count);
    }
}
__declspec(noinline) std::uint64_t __fastcall PropertiesAdapter(void* item,char* output,int capacity,int mode,
    int state1,int state2,int flags,int extra,const void* definitions,void* count) noexcept {
    const bool scoped=reinterpret_cast<std::uintptr_t>(_ReturnAddress())==propertiesReturn && !rendering && Held();
    void* source=scoped?SourceItem(item):nullptr;
    if (scoped && !source) TraceProperties(0,0,"Overlay source unavailable; native fallback.","");
    const auto* sourceBytes=static_cast<const unsigned char*>(source);
    const auto* itemData=Readable(source,0x18)?Affixes::Read<const unsigned char*>(sourceBytes,0x10):nullptr;
    const bool identified=Readable(source,4) && Affixes::Read<unsigned>(sourceBytes,0)==4 &&
        Readable(itemData,0x1c) && (Affixes::Read<unsigned>(itemData,0x18)&0x10)!=0;
    if (!identified || !output || capacity<=1 || capacity>65536 || !Readable(output,static_cast<std::size_t>(capacity)))
        return originalProperties(item,output,capacity,mode,state1,state2,flags,extra,definitions,count);
    return RenderProperties(source,item,output,capacity,mode,state1,state2,flags,extra,definitions,count);
}
__declspec(noinline) std::uint64_t __fastcall KeyAdapter() noexcept {
    const bool tooltip = reinterpret_cast<std::uintptr_t>(_ReturnAddress()) == keyReturn;
    const auto result = originalKey();
    if (!tooltip || !active.load(std::memory_order_acquire)) return result;
    rangePad=XUSER_MAX_COUNT;
    rangeRequested=Allowed() && (GetAsyncKeyState(VK_CONTROL)&0x8000)!=0;
    return RollRanges::Keyboard(result,true,rangeRequested,rangeRequested);
}
__declspec(noinline) std::uint64_t __fastcall PadAdapter(void* input, unsigned index, unsigned mask) noexcept {
    const bool tooltip = reinterpret_cast<std::uintptr_t>(_ReturnAddress()) == padReturn;
    if (!tooltip || !active.load(std::memory_order_acquire)) return originalPad(input, index, mask);
    XINPUT_STATE state{};
    const bool allowed = Allowed();
    const bool connected = allowed && index < XUSER_MAX_COUNT && readPad && readPad(index, &state) == ERROR_SUCCESS;
    rangePad=index;
    rangeRequested=allowed && connected && (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)!=0;
    return static_cast<std::uint64_t>(rangeRequested);
}
bool Executable(const void* address) noexcept {
    MEMORY_BASIC_INFORMATION info{};
    if (!address || !VirtualQuery(address, &info, sizeof(info)) || info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD)) return false;
    const auto protection = info.Protect & 0xff;
    return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
        protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}
bool Readable(const void* address, std::size_t length) noexcept {
    MEMORY_BASIC_INFORMATION info{};
    if (!address || !VirtualQuery(address, &info, sizeof(info)) || info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
    const auto offset = reinterpret_cast<std::uintptr_t>(address) - reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    return offset <= info.RegionSize && length <= info.RegionSize - offset;
}
bool Match(const unsigned char* base, std::size_t rva, const unsigned char* expected, std::size_t size) noexcept {
    return Readable(base+rva, size) && std::memcmp(base+rva, expected, size) == 0;
}
bool VerifyProviderFile(HMODULE module) noexcept {
    // Same exact-provider hash already used by Controller QOL's native reader.
    wchar_t path[32768]{};
    const DWORD length = GetModuleFileNameW(module, path, 32768);
    if (!length || length >= 32768) return false;
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    unsigned char digest[32]{}, buffer[65536];
    bool ok = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0;
    if (ok) ok = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) >= 0;
    while (ok) {
        DWORD received{};
        if (!ReadFile(file, buffer, sizeof(buffer), &received, nullptr)) { ok = false; break; }
        if (!received) break;
        ok = BCryptHashData(hash, buffer, received, 0) >= 0;
    }
    if (ok) ok = BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0;
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    constexpr unsigned char expected[] = {0x2a,0x86,0x8d,0x01,0x3d,0x2e,0x08,0x30,0xbd,0x2d,0x9e,0x04,0xb9,0x18,0xb1,0x9e,0x46,0xa7,0x3c,0xf7,0x26,0xc8,0x33,0xe7,0x0d,0x08,0x9b,0x94,0x8f,0xde,0xb5,0xa2};
    return ok && std::memcmp(digest, expected, sizeof(expected)) == 0;
}
bool Exchange(void** slot, void* expected, void* replacement) noexcept {
    // Aligned data pointers only; no executable bytes or game input state are patched.
    DWORD old{};
    if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &old)) return false;
    auto found = InterlockedCompareExchangePointer(slot, replacement, expected);
    DWORD ignored{};
    const bool restored = VirtualProtect(slot, sizeof(void*), old, &ignored) != FALSE;
    // A successful exchange stays valid even if restoring protection fails. The
    // caller performs ownership-checked rollback; the module is already pinned.
    return found == expected && restored;
}
void Restore() noexcept {
    active.store(false, std::memory_order_release);
    if (specialSlot) Exchange(specialSlot,reinterpret_cast<void*>(&SpecialAdapter),reinterpret_cast<void*>(originalSpecial));
    if (rangeHelperSlot) Exchange(rangeHelperSlot,reinterpret_cast<void*>(&RangeHelperAdapter),reinterpret_cast<void*>(originalRangeHelper));
    if (singleSlot) Exchange(singleSlot,reinterpret_cast<void*>(&SingleAdapter),reinterpret_cast<void*>(originalSingle));
    if (propertiesSlot) Exchange(propertiesSlot,reinterpret_cast<void*>(&PropertiesAdapter),reinterpret_cast<void*>(originalProperties));
    if (padSlot) Exchange(padSlot, reinterpret_cast<void*>(&PadAdapter), reinterpret_cast<void*>(originalPad));
    if (keySlot) Exchange(keySlot, reinterpret_cast<void*>(&KeyAdapter), reinterpret_cast<void*>(originalKey));
}
bool Install(const D2RL::PluginContext* context) noexcept {
    const auto core = GetModuleHandleW(L"D2RCore.dll");
    if (!core) { context->LogError("Item Roll Ranges: D2RCore.dll is not loaded."); return false; }
    if (!VerifyProviderFile(core)) { context->LogError("Item Roll Ranges: unsupported D2RCore file fingerprint; requires the qualified 1.3.1-beta build. No adapters installed."); return false; }
    auto* base = reinterpret_cast<unsigned char*>(core);
    // This profile is tied to the installed provider; all dependencies are
    // fingerprinted before publishing either pointer. See docs/NATIVE-CONTRACT.md.
    if (GetProcAddress(core, "BuildItemTooltipWithStatRanges") != reinterpret_cast<FARPROC>(base+0x819530)) return false;
    using namespace ProviderProfile;
    if (!Match(base, SelectionRva, Selection, sizeof(Selection)) ||
        !Match(base, ControllerCallRva, ControllerCall, sizeof(ControllerCall)) ||
        !Match(base, PanelWitnessRva, PanelWitness, sizeof(PanelWitness)) ||
        !Match(base, WideCopyRva, WideCopy, sizeof(WideCopy)) ||
        !Match(base, WideReadRva, WideRead, sizeof(WideRead)) ||
        !Match(base, WideRangeCallerRva, WideRangeCaller, sizeof(WideRangeCaller)) ||
        !Match(base, WideActualRva, WideActual, sizeof(WideActual)) ||
        !Match(base, PropertiesOverlayRva, PropertiesOverlay, sizeof(PropertiesOverlay)) ||
        !Match(base, OverlayScopeRva, OverlayScope, sizeof(OverlayScope)) ||
        !Match(base, AffixSelectionRva, AffixSelection, sizeof(AffixSelection)) ||
        !Match(base, SingleObserverCallerRva, SingleObserverCaller, sizeof(SingleObserverCaller)) ||
        !Match(base, EligibleCallerRva, EligibleCaller, sizeof(EligibleCaller)) ||
        !Match(base, ActualPairCallerRva, ActualPairCaller, sizeof(ActualPairCaller))) { context->LogError("Item Roll Ranges: live Core instruction witness mismatch; possible native-hook conflict."); return false; }
    if (!Readable(base+0x702ae0, 8) || !Readable(base+0x6fe3b0, 8) || !Readable(base+0x6fe470, 8) || !Readable(base+0x704490,8)) return false;
    coreBase=base;
    const auto* game=reinterpret_cast<const unsigned char*>(context->exeBase);
    for (const auto& witness:AffixProfile::Witnesses)
        if (!Match(game,witness.rva,witness.bytes,witness.size)) { context->LogError("Item Roll Ranges: game affix/formatter instruction witness mismatch."); return false; }
    for (const auto& witness:SourceProfile::Witnesses)
        if (!Match(game,witness.rva,witness.bytes,witness.size)) { context->LogError("Item Roll Ranges: source/property layout witness mismatch."); return false; }
    for (const auto& witness:SpecialProfile::Witnesses)
        if (!Match(witness.core?base:game,witness.rva,witness.bytes,witness.size)) { context->LogError("Item Roll Ranges: special damage formatter witness mismatch."); return false; }
    for (const auto& witness:UniqueRangeProfile::Witnesses)
        if (!Match(game,witness.rva,witness.bytes,witness.size)) { context->LogError("Item Roll Ranges: unique scalar range witness mismatch."); return false; }
    if (!Readable(base+0x7043a0,8)) return false;
    specialSlot=reinterpret_cast<void**>(base+0x7043a0);
    originalSpecial=reinterpret_cast<SpecialFn>(*specialSlot);
    if (reinterpret_cast<const void*>(originalSpecial)!=game+0x2db800) {
        context->LogError("Item Roll Ranges: special damage formatter slot conflict."); return false;
    }
    specialReturn=reinterpret_cast<std::uintptr_t>(base)+0x3e7fd6;
    if (!Readable(base+0x7043e8,8) || !Readable(base+0x701e70,8) || !Readable(base+0x6ff370,8)) return false;
    if (!Readable(base+0x704400,8)) return false;
    rangeHelperSlot=reinterpret_cast<void**>(base+0x704400);
    originalRangeHelper=reinterpret_cast<RangeHelperFn>(*rangeHelperSlot);
    if (reinterpret_cast<const void*>(originalRangeHelper)!=game+0x2d6330) {
        context->LogError("Item Roll Ranges: native range-helper slot target conflict."); return false;
    }
    rangeHelperReturn=reinterpret_cast<std::uintptr_t>(base)+0x3e80b2;
    rangeHelperActualReturn=reinterpret_cast<std::uintptr_t>(base)+0x3e80fd;
    singleSlot=reinterpret_cast<void**>(base+0x7043e8);
    originalSingle=reinterpret_cast<SingleFn>(*singleSlot);
    getTables=*reinterpret_cast<TablesFn*>(base+0x701e70);
    eligibleAffix=*reinterpret_cast<EligibleFn*>(base+0x6ff370);
    if (reinterpret_cast<const void*>(originalSingle)!=game+0x2d6520 ||
        reinterpret_cast<const void*>(getTables)!=game+0x300a90 ||
        reinterpret_cast<const void*>(eligibleAffix)!=game+0x3d4220) { context->LogError("Item Roll Ranges: affix/formatter slot target conflict."); return false; }
    singleReturn=reinterpret_cast<std::uintptr_t>(base)+0x3e82ea;
    propertiesSlot=reinterpret_cast<void**>(base+0x704490);
    originalProperties=reinterpret_cast<PropertiesFn>(*propertiesSlot);
    propertiesReturn=reinterpret_cast<std::uintptr_t>(base)+0x81d53b;
    keySlot = reinterpret_cast<void**>(base+0x6fe3b0);
    padSlot = reinterpret_cast<void**>(base+0x6fe470);
    originalKey = reinterpret_cast<KeyFn>(*keySlot);
    originalPad = reinterpret_cast<PadFn>(*padSlot);
    testPanel = *reinterpret_cast<PanelFn*>(base+0x702ae0);
    if (!Executable(reinterpret_cast<void*>(originalProperties)) || !Executable(reinterpret_cast<void*>(originalKey)) || !Executable(reinterpret_cast<void*>(originalPad)) ||
        !Executable(reinterpret_cast<void*>(testPanel))) return false;
    const auto xi = LoadLibraryExW(L"xinput1_4.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!xi) return false;
    readPad = reinterpret_cast<XInputFn>(GetProcAddress(xi, "XInputGetState"));
    if (!readPad) { FreeLibrary(xi); return false; }
    // A delayed caller can have fetched our pointer before Restore. Pinning
    // keeps code/original targets available until process exit after unload.
    HMODULE pinned{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&KeyAdapter), &pinned)) { FreeLibrary(xi); return false; }
    wchar_t modulePath[MAX_PATH]{};
    const DWORD pathLength=GetModuleFileNameW(pinned,modulePath,MAX_PATH);
    if (pathLength && pathLength<MAX_PATH) {
        auto slash=wcsrchr(modulePath,L'\\');
        if (slash) { *slash=0; slash=wcsrchr(modulePath,L'\\'); }
        if (slash) {
            *slash=0;
            swprintf_s(diagnosticPath,L"%s\\logs\\item-roll-ranges-format.log",modulePath);
        }
    }
    keyReturn = reinterpret_cast<std::uintptr_t>(base)+0x8195e0;
    padReturn = reinterpret_cast<std::uintptr_t>(base)+0x81995a;
    if (!Exchange(specialSlot,reinterpret_cast<void*>(originalSpecial),reinterpret_cast<void*>(&SpecialAdapter)) ||
        !Exchange(rangeHelperSlot,reinterpret_cast<void*>(originalRangeHelper),reinterpret_cast<void*>(&RangeHelperAdapter)) ||
        !Exchange(singleSlot,reinterpret_cast<void*>(originalSingle),reinterpret_cast<void*>(&SingleAdapter)) ||
        !Exchange(propertiesSlot,reinterpret_cast<void*>(originalProperties),reinterpret_cast<void*>(&PropertiesAdapter)) ||
        !Exchange(keySlot, reinterpret_cast<void*>(originalKey), reinterpret_cast<void*>(&KeyAdapter)) ||
        !Exchange(padSlot, reinterpret_cast<void*>(originalPad), reinterpret_cast<void*>(&PadAdapter))) {
        context->LogError("Item Roll Ranges: pointer publication failed; restoring owned slots.");
        Restore();
        return false;
    }
    active.store(true, std::memory_order_release);
    context->LogInfo("Item Roll Ranges 1.3.1+rev.13 by PyreFly: Ctrl / R1(RB); inventory, stash, Cube and vendor only. Actual values, native ranges, named P/S tiers and guarded stacked-damage details. Unsupported or ambiguous provenance is omitted.");
    return true;
}
}

D2RL_PLUGIN_EXPORT const D2RL::PluginInfo* __cdecl D2RLoaderGetPluginInfo() noexcept {
    static const D2RL::PluginInfo info{sizeof(D2RL::PluginInfo), D2RL_PLUGIN_ABI_VERSION,
        "item-roll-ranges", "Item Roll Ranges", "1.3.1+rev.13", "PyreFly",
        "Hold Ctrl or R1/RB for native item stat ranges in item-management screens.",
        D2RL::PluginFlags::Client | D2RL::PluginFlags::NativeHooks, {}};
    return &info;
}
D2RL_PLUGIN_EXPORT bool __cdecl D2RLoaderLoadPlugin(const D2RL::PluginContext* context) noexcept {
    if (!context || !context->GetApi() || loaded.exchange(true)) return false;
    if (Install(context)) return true;
    context->LogError("Item Roll Ranges: unsupported/unavailable provider or slot conflict; adapters not activated. See NATIVE-CONTRACT.md.");
    return false;
}
D2RL_PLUGIN_EXPORT void __cdecl D2RLoaderUnloadPlugin() noexcept { Restore(); }

