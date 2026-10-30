// Included inside the plugin's private namespace after its native helpers.
using DefinitionTable=ContributionDefinitions::Table;
std::optional<Contributions::Snapshot> CaptureItemContributions(const void* source) {
    if(!contributionsReady.load())return {};
    return Contributions::Capture(reinterpret_cast<std::uintptr_t>(source),[](std::uintptr_t at,void* out,std::size_t size){
        const auto* ptr=reinterpret_cast<const void*>(at);if(!Readable(ptr,size))return false;std::memcpy(out,ptr,size);return true;},propertySourcesReady.load()?171u:0u,propertySourcesReady.load());
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
    for(unsigned stat:{361u,362u,392u,393u,394u,395u,419u,420u})if(Contributions::Value(capture.own,stat))return true;
    return false;
}
// The permanent set formatter displays owned modifiers separately from both
// base defense and activated set states. Use that same source scope for solving.
bool PermanentSetTotals(Contributions::Snapshot& capture) {
    if(capture.identity.quality!=5)return true;
    capture.total=capture.own;
    for(const auto& socket:capture.sockets)if(!Contributions::Add(capture.total,socket.values))return false;
    return true;
}
struct SourceGroups {
    Contributions::Snapshot capture;std::vector<ContributionModel::Group> groups;
    std::vector<Affixes::Rolled> corruption;std::vector<Affixes::Rolled> craft;std::vector<Affixes::Rolled> enchantment;
    std::deque<std::string> names;bool enchanted{},unresolvedCorruption{};
};
std::optional<SourceGroups> GatherSourceGroups(void* source,std::span<const Affixes::Rolled> hostDefinitions,
    std::span<const Affixes::Row> rows,std::size_t prefixIndex,std::size_t autoIndex,
    std::span<const Affixes::Property> properties) {
    auto capture=CaptureItemContributions(source);
    if(!capture || !PermanentSetTotals(*capture))return {};
    long long upgradeTotal=0;unsigned activeUpgradeStat=0;
    for(unsigned stat:{392u,393u,394u,395u,419u,420u}){
        const auto count=Contributions::Value(capture->own,stat);
        if(count<0)return {};
        if(count>0){upgradeTotal+=count;activeUpgradeStat=stat;}
    }
    const bool enchanted=upgradeTotal>0;
    if(enchanted && !propertySourcesReady.load())return {};
    const bool corrupted=Contributions::Value(capture->own,361)||Contributions::Value(capture->own,362);
    const bool maybeCraft=(capture->identity.quality==4||capture->identity.quality==6||capture->identity.quality==8)&&
        !(capture->identity.flags&0x04000000u)&&propertySourcesReady.load();
    if(capture->sockets.empty()&&!corrupted&&!enchanted&&!maybeCraft)return {};
    SourceGroups out;out.capture=std::move(*capture);out.groups.reserve(8);out.enchanted=enchanted;
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
        bool resolved=false;
        if(items&&types){
            if(const auto cube=ContributionTable(D2RL::DataTables::TableId::CubeMain,out.capture.identity.bank,364,revision)){
                unsigned flag=UINT_MAX,dummy=UINT_MAX;
                for(std::size_t n=0;n<properties.size();++n){const auto& p=properties[n];if(p.bytes[0x18]!=1)continue;
                    const auto target=Affixes::Read<std::uint16_t>(p.bytes.data(),0x20);
                    if(target==361)flag=flag==UINT_MAX?static_cast<unsigned>(n):UINT_MAX-1;
                    if(target==362)dummy=dummy==UINT_MAX?static_cast<unsigned>(n):UINT_MAX-1;}
                if(flag<UINT_MAX-1 && dummy<UINT_MAX-1){
                    const auto outcome=ContributionDefinitions::ResolveCorruption(*cube,*items,*types,out.capture.identity.classId,
                        out.capture.identity.quality,Contributions::Value(out.capture.own,361),Contributions::Value(out.capture.own,362),flag,dummy);
                    if(outcome && outcome->recognized && !outcome->conversion && outcome->specs.size()<=3){
                        resolved=true;
                        if(!outcome->specs.empty()){
                            auto block=SpecBlock(outcome->specs,"[C]");
                            block.uniqueScalar=true;block.unconditionalScalar=true;
                            out.corruption.push_back(block);
                        }
                    }
                }
            }
        }
        if(!resolved){
            if(!propertySourcesReady.load())return {};
            out.unresolvedCorruption=true;
        }
    }
    auto defStatBounds=[&](const Affixes::Rolled& d,unsigned stat,unsigned layer,int& lo,int& hi)->bool{
        if(layer || stat==83){
            const auto b=PropertyRolls::LayerBounds(d,properties,stat,layer);
            if(!b)return false;
            lo=b->first;hi=b->second;return true;
        }
        if(ContributionModel::Bounds(d,properties,static_cast<int>(stat),lo,hi))return true;
        if((stat>=21 && stat<=24) || (stat>=48 && stat<=55)){
            if(!d.unconditionalScalar || (d.source&&(std::string_view(d.source)=="[U]"||std::string_view(d.source)=="[RW]")&&!d.uniqueScalar))return false;
            auto plain=d;plain.source=nullptr;
            const int minStat=(stat>=21 && stat<=24)?((stat<=22)?21:23):static_cast<int>(stat&~1u);
            const int maxStat=minStat+1;
            int mnLo{},mnHi{},mxLo{},mxHi{};
            if(!Affixes::DamageEndpointBounds(plain,properties,minStat,maxStat,mnLo,mnHi,mxLo,mxHi,true))return false;
            lo=(static_cast<int>(stat)==minStat)?mnLo:mxLo;
            hi=(static_cast<int>(stat)==minStat)?mnHi:mxHi;
            return true;
        }
        return false;
    };
    auto sumStatBounds=[&](std::span<const Affixes::Rolled> defs,unsigned stat,unsigned layer,long long& totalLo,long long& totalHi)->bool{
        totalLo=0;totalHi=0;
        for(const auto& d:defs){
            if(!Affixes::Contributes(d.row,properties,static_cast<int>(stat),static_cast<int>(layer)))continue;
            int lo{},hi{};
            if(!defStatBounds(d,stat,layer,lo,hi))return false;
            totalLo+=lo;totalHi+=hi;
        }
        return true;
    };
    if(maybeCraft && items && types){
        std::vector<Affixes::Rolled> baseDefs=out.groups[0].definitions;
        baseDefs.insert(baseDefs.end(),out.corruption.begin(),out.corruption.end());
        bool hasUnexplained=out.capture.identity.quality==8;
        if(!hasUnexplained){
            for(const auto& entry:out.groups[0].values){
                if(entry.value<=0)continue;
                const unsigned stat=static_cast<unsigned>(entry.key>>32);
                const unsigned layer=static_cast<unsigned>(entry.key&0xffffffffu);
                if(stat==83 && layer<8){
                    long long bLo=0,bHi=0;
                    if(sumStatBounds(baseDefs,83,layer,bLo,bHi) && entry.value>bHi){hasUnexplained=true;break;}
                    continue;
                }
                if(layer!=0 || stat==7 || stat==9 || stat==11 || (stat>=21 && stat<=24) || stat==31 ||
                   (stat>=48 && stat<=59) || (stat>=216 && stat<=250) ||
                   (stat>=72 && stat<=75) || stat==194 || (stat>=359 && stat<=363) || (stat>=391 && stat<=398) || (stat>=404 && stat<=412) || (stat>=417 && stat<=420))continue;
                long long bLo=0,bHi=0;
                if(sumStatBounds(baseDefs,stat,layer,bLo,bHi) && entry.value>bHi){hasUnexplained=true;break;}
            }
        }
        if(hasUnexplained){
            if(const auto cube=ContributionTable(D2RL::DataTables::TableId::CubeMain,out.capture.identity.bank,364,revision)){
                if(const auto candidates=ContributionDefinitions::ResolveCraftCandidates(*cube,*items,*types,properties,
                    out.capture.identity.classId,out.capture.identity.quality)){
                    std::vector<Affixes::Rolled> bestCand;unsigned bestExplained=0,bestCount=0;
                    for(const auto& candidate:*candidates){
                        std::vector<Affixes::Rolled> candDefs;
                        for(std::size_t i=0;i<candidate.specs.size();i+=3){
                            std::vector<ContributionDefinitions::Spec> chunk(
                                candidate.specs.begin()+static_cast<std::ptrdiff_t>(i),
                                candidate.specs.begin()+static_cast<std::ptrdiff_t>(std::min(candidate.specs.size(),i+3)));
                            auto block=SpecBlock(chunk,"[Craft]");
                            block.uniqueScalar=true;block.unconditionalScalar=true;
                            candDefs.push_back(block);
                        }
                        bool valid=true;unsigned explained=0;
                        std::set<std::uint64_t> checkedKeys;
                        for(const auto& spec:candidate.specs){
                            if(spec.property>=properties.size()){valid=false;break;}
                            const auto& p=properties[spec.property].bytes;
                            const bool groupFn3=p[0x18]==1 && p[0x19]==3;
                            std::optional<long long> groupResidual;
                            bool specHasTarget=false;
                            for(unsigned slot=0;slot<7;++slot){
                                const unsigned fn=p[0x18+slot];if(!fn)continue;
                                if(fn==17 && slot>0 && (spec.parameter==0 || Affixes::Read<std::uint16_t>(p.data(),0x20+slot*2)==160))continue;
                                std::vector<unsigned> targets;unsigned layer=0;
                                if(fn==7)targets={17u,18u};
                                else if(fn==5)targets={Contributions::Value(out.groups[0].values,21)?21u:23u};
                                else if(fn==6)targets={Contributions::Value(out.groups[0].values,22)?22u:24u};
                                else{
                                    const unsigned t=Affixes::Read<std::uint16_t>(p.data(),0x20+slot*2);
                                    if(t>=4096){valid=false;break;}
                                    if(fn==15 && t==21 && !Contributions::Value(out.groups[0].values,21) && Contributions::Value(out.groups[0].values,23))targets={23u};
                                    else if(fn==16 && t==22 && !Contributions::Value(out.groups[0].values,22) && Contributions::Value(out.groups[0].values,24))targets={24u};
                                    else targets={t};
                                    if(fn==21)layer=Affixes::Read<std::uint16_t>(p.data(),0x0a+slot*2);
                                    else if(fn==10 && spec.parameter>=0 && spec.parameter<24)layer=static_cast<unsigned>(spec.parameter/3*8+spec.parameter%3);
                                    else if((fn==22 || fn==24) && spec.parameter>=0 && spec.parameter<=65535)layer=static_cast<unsigned>(spec.parameter);
                                    else if(fn!=1 && fn!=2 && fn!=3 && fn!=8 && fn!=15 && fn!=16 && fn!=17){valid=false;break;}
                                }
                                for(unsigned target:targets){
                                    specHasTarget=true;
                                    const unsigned shift=(target==7 || target==9)?8u:0u;
                                    const auto raw=Contributions::Value(out.groups[0].values,target,layer);
                                    const auto actualVal=Contributions::Normalize(raw,shift);
                                    if(!actualVal || *actualVal<=0){valid=false;break;}
                                    long long cLo=0,cHi=0;
                                    if(!sumStatBounds(candDefs,target,layer,cLo,cHi) || cHi<=0){valid=false;break;}
                                    long long bLo=0,bHi=0;
                                    const bool bKnown=sumStatBounds(baseDefs,target,layer,bLo,bHi);
                                    if(bKnown){
                                        if(*actualVal<bLo+cLo){valid=false;break;}
                                        if(!enchanted && !out.unresolvedCorruption && *actualVal>bHi+cHi){valid=false;break;}
                                        if(groupFn3 && !enchanted && !out.unresolvedCorruption && bLo==bHi){
                                            const long long res=*actualVal-bLo;
                                            if(!groupResidual)groupResidual=res;
                                            else if(*groupResidual!=res){valid=false;break;}
                                        }
                                    }
                                    const std::uint64_t key=(std::uint64_t{target}<<32)|layer;
                                    if(checkedKeys.insert(key).second && (!bKnown || *actualVal>bHi))++explained;
                                }
                                if(!valid)break;
                            }
                            if(!valid || !specHasTarget){valid=false;break;}
                        }
                        if(valid && explained>0){
                            if(explained>bestExplained){bestExplained=explained;bestCount=1;bestCand=std::move(candDefs);}
                            else if(explained==bestExplained)++bestCount;
                        }
                    }
                    if(bestExplained>0 && bestCount==1)out.craft=std::move(bestCand);
                }
            }
        }
    }
    if(enchanted && (upgradeTotal==1 || (upgradeTotal>1 && upgradeTotal<=5 && (activeUpgradeStat==419 || activeUpgradeStat==420))) && items && types && !out.unresolvedCorruption){
        if(const auto cube=ContributionTable(D2RL::DataTables::TableId::CubeMain,out.capture.identity.bank,364,revision)){
            if(const auto candidates=ContributionDefinitions::ResolveEnchantmentCandidates(*cube,*items,*types,properties,
                out.capture.identity.classId,out.capture.identity.quality,activeUpgradeStat,out.capture.identity.qualityRecord,upgradeTotal)){
                std::vector<Affixes::Rolled> baseDefs=out.groups[0].definitions;
                baseDefs.insert(baseDefs.end(),out.corruption.begin(),out.corruption.end());
                const Affixes::LayerEncoding selectorEncoding{6,63,randomSkillsReady.load()};
                std::optional<Affixes::Rolled> matched,fallbackMatched;
                unsigned bestExplained=0,matches=0,fallbackMatches=0;
                std::vector<std::vector<ContributionDefinitions::Spec>> tiedSpecs,fallbackTiedSpecs;
                baseDefs.insert(baseDefs.end(),out.craft.begin(),out.craft.end());
                std::vector<Affixes::Rolled> allEnchantDefs;
                allEnchantDefs.reserve(candidates->size());
                for(const auto& candidate:*candidates)allEnchantDefs.push_back(SpecBlock(candidate.specs,"[E]"));
                for(const auto& candidate:*candidates){
                    auto block=SpecBlock(candidate.specs,"[E]");
                    block.uniqueScalar=true;block.unconditionalScalar=true;
                    const std::array<Affixes::Rolled,1> candDefs{block};
                    bool allPresent=true;unsigned explained=0;
                    std::set<std::uint64_t> checkedKeys;
                    for(const auto& spec:candidate.specs){
                        if(spec.property>=properties.size()){allPresent=false;break;}
                        const auto& p=properties[spec.property].bytes;
                        const bool groupFn3=p[0x18]==1 && p[0x19]==3;
                        std::optional<long long> groupResidual;
                        bool specHasTarget=false;
                        for(unsigned slot=0;slot<7;++slot){
                            const auto fn=p[0x18+slot];if(!fn)continue;
                            if(fn==17 && slot>0 && (spec.parameter==0 || Affixes::Read<std::uint16_t>(p.data(),0x20+slot*2)==160))continue;
                            if(fn==20 && Affixes::Read<std::uint16_t>(p.data(),0x20+slot*2)==65535){
                                specHasTarget=true;
                                if(Contributions::Value(out.groups[0].values,152)<=0){allPresent=false;break;}
                                const bool inBase152=std::any_of(baseDefs.begin(),baseDefs.end(),[&](const auto& d){
                                    for(unsigned n=0;n<3;++n){
                                        const int id=d.row.Number(0x24+16*n);
                                        if(id>=0 && static_cast<std::size_t>(id)<properties.size()){
                                            const auto* bp=properties[static_cast<std::size_t>(id)].bytes.data();
                                            for(unsigned s=0;s<7;++s)
                                                if(bp[0x18+s]==20 || (bp[0x18+s] && Affixes::Read<std::uint16_t>(bp,0x20+s*2)==152))return true;
                                        }
                                    }
                                    return false;
                                });
                                if(checkedKeys.insert(std::uint64_t{152}<<32).second && !inBase152)++explained;
                                continue;
                            }
                            if(fn==12){
                                const unsigned t=Affixes::Read<std::uint16_t>(p.data(),0x20+slot*2);
                                if(t!=107 || !selectorEncoding.randomSkills || spec.parameter<=0 || spec.low<0 || spec.low>spec.high || spec.high>65535){allPresent=false;break;}
                                specHasTarget=true;
                                long long poolActual=0,poolBaseLo=0,poolBaseHi=0;
                                bool hasEligibleLayer=false,bPoolKnown=true;
                                for(const auto& entry:out.groups[0].values){
                                    if(entry.value<=0 || static_cast<unsigned>(entry.key>>32)!=107)continue;
                                    const unsigned lay=static_cast<unsigned>(entry.key&0xffffffffu);
                                    if(!Affixes::RandomLayerSpec(spec.parameter,spec.low,spec.high,lay))continue;
                                    poolActual+=entry.value;
                                    long long fLo=0,fHi=0;
                                    if(sumStatBounds(baseDefs,107,lay,fLo,fHi) && entry.value>=fLo+spec.parameter)hasEligibleLayer=true;
                                }
                                for(const auto& d:baseDefs){
                                    for(unsigned n=0;n<3;++n){
                                        const int id=d.row.Number(0x24+16*n);
                                        if(id<0 || static_cast<std::size_t>(id)>=properties.size())continue;
                                        const auto* bp=properties[static_cast<std::size_t>(id)].bytes.data();
                                        const int bParam=d.row.Number(0x28+16*n),bLo=d.row.Number(0x2c+16*n),bHi=d.row.Number(0x30+16*n);
                                        for(unsigned s=0;s<7;++s){
                                            const unsigned bFn=bp[0x18+s];
                                            if(!bFn || Affixes::Read<std::uint16_t>(bp,0x20+s*2)!=107)continue;
                                            if(bFn==22 && bParam>=spec.low && bParam<=spec.high){poolBaseLo+=bLo;poolBaseHi+=bHi;}
                                            else if(bFn==12 && bLo==spec.low && bHi==spec.high && bParam>0){poolBaseLo+=bParam;poolBaseHi+=bParam;}
                                            else if(bFn==12 && !(bHi<spec.low || bLo>spec.high))bPoolKnown=false;
                                        }
                                    }
                                }
                                if(!hasEligibleLayer || (bPoolKnown && (poolActual<poolBaseLo+spec.parameter || poolActual>poolBaseHi+spec.parameter))){allPresent=false;break;}
                                const std::uint64_t key=(std::uint64_t{107}<<32)|static_cast<unsigned>(spec.low);
                                if(checkedKeys.insert(key).second && (!bPoolKnown || poolActual>poolBaseHi))++explained;
                                continue;
                            }
                            std::vector<unsigned> targets;unsigned layer=0;
                            if(fn==7)targets={17u,18u};
                            else if(fn==5)targets={Contributions::Value(out.groups[0].values,21)?21u:23u};
                            else if(fn==6)targets={Contributions::Value(out.groups[0].values,22)?22u:24u};
                            else{
                                unsigned t=Affixes::Read<std::uint16_t>(p.data(),0x20+slot*2);
                                if(t>=4096){allPresent=false;break;}
                                if(fn==15 && t==21 && !Contributions::Value(out.groups[0].values,21) && Contributions::Value(out.groups[0].values,23))t=23u;
                                if(fn==16 && t==22 && !Contributions::Value(out.groups[0].values,22) && Contributions::Value(out.groups[0].values,24))t=24u;
                                targets={t};
                                if(fn==21)layer=Affixes::Read<std::uint16_t>(p.data(),0x0a+slot*2);
                                else if(fn==10 && spec.parameter>=0 && spec.parameter<24)layer=static_cast<unsigned>(spec.parameter/3*8+spec.parameter%3);
                                else if((fn==22 || fn==24) && spec.parameter>=0 && spec.parameter<=65535)layer=static_cast<unsigned>(spec.parameter);
                                else if(fn!=1 && fn!=2 && fn!=3 && fn!=8 && fn!=15 && fn!=16 && fn!=17){allPresent=false;break;}
                            }
                            for(unsigned target:targets){
                                specHasTarget=true;
                                const unsigned shift=(target==7 || target==9)?8u:0u;
                                const auto raw=Contributions::Value(out.groups[0].values,target,layer);
                                const auto actualVal=Contributions::Normalize(raw,shift);
                                if(!actualVal || *actualVal<=0){allPresent=false;break;}
                                long long cLo=0,cHi=0;
                                if(!sumStatBounds(candDefs,target,layer,cLo,cHi) || cHi<=0){allPresent=false;break;}
                                long long bLo=0,bHi=0;
                                const bool bKnown=sumStatBounds(baseDefs,target,layer,bLo,bHi);
                                if(bKnown){
                                    if(*actualVal<bLo+cLo || *actualVal>bHi+cHi){allPresent=false;break;}
                                    if(groupFn3 && bLo==bHi){
                                        const long long res=*actualVal-bLo;
                                        if(!groupResidual)groupResidual=res;
                                        else if(*groupResidual!=res){allPresent=false;break;}
                                    }
                                }
                                const bool inBase=std::any_of(baseDefs.begin(),baseDefs.end(),[&](const auto& d){
                                    return Affixes::Contributes(d.row,properties,static_cast<int>(target),static_cast<int>(layer),selectorEncoding);});
                                const std::uint64_t key=(std::uint64_t{target}<<32)|layer;
                                if(checkedKeys.insert(key).second && (!inBase || !bKnown || *actualVal>bHi))++explained;
                            }
                            if(!allPresent)break;
                        }
                        if(!allPresent || !specHasTarget){allPresent=false;break;}
                    }
                    if(allPresent){
                        for(std::size_t dIdx=0;dIdx<baseDefs.size() && allPresent;++dIdx){
                            const auto& d=baseDefs[dIdx];
                            if(!d.unconditionalScalar || (d.source && (std::string_view(d.source)=="[U]" || std::string_view(d.source)=="[RW]") && !d.uniqueScalar))continue;
                            for(unsigned n=0;n<3 && allPresent;++n){
                                const int id=d.row.Number(0x24+16*n);
                                if(id<0 || static_cast<std::size_t>(id)>=properties.size())continue;
                                const auto* bp=properties[static_cast<std::size_t>(id)].bytes.data();
                                if(bp[0x18]!=1 || bp[0x19]!=3)continue;
                                long long groupLo=d.row.Number(0x2c+16*n),groupHi=d.row.Number(0x30+16*n);
                                if(groupLo>groupHi)continue;
                                for(unsigned s=0;s<7;++s){
                                    if((bp[0x18+s]!=1 && bp[0x18+s]!=3) || bp[2+s])continue;
                                    const unsigned t=Affixes::Read<std::uint16_t>(bp,0x20+s*2);
                                    if(t>=4096)continue;
                                    bool otherSlotOnSameDef=false;
                                    for(unsigned m=0;m<3;++m){
                                        if(m==n)continue;
                                        const int mid=d.row.Number(0x24+16*m);
                                        if(mid>=0 && static_cast<std::size_t>(mid)<properties.size()){
                                            const auto* mp=properties[static_cast<std::size_t>(mid)].bytes.data();
                                            for(unsigned ms=0;ms<7;++ms)
                                                if(mp[0x18+ms] && Affixes::Read<std::uint16_t>(mp,0x20+ms*2)==t)otherSlotOnSameDef=true;
                                        }
                                    }
                                    if(otherSlotOnSameDef)continue;
                                    const unsigned shift=(t==7 || t==9)?8u:0u;
                                    const auto actualVal=Contributions::Normalize(Contributions::Value(out.groups[0].values,t,0),shift);
                                    if(!actualVal){allPresent=false;break;}
                                    long long cLo=0,cHi=0;
                                    if(!sumStatBounds(candDefs,t,0,cLo,cHi))continue;
                                    long long oLo=0,oHi=0;bool oKnown=true;
                                    for(std::size_t oIdx=0;oIdx<baseDefs.size();++oIdx){
                                        if(oIdx==dIdx || !Affixes::Contributes(baseDefs[oIdx].row,properties,static_cast<int>(t),0))continue;
                                        int obLo=0,obHi=0;
                                        if(!defStatBounds(baseDefs[oIdx],t,0,obLo,obHi)){oKnown=false;break;}
                                        oLo+=obLo;oHi+=obHi;
                                    }
                                    if(!oKnown)continue;
                                    groupLo=std::max(groupLo,*actualVal-cHi-oHi);
                                    groupHi=std::min(groupHi,*actualVal-cLo-oLo);
                                    if(groupLo>groupHi){allPresent=false;break;}
                                }
                            }
                        }
                    }
                    if(allPresent){
                        for(const auto& entry:out.groups[0].values){
                            if(entry.value<=0 || entry.value>100000)continue;
                            const unsigned st=static_cast<unsigned>(entry.key>>32);
                            const unsigned lay=static_cast<unsigned>(entry.key&0xffffffffu);
                            if(lay!=0 && !(st==83 && lay<8))continue;
                            if(st==11 || (st>=16 && st<=24) || st==31 || (st>=48 && st<=59) ||
                               st==72 || st==73 || st==75 || st==97 || st==126 || st==152 || st==194 ||
                               (st>=216 && st<=250) || (st>=359 && st<=363) || (st>=391 && st<=398) ||
                               (st>=404 && st<=412) || (st>=417 && st<=420))continue;
                            if(std::none_of(allEnchantDefs.begin(),allEnchantDefs.end(),[&](const auto& d){
                                return Affixes::Contributes(d.row,properties,static_cast<int>(st),static_cast<int>(lay),selectorEncoding);}))continue;
                            const unsigned shift=(st==7 || st==9)?8u:0u;
                            const auto actualVal=Contributions::Normalize(entry.value,shift);
                            if(!actualVal || *actualVal<=0)continue;
                            long long bLo=0,bHi=0;
                            if(!sumStatBounds(baseDefs,st,lay,bLo,bHi) || *actualVal<=bHi)continue;
                            long long cLo=0,cHi=0;
                            if(!sumStatBounds(candDefs,st,lay,cLo,cHi) || cHi<=0 || *actualVal>bHi+cHi){allPresent=false;break;}
                        }
                    }
                    if(allPresent && explained>0){
                        if(explained>bestExplained){bestExplained=explained;matches=1;matched=block;tiedSpecs={candidate.specs};}
                        else if(explained==bestExplained){++matches;tiedSpecs.push_back(candidate.specs);}
                    }else if(allPresent && explained==0){
                        ++fallbackMatches;fallbackMatched=block;fallbackTiedSpecs.push_back(candidate.specs);
                    }
                }
                auto mergeTiedSpecs=[&](const std::vector<std::vector<ContributionDefinitions::Spec>>& tied)->std::optional<Affixes::Rolled>{
                    if(tied.size()<2 || tied[0].empty() || tied[0].size()>3)return {};
                    auto merged=tied[0];
                    for(std::size_t i=1;i<tied.size();++i){
                        if(tied[i].size()!=merged.size())return {};
                        for(std::size_t k=0;k<merged.size();++k){
                            if(tied[i][k].property!=merged[k].property || tied[i][k].parameter!=merged[k].parameter)return {};
                            merged[k].low=std::min(merged[k].low,tied[i][k].low);
                            merged[k].high=std::max(merged[k].high,tied[i][k].high);
                        }
                    }
                    auto block=SpecBlock(merged,"[E]");
                    block.uniqueScalar=true;block.unconditionalScalar=true;
                    return block;
                };
                if(bestExplained>0){
                    if(matches==1 && matched)out.enchantment.push_back(*matched);
                    else if(const auto merged=mergeTiedSpecs(tiedSpecs))out.enchantment.push_back(*merged);
                }else if(fallbackMatches==1 && fallbackMatched){
                    out.enchantment.push_back(*fallbackMatched);
                }else if(fallbackMatches>1){
                    if(const auto merged=mergeTiedSpecs(fallbackTiedSpecs))out.enchantment.push_back(*merged);
                }
            }
        }
    }
    if(enchanted && out.enchantment.empty() && !out.unresolvedCorruption){
        std::vector<Affixes::Rolled> baseDefs=out.groups[0].definitions;
        baseDefs.insert(baseDefs.end(),out.corruption.begin(),out.corruption.end());
        baseDefs.insert(baseDefs.end(),out.craft.begin(),out.craft.end());
        auto findClassSkillProp=[&](unsigned layer)->std::optional<unsigned>{
            for(std::size_t id=0;id<properties.size();++id){
                const auto* p=properties[id].bytes.data();
                if(p[0x18]==21 && !p[0x19] && !p[2] && Affixes::Read<std::uint16_t>(p,0x20)==83 &&
                   Affixes::Read<std::uint16_t>(p,0x0a)==layer)
                    return static_cast<unsigned>(id);
            }
            return {};
        };
        auto findSingleProp=[&](unsigned target)->std::optional<unsigned>{
            for(std::size_t id=0;id<properties.size();++id){
                const auto* p=properties[id].bytes.data();
                if(p[0x18]==1 && !p[0x19] && !p[2] && Affixes::Read<std::uint16_t>(p,0x20)==target)
                    return static_cast<unsigned>(id);
                if(p[0x18]==21 && p[0x19]==3 && !p[0x1a] && !p[2] && !p[3] &&
                   Affixes::Read<std::uint16_t>(p,0x0a)!=0 && Affixes::Read<std::uint16_t>(p,0x22)==target)
                    return static_cast<unsigned>(id);
                if(target==16 && p[0x18]==2 && !p[0x19] && !p[2] && Affixes::Read<std::uint16_t>(p,0x20)==16)
                    return static_cast<unsigned>(id);
                if(target==17 && p[0x18]==7 && !p[0x19] && !p[2])return static_cast<unsigned>(id);
                if((target==21 || target==23) && p[0x18]==5 && !p[0x19] && !p[2])return static_cast<unsigned>(id);
                if((target==22 || target==24) && p[0x18]==6 && !p[0x19] && !p[2])return static_cast<unsigned>(id);
            }
            return {};
        };
        auto findPairProp=[&](unsigned minTarget,unsigned maxTarget)->std::optional<unsigned>{
            for(std::size_t id=0;id<properties.size();++id){
                const auto* p=properties[id].bytes.data();
                if(p[0x18]==15 && p[0x19]==16 && (!p[0x1a] || p[0x1a]==17) && !p[2] && !p[3] &&
                   Affixes::Read<std::uint16_t>(p,0x20)==minTarget && Affixes::Read<std::uint16_t>(p,0x22)==maxTarget)
                    return static_cast<unsigned>(id);
            }
            return {};
        };
        std::vector<ContributionDefinitions::Spec> residualSpecs;
        for(unsigned minStat:{48u,50u,52u,54u}){
            const unsigned maxStat=minStat+1;
            const auto actualMin=Contributions::Value(out.groups[0].values,minStat);
            const auto actualMax=Contributions::Value(out.groups[0].values,maxStat);
            if(actualMin<=0 || actualMax<=0 || actualMin>100000 || actualMax>100000)continue;
            long long baseMin=0,baseMax=0;bool fixedBase=true;
            for(const auto& d:baseDefs){
                if(!Affixes::Contributes(d.row,properties,static_cast<int>(minStat),0) &&
                   !Affixes::Contributes(d.row,properties,static_cast<int>(maxStat),0))continue;
                auto plain=d;plain.source=nullptr;
                int mnLo{},mnHi{},mxLo{},mxHi{};
                if(!d.unconditionalScalar || (d.source&&(std::string_view(d.source)=="[U]"||std::string_view(d.source)=="[RW]")&&!d.uniqueScalar) ||
                   !Affixes::DamageEndpointBounds(plain,properties,static_cast<int>(minStat),static_cast<int>(maxStat),mnLo,mnHi,mxLo,mxHi,true) ||
                   mnLo!=mnHi || mxLo!=mxHi){fixedBase=false;break;}
                baseMin+=mnLo;baseMax+=mxLo;
            }
            if(!fixedBase || actualMin<=baseMin || actualMax<=baseMax)continue;
            const int resMin=static_cast<int>(actualMin-baseMin),resMax=static_cast<int>(actualMax-baseMax);
            if(const auto pairProp=findPairProp(minStat,maxStat)){
                residualSpecs.push_back({*pairProp,0,resMin,resMax});
            }else if(const auto minProp=findSingleProp(minStat)){
                if(const auto maxProp=findSingleProp(maxStat)){
                    residualSpecs.push_back({*minProp,0,resMin,resMin});
                    residualSpecs.push_back({*maxProp,0,resMax,resMax});
                }
            }
        }
        for(const auto& entry:out.groups[0].values){
            if(entry.value<=0 || entry.value>100000)continue;
            const unsigned entryStat=static_cast<unsigned>(entry.key>>32);
            const unsigned layer=static_cast<unsigned>(entry.key&0xffffffffu);
            if(entryStat==83 && layer<8){
                long long bLo=0,bHi=0;
                if(sumStatBounds(baseDefs,83,layer,bLo,bHi) && bLo==bHi && entry.value>bLo){
                    const int res=static_cast<int>(entry.value-bLo);
                    if(const auto prop=findClassSkillProp(layer))
                        residualSpecs.push_back({*prop,0,res,res});
                }
                continue;
            }
            if(layer!=0){
                if((entryStat==107 || entryStat==387) && layer<=65535){
                    long long bLo=0,bHi=0;
                    const Affixes::LayerEncoding selectorEncoding{6,63,randomSkillsReady.load()};
                    const bool randomBase=entryStat==107 && std::any_of(baseDefs.begin(),baseDefs.end(),[&](const auto& d){
                        return !Affixes::Contributes(d.row,properties,107,static_cast<int>(layer)) &&
                            Affixes::Contributes(d.row,properties,107,static_cast<int>(layer),selectorEncoding);});
                    if(!randomBase && sumStatBounds(baseDefs,entryStat,layer,bLo,bHi) && bLo==bHi && entry.value>bLo){
                        const int res=static_cast<int>(entry.value-bLo);
                        for(std::size_t id=0;id<properties.size();++id){
                            const auto* p=properties[id].bytes.data();
                            const bool match107=entryStat==107 && p[0x18]==22 && !p[0x19] && !p[2] && Affixes::Read<std::uint16_t>(p,0x20)==107;
                            const bool match387=entryStat==387 && p[0x18]==22 && p[0x19]==24 && !p[0x1a] && !p[2] && !p[3] &&
                                Affixes::Read<std::uint16_t>(p,0x20)==387 && Affixes::Read<std::uint16_t>(p,0x22)==97;
                            if(match107 || match387){residualSpecs.push_back({static_cast<unsigned>(id),static_cast<int>(layer),res,res});break;}
                        }
                    }
                }
                continue;
            }
            const unsigned stat=static_cast<unsigned>(entry.key>>32);
            if(stat==11 || stat==18 || stat==31 || (stat==23 && Contributions::Value(out.groups[0].values,21)!=0) || (stat==24 && Contributions::Value(out.groups[0].values,22)!=0) ||
               (out.capture.identity.quality==3 && (stat==16 || stat==17 || stat==19 || (stat>=21 && stat<=24) || stat==75)) ||
               (stat>=48 && stat<=59) || stat==72 || stat==73 || stat==75 || stat==194 ||
               (stat>=216 && stat<=250) || (stat>=359 && stat<=363) || (stat>=391 && stat<=398) ||
               (stat>=404 && stat<=412) || (stat>=417 && stat<=420))continue;
            const unsigned shift=(stat==7 || stat==9)?8u:0u;
            const auto actualVal=Contributions::Normalize(entry.value,shift);
            if(!actualVal || *actualVal<=0)continue;
            int bLo=0,bHi=0;bool bFixed=true;long long baseSum=0;
            for(const auto& d:baseDefs){
                if(!Affixes::Contributes(d.row,properties,static_cast<int>(stat),0))continue;
                if(!defStatBounds(d,stat,0,bLo,bHi) || bLo!=bHi){bFixed=false;break;}
                baseSum+=bLo;
            }
            if(!bFixed || *actualVal<=baseSum)continue;
            const int res=static_cast<int>(*actualVal-baseSum);
            if(const auto prop=findSingleProp(stat))
                residualSpecs.push_back({*prop,0,res,res});
        }
        for(std::size_t i=0;i<residualSpecs.size();i+=3){
            std::vector<ContributionDefinitions::Spec> chunk(
                residualSpecs.begin()+static_cast<std::ptrdiff_t>(i),
                residualSpecs.begin()+static_cast<std::ptrdiff_t>(std::min(residualSpecs.size(),i+3)));
            auto block=SpecBlock(chunk,"[E]");
            block.uniqueScalar=true;block.unconditionalScalar=true;
            out.enchantment.push_back(block);
        }
    }
    if(out.capture.sockets.empty()&&!corrupted&&!enchanted&&out.craft.empty())return {};
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
void AddOwnedSourceLabels(const SourceGroups& sources,std::vector<CapturedLine>& observed,std::string_view actual,
    std::span<const Affixes::Property> properties,const unsigned char* itemStats,std::size_t statCount,
    Affixes::LayerEncoding encoding,std::span<const Affixes::PropertyGroup> groups,std::vector<RangeText::Label>& labels) {
    std::vector<Affixes::Rolled> all;
    for(const auto& group:sources.groups)all.insert(all.end(),group.definitions.begin(),group.definitions.end());
    all.insert(all.end(),sources.corruption.begin(),sources.corruption.end());
    all.insert(all.end(),sources.craft.begin(),sources.craft.end());
    all.insert(all.end(),sources.enchantment.begin(),sources.enchantment.end());
    AddUnobservedCompositeIdentities(observed,actual,all,properties,encoding,groups,itemStats,statCount);
    if(sources.enchanted || sources.unresolvedCorruption)return;
    std::vector<std::string> handled;
    for(const auto& line:observed){
        if(line.layer||line.grouped||line.stat<0||static_cast<std::size_t>(line.stat)>=statCount||
           std::find(handled.begin(),handled.end(),line.key)!=handled.end())continue;
        handled.push_back(line.key);unsigned stat=static_cast<unsigned>(line.stat);
        const auto* metadata=itemStats+stat*0x144;if(Affixes::Read<std::uint16_t>(metadata,0)!=stat)continue;
        const auto shift=metadata[0x14];if(shift>16)continue;
        const bool isDamageEndpoint=(stat>=21&&stat<=24)||(stat>=48&&stat<=55);
        const bool paired=isDamageEndpoint && std::count(line.key.begin(),line.key.end(),'#')==2;
        if(!paired&&Affixes::Read<std::uint16_t>(metadata,0x32)!=19)continue;
        if(paired){stat=(stat>=21&&stat<=24)?((stat&1)?stat:stat-1):(stat&~1u);if(stat+1>=statCount||itemStats[(stat+1)*0x144+0x14]!=shift)continue;}

        auto input=sources.groups;
        for(auto d:sources.craft){d.uniqueScalar=true;d.unconditionalScalar=true;input[0].definitions.push_back(std::move(d));}
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
        } else if(stat!=16 && stat!=20 && !(stat>=21&&stat<=24) && stat!=31){
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
    if(sources.groups.empty() || sources.groups.size()>8)return;
    auto groups=sources.groups;
    for(auto d:sources.corruption){d.uniqueScalar=true;d.unconditionalScalar=true;groups[0].definitions.push_back(std::move(d));}
    for(auto d:sources.craft){d.uniqueScalar=true;d.unconditionalScalar=true;groups[0].definitions.push_back(std::move(d));}
    for(auto d:sources.enchantment){d.uniqueScalar=true;d.unconditionalScalar=true;groups[0].definitions.push_back(std::move(d));}
    const Affixes::LayerEncoding selectorEncoding{6,63,randomSkillsReady.load()};
    const auto normalize=[&](unsigned stat,std::int64_t raw)->std::optional<int>{
        if(stat>=count)return {};const auto* row=metadata+stat*0x144;
        if(Affixes::Read<std::uint16_t>(row,0)!=stat || !PropertyRolls::ItemScalarOperand(row))return {};
        return Contributions::Normalize(raw,row[0x14]);
    };
    std::vector<PropertyRolls::Solution> solved;
    for(const auto& group:groups)solved.push_back(PropertyRolls::SolveSources(group.definitions,properties,group.values,normalize,propertyGroups));
    std::set<std::string> handled;
    for(const auto& line:observed){
        if(line.stat<0 || line.stat==204 || static_cast<std::size_t>(line.stat)>=count || !handled.insert(line.key).second)continue;
        const unsigned stat=static_cast<unsigned>(line.stat);std::vector<unsigned> members;
        if(stat==361 || stat==362 || (stat>=367 && stat<=376) ||
           (stat>=384 && stat<=386) || (stat>=392 && stat<=398) ||
           (stat>=404 && stat<=412) || stat==419 || stat==420 ||
           stat==433 || (stat>=438 && stat<=440) || stat==443)continue;
        // Physical endpoints, enhanced defense/damage, and durability percent live on owned modifiers.
        // Aggregate physical damage/defense includes the base item; aggregate
        // durability contains the resulting capacity, not stat75's percentage.
        // Require the displayed modifier to match the complete owned value.
        if(!line.layer && !line.grouped && (stat==16 || stat==17 || stat==18 || stat==75 || (stat>=21 && stat<=24))){
            const bool physical=stat>=21 && stat<=24;
            const bool paired=physical && std::count(line.key.begin(),line.key.end(),'#')==2;
            const unsigned minimum=paired?((stat&1)?stat:stat-1):stat;
            const unsigned maximum=paired?minimum+1:minimum;

            if(maximum>=count)continue;
            bool admitted=true;
            for(unsigned member=minimum;member<=maximum;++member){const auto* row=metadata+member*0x144;
                admitted &= Affixes::Read<std::uint16_t>(row,0)==member && row[0x14]==0 &&
                    row[0x50]==(physical?0:13) && Affixes::Read<std::uint16_t>(row,0x32)==19;}
            if(stat==17 || stat==18)for(const auto& g:groups)admitted &= Contributions::Value(g.values,17)==Contributions::Value(g.values,18);
            if(!admitted)continue;
            const auto owned=ContributionModel::Build(line.key,minimum,0,groups,properties,paired);
            if(!owned)continue;unsigned matches=0;bool renders=false;
            for(const auto text:RangeText::Lines(actual))if(RangeText::Analyze(text).key==line.key){++matches;renders=!RangeText::ExpandOwned(text,*owned).empty();}
            if(matches==1 && renders){std::erase_if(labels,[&](const auto& old){return old.key==line.key;});labels.push_back(*owned);}continue;
        }
        if(!line.layer && !line.grouped && stat>=57 && stat<=59){
            // Poison stores 1/256 damage-per-frame rates, not displayed totals.
            // Only one proven source with fixed duration is currently split.
            if(!poisonDisplayReady.load() || count<=59)continue;
            bool metadataValid=true;
            for(unsigned member=57;member<=59;++member)
                metadataValid &= Affixes::Read<std::uint16_t>(metadata+member*0x144,0)==member &&
                    !metadata[member*0x144+0x14] && !metadata[member*0x144+0x50];
            if(!metadataValid)continue;
            const ContributionModel::Group* poison=nullptr;bool valid=true;
            for(const auto& g:groups)if(Contributions::Value(g.values,57)||Contributions::Value(g.values,58)||Contributions::Value(g.values,59)){
                if(poison){valid=false;break;}poison=&g;
            }
            if(!valid || !poison || Contributions::Value(poison->values,326)>1)continue;
            const auto frames=Contributions::Value(poison->values,59);
            if(frames<=0 || frames>1000000)continue;
            unsigned durations=0;
            for(const auto& d:poison->definitions){
                if(!Affixes::Contributes(d.row,properties,59,0))continue;
                if(!d.unconditionalScalar || (d.source && !d.uniqueScalar)){valid=false;break;}
                for(unsigned n=0;n<3;++n){
                    const auto id=d.row.Number(0x24+n*16);if(id<0 || static_cast<std::size_t>(id)>=properties.size())continue;
                    const auto* p=properties[id].bytes.data();
                    for(unsigned slot=0;slot<7;++slot)if(p[0x18+slot] && Affixes::Read<std::uint16_t>(p,0x20+slot*2)==59){
                        if(p[0x18+slot]!=17 || p[2+slot] || d.row.Number(0x28+n*16)!=frames)valid=false;
                        ++durations;
                    }
                }
            }
            if(!valid || durations!=1)continue;
            auto poisonLabel=ContributionModel::Build(line.key,57,0,groups,properties,true);
            if(!poisonLabel || poisonLabel->ownedParts.size()!=1)continue;
            const auto convert=[&](int rate)->std::optional<int>{
                const auto product=std::int64_t{rate}*frames;
                if(rate<0 || product>INT_MAX-128)return {};
                return static_cast<int>((product+128)/256);
            };
            auto& part=poisonLabel->ownedParts.front();
            const auto low=convert(part.low),high=convert(part.high),maxLow=convert(part.maxLow),maxHigh=convert(part.maxHigh);
            const auto actualLow=convert(*poisonLabel->ownedTotal),actualHigh=convert(*poisonLabel->ownedMaxTotal);
            if(!low||!high||!maxLow||!maxHigh||!actualLow||!actualHigh)continue;
            part.low=*low;part.high=*high;part.maxLow=*maxLow;part.maxHigh=*maxHigh;
            part.value=*actualLow;part.maxValue=*actualHigh;
            poisonLabel->ownedTotal=*actualLow;poisonLabel->ownedMaxTotal=*actualHigh;
            poisonLabel->ownedDurationSeconds=static_cast<int>(frames/25);
            unsigned matches=0;bool renders=false;
            for(const auto text:RangeText::Lines(actual))if(RangeText::Analyze(text).key==line.key){++matches;renders=!RangeText::ExpandOwned(text,*poisonLabel).empty();}
            if(matches==1 && renders){std::erase_if(labels,[&](const auto& old){return old.key==line.key;});labels.push_back(*poisonLabel);}continue;
        }
        if(!line.layer && !line.grouped && stat>=48 && stat<=55){
            const bool pairedLine=std::count(line.key.begin(),line.key.end(),'#')==2;
            const unsigned minimum=pairedLine?(stat&~1u):stat,maximum=pairedLine?(minimum+1):stat;
            if(maximum>=count || metadata[minimum*0x144+0x14]!=metadata[maximum*0x144+0x14] ||
               metadata[minimum*0x144+0x50] || metadata[maximum*0x144+0x50])continue;
            long long low=0,high=0;
            std::vector<std::string> fallbackSources;bool fallbackValid=true;
            for(std::size_t n=0;n<groups.size();++n){
                const auto& g=groups[n];
                const auto vMin=Contributions::Value(g.values,minimum),vMax=pairedLine?Contributions::Value(g.values,maximum):0;
                low+=vMin;high+=vMax;
                if(vMin>0 || vMax>0){
                    auto lbl=Affixes::Label(g.definitions,properties,static_cast<int>(minimum),0,selectorEncoding,propertyGroups);
                    if(lbl.empty() && pairedLine)lbl=Affixes::Label(g.definitions,properties,static_cast<int>(maximum),0,selectorEncoding,propertyGroups);
                    if(lbl.empty() && n==0 && sources.enchanted)lbl="[E]";
                    if(lbl.empty())fallbackValid=false;
                    else fallbackSources.push_back(g.prefix+lbl);
                }
            }
            if(low!=Contributions::Value(sources.capture.total,minimum) || (pairedLine && high!=Contributions::Value(sources.capture.total,maximum)))continue;
            if(const auto paired=ContributionModel::Build(line.key,minimum,metadata[minimum*0x144+0x14],groups,properties,pairedLine)){

                unsigned matches=0;bool renders=false;
                for(const auto text:RangeText::Lines(actual))if(RangeText::Analyze(text).key==line.key){++matches;renders=!RangeText::ExpandOwned(text,*paired).empty();}
                if(matches==1 && renders){std::erase_if(labels,[&](const auto& old){return old.key==line.key;});labels.push_back(*paired);continue;}
            }
            if(fallbackValid && !fallbackSources.empty()){
                unsigned matches=0;
                for(const auto text:RangeText::Lines(actual))if(RangeText::Analyze(text).key==line.key)++matches;
                if(matches==1){
                    RangeText::Label fallback;fallback.key=line.key;
                    fallback.text=fallbackSources.size()==1?fallbackSources.front():"[Combined]";
                    fallback.sourceOnly=true;
                    std::erase_if(labels,[&](const auto& old){return old.key==line.key;});
                    labels.push_back(std::move(fallback));
                }
            }
            continue;
        }
        if(!line.grouped && line.layer>0 && stat>=195 && stat<=201 && selectorEncoding.skillBits && selectorEncoding.skillBits<16){
            const auto totalChance=Contributions::Value(sources.capture.total,stat,static_cast<unsigned>(line.layer));
            if(totalChance<=0 || totalChance>100000)continue;
            long long groupSum=0;bool procValid=true,allVerified=true;
            std::vector<std::string> contributors;
            for(std::size_t n=0;n<groups.size();++n){
                const auto val=Contributions::Value(groups[n].values,stat,static_cast<unsigned>(line.layer));
                if(val<0){procValid=false;break;}
                if(!val)continue;
                groupSum+=val;
                long long defSum=0;std::vector<std::string> groupNames;
                for(const auto& d:groups[n].definitions){
                    if(const auto c=ScalingAffixes::Proc(d,properties,static_cast<int>(stat),line.layer,selectorEncoding)){
                        defSum+=c->first;
                        groupNames.push_back(groups[n].prefix+Affixes::NamedLabel(d));
                    }
                }
                if(defSum==val && !groupNames.empty()){
                    contributors.insert(contributors.end(),groupNames.begin(),groupNames.end());
                }else if(n>0 && !groups[n].definitions.empty() &&
                         std::all_of(groups[n].definitions.begin(),groups[n].definitions.end(),
                             [](const auto& d){return d.source && std::string_view(d.source)=="[U]";})) {
                    allVerified=false;
                    contributors.push_back(groups[n].prefix+"[U]");
                }else if(n==0 && sources.enchanted && defSum==0 &&
                         std::none_of(groups[0].definitions.begin(),groups[0].definitions.end(),[&](const auto& d){
                             return Affixes::Contributes(d.row,properties,static_cast<int>(stat),line.layer,selectorEncoding,propertyGroups);})){
                    allVerified=false;
                    contributors.push_back("[E]");
                }else if(n==0 && sources.capture.identity.quality==7 && defSum==0){
                    allVerified=false;
                    contributors.push_back("[U]");
                }else{
                    procValid=false;break;
                }
            }
            if(procValid && groupSum==totalChance && !contributors.empty()){
                const int level=line.layer&static_cast<int>(selectorEncoding.skillMask);
                std::string chanceRange;unsigned matches=0;
                for(const auto text:RangeText::Lines(actual))if(RangeText::Analyze(text).key==line.key){
                    ++matches;chanceRange=RangeText::FixedProcChance(text,static_cast<int>(totalChance),level);
                }
                if(matches==1){
                    RangeText::Label procLabel;procLabel.key=line.key;
                    procLabel.text=contributors.size()==1?contributors.front():"[Combined]";
                    if(allVerified && !chanceRange.empty()){
                        procLabel.fallbackRange=std::move(chanceRange);
                        procLabel.replaceNativeRange=true;
                    }else{
                        procLabel.sourceOnly=true;
                    }
                    std::erase_if(labels,[&](const RangeText::Label& old){return old.key==line.key;});
                    labels.push_back(std::move(procLabel));
                }
            }
            continue;
        }
        const bool selector=!line.grouped && std::any_of(groups.begin(),groups.end(),[&](const auto& g){
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
        for(std::size_t n=0;n<groups.size();++n){
            const auto& group=groups[n];
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
        if(!valid && !line.layer && !line.grouped && !selector){
            long long rawSum=0;bool fallbackOk=true,socketImpossible=false;
            std::vector<std::string> groupLabels;
            for(std::size_t n=0;n<groups.size();++n){
                const auto v=normalize(stat,Contributions::Value(groups[n].values,stat));
                if(!v){fallbackOk=false;break;}
                rawSum+=*v;
                auto text=Affixes::Label(groups[n].definitions,properties,static_cast<int>(stat),0,selectorEncoding,propertyGroups);
                if(*v==0){
                    if(!text.empty()){fallbackOk=false;break;}
                    continue;
                }
                if(text.empty() && n==0 && sources.enchanted){
                    text="[E]";
                    socketImpossible=true;
                }
                if(text.empty()){fallbackOk=false;break;}
                if(solved[n].impossible.contains(stat))socketImpossible=true;
                groupLabels.push_back(groups[n].prefix+text);
            }
            if(fallbackOk && socketImpossible && rawSum==*total && !groupLabels.empty()){
                unsigned matches=0;
                for(const auto text:RangeText::Lines(actual))if(RangeText::Analyze(text).key==line.key)++matches;
                if(matches==1){
                    RangeText::Label fallback;fallback.key=line.key;
                    fallback.text=groupLabels.size()==1?groupLabels.front():"[Combined]";
                    fallback.sourceOnly=true;
                    std::erase_if(labels,[&](const auto& old){return old.key==line.key;});
                    labels.push_back(std::move(fallback));
                }
            }
            continue;
        }
        const bool modOnly=!line.layer && !line.grouped && (stat==20 || stat==31);
        if(modOnly)label.ownedTotal=static_cast<int>(sum);
        if(!valid || (modOnly?(sum<=0 || (*total>0 && sum>*total)):(sum!=*total)) || label.ownedParts.empty() || label.ownedParts.size()>12)continue;
        if(stat==253 && !line.grouped && !line.layer){if(!quantityRateReady.load())continue;label.quantityRate=true;}
        unsigned matches=0;bool renders=false;
        for(const auto text:RangeText::Lines(actual))if(RangeText::Analyze(text).key==line.key){++matches;renders=!RangeText::ExpandOwned(text,label).empty();}
        if(matches!=1 || !renders)continue;
        std::erase_if(labels,[&](const RangeText::Label& old){return old.key==line.key;});labels.push_back(std::move(label));
    }
}

// The already-qualified native Normalize witness adds50 to stat122 for item
// type57. This bonus has no saved stat entry or rolled affix definition.
void AddIntrinsicUndeadLabel(void* source,const unsigned char* tables,
    const Contributions::Snapshot& capture,std::span<const Affixes::Rolled> definitions,
    std::span<const Affixes::Row> affixes,std::size_t prefixIndex,std::size_t autoIndex,
    std::span<const Affixes::Property> properties,const unsigned char* metadata,
    std::size_t count,std::string_view actual,std::vector<RangeText::Label>& labels) {
    constexpr unsigned stat=122,type=57;constexpr int bonus=50;
    if(!getArmor || !originalSingle || stat>=count || !Readable(tables,0x1358) ||
       metadata[stat*0x144+0x14] || metadata[stat*0x144+0x50] ||
       Affixes::Read<std::uint16_t>(metadata+stat*0x144,0)!=stat)return;
    const auto* row=getArmor(static_cast<unsigned char>(capture.identity.bank),capture.identity.classId);
    const auto* typeRows=Affixes::Read<const unsigned char*>(tables,0x1348);
    const auto typeCount=Affixes::Read<std::uint64_t>(tables,0x1350);
    if(!Readable(row,0x132) || typeCount<=type || typeCount>32768 ||
       !Readable(typeRows,static_cast<std::size_t>(typeCount)*232) ||
       Affixes::Read<unsigned>(typeRows+type*232,0)!=ContributionDefinitions::Code("blun"))return;
    const auto blunt=ContributionDefinitions::Type({typeRows,static_cast<std::size_t>(typeCount),232},
        Affixes::Read<std::int16_t>(row,0x12e),Affixes::Read<std::int16_t>(row,0x130),type);
    if(!blunt || !*blunt)return;
    auto owned=GatherSourceGroups(source,definitions,affixes,prefixIndex,autoIndex,properties);
    std::vector<ContributionModel::Group> groups;
    if(owned){if(!owned->corruption.empty()||!owned->craft.empty())return;groups=owned->groups;}
    else {
        if(!capture.sockets.empty() || (capture.identity.flags&0x04000000u) || HasRecipeExtras(capture))return;
        groups.push_back({capture.own,{definitions.begin(),definitions.end()},""});
    }
    long long stored=0;for(const auto& group:groups)stored+=Contributions::Value(group.values,stat);
    if(stored!=Contributions::Value(capture.total,stat) || stored<0 || stored>1000000-bonus)return;
    // Find an equivalent additive property from the active table; no named
    // property/item catalog or speculative residual range is introduced.
    std::optional<unsigned> property;
    for(std::size_t n=0;n<properties.size();++n){
        const auto* p=properties[n].bytes.data();bool matches=false,valid=true;
        for(unsigned k=0;k<7;++k){const auto fn=p[0x18+k];if(!fn)continue;
            if(matches || fn!=1 || p[2+k] || Affixes::Read<std::uint16_t>(p,0x20+k*2)!=stat){valid=false;break;}matches=true;}
        if(valid && matches){property=static_cast<unsigned>(n);break;}
    }
    if(!property)return;
    const std::array<ContributionDefinitions::Spec,1> spec={ContributionDefinitions::Spec{*property,0,bonus,bonus}};
    groups.push_back({{{std::uint64_t{stat}<<32,bonus}},{SpecBlock(spec,"[Base]")},""});
    char text[256]{};
    if(!originalSingle(source,metadata+stat*0x144,static_cast<int>(stored)+bonus,0,false,text,1))return;
    const auto size=strnlen(text,256);if(!size || size==256)return;
    const auto key=RangeText::Analyze({text,size}).key;
    const auto label=ContributionModel::Build(key,stat,0,groups,properties);if(!label)return;
    unsigned matches=0;bool renders=false;
    for(const auto line:RangeText::Lines(actual))if(RangeText::Analyze(line).key==key){++matches;renders=!RangeText::ExpandOwned(line,*label).empty();}
    if(matches!=1 || !renders)return;
    std::erase_if(labels,[&](const auto& old){return old.key==key;});labels.push_back(*label);
}
