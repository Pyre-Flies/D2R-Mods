// Optional private replay. Captured game tables stay outside the repository.
namespace {
void TestReportedCapture(const char* path,const char* which) {
    std::ifstream input(path,std::ios::binary);char magic[4]{};input.read(magic,4);
    check(std::memcmp(magic,"IR47",4)==0);
    auto number=[&](){unsigned value{};input.read(reinterpret_cast<char*>(&value),4);check(input.good());return value;};
    const std::array<unsigned,6> strides{48,200,348,460,324,140};
    std::array<unsigned,6> counts{};std::array<std::vector<unsigned char>,6> tables;
    for(unsigned n=0;n<6;++n){counts[n]=number();check(number()==strides[n] && counts[n]<=32768);
        tables[n].resize(static_cast<std::size_t>(counts[n])*strides[n]);
        input.read(reinterpret_cast<char*>(tables[n].data()),static_cast<std::streamsize>(tables[n].size()));check(input.good());}
    const auto prefix=number(),automagic=number(),items=number();check(prefix<=automagic && automagic<=counts[5] && items<=64);
    if(std::strcmp(which,"catalog")==0){TestDefinitionCatalog(tables,counts,prefix,automagic);return;}
    bool checked=false;
    for(unsigned n=0;n<items;++n){
        const auto quality=number(),itemClass=number(),flags=number(),record=number(),bank=number();
        std::array<Contributions::Stats,3> values;
        for(auto& list:values){const auto count=number();check(count<=2048);list.resize(count);
            for(auto& value:list){input.read(reinterpret_cast<char*>(&value.key),8);input.read(reinterpret_cast<char*>(&value.value),8);check(input.good());}}
        const bool crest=std::strcmp(which,"crest")==0 && quality==5 && record==80;
        const bool poison=std::strcmp(which,"poison")==0 && quality==5 && record==7;
        const bool cold=std::strcmp(which,"cold")==0 && quality==7 && record==384;
        const bool fire=std::strcmp(which,"fire")==0 && quality==7 && record==30;
        const bool set=std::strcmp(which,"set")==0 && quality==5 && record==437;
        const bool durability=std::strcmp(which,"durability")==0 && quality==7 && (record==894 || record==900);
        const bool physical=std::strcmp(which,"physical")==0 && quality==7 && record==894;
        if(!crest && !poison && !cold && !fire && !set && !durability && !physical)continue;
        ContributionFixture f;propertySourcesReady.store(true);
        put(f.host.data(),4,itemClass);check(bank>=1 && bank<=3); // Fixture bank2 binds this captured bank explicitly.
        put(f.Data(),0,quality);put(f.Data(),0x18,flags);put(f.Data(),0x34,record);put(f.Data()-24,0,0u);
        put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));
        f.own=values[1];f.total=values[2];f.Bind();
        put(f.root.data(),0x30,values[0].data());put(f.root.data(),0x38,static_cast<std::uint64_t>(values[0].size()));
        put(f.root.data(),0x40,static_cast<std::uint64_t>(values[0].size()));
        const unsigned offsets[]={0x240,0x258,0x13c8,0x13a8,0x1258,0x15e8};
        for(unsigned t=0;t<6;++t){put(fakeTables,offsets[t],tables[t].data());put(fakeTables,offsets[t]+8,std::uint64_t{counts[t]});}
        put(fakeTables,0x1600,tables[5].data());put(fakeTables,0x1608,tables[5].data()+prefix*140);
        put(fakeTables,0x1610,tables[5].data()+automagic*140);
        if(crest){
            const auto socketCount=number();check(socketCount==4);
            std::array<std::array<unsigned char,0x200>,4> units{},allocations{};
            std::array<std::array<unsigned char,0xc0>,4> roots{};
            std::array<Contributions::Stats,4> socketValues;
            for(unsigned j=0;j<socketCount;++j){
                const auto id=number(),cls=number(),socketFlags=number(),socketRecord=number();
                const auto entries=number();check(entries<=2048);socketValues[j].resize(entries);
                for(auto& value:socketValues[j]){input.read(reinterpret_cast<char*>(&value.key),8);input.read(reinterpret_cast<char*>(&value.value),8);check(input.good());}
                auto* data=allocations[j].data()+28;auto* unit=units[j].data();auto* root=roots[j].data();
                put(unit,0,4u);put(unit,4,cls);put(unit,8,id);put(unit,0x10,data);unit[0x1bd]=2;
                put(data,0,7u);put(data,0x18,socketFlags);put(data,0x34,socketRecord);put(data,0x40,std::uint16_t{100});
                put(unit,0x88,root);put(root,0x1c,0x80000000u);put(root,0x78,f.root.data());put(root,0xa0,unit);
                put(root,0xa8,socketValues[j].data());put(root,0xb0,std::uint64_t{entries});put(root,0xb8,std::uint64_t{entries});
                if(j){put(roots[j-1].data(),0x68,root);put(allocations[j-1].data()+28,0xb0,unit);}
            }
            put(f.modifier.data(),0x68,roots[0].data());put(f.inventory.data(),0x10,units[0].data());
            const auto length=number();check(length<1024);std::string actual(length,'\0');input.read(actual.data(),length);check(input.good());
            std::vector<CapturedLine> lines;
            for(auto [stat,text]:std::initializer_list<std::pair<int,const char*>>{
                {39,"+15% to All Resistances"},{9,"+30 to Mana"},{7,"+60 to Life"},{31,"+45 Defense"},
                {107,"+3 to Warmth (Sorceress Only)"},{60,"+10% Life stolen per hit"},{62,"+10% Mana stolen per hit"},
                {329,"+5% to Fire Skill Damage"},{331,"+15% to Cold Skill Damage"},{333,"-5% to Enemy Fire Resistance"},{335,"-15% to Enemy Cold Resistance"},
                {54,"Adds 72-114 Weapon Cold Damage"},{48,"Adds 17-45 Weapon Fire Damage"}})
                lines.push_back({RangeText::Analyze(text).key,stat,stat==107?37:0,false});
            const auto labels=AffixLabels(f.host.data(),lines,actual);
            const auto large=RangeText::Merge(actual,actual,16384,labels);
            const auto native=RangeText::Merge(actual,actual,1024,labels);
            std::printf("Crest replay: actual=%zu expanded=%zu labels=%zu native-labeled=%u\n",actual.size(),large.text.size(),labels.size(),native.labeled);
            check(large.labeled>0 && native.labeled>0 && native.text.find("[Set]")!=std::string::npos && native.text.size()<1024);
            for(const auto line:RangeText::Lines(actual))check(native.text.find(line)!=std::string::npos);
            check(native.text.find("[Combined]")!=std::string::npos && native.text.find("[Socket 1]")==std::string::npos);
            propertySourcesReady.store(false);checked=true;continue;
        }
        std::string rendered;
        if(poison){poisonDisplayReady.store(true);const std::string actual="Adds 4-8 Weapon Poison Damage over 4 Seconds";
            rendered=RangeText::Merge(actual,actual,4096,AffixLabels(f.host.data(),{},actual)).text;
            std::printf("Captured poison replay: %s\n",rendered.c_str());
            check(rendered.find("[4 / 8]")!=std::string::npos && rendered.find("[Set]")!=std::string::npos);poisonDisplayReady.store(false);}

        if(cold){const std::string actual="Adds 205-455 Weapon Cold Damage\n+50% Damage to Undead";
            const auto labels=AffixLabels(f.host.data(),{},actual);rendered=RangeText::Merge(actual,actual,4096,labels).text;
            std::printf("Cold replay: %s\n",rendered.c_str());check(rendered.find("[205 / 455]")!=std::string::npos && rendered.find("[U]")!=std::string::npos);}
        if(fire){rendered=f.Render(48,"Adds 12-15 Weapon Fire Damage");
            std::printf("Fire replay: %s\n",rendered.c_str());check(rendered.find("[10 - 12 / 15 - 25]")!=std::string::npos);}
        if(set){rendered=f.Render(80,"+26% Chance Items Roll Magic or Better");
            std::printf("Set replay: %s\n",rendered.c_str());check(rendered.find("[+10% - +30%]")!=std::string::npos);
            rendered=f.Render(127,"+2 to All Skills");check(rendered.find("[+1 - +2]")!=std::string::npos);}
        if(durability){
            const char* text=record==894?"Increase Maximum Durability 25%":"Increase Maximum Durability 100%";
            rendered=f.Render(75,text);std::printf("Durability replay: %s\n",rendered.c_str());
            check(rendered.find(record==894?"[25%]":"[100%]")!=std::string::npos);
            check(rendered.find("[U]")!=std::string::npos);
        }
        if(physical){rendered=f.Render(21,"Adds 5-10 Weapon Damage");
            std::printf("Physical replay: %s\n",rendered.c_str());check(rendered.find("[5 / 10]")!=std::string::npos);
            check(rendered.find("[U]")!=std::string::npos);
        }
        propertySourcesReady.store(false);checked=true;
    }
    check(checked && input.peek()==std::char_traits<char>::eof());
}
}
