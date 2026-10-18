// Optional local catalog audit: uses private captured definitions, never item-name exceptions.
namespace {
void TestDefinitionCatalog(std::array<std::vector<unsigned char>,6>& tables,
    const std::array<unsigned,6>& counts,unsigned prefix,unsigned automagic){
    const auto properties=std::span(reinterpret_cast<const Affixes::Property*>(tables[0].data()),counts[0]);
    unsigned paired=0,scalar=0,refused=0,failures=0,gaps=0;std::set<unsigned> setRows,gapRows;
    auto require=[&](bool ok,unsigned category,unsigned row,unsigned stat){if(!ok){
        ++failures;std::printf("CATALOG FAILURE table=%u row=%u stat=%u\n",category,row,stat);}};
    for(unsigned category: {2u,3u,5u})for(unsigned index=0;index<counts[category];++index){
        std::vector<Affixes::Rolled> definitions;
        if(category==5){Affixes::Row row;std::memcpy(row.bytes.data(),tables[5].data()+index*140,140);
            definitions.push_back({index,row,false,1,nullptr,true,true});}
        else {const auto stride=category==2?348u:460u,slots=category==2?12u:9u;
            for(unsigned n=0;n<slots;++n)AddUniqueSource(tables[category].data()+index*stride+0x98+n*16,
                tables[1].data(),counts[1],counts[0],definitions,0,true,category==2?"[U]":"[Set]");}
        // Independent oracle for additive fn1 and fixed endpoint fn15/fn16.
        std::map<unsigned,std::pair<int,int>> bounds;std::set<unsigned> blocked,pairedOnly;
        for(const auto& d:definitions)for(unsigned n=0;n<3;++n){const auto at=0x24+n*16;
            const int id=d.row.Number(at);if(id<0 || static_cast<unsigned>(id)>=counts[0])continue;
            const auto* p=properties[id].bytes.data();const int low=d.row.Number(at+8),high=d.row.Number(at+12);
            bool previousScalar=false;
            for(unsigned k=0;k<7;++k){const unsigned fn=p[0x18+k];if(!fn)continue;
                const unsigned stat=Affixes::Read<std::uint16_t>(p,0x20+k*2);if(stat>=counts[4]){previousScalar=false;continue;}
                if((fn==3 && !previousScalar) || !d.unconditionalScalar || p[2+k] || low>high || (fn!=1 && fn!=3 && fn!=8 && fn!=15 && fn!=16)){
                    blocked.insert(stat);previousScalar=false;continue;}
                previousScalar=fn==1 || fn==3 || fn==8;
                if(fn==15 || fn==16)pairedOnly.insert(stat);
                auto& b=bounds[stat];b.first+=fn==16?high:low;b.second+=fn==15?low:high;
            }
        }
        for(unsigned minimum: {21u,23u,48u,50u,52u,54u}){
            if(!bounds.contains(minimum) && !bounds.contains(minimum+1))continue;
            if(blocked.contains(minimum)||blocked.contains(minimum+1)){++refused;continue;}
            const auto a=bounds[minimum],b=bounds[minimum+1];
            for(unsigned sample=0;sample<3;++sample){const int lo=sample==0?a.first:sample==1?a.second:a.first+(a.second-a.first)/2;
                const int hi=sample==0?b.first:sample==1?b.second:b.first+(b.second-b.first)/2;
                ContributionModel::Group group;group.definitions=definitions;
                group.values=ContributionFixture::Stats({{minimum,lo},{minimum+1,hi}});
                const auto label=ContributionModel::Build("Adds #-# catalog damage",minimum,0,{&group,1},properties,true);
                bool ok=label.has_value();if(label){int al=0,ah=0,bl=0,bh=0;
                    for(const auto& part:label->ownedParts){al+=part.low;ah+=part.high;bl+=part.maxLow;bh+=part.maxHigh;}
                    ok=al==a.first&&ah==a.second&&bl==b.first&&bh==b.second;
                    const auto actual="Adds "+std::to_string(lo)+"-"+std::to_string(hi)+" catalog damage";
                    ok&=!RangeText::ExpandOwned(actual,*label).empty();}
                require(ok,category,index,minimum);++paired;
            }
        }
        if(category!=3)continue;
        for(const auto& [stat,b]:bounds){const auto* meta=tables[4].data()+stat*324;
            if(blocked.contains(stat)||meta[0x14]||meta[0x50]||(stat>=48&&stat<=59)||pairedOnly.contains(stat)||b.first<=0||b.second>100000)continue;
            ContributionFixture f;propertySourcesReady.store(true);
            put(f.Data(),0,5u);put(f.Data(),0x34,index);put(f.Data()-24,0,0u);
            put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));
            const unsigned offsets[]={0x240,0x258,0x13c8,0x13a8,0x1258,0x15e8};
            for(unsigned t=0;t<6;++t){put(fakeTables,offsets[t],tables[t].data());put(fakeTables,offsets[t]+8,std::uint64_t{counts[t]});}
            put(fakeTables,0x1600,tables[5].data());put(fakeTables,0x1608,tables[5].data()+prefix*140);
            put(fakeTables,0x1610,tables[5].data()+automagic*140);
            for(int value: {b.first,b.second}){f.own.clear();
                for(const auto& [member,domain]:bounds)if(!blocked.contains(member))f.own.push_back({std::uint64_t{member}<<32,std::int64_t{value==b.first?domain.first:domain.second}*(std::int64_t{1}<<tables[4][member*324+0x14])});
                f.total=f.own;
                const auto text="+"+std::to_string(value)+" catalog modifier";
                const auto labels=f.Labels(stat,text.c_str());bool ok=false;
                for(const auto& label:labels)if(label.key==RangeText::Analyze(text).key&&!label.ownedParts.empty()){
                    int low=0,high=0;for(const auto& part:label.ownedParts){low+=part.low;high+=part.high;}
                    ok=low==b.first&&high==b.second&&!RangeText::ExpandOwned(text,label).empty();}
                if(!ok && !PropertyRolls::Compile(definitions,properties).complete){
                    ++gaps;gapRows.insert(index);
                }else require(ok,category,index,stat);++scalar;}
            setRows.insert(index);propertySourcesReady.store(false);
        }
    }
    for(auto row:gapRows)std::printf("KNOWN COVERAGE GAP set row=%u: unmodeled implicit property invalidates source model\n",row);
    std::printf("Catalog: rows unique=%u set=%u affix=%u; paired samples=%u; permanent set scalar samples=%u across %zu rows; unproved paired families=%u; incomplete-model samples=%u across %zu rows; unexpected failures=%u\n",
        counts[2],counts[3],counts[5],paired,scalar,setRows.size(),refused,gaps,gapRows.size(),failures);
    check(paired>0 && scalar>0 && failures==0 && gaps==0);
}
}
