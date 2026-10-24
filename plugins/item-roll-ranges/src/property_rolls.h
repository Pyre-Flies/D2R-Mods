#pragma once
#include "affixes.h"
#include "contribution_sources.h"
#include "range_text.h"
#include <map>
#include <optional>
#include <set>
#include <algorithm>
namespace PropertyRolls {
// Ops8/9 consume item attribute operands to derive equipped character resources;
// they do not scale the stored/displayed attribute points. Level operations
// and other transformations still require their own qualified paths.
inline bool ItemScalarOperand(const unsigned char* metadata){
    return metadata[0x50]==0 || metadata[0x50]==8 || metadata[0x50]==9 || metadata[0x50]==11;
}
// A variable is one property-function roll, not one displayed stat. Function 3
// can copy a nonzero preceding roll to several stats; zero-inclusive domains
// cannot establish that relation because the engine can roll again at zero.
struct Variable {int low{},high{};std::size_t source{};std::vector<unsigned> stats;};
struct Equation {unsigned stat{};int actual{};std::vector<std::size_t> variables;};
struct Model {
    std::vector<Variable> variables;
    std::map<unsigned,std::vector<std::size_t>> terms;
    std::set<unsigned> blocked;
    std::vector<std::string> names;
    bool complete{true};
};
// Known implicit item operations do not invalidate unrelated scalar equations.
// Conservatively exclude their base/durability domain; those values retain the
// existing native/header paths rather than being solved as additive affixes.
inline bool BlockImplicit(unsigned fn,unsigned stat,unsigned replacement,std::set<unsigned>& blocked){
    if(stat!=65535 || replacement || (fn!=20 && fn!=23))return false;
    for(unsigned affected:{72u,73u,152u})blocked.insert(affected);
    if(fn==23)for(unsigned affected:{16u,21u,22u,23u,24u,91u,92u})blocked.insert(affected);
    return true;
}
inline bool BlockPossible(std::uint32_t id,std::span<const Affixes::Property> properties,
    std::span<const Affixes::PropertyGroup> groups,std::set<unsigned>& blocked,unsigned depth=0){
    if(id==UINT32_MAX)return true;if(depth>4)return false;
    if((id>>16)==0){
        if(id>=properties.size())return false;const auto& row=properties[id].bytes;
        for(unsigned slot=0;slot<7;++slot){const unsigned fn=row[0x18+slot];if(!fn)continue;
            const unsigned stat=Affixes::Read<std::uint16_t>(row.data(),0x20+slot*2);if(stat<4096)blocked.insert(stat);
            if(BlockImplicit(fn,stat,row[2+slot],blocked))continue;
            if(stat>=4096 && fn!=5 && fn!=6 && fn!=7)return false;
            if(fn==7){blocked.insert(17);blocked.insert(18);}if(fn==5||(fn==15&&stat==21)){blocked.insert(21);blocked.insert(23);}if(fn==6||(fn==16&&stat==22)){blocked.insert(22);blocked.insert(24);}}
        return true;
    }
    if((id>>16)!=1 || (id&0xffff)>=groups.size())return false;
    const auto* row=groups[id&0xffff].bytes.data();
    for(unsigned n=0;n<8;++n){const auto* entry=row+8+n*24;if(Affixes::Read<int>(entry,20)<=0)continue;
        if(!BlockPossible(Affixes::Read<std::uint32_t>(entry,0),properties,groups,blocked,depth+1))return false;}
    return true;
}
inline Model Compile(std::span<const Affixes::Rolled> sources,std::span<const Affixes::Property> properties,
    std::span<const Affixes::PropertyGroup> groups={}) {
    Model out;
    if(sources.size()>128){out.complete=false;return out;}
    for(std::size_t source=0;source<sources.size();++source){
        const auto& definition=sources[source];out.names.push_back(Affixes::NamedLabel(definition));
        for(unsigned n=0;n<3;++n){
            const int id=definition.row.Number(0x24+n*16);
            if(id==-1)continue;
            if(id<0){out.complete=false;continue;}
            if(static_cast<std::size_t>(id)>=properties.size()){
                // An unexpanded group can still contribute to other sources'
                // stats. Taint those equations rather than omit that source.
                if((static_cast<unsigned>(id)>>16)!=1 || groups.empty()){out.complete=false;continue;}
                if(!BlockPossible(static_cast<unsigned>(id),properties,groups,out.blocked))out.complete=false;
                continue;
            }
            const auto& row=properties[static_cast<std::size_t>(id)].bytes;
            const int low=definition.row.Number(0x2c+n*16),high=definition.row.Number(0x30+n*16);
            std::vector<Variable> pending;std::set<unsigned> affected;
            bool valid=low<=high && low>=-1000000 && high<=1000000 &&
                definition.unconditionalScalar && !(definition.source && (std::string_view(definition.source)=="[U]" || std::string_view(definition.source)=="[RW]") && !definition.uniqueScalar);
            std::optional<std::size_t> previous;
            for(unsigned slot=0;slot<7;++slot){
                const unsigned fn=row[0x18+slot];if(!fn)continue;
                const unsigned stat=Affixes::Read<std::uint16_t>(row.data(),0x20+slot*2);
                if(stat<4096)affected.insert(stat);
                if(fn==7){affected.insert(17);affected.insert(18);}
                if(fn==5){affected.insert(21);affected.insert(23);}
                if(fn==6||(fn==16&&stat==22)){affected.insert(22);affected.insert(24);}
                if(fn==15&&stat==21){affected.insert(21);affected.insert(23);}
                if(BlockImplicit(fn,stat,row[2+slot],affected)){valid=false;previous.reset();continue;}
                // Compiled uint16 property id, then seven replacement flags.
                if(stat>=4096 || row[2+slot]){if(stat>=4096 && fn!=5 && fn!=6 && fn!=7)out.complete=false;valid=false;previous.reset();continue;}
                if(fn==1 || fn==8 || fn==17){
                    // Function 8 has reuse operands too; only its independent
                    // single-handler form is included in this linear model.
                    if(fn==8 && (!pending.empty() || std::count_if(row.begin()+0x18,row.begin()+0x1f,[](unsigned char f){return f!=0;})!=1))valid=false;
                    if(fn==17 && std::count_if(row.begin()+0x18,row.begin()+0x1f,[](unsigned char f){return f!=0;})!=1)valid=false;
                    const int parameter=definition.row.Number(0x28+n*16);
                    if(fn==17 && (parameter<0 || parameter>1000000))valid=false;
                    pending.push_back({fn==17&&parameter?parameter:low,fn==17&&parameter?parameter:high,source,{stat}});previous=pending.size()-1;
                }else if(fn==3 && previous && (low>0 || high<0)){
                    const bool duplicate=std::any_of(pending.begin(),pending.end(),[&](const Variable& v){
                        return std::find(v.stats.begin(),v.stats.end(),stat)!=v.stats.end();});
                    if(duplicate)valid=false;
                    else if(low==high)pending.push_back({low,high,source,{stat}});
                    else pending[*previous].stats.push_back(stat);
                }else if(slot==0 && fn==21 && row[0x19]==3 && !row[0x1a] && !row[3] &&
                         Affixes::Read<std::uint16_t>(row.data(),0x0a)!=0 && (low>0 || high<0)){
                    const unsigned stat2=Affixes::Read<std::uint16_t>(row.data(),0x22);
                    if(stat2>=4096){out.complete=false;valid=false;}
                    else{affected.insert(stat2);pending.push_back({low,high,source,{stat2}});}
                    break;
                }else{valid=false;previous.reset();}
            }
            if(!valid || pending.empty() || out.variables.size()+pending.size()>128){
                out.blocked.insert(affected.begin(),affected.end());continue;
            }
            for(auto& variable:pending){
                const auto index=out.variables.size();
                for(const auto stat:variable.stats)out.terms[stat].push_back(index);
                out.variables.push_back(std::move(variable));
            }
        }
    }
    return out;
}
using Interval=std::pair<int,int>;
// Skill/class/tab selectors are stored as stat layers. This establishes their
// marginal bounds; it does not treat packed proc/charge layers as scalar rolls.
inline std::optional<Interval> LayerBounds(const Affixes::Rolled& definition,
    std::span<const Affixes::Property> properties,unsigned stat,unsigned layer,bool randomSkills=false){
    if(!definition.unconditionalScalar || (definition.source && (std::string_view(definition.source)=="[U]" || std::string_view(definition.source)=="[RW]") && !definition.uniqueScalar))return {};
    long long low=0,high=0;bool found=false;
    for(unsigned n=0;n<3;++n){
        const int id=definition.row.Number(0x24+n*16);if(id==-1)continue;
        if(id<0 || static_cast<std::size_t>(id)>=properties.size())return {};
        const auto* row=properties[static_cast<std::size_t>(id)].bytes.data();
        const int param=definition.row.Number(0x28+n*16);
        const int lo=definition.row.Number(0x2c+n*16),hi=definition.row.Number(0x30+n*16);
        for(unsigned slot=0;slot<7;++slot){
            const unsigned fn=row[0x18+slot];if(!fn || Affixes::Read<std::uint16_t>(row,0x20+slot*2)!=stat)continue;
            bool match=false;
            if(fn==21)match=layer==Affixes::Read<std::uint16_t>(row,0x0a+slot*2);
            else if(fn==10)match=param>=0 && param<24 && layer==static_cast<unsigned>(param/3*8+param%3);
            else if(fn==22 || fn==24)match=param>=0 && param<=65535 && layer==static_cast<unsigned>(param);
            else if(fn==12 && randomSkills) {
                if(std::count_if(row+0x18,row+0x1f,[](unsigned char f){return f!=0;})!=1)return {};
                match=Affixes::RandomLayerSpec(param,lo,hi,layer);
            }
            else return {};
            if(!match)continue;
            if(row[2+slot] || lo>hi || lo< -1000000 || hi>1000000)return {};
            low+=fn==12?param:lo;high+=fn==12?param:hi;found=true;
        }
    }
    if(!found || low<INT_MIN || high>INT_MAX)return {};
    return Interval{static_cast<int>(low),static_cast<int>(high)};
}
inline bool Propagate(std::vector<Interval>& bounds,std::span<const Equation> equations,bool* unknown=nullptr) {
    if(unknown)*unknown=false;
    // Monotonic integer bound consistency. A budget refuses rather than renders
    // a partial proof. Every equation uses each independent variable once.
    for(unsigned pass=0;pass<256;++pass){
        bool changed=false;
        for(const auto& equation:equations){
            long long low=0,high=0;
            for(auto index:equation.variables){if(index>=bounds.size())return false;low+=bounds[index].first;high+=bounds[index].second;}
            if(equation.actual<low || equation.actual>high)return false;
            for(auto index:equation.variables){
                auto& b=bounds[index];
                const auto lo=std::max<long long>(b.first,equation.actual-(high-b.second));
                const auto hi=std::min<long long>(b.second,equation.actual-(low-b.first));
                if(lo>hi)return false;
                changed|=lo!=b.first || hi!=b.second;b={static_cast<int>(lo),static_cast<int>(hi)};
            }
        }
        if(!changed)return true;
    }
    if(unknown)*unknown=true;return false;
}
inline bool Feasible(std::vector<Interval> bounds,std::span<const Equation> equations,unsigned& budget,bool* unknown=nullptr) {
    if(!budget){if(unknown)*unknown=true;return false;}
    --budget;
    bool limited=false;if(!Propagate(bounds,equations,&limited)){if(limited && unknown)*unknown=true;return false;}
    std::optional<std::size_t> split;
    for(const auto& equation:equations)for(auto index:equation.variables)
        if(bounds[index].first!=bounds[index].second && (!split || bounds[index].second-bounds[index].first<bounds[*split].second-bounds[*split].first))split=index;
    if(!split)return true;
    const auto at=*split;const int high=bounds[at].second;
    const int mid=bounds[at].first+(high-bounds[at].first)/2;
    auto left=bounds;left[at].second=mid;
    if(Feasible(std::move(left),equations,budget,unknown))return true;
    if(!budget){if(unknown)*unknown=true;return false;}
    bounds[at].first=mid+1;return Feasible(std::move(bounds),equations,budget,unknown);
}
struct Solution {
    Model model;std::vector<Interval> bounds;std::set<unsigned> verified;
    Contributions::Stats actual;
    std::set<unsigned> impossible;
};
template<class Normalize> inline Solution Solve(Model model,const Contributions::Stats& actual,Normalize normalize) {
    Solution out{std::move(model),{}, {},actual,{}};
    if(!out.model.complete)return out;
    for(const auto& v:out.model.variables)out.bounds.push_back({v.low,v.high});
    std::vector<Equation> equations;
    for(const auto& [stat,terms]:out.model.terms){
        if(out.model.blocked.contains(stat))continue;
        const auto value=normalize(stat,Contributions::Value(actual,stat));
        if(value)equations.push_back({stat,*value,terms});
    }
    // Refuse only a disagreeing connected component, not unrelated modifiers.
    std::set<unsigned> visited;
    for(const auto& seed:equations){
        if(visited.contains(seed.stat))continue;
        std::set<std::size_t> variables(seed.variables.begin(),seed.variables.end());
        std::vector<Equation> component;bool changed=true;
        while(changed){changed=false;
            for(const auto& e:equations){if(visited.contains(e.stat))continue;
                if(std::none_of(e.variables.begin(),e.variables.end(),[&](auto v){return variables.contains(v);}))continue;
                visited.insert(e.stat);component.push_back(e);variables.insert(e.variables.begin(),e.variables.end());changed=true;
            }
        }
        auto candidate=out.bounds;unsigned budget=4096;bool unknown=false;
        if(!Propagate(candidate,component,&unknown) || !Feasible(candidate,component,budget,&unknown)){
            if(!unknown)for(const auto& e:component)out.impossible.insert(e.stat);continue;
        }
        for(const auto index:variables)out.bounds[index]=candidate[index];
        for(const auto& e:component)out.verified.insert(e.stat);
    }
    return out;
}
template<class Normalize> inline Solution SolveSources(std::span<const Affixes::Rolled> definitions,
    std::span<const Affixes::Property> properties,const Contributions::Stats& actual,Normalize normalize,
    std::span<const Affixes::PropertyGroup> groups={}){
    struct Pool {unsigned id{};std::set<unsigned> entries,stats;};
    std::map<unsigned,Pool> pools;
    for(auto d:definitions)if(d.choicePool){
        auto& pool=pools[d.choicePool];pool.id=d.choicePool;pool.entries.insert(d.choiceEntry);
        d.unconditionalScalar=true;const auto model=Compile(std::span(&d,1),properties,groups);
        if(!model.complete)return Solve(Compile(definitions,properties,groups),actual,normalize);
        pool.stats.insert(model.blocked.begin(),model.blocked.end());for(const auto& [stat,terms]:model.terms){(void)terms;pool.stats.insert(stat);}
    }
    if(pools.empty())return Solve(Compile(definitions,properties,groups),actual,normalize);
    // Connected choice pools share potentially affected stats. Enumerate a
    // bounded relaxation; a choice is admitted only if all feasible layouts
    // select it. Unknown operations cannot rule out an alternative.
    std::map<unsigned,unsigned> forced;std::set<unsigned> visited;
    for(const auto& [id,seed]:pools){
        if(visited.contains(id))continue;
        std::vector<unsigned> component{id};std::set<unsigned> scope=seed.stats;visited.insert(id);
        bool changed=true;while(changed){changed=false;for(const auto& [other,pool]:pools){
            if(visited.contains(other))continue;
            if(std::none_of(pool.stats.begin(),pool.stats.end(),[&](auto s){return scope.contains(s);}))continue;
            visited.insert(other);component.push_back(other);scope.insert(pool.stats.begin(),pool.stats.end());changed=true;}}
        std::size_t layouts=1;bool bounded=true;
        for(auto key:component){const auto count=pools[key].entries.size();if(!count || layouts>128/count){bounded=false;break;}layouts*=count;}
        if(!bounded || definitions.size()>8192/layouts)continue;
        std::map<unsigned,unsigned> selected,common;bool any=false;
        const auto enumerate=[&](auto&& self,std::size_t position)->void{
            if(position<component.size()){
                const unsigned key=component[position];for(auto entry:pools[key].entries){selected[key]=entry;self(self,position+1);}return;
            }
            std::vector<Affixes::Rolled> variant;
            for(auto d:definitions){const auto at=selected.find(d.choicePool);
                if(at!=selected.end()){if(d.choiceEntry!=at->second)continue;d.unconditionalScalar=true;}
                variant.push_back(d);}
            auto solution=Solve(Compile(variant,properties,groups),actual,normalize);
            if(!solution.model.complete){if(!any){common=selected;any=true;}else for(auto& [key,entry]:common)if(selected[key]!=entry)entry=0;return;}
            for(auto stat:scope){
                const auto value=normalize(stat,Contributions::Value(actual,stat));
                if(!value || solution.model.blocked.contains(stat))continue;
                if(!solution.model.terms.contains(stat)){if(*value)return;}
                else if(solution.impossible.contains(stat))return;
            }
            if(!any){common=selected;any=true;}
            else for(auto& [key,entry]:common)if(selected[key]!=entry)entry=0;
        };
        enumerate(enumerate,0);
        if(any)for(const auto& [key,entry]:common)if(entry)forced[key]=entry;
    }
    std::vector<Affixes::Rolled> selected;
    for(auto d:definitions){const auto at=forced.find(d.choicePool);
        if(at!=forced.end()){if(d.choiceEntry!=at->second)continue;d.unconditionalScalar=true;}
        selected.push_back(d);}
    return Solve(Compile(selected,properties,groups),actual,normalize);
}
inline std::optional<RangeText::Label> Label(const Solution& solution,std::string key,
    unsigned stat,int actual,std::span<const unsigned> group={}) {
    if(!solution.verified.contains(stat))return {};
    const auto& terms=solution.model.terms.at(stat);
    for(auto member:group){
        const auto at=solution.model.terms.find(member);
        if(!solution.verified.contains(member) || at==solution.model.terms.end())return {};
        if(at->second!=terms){
            auto sameGroupTerms=[&](const std::vector<std::size_t>& a,const std::vector<std::size_t>& b){
                if(a.size()!=b.size())return false;
                for(std::size_t k=0;k<a.size();++k){
                    if(a[k]==b[k])continue;
                    const auto& va=solution.model.variables[a[k]];
                    const auto& vb=solution.model.variables[b[k]];
                    if(va.low!=va.high || vb.low!=vb.high || va.low!=vb.low)return false;
                    if(va.source!=vb.source && solution.model.names[va.source]!=solution.model.names[vb.source])return false;
                }
                return true;
            };
            if(!sameGroupTerms(terms,at->second))return {};
        }
    }
    RangeText::Label label;label.key=std::move(key);label.ownedTotal=actual;
    std::map<std::size_t,std::vector<std::size_t>> bySource;
    long long totalLow=0,totalHigh=0;
    for(auto index:terms){bySource[solution.model.variables[index].source].push_back(index);totalLow+=solution.bounds[index].first;totalHigh+=solution.bounds[index].second;}
    for(const auto& [source,indices]:bySource){
        long long low=0,high=0,inferredLow=0,inferredHigh=0;
        for(auto index:indices){const auto& v=solution.model.variables[index];low+=v.low;high+=v.high;inferredLow+=solution.bounds[index].first;inferredHigh+=solution.bounds[index].second;}
        const auto a=std::max<long long>(inferredLow,actual-(totalHigh-inferredHigh));
        const auto b=std::min<long long>(inferredHigh,actual-(totalLow-inferredLow));
        if(low<INT_MIN || high>INT_MAX || a>b)return {};
        RangeText::OwnedContribution part;part.low=static_cast<int>(low);part.high=static_cast<int>(high);part.label=solution.model.names[source];
        if(a==b)part.value=static_cast<int>(a);
        label.ownedParts.push_back(std::move(part));
    }
    if(label.ownedParts.empty() || label.ownedParts.size()>12)return {};
    return label;
}
// Project one layered stat into bounded virtual scalar cells. Each function12
// application chooses exactly one observed positive layer; fixed selectors are
// mandatory. Solving all cells proves the selected layer instead of treating
// every eligible random selector as an actual contribution to the hovered line.
template<class Normalize> inline std::optional<RangeText::Label> LayerLabel(
    std::span<const Affixes::Rolled> definitions,std::span<const Affixes::Property> properties,
    const Contributions::Stats& owned,unsigned stat,unsigned layer,std::string key,
    Normalize normalize,bool randomSkills,bool& proved,std::span<const Affixes::PropertyGroup> groups={}) {
    proved=false;if(definitions.size()>128 || owned.size()>256)return {};
    bool hasRandom=false;
    for(const auto& d:definitions)for(unsigned n=0;n<3;++n) {
        const int id=d.row.Number(0x24+n*16);if(id<0 || static_cast<std::size_t>(id)>=properties.size())continue;
        const auto* p=properties[static_cast<std::size_t>(id)].bytes.data();
        for(unsigned slot=0;slot<7;++slot)hasRandom|=p[0x18+slot]==12 && Affixes::Read<std::uint16_t>(p,0x20+slot*2)==stat;
    }
    std::map<unsigned,unsigned> cells;std::map<unsigned,int> values;
    for(const auto& entry:owned)if(static_cast<unsigned>(entry.key>>32)==stat) {
        const unsigned selected=static_cast<unsigned>(entry.key);const auto value=normalize(entry.value);
        if(selected>65535 || !value || (hasRandom && *value<0) || values.contains(selected))return {};
        values[selected]=*value;
    }
    std::vector<Affixes::Property> virtualProperties;std::vector<Affixes::Rolled> projected;
    const auto cell=[&](unsigned selected)->unsigned {
        const auto found=cells.find(selected);if(found!=cells.end())return found->second;
        const auto id=static_cast<unsigned>(virtualProperties.size());Affixes::Property p{};
        p.bytes[0x18]=1;const auto target=static_cast<std::uint16_t>(id);std::memcpy(p.bytes.data()+0x20,&target,2);
        virtualProperties.push_back(p);cells[selected]=id;return id;
    };
    for(const auto& [selected,value]:values){(void)value;cell(selected);}
    const auto append=[&](Affixes::Rolled d,unsigned selected,int low,int high,unsigned pool) {
        const auto id=cell(selected);d.choicePool=pool;d.choiceEntry=pool?id+1:0;d.unconditionalScalar=!pool;
        for(unsigned n=0;n<3;++n){const int absent=-1;std::memcpy(d.row.bytes.data()+0x24+n*16,&absent,4);}
        std::memcpy(d.row.bytes.data()+0x24,&id,4);std::memcpy(d.row.bytes.data()+0x2c,&low,4);std::memcpy(d.row.bytes.data()+0x30,&high,4);
        projected.push_back(d);
    };
    unsigned pool=0;
    for(const auto& d:definitions)for(unsigned n=0;n<3;++n) {
        const int id=d.row.Number(0x24+n*16);if(id==-1)continue;if(id<0)return {};
        if(static_cast<std::size_t>(id)>=properties.size()) {
            std::set<unsigned> blocked;if(!BlockPossible(static_cast<unsigned>(id),properties,groups,blocked) || blocked.contains(stat))return {};
            continue;
        }
        const auto* p=properties[static_cast<std::size_t>(id)].bytes.data();
        const int param=d.row.Number(0x28+n*16),low=d.row.Number(0x2c+n*16),high=d.row.Number(0x30+n*16);
        for(unsigned slot=0;slot<7;++slot) {
            const unsigned fn=p[0x18+slot];if(!fn)continue;
            const unsigned target=Affixes::Read<std::uint16_t>(p,0x20+slot*2);
            if(target>=4096 && fn!=5 && fn!=6 && fn!=7 && fn!=20 && fn!=23)return {};
            if(target!=stat)continue;
            if(p[2+slot] || !d.unconditionalScalar ||
               (d.source && (std::string_view(d.source)=="[U]" || std::string_view(d.source)=="[RW]") && !d.uniqueScalar))return {};
            if(fn==12) {
                if(!randomSkills || std::count_if(p+0x18,p+0x1f,[](unsigned char f){return f!=0;})!=1 ||
                   !Affixes::RandomLayerSpec(param,low,high,static_cast<unsigned>(low)))return {};
                ++pool;const auto before=projected.size();
                for(const auto& [selected,value]:values)if(value>0 && Affixes::RandomLayerSpec(param,low,high,selected))append(d,selected,param,param,pool);
                if(projected.size()==before)return {};
            }else {
                unsigned selected{};
                if(fn==21)selected=Affixes::Read<std::uint16_t>(p,0x0a+slot*2);
                else if(fn==10 && param>=0 && param<24)selected=static_cast<unsigned>(param/3*8+param%3);
                else if((fn==22 || fn==24) && param>=0 && param<=65535)selected=static_cast<unsigned>(param);
                else return {};
                // Nonnegative bonuses ensure an unobserved zero layer cannot
                // conceal a selected random bonus through cancellation.
                if(low<(hasRandom?0:-1000000) || low>high || high>1000000)return {};
                append(d,selected,low,high,0);
            }
            if(projected.size()>128 || cells.size()>512)return {};
        }
    }
    Contributions::Stats actual;
    for(const auto& [selected,id]:cells)actual.push_back({std::uint64_t{id}<<32,values[selected]});
    std::sort(actual.begin(),actual.end(),[](const auto& a,const auto& b){return a.key<b.key;});
    const auto solution=SolveSources(projected,virtualProperties,actual,[](unsigned,std::int64_t value){return Contributions::Normalize(value,0);});
    if(!solution.model.complete)return {};
    for(const auto& [selected,id]:cells)if(values[selected] && (!solution.model.terms.contains(id) || solution.impossible.contains(id)))return {};
    const auto found=cells.find(layer);if(found==cells.end()){proved=true;return {};}
    const unsigned id=found->second;
    if(solution.model.blocked.contains(id) || solution.impossible.contains(id))return {};
    if(!solution.model.terms.contains(id)){proved=values[layer]==0;return {};}
    auto result=Label(solution,std::move(key),id,values[layer]);proved=result.has_value();return result;
}
}
