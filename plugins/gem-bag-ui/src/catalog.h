#pragma once
#include "policy.h"
#include <D2RLPlugin/data_tables.h>
#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <string>
#include <vector>

namespace GemBag::Catalog {
using Id=D2RL::DataTables::TableId;
inline constexpr std::array<Id,6> Ids{Id::Items,Id::ItemTypes,Id::Gems,Id::Properties,Id::ItemStatCost,Id::CubeMain};
inline constexpr std::array<unsigned,6> Sizes{448,232,192,48,324,364};
inline constexpr std::array<unsigned,6> Limits{65536,4096,65536,65536,512,100000};
struct Table {
    const unsigned char* rows{};unsigned count{},size{};
    const unsigned char* Row(unsigned index) const {return index<count?rows+static_cast<size_t>(index)*size:nullptr;}
};
template<class T> T Read(const unsigned char* row,size_t offset) {T value{};std::memcpy(&value,row+offset,sizeof(value));return value;}
template<class T,size_t N> void Put(std::array<unsigned char,N>& row,size_t offset,T value){std::memcpy(row.data()+offset,&value,sizeof(value));}
struct Choice {uint32_t code{},classId{},nameId{},recipe{};};
struct DepositChoice {uint32_t code{},classId{},minimum{},maximum{},recipe{};};
struct Witness {Id table{};uint32_t row{};std::vector<unsigned char> bytes;};
struct Definition {
    uint64_t revision{},fingerprint{};unsigned bank{},bagClass{},bagCode{},creditStat{},property{},mask{};
    std::array<unsigned,6> counts{};
    std::array<Choice,8> choices{};std::vector<Witness> witnesses;
    std::vector<DepositChoice> deposits;
};
using Definitions=std::vector<Definition>;
using Views=std::array<D2RL::DataTables::TableView,Ids.size()>;
inline bool ItemCode(uint32_t code) {
    for(unsigned i=0;i<4;++i){const auto c=(code>>(i*8))&255;if(c<32||c>126)return false;}
    return (code&0xffffff)!=0x202020;
}
inline bool FlatProperty(const unsigned char* row,unsigned property,unsigned stat) {
    std::array<unsigned char,48> expected{};Put(expected,0,property);expected[0x18]=17;
    for(unsigned i=0;i<7;++i)Put(expected,0x20+2*i,static_cast<uint16_t>(i?0xffff:stat));
    return !std::memcmp(row,expected.data(),expected.size());
}
// The admitted setter supports an unshifted, unsigned, flat 16-bit credit.
// Display names/priorities/groups are deliberately not part of this contract.
inline bool FlatStat(const unsigned char* row,unsigned stat) {
    constexpr unsigned char prefix[]={
        0,0,0,2,0,0,16,0,0,0,0,0,0,0,1,0,0,0,0,16,16,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xff,0xff};
    return Read<uint16_t>(row,0)==stat&&!std::memcmp(row+2,prefix,sizeof(prefix))&&
        row[0x50]==0&&row[0x51]==0;
}
inline bool Recipe(const unsigned char* row,unsigned index,unsigned bank,unsigned bag,unsigned grabber,unsigned gem,unsigned stat,unsigned property) {
    const auto version=Read<uint16_t>(row,0xe);
    if((version!=0&&version!=100)||(bank==1&&version==100))return false;
    std::array<unsigned char,364> expected{};
    expected[0]=1;expected[2]=0xff;expected[3]=17;Put(expected,4,stat);expected[0xc]=2;Put(expected,0xe,version);
    for(unsigned i=0;i<2;++i) {
        const auto start=0x14+i*10;const auto quantity=row[start+7];if(quantity>1)return false;
        Put(expected,start,uint16_t{1});Put(expected,start+2,i?grabber:bag);expected[start+7]=quantity;
    }
    for(unsigned i=0;i<3;++i) {
        const auto start=0x5a+i*91;Put(expected,start,index);Put(expected,start+4,static_cast<uint16_t>(i?0:1));
        Put(expected,start+6,i==1?grabber:i==2?gem:0);expected[start+13]=static_cast<unsigned char>(i?0xfc:0xff);
        const auto quality=row[start+10];if(quality!=0&&quality!=2)return false;expected[start+10]=quality;
        // usetype,mod retains the bag's quality; an explicit quality would not.
        if(!i&&quality)return false;
        for(unsigned n=0;n<5;++n)Put(expected,start+29+n*12,UINT32_MAX);
    }
    Put(expected,0x77,property);Put(expected,0x77+6,int16_t{-1});Put(expected,0x77+8,int16_t{-1});expected[0x77+10]=100;
    return !std::memcmp(row,expected.data(),expected.size());
}
inline std::optional<unsigned> Family(const Table& types,const unsigned char* item) {
    constexpr const char* families[]={"gema","gemt","gems","geme","gemd","gemr","gemz","gemo"};
    std::set<unsigned> active,complete;std::optional<unsigned> found;bool valid=true;
    auto visit=[&](auto&& self,int id)->void {
        if(!valid||id<=0)return;
        if(static_cast<unsigned>(id)>=types.count||active.contains(static_cast<unsigned>(id))||active.size()+complete.size()>=64){valid=false;return;}
        if(complete.contains(static_cast<unsigned>(id)))return;
        active.insert(static_cast<unsigned>(id));const auto row=types.Row(static_cast<unsigned>(id));
        for(unsigned i=0;i<std::size(families);++i)if(!std::memcmp(row,families[i],4)) {
            if(found&&*found!=i){valid=false;return;}found=i;
        }
        self(self,Read<int16_t>(row,4));self(self,Read<int16_t>(row,6));
        active.erase(static_cast<unsigned>(id));complete.insert(static_cast<unsigned>(id));
    };
    visit(visit,Read<int16_t>(item,0x12e));visit(visit,Read<int16_t>(item,0x130));
    return valid?found:std::optional<unsigned>{};
}
inline bool Build(const Views& views,unsigned bank,Definitions& result,std::string& why) {
    result.clear();const auto revision=views[0].revision;std::array<Table,Ids.size()> t{};
    for(unsigned i=0;i<t.size();++i) {
        const auto& v=views[i];
        if(!D2RL::DataTables::HasTableViewField(&v,D2RL::DataTables::TableViewRequiredSize)||v.tableId!=Ids[i]||
            static_cast<unsigned>(v.bank)!=bank||!revision||v.revision!=revision||v.rowSize!=Sizes[i]||!v.rows||!v.rowCount||v.rowCount>Limits[i]) {
            why="compiled table layout, bounds, bank or revision mismatch (table="+std::to_string(static_cast<unsigned>(Ids[i]))+")";return false;
        }
        t[i]={static_cast<const unsigned char*>(v.rows),v.rowCount,v.rowSize};
    }
    const auto& items=t[0];const auto& types=t[1];const auto& gems=t[2];const auto& properties=t[3];const auto& stats=t[4];const auto& cube=t[5];
    std::map<uint32_t,unsigned> codeCounts,gemCounts;
    for(unsigned i=0;i<items.count;++i)++codeCounts[Read<uint32_t>(items.Row(i),0x80)];
    for(unsigned i=0;i<gems.count;++i)++gemCounts[Read<uint32_t>(gems.Row(i),0x28)];
    std::map<unsigned,Definition> definitions;std::set<unsigned> rejected;std::string firstRefusal;
    auto add=[&](Definition& d,unsigned table,unsigned index) {
        if(std::any_of(d.witnesses.begin(),d.witnesses.end(),[&](const Witness& w){return w.table==Ids[table]&&w.row==index;}))return;
        const auto row=t[table].Row(index);d.witnesses.push_back({Ids[table],index,{row,row+Sizes[table]}});
    };
    for(unsigned index=0;index<cube.count;++index) {
        const auto row=cube.Row(index);
        if(row[0]!=1||row[3]!=17||row[0xc]!=2||Read<uint16_t>(row,0x14)!=1||Read<uint16_t>(row,0x1e)!=1||row[0x67]!=0xff)continue;
        const unsigned bag=Read<uint32_t>(row,0x16),grabber=Read<uint32_t>(row,0x20),gem=Read<uint32_t>(row,0x116);
        if(bag>=items.count||grabber>=items.count||gem>=items.count||bag==grabber||bag==gem||grabber==gem)continue;
        const auto output=items.Row(gem);const auto family=Family(types,output);if(!family)continue;
        const unsigned stat=Read<uint32_t>(row,4),property=Read<uint32_t>(row,0x77);
        auto itemOk=[&](unsigned id){const auto r=items.Row(id);const auto code=Read<uint32_t>(r,0x80);return ItemCode(code)&&codeCounts[code]==1&&r[0x11e]==1&&r[0x11f]==1;};
        const char* refusal=stat>=stats.count||property>=properties.count?"stat/property bounds":
            !itemOk(bag)||!itemOk(grabber)||!itemOk(gem)?"item code/size ambiguity":gemCounts[gem]!=1||gemCounts[bag]?"gem identity ambiguity":
            !FlatStat(stats.Row(stat),stat)?"unsupported credit encoding":!FlatProperty(properties.Row(property),property,stat)?"unsupported credit property":
            !Recipe(row,index,bank,bag,grabber,gem,stat,property)?"unsupported recipe restrictions/effects":nullptr;
        if(refusal) {
            if(firstRefusal.empty())firstRefusal="; first refused bag="+std::to_string(bag)+" recipe="+std::to_string(index)+" ("+refusal+")";
            rejected.insert(bag);continue;
        }
        auto& d=definitions[bag];
        if(!d.mask){d.revision=revision;d.bank=bank;d.bagClass=bag;d.bagCode=Read<uint32_t>(items.Row(bag),0x80);d.creditStat=stat;d.property=property;
            for(unsigned n=0;n<t.size();++n)d.counts[n]=t[n].count;}
        if(d.creditStat!=stat||d.property!=property||(d.mask&(1u<<*family))){rejected.insert(bag);continue;}
        d.mask|=1u<<*family;d.choices[*family]={Read<uint32_t>(output,0x80),gem,Read<uint16_t>(output,0xfc),index};
        add(d,0,bag);add(d,0,grabber);add(d,0,gem);add(d,3,property);add(d,4,stat);add(d,5,index);
        // Own all ancestry and gem definitions too; runtime changes must refuse.
        // These small tables are checked in full during revalidation.
    }
    for(auto& [bag,d]:definitions)if(d.mask&&!rejected.contains(bag)) {
        // Deposit eligibility is an independent, optional recipe capability.
        // Only one-unit, unrestricted bag-preserving conversions are modeled.
        bool depositRefused=false;
        std::set<unsigned> depositItems;
        for(unsigned index=0;index<cube.count;++index) {
            const auto row=cube.Row(index);
            if(row[0]!=1||row[0xc]!=2||Read<uint16_t>(row,0x14)!=1||Read<uint32_t>(row,0x16)!=bag||
                Read<uint32_t>(row,0x77)!=d.property||Read<int16_t>(row,0x7d)<=0)continue;
            const auto version=Read<uint16_t>(row,0xe);
            if(bank==1&&version==100)continue;
            const auto kind=Read<uint16_t>(row,0x1e);const auto selector=Read<uint32_t>(row,0x20);
            const auto minimum=Read<int16_t>(row,0x7d),maximum=Read<int16_t>(row,0x7f);
            std::array<unsigned char,364> expected{};
            expected[0]=1;expected[2]=0xff;expected[0xc]=2;Put(expected,0xe,version);
            Put(expected,0x14,uint16_t{1});Put(expected,0x16,bag);expected[0x1b]=row[0x1b];
            Put(expected,0x1e,kind);Put(expected,0x20,selector);expected[0x25]=1;
            Put(expected,0x5a,index);Put(expected,0x5e,uint16_t{1});expected[0x67]=0xff;
            for(unsigned output=0;output<3;++output)for(unsigned n=0;n<5;++n)Put(expected,0x77+output*91+n*12,UINT32_MAX);
            Put(expected,0x77,d.property);Put(expected,0x7d,minimum);Put(expected,0x7f,maximum);expected[0x81]=100;
            if((version!=0&&version!=100)||row[0x1b]>1||maximum<minimum||
                (kind!=1&&kind!=2)||(kind==1?selector>=items.count:!selector||selector>=types.count)||
                std::memcmp(row,expected.data(),expected.size())){depositRefused=true;break;}
            add(d,5,index);
            for(unsigned id=0;id<items.count;++id) {
                if(id==bag)continue;
                const auto item=items.Row(id);bool match=kind==1&&selector==id,valid=true;
                if(kind==2) {
                    std::set<unsigned> active,done;
                    auto visit=[&](auto&& self,int type)->void {
                        if(!valid||type<=0)return;
                        if(static_cast<unsigned>(type)>=types.count||active.contains(type)||active.size()+done.size()>=64){valid=false;return;}
                        if(done.contains(type))return;active.insert(type);match|=static_cast<unsigned>(type)==selector;
                        const auto parent=types.Row(type);self(self,Read<int16_t>(parent,4));self(self,Read<int16_t>(parent,6));
                        active.erase(type);done.insert(type);
                    };
                    visit(visit,Read<int16_t>(item,0x12e));visit(visit,Read<int16_t>(item,0x130));
                }
                if(!valid){depositRefused=true;break;}
                if(!match)continue;
                const auto code=Read<uint32_t>(item,0x80);
                if(!ItemCode(code)||codeCounts[code]!=1||!depositItems.insert(id).second){depositRefused=true;break;}
                d.deposits.push_back({code,id,static_cast<unsigned>(minimum),static_cast<unsigned>(maximum),index});add(d,0,id);
            }
            if(depositRefused)break;
        }
        if(depositRefused)d.deposits.clear();
        for(unsigned table:{1u,2u})for(unsigned index=0;index<t[table].count;++index)add(d,table,index);
        uint64_t hash=14695981039346656037ull;
        auto number=[&](uint32_t n){for(unsigned i=0;i<4;++i){hash^=(n>>(i*8))&255;hash*=1099511628211ull;}};
        for(unsigned n:{d.bank,d.bagClass,d.bagCode,d.creditStat,d.property,d.mask})number(n);
        for(const auto& choice:d.choices){number(choice.code);number(choice.classId);}
        number(static_cast<uint32_t>(d.deposits.size()));
        for(const auto& deposit:d.deposits)for(unsigned n:{deposit.code,deposit.classId,deposit.minimum,deposit.maximum,deposit.recipe})number(n);
        for(const auto& witness:d.witnesses){number(static_cast<unsigned>(witness.table));number(witness.row);
            // The localized name reference is presentation data, not host authority.
            // Per-process Recheck still compares every original witness byte.
            for(size_t i=0;i<witness.bytes.size();++i) {
                if(witness.table==Id::Items&&(i==0xfc||i==0xfd))continue;
                hash^=witness.bytes[i];hash*=1099511628211ull;
            }}
        d.fingerprint=hash?hash:1;result.push_back(std::move(d));
    }
    why=(result.empty()?"no unambiguous compatible one-credit gem withdrawal recipes":"compatible compiled bag recipes found")+firstRefusal;
    return true;
}
bool Initialize(const D2RL::PluginContext*);
void Stop();
bool Candidate(const Info&);
bool CandidateClass(unsigned);
std::shared_ptr<const Definition> Find(unsigned,const Info&);
bool Recheck(const D2RL::PluginContext*,const Definition&);
// Build/Preset are also used by owned table replay tests; no native pointer is retained.
bool Refresh(const D2RL::PluginContext*,uint64_t);
}
