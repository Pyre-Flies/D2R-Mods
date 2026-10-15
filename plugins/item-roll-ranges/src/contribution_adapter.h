// Included inside the plugin's private namespace after its native helpers.
using DefinitionTable=ContributionDefinitions::Table;
std::optional<Contributions::Snapshot> CaptureItemContributions(const void* source) {
    if(!contributionsReady.load())return {};
    return Contributions::Capture(reinterpret_cast<std::uintptr_t>(source),[](std::uintptr_t at,void* out,std::size_t size){
        const auto* ptr=reinterpret_cast<const void*>(at);if(!Readable(ptr,size))return false;std::memcpy(out,ptr,size);return true;},propertySourcesReady.load()?171u:0u);
}
std::optional<DefinitionTable> ContributionTable(D2RL::DataTables::TableId id,unsigned bank,
    unsigned stride,std::uint64_t& revision,bool allowEmpty=false) {
    if(!contributionContext||!contributionTables||!contributionTables->getTable)return {};
    D2RL::DataTables::TableView view{.structSize=D2RL::DataTables::TableViewSize};
    if(contributionTables->getTable(contributionContext,static_cast<D2RL::DataTables::Bank>(bank),id,&view)!=D2RL::DataTables::Result::Success||
       !D2RL::DataTables::HasTableViewField(&view,D2RL::DataTables::TableViewRequiredSize)||view.tableId!=id||
       static_cast<unsigned>(view.bank)!=bank||view.rowSize!=stride||(!allowEmpty&&!view.rowCount)||view.rowCount>65536||
       !view.revision||(revision&&view.revision!=revision)||(view.rowCount&&!Readable(view.rows,static_cast<std::size_t>(view.rowCount)*stride)))return {};
    revision=view.revision;return DefinitionTable{static_cast<const unsigned char*>(view.rows),view.rowCount,stride};
}
const unsigned char* SocketUnit(const unsigned char* host,const Contributions::Identity& identity) {
    const auto* root=Affixes::Read<const unsigned char*>(host,0x88);
    if(!Readable(root,0xb8))return nullptr;
    const auto* child=Affixes::Read<const unsigned char*>(root,0x90);
    std::vector<const unsigned char*> seen;
    while(child){
        if(!Readable(child,0xb8)||seen.size()>=64||std::find(seen.begin(),seen.end(),child)!=seen.end())return nullptr;
        seen.push_back(child);
        if(Contributions::ExtendedFlags(Affixes::Read<unsigned>(child,0x1c))){
            const auto* owner=Affixes::Read<const unsigned char*>(child,0xa0);
            if(Readable(owner,0x1be)&&Affixes::Read<unsigned>(owner,0)==4&&
               Affixes::Read<unsigned>(owner,8)==identity.id&&Affixes::Read<unsigned>(owner,4)==identity.classId&&
               Affixes::Read<const unsigned char*>(owner,0x88)==child&&Affixes::Read<const unsigned char*>(child,0x78)==root){
                const auto* data=Affixes::Read<const unsigned char*>(owner,0x10);
                if(reinterpret_cast<std::uintptr_t>(data)<0x1001c||!Readable(data-28,0x64)||owner[0x1bd]!=identity.bank||
                   Affixes::Read<unsigned>(data,0)!=identity.quality||Affixes::Read<unsigned>(data,0x18)!=identity.flags||
                   Affixes::Read<std::uint16_t>(data,0x40)!=identity.version||Affixes::Read<int>(data,0x34)!=identity.qualityRecord||
                   Affixes::Read<unsigned>(data-28,0)!=identity.autoId||std::memcmp(data-24,identity.affixes.data(),24))return nullptr;
                return owner;
            }
        }
        child=Affixes::Read<const unsigned char*>(child,0x68);
    }
    return nullptr;
}
std::optional<std::vector<Affixes::Rolled>> SocketDefinitions(const unsigned char* host,
    const Contributions::Identity& identity,std::span<const Affixes::Row> rows,std::size_t prefixIndex,std::size_t autoIndex,
    std::span<const Affixes::Property> properties) {
    if(identity.quality!=4&&identity.quality!=6)return {};
    const auto* unit=SocketUnit(host,identity);if(!unit)return {};
    std::vector<Affixes::Rolled> out;
    for(const auto id:identity.affixes){
        if(!id)continue;if(id>autoIndex)return {};
        if(std::any_of(out.begin(),out.end(),[&](const Affixes::Rolled& row){return row.id==id;}))return {};
        const auto index=static_cast<std::size_t>(id-1);const bool prefix=index>=prefixIndex;
        const auto begin=prefix?prefixIndex:0,end=prefix?autoIndex:prefixIndex;
        const auto& row=rows[index];
        const auto tier=Affixes::Tier(row,rows.subspan(begin,end-begin),identity.quality!=4,identity.version,
            [&](const Affixes::Row& candidate){return eligibleAffix(const_cast<unsigned char*>(unit),&candidate)!=0;},
            scalingReady.load()?properties:std::span<const Affixes::Property>{});
        out.push_back({id,row,prefix,tier});
    }
    if(identity.autoId)return {};
    return out;
}
Affixes::Rolled SpecBlock(std::span<const ContributionDefinitions::Spec> specs,const char* name) {
    Affixes::Rolled out{};out.source=name;const int absent=-1;
    for(unsigned n=0;n<3;++n)std::memcpy(out.row.bytes.data()+0x24+n*16,&absent,4);
    for(std::size_t n=0;n<specs.size()&&n<3;++n){const auto& s=specs[n];auto* p=out.row.bytes.data()+0x24+n*16;
        std::memcpy(p,&s.property,4);std::memcpy(p+4,&s.parameter,4);std::memcpy(p+8,&s.low,4);std::memcpy(p+12,&s.high,4);}
    return out;
}
std::optional<std::vector<Affixes::Rolled>> ReadWordDefinitions(const Contributions::Snapshot& capture,
    std::span<const Affixes::Property> properties,std::span<const Affixes::PropertyGroup> groups) {
    if(!propertySourcesReady.load() || !(capture.identity.flags&0x04000000u))return {};
    std::uint64_t revision{};const auto bank=capture.identity.bank;
    const auto p=ContributionTable(D2RL::DataTables::TableId::Properties,bank,sizeof(Affixes::Property),revision);
    const auto g=groups.empty()?std::optional<DefinitionTable>{DefinitionTable{nullptr,0,sizeof(Affixes::PropertyGroup)}}:
        ContributionTable(D2RL::DataTables::TableId::PropertyGroups,bank,sizeof(Affixes::PropertyGroup),revision,true);
    if(!p || !g || p->rows!=reinterpret_cast<const unsigned char*>(properties.data()) || p->count!=properties.size() ||
       (g->count && g->rows!=reinterpret_cast<const unsigned char*>(groups.data())) || g->count!=groups.size())return {};
    const auto items=ContributionTable(D2RL::DataTables::TableId::Items,bank,448,revision);
    const auto types=ContributionTable(D2RL::DataTables::TableId::ItemTypes,bank,232,revision);
    const auto runes=ContributionTable(D2RL::DataTables::TableId::Runes,bank,232,revision);
    const auto* table=getTables(static_cast<unsigned char>(bank));
    if(!items || !types || !runes || !Readable(table,0x1870) ||
       g->count!=Affixes::Read<std::uint64_t>(table,0x260) ||
       Contributions::Value(capture.total,194)!=static_cast<std::int64_t>(capture.sockets.size()) ||
       runes->rows!=Affixes::Read<const unsigned char*>(table,0x1860) || runes->count!=Affixes::Read<std::uint64_t>(table,0x1868))return {};
    const auto specs=RunewordSources::Resolve(*runes,*items,*types,capture.identity.classId,capture.identity.quality,
        capture.identity.flags,capture.sockets,properties.size(),groups.size());
    if(!specs)return {};
    std::vector<Affixes::Rolled> out;
    for(const auto& spec:*specs){unsigned char block[16]{};
        std::memcpy(block,&spec.property,4);std::memcpy(block+4,&spec.parameter,4);
        std::memcpy(block+8,&spec.low,4);std::memcpy(block+12,&spec.high,4);
        AddUniqueSource(block,reinterpret_cast<const unsigned char*>(groups.data()),groups.size(),properties.size(),out,0,true,"[RW]");
    }
    return out.empty()?std::optional<std::vector<Affixes::Rolled>>{}:out;
}
bool HasRecipeExtras(const Contributions::Snapshot& capture){
    for(unsigned stat:{361u,362u,392u,393u,394u,395u})if(Contributions::Value(capture.own,stat))return true;
    return false;
}
struct SourceGroups {
    Contributions::Snapshot capture;std::vector<ContributionModel::Group> groups;
    std::vector<Affixes::Rolled> corruption;std::deque<std::string> names;
};
std::optional<SourceGroups> GatherSourceGroups(void* source,std::span<const Affixes::Rolled> hostDefinitions,
    std::span<const Affixes::Row> rows,std::size_t prefixIndex,std::size_t autoIndex,
    std::span<const Affixes::Property> properties) {
    auto capture=CaptureItemContributions(source);
    if(!capture)return {};
    for(unsigned stat:{392u,393u,394u,395u})if(Contributions::Value(capture->own,stat))return {};
    const bool corrupted=Contributions::Value(capture->own,361)||Contributions::Value(capture->own,362);
    if(capture->sockets.empty()&&!corrupted)return {};
    SourceGroups out;out.capture=std::move(*capture);out.groups.reserve(8);
    out.groups.push_back({out.capture.own,{hostDefinitions.begin(),hostDefinitions.end()},""});
    if(out.capture.identity.flags&0x04000000u){
        std::vector<Affixes::Rolled> word;
        for(const auto& d:hostDefinitions)if(d.source && std::string_view(d.source)=="[RW]")word.push_back(d);
        if(word.empty())return {};
        auto subtraction=out.capture.word;for(auto& entry:subtraction)entry.value=-entry.value;
        if(!Contributions::Add(out.groups[0].values,subtraction))return {};
        std::erase_if(out.groups[0].definitions,[](const auto& d){return d.source && std::string_view(d.source)=="[RW]";});
        out.groups.push_back({out.capture.word,std::move(word),""});
    }
    std::uint64_t revision{};
    const auto propertyView=ContributionTable(D2RL::DataTables::TableId::Properties,out.capture.identity.bank,sizeof(Affixes::Property),revision);
    const bool metadata=propertyView&&propertyView->rows==reinterpret_cast<const unsigned char*>(properties.data())&&propertyView->count==properties.size();
    const auto items=metadata?ContributionTable(D2RL::DataTables::TableId::Items,out.capture.identity.bank,448,revision):std::optional<DefinitionTable>{};
    const auto types=metadata?ContributionTable(D2RL::DataTables::TableId::ItemTypes,out.capture.identity.bank,232,revision):std::optional<DefinitionTable>{};
    const auto gems=metadata?ContributionTable(D2RL::DataTables::TableId::Gems,out.capture.identity.bank,192,revision):std::optional<DefinitionTable>{};
    if(corrupted){
        if(!items||!types)return {};
        const auto cube=ContributionTable(D2RL::DataTables::TableId::CubeMain,out.capture.identity.bank,364,revision);if(!cube)return {};
        unsigned flag=UINT_MAX,dummy=UINT_MAX;
        for(std::size_t n=0;n<properties.size();++n){const auto& p=properties[n];if(p.bytes[0x18]!=1)continue;
            const auto target=Affixes::Read<std::uint16_t>(p.bytes.data(),0x20);
            if(target==361){if(flag!=UINT_MAX)return {};flag=static_cast<unsigned>(n);}
            if(target==362){if(dummy!=UINT_MAX)return {};dummy=static_cast<unsigned>(n);}}
        if(flag==UINT_MAX||dummy==UINT_MAX)return {};
        const auto outcome=ContributionDefinitions::ResolveCorruption(*cube,*items,*types,out.capture.identity.classId,
            out.capture.identity.quality,Contributions::Value(out.capture.own,361),Contributions::Value(out.capture.own,362),flag,dummy);
        if(!outcome||!outcome->recognized||outcome->conversion||outcome->specs.size()>3)return {};
        if(!outcome->specs.empty())out.corruption.push_back(SpecBlock(outcome->specs,"[C]"));
    }
    for(std::size_t index=0;index<out.capture.sockets.size();++index){
        const auto& socket=out.capture.sockets[index];std::vector<Affixes::Rolled> definitions;
        if(socket.identity.quality==4||socket.identity.quality==6){
            const auto found=SocketDefinitions(static_cast<const unsigned char*>(source),socket.identity,rows,prefixIndex,autoIndex,properties);
            if(!found)return {};definitions=*found;
        } else if(socket.identity.quality==7){
            const auto* table=getTables(static_cast<unsigned char>(socket.identity.bank));if(!Readable(table,0x13d8))return {};
            const auto* unique=Affixes::Read<const unsigned char*>(table,0x13c8);
            const auto count=Affixes::Read<std::uint64_t>(table,0x13d0);
            const auto* propertyGroups=Affixes::Read<const unsigned char*>(table,0x258);
            const auto groupCount=Affixes::Read<std::uint64_t>(table,0x260);
            const int record=socket.identity.qualityRecord;
            if(record<0||count>32768||static_cast<std::uint64_t>(record)>=count||groupCount>32768||
               !Readable(unique,static_cast<std::size_t>(count)*0x15c)||(groupCount&&!Readable(propertyGroups,static_cast<std::size_t>(groupCount)*0xc8)))return {};
            for(unsigned n=0;n<12;++n)AddUniqueSource(unique+static_cast<std::size_t>(record)*0x15c+0x98+n*16,
                propertyGroups,static_cast<std::size_t>(groupCount),properties.size(),definitions);
        } else {
            if(!items||!types||!gems)return {};
            const auto context=ContributionDefinitions::Context(*items,*types,out.capture.identity.classId);if(!context)return {};
            const auto specs=ContributionDefinitions::Gem(*gems,socket.identity.classId,*context);if(!specs)return {};
            const unsigned char* named=nullptr;
            for(std::size_t n=0;n<gems->count;++n)if(Affixes::Read<unsigned>(gems->Row(n),0x28)==socket.identity.classId)named=gems->Row(n);
            if(!named)return {};const auto length=strnlen(reinterpret_cast<const char*>(named),32);if(!length||length==32)return {};
            std::string name(reinterpret_cast<const char*>(named),length);
            if(std::any_of(name.begin(),name.end(),[](char c){return static_cast<unsigned char>(c)<32||c=='['||c==']';}))return {};
            out.names.push_back("["+name+"]");definitions.push_back(SpecBlock(*specs,out.names.back().c_str()));
        }
        out.groups.push_back({socket.values,std::move(definitions),"[Socket "+std::to_string(index+1)+"] "});
    }
    return out;
}
void AddOwnedSourceLabels(const SourceGroups& sources,std::vector<CapturedLine> observed,std::string_view actual,
    std::span<const Affixes::Property> properties,const unsigned char* itemStats,std::size_t statCount,
    Affixes::LayerEncoding encoding,std::span<const Affixes::PropertyGroup> groups,std::vector<RangeText::Label>& labels) {
    std::vector<Affixes::Rolled> all;
    for(const auto& group:sources.groups)all.insert(all.end(),group.definitions.begin(),group.definitions.end());
    all.insert(all.end(),sources.corruption.begin(),sources.corruption.end());
    AddUnobservedCompositeIdentities(observed,actual,all,properties,encoding,groups,itemStats,statCount);
    std::vector<std::string> handled;
    for(const auto& line:observed){
        if(line.layer||line.grouped||line.stat<0||static_cast<std::size_t>(line.stat)>=statCount||
           std::find(handled.begin(),handled.end(),line.key)!=handled.end())continue;
        handled.push_back(line.key);unsigned stat=static_cast<unsigned>(line.stat);
        const auto* metadata=itemStats+stat*0x144;if(Affixes::Read<std::uint16_t>(metadata,0)!=stat)continue;
        const auto shift=metadata[0x14];if(shift>16)continue;
        const bool paired=stat>=48&&stat<=55;
        if(!paired&&Affixes::Read<std::uint16_t>(metadata,0x32)!=19)continue;
        if(paired){stat&=~1u;if(stat+1>=statCount||itemStats[(stat+1)*0x144+0x14]!=shift)continue;}
        auto input=sources.groups;
        const auto corruption=ContributionModel::Exact(sources.corruption,properties,stat);
        const auto maxCorruption=paired?ContributionModel::Exact(sources.corruption,properties,stat+1):std::optional<int>{0};
        if(!corruption||!maxCorruption)continue;
        bool external=*corruption!=0||*maxCorruption!=0;
        for(std::size_t n=1;n<input.size();++n)external|=Contributions::Value(input[n].values,stat)!=0||
            (paired&&Contributions::Value(input[n].values,stat+1)!=0);
        if(!external)continue;
        if(stat==17||stat==18){
            bool agrees=true;for(const auto& g:input)agrees&=Contributions::Value(g.values,17)==Contributions::Value(g.values,18);
            if(!agrees)continue;
        } else if(stat!=16 && !(stat>=21&&stat<=24) && stat!=31){
            long long sum=0,maxSum=0;for(const auto& g:input){sum+=Contributions::Value(g.values,stat);if(paired)maxSum+=Contributions::Value(g.values,stat+1);}
            if(sum!=Contributions::Value(sources.capture.total,stat)||(paired&&maxSum!=Contributions::Value(sources.capture.total,stat+1)))continue;
        }
        if(*corruption||*maxCorruption){
            const auto factor=std::int64_t{1}<<shift;
            Contributions::Stats values{{std::uint64_t{stat}<<32,*corruption*factor}};
            if(paired)values.push_back({std::uint64_t{stat+1}<<32,*maxCorruption*factor});
            auto subtract=values;for(auto& value:subtract)value.value=-value.value;
            if(!Contributions::Add(input[0].values,subtract))continue;
            input.push_back({std::move(values),sources.corruption,""});
        }
        const auto label=ContributionModel::Build(line.key,stat,shift,input,properties,paired);if(!label)continue;
        unsigned matches=0;bool renders=false;
        for(const auto text:RangeText::Lines(actual))if(RangeText::Analyze(text).key==line.key){++matches;renders=!RangeText::ExpandOwned(text,*label).empty();}
        if(matches!=1||!renders)continue;
        labels.erase(std::remove_if(labels.begin(),labels.end(),[&](const RangeText::Label& old){return old.key==line.key;}),labels.end());labels.push_back(*label);
    }
}
void AddLinkedSourceLabels(const SourceGroups& sources,std::span<const CapturedLine> observed,std::string_view actual,
    std::span<const Affixes::Property> properties,const unsigned char* metadata,std::size_t count,
    std::vector<RangeText::Label>& labels,std::span<const Affixes::PropertyGroup> propertyGroups={}) {
    // Corruption retains its established independent reconstruction. A future
    // linked solve must include that operation before it can use its totals.
    if(!sources.corruption.empty() || sources.groups.empty() || sources.groups.size()>8)return;
    const Affixes::LayerEncoding selectorEncoding{6,63,randomSkillsReady.load()};
    const auto normalize=[&](unsigned stat,std::int64_t raw)->std::optional<int>{
        if(stat>=count)return {};const auto* row=metadata+stat*0x144;
        if(Affixes::Read<std::uint16_t>(row,0)!=stat || !PropertyRolls::ItemScalarOperand(row))return {};
        return Contributions::Normalize(raw,row[0x14]);
    };
    std::vector<PropertyRolls::Solution> solved;
    for(const auto& group:sources.groups)solved.push_back(PropertyRolls::SolveSources(group.definitions,properties,group.values,normalize,propertyGroups));
    std::set<std::string> handled;
    for(const auto& line:observed){
        if(line.stat<0 || static_cast<std::size_t>(line.stat)>=count || !handled.insert(line.key).second)continue;
        const unsigned stat=static_cast<unsigned>(line.stat);std::vector<unsigned> members;
        if(!line.layer && !line.grouped && stat>=48 && stat<=55){
            const unsigned minimum=stat&~1u,maximum=minimum+1;
            if(maximum>=count || metadata[minimum*0x144+0x14]!=metadata[maximum*0x144+0x14] ||
               metadata[minimum*0x144+0x50] || metadata[maximum*0x144+0x50])continue;
            long long low=0,high=0;for(const auto& g:sources.groups){low+=Contributions::Value(g.values,minimum);high+=Contributions::Value(g.values,maximum);}
            if(low!=Contributions::Value(sources.capture.total,minimum) || high!=Contributions::Value(sources.capture.total,maximum))continue;
            const auto paired=ContributionModel::Build(line.key,minimum,metadata[minimum*0x144+0x14],sources.groups,properties,true);
            if(!paired)continue;unsigned matches=0;bool renders=false;
            for(const auto text:RangeText::Lines(actual))if(RangeText::Analyze(text).key==line.key){++matches;renders=!RangeText::ExpandOwned(text,*paired).empty();}
            if(matches==1 && renders){std::erase_if(labels,[&](const auto& old){return old.key==line.key;});labels.push_back(*paired);}continue;
        }
        const bool selector=!line.grouped && std::any_of(sources.groups.begin(),sources.groups.end(),[&](const auto& g){
            return std::any_of(g.definitions.begin(),g.definitions.end(),[&](const auto& d){return line.layer>=0 && PropertyRolls::LayerBounds(d,properties,stat,static_cast<unsigned>(line.layer),selectorEncoding.randomSkills).has_value();});});
        if(line.grouped){
            if(!line.group || line.layer)continue;
            for(std::size_t member=0;member<count;++member)
                if(Affixes::Read<std::uint16_t>(metadata+member*0x144,0x3a)==line.group)members.push_back(static_cast<unsigned>(member));
            if(members.size()<2 || members.size()>7)continue;
        }
        const auto total=line.layer?Contributions::Normalize(Contributions::Value(sources.capture.total,stat,static_cast<unsigned>(line.layer)),metadata[stat*0x144+0x14]):
            normalize(stat,Contributions::Value(sources.capture.total,stat));
        if(!total)continue;
        for(auto member:members)if(normalize(member,Contributions::Value(sources.capture.total,member))!=total){members.clear();break;}
        if(line.grouped && members.empty())continue;
        RangeText::Label label;label.key=line.key;label.ownedTotal=*total;long long sum=0;bool valid=true;
        for(std::size_t n=0;n<sources.groups.size();++n){
            const auto& group=sources.groups[n];
            if(line.layer || selector){
                // Only selector functions with verified additive semantics;
                // packed proc/charge layers stay native.
                if(line.layer<0 || metadata[stat*0x144+0x50]){valid=false;break;}
                const auto value=normalize(stat,Contributions::Value(group.values,stat,static_cast<unsigned>(line.layer)));
                if(!value){valid=false;break;}sum+=*value;
                bool proved=false;
                const auto part=PropertyRolls::LayerLabel(group.definitions,properties,group.values,stat,
                    static_cast<unsigned>(line.layer),line.key,[&](std::int64_t raw){return normalize(stat,raw);},
                    selectorEncoding.randomSkills,proved,propertyGroups);
                if(!proved){valid=false;break;}
                if(part)for(auto p:part->ownedParts){p.label=group.prefix+p.label;label.ownedParts.push_back(std::move(p));}
                continue;
            }
            const auto value=normalize(stat,Contributions::Value(group.values,stat));if(!value){valid=false;break;}sum+=*value;
            const auto part=PropertyRolls::Label(solved[n],line.key,stat,*value,members);
            if(!part){
                // Zero without a definition can be an empty source; a refused
                // or blocked defined stat is never silently dropped.
                if(*value || solved[n].model.terms.contains(stat) || solved[n].model.blocked.contains(stat)){valid=false;break;}
                continue;
            }
            for(auto p:part->ownedParts){p.label=group.prefix+p.label;label.ownedParts.push_back(std::move(p));}
        }
        if(!valid || sum!=*total || label.ownedParts.empty() || label.ownedParts.size()>12)continue;
        if(stat==253 && !line.grouped && !line.layer){if(!quantityRateReady.load())continue;label.quantityRate=true;}
        unsigned matches=0;bool renders=false;
        for(const auto text:RangeText::Lines(actual))if(RangeText::Analyze(text).key==line.key){++matches;renders=!RangeText::ExpandOwned(text,label).empty();}
        if(matches!=1 || !renders)continue;
        std::erase_if(labels,[&](const RangeText::Label& old){return old.key==line.key;});labels.push_back(std::move(label));
    }
}
