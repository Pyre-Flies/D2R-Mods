// Full production source capture -> active definitions -> labels -> merge.
namespace {
std::array<DefinitionTable,77> contributionFixtureTables{};
bool contributionFixtureBusy{},contributionFixtureStale{};
D2RL::DataTables::Result __cdecl FixtureTable(const D2RL::PluginContext*,D2RL::DataTables::Bank bank,
    D2RL::DataTables::TableId id,D2RL::DataTables::TableView* view) noexcept {
    if(contributionFixtureBusy)return D2RL::DataTables::Result::Busy;
    const auto index=static_cast<unsigned>(id);if(index>=contributionFixtureTables.size())return D2RL::DataTables::Result::NotFound;
    const auto table=contributionFixtureTables[index];if(!table.rows)return D2RL::DataTables::Result::NotFound;
    *view={.structSize=D2RL::DataTables::TableViewSize,.tableId=id,.bank=bank,
        .revision=contributionFixtureStale&&id==D2RL::DataTables::TableId::CubeMain?8u:7u,
        .rows=table.rows,.rowCount=static_cast<unsigned>(table.count),.rowSize=static_cast<unsigned>(table.stride)};
    return D2RL::DataTables::Result::Success;
}
struct ContributionFixture {
    std::array<unsigned char,0x200> host{},allocation{},jewel{},jewelAllocation{};
    std::array<unsigned char,0xc0> root{},modifier{},socketRoot{};
    std::array<unsigned char,0x20> inventory{};
    std::array<Affixes::Row,2> rows{};
    std::array<Affixes::Property,6> props{};
    std::vector<unsigned char> statRows=std::vector<unsigned char>(400*0x144);
    Contributions::Stats own,total,socket;
    unsigned char* Data(){return allocation.data()+28;}
    unsigned char* JewelData(){return jewelAllocation.data()+28;}
    ContributionFixture(){
        std::memset(fakeTables,0,sizeof(fakeTables));
        put(host.data(),0,4u);put(host.data(),4,0u);put(host.data(),8,1u);put(host.data(),0x10,Data());host[0x1bd]=2;
        put(Data(),0,6u);put(Data(),0x40,std::uint16_t{100});put(Data()-24,0,1u);
        put(host.data(),0x88,root.data());put(host.data(),0x90,inventory.data());
        put(root.data(),0x1c,0x80000000u);put(root.data(),0x90,modifier.data());
        put(modifier.data(),0,host.data());put(modifier.data(),0x18,-1);put(modifier.data(),0x1c,0x40u);
        put(modifier.data(),0x78,root.data());put(modifier.data(),0x68,socketRoot.data());
        put(jewel.data(),0,4u);put(jewel.data(),4,2u);put(jewel.data(),8,2u);put(jewel.data(),0x10,JewelData());jewel[0x1bd]=2;
        put(JewelData(),0,4u);put(JewelData(),0x40,std::uint16_t{100});put(JewelData()-24,0,2u);
        put(jewel.data(),0x88,socketRoot.data());put(socketRoot.data(),0x1c,0x80000000u);
        put(socketRoot.data(),0x78,root.data());put(socketRoot.data(),0xa0,jewel.data());
        put(inventory.data(),0,0x01020304u);put(inventory.data(),0x10,jewel.data());
        for(unsigned i=0;i<2;++i){auto* row=rows[i].bytes.data();row[0x54]=row[0x64]=row[0x82]=1;
            put(row,0x34,-1);put(row,0x44,-1);put(row,0x58,1);put(row,0x5c,1);}
        for(unsigned stat=0;stat<400;++stat){put(statRows.data()+stat*0x144,0,static_cast<std::uint16_t>(stat));
            put(statRows.data()+stat*0x144,0x32,std::uint16_t{19});}
        put(statRows.data()+7*0x144,0x14,std::uint8_t{8});
        put(fakeTables,0x15e8,rows.data());put(fakeTables,0x15f0,std::uint64_t{2});
        put(fakeTables,0x1600,rows.data());put(fakeTables,0x1608,rows.data()+2);put(fakeTables,0x1610,rows.data()+2);
        put(fakeTables,0x240,props.data());put(fakeTables,0x248,std::uint64_t{6});
        put(fakeTables,0x1258,statRows.data());put(fakeTables,0x1260,std::uint64_t{400});
        getTables=FakeTables;eligibleAffix=FakeEligible;contributionsReady.store(true);
        contributionContext=nullptr;contributionTables=nullptr;contributionFixtureTables={};
        contributionFixtureBusy=contributionFixtureStale=false;
    }
    ~ContributionFixture(){contributionsReady.store(false);contributionContext=nullptr;contributionTables=nullptr;
        getTables=nullptr;eligibleAffix=nullptr;}
    void Property(unsigned index,unsigned fn,unsigned stat){props[index]={};props[index].bytes[0x18]=static_cast<unsigned char>(fn);
        put(props[index].bytes.data(),0x20,static_cast<std::uint16_t>(stat));}
    void Roll(unsigned index,unsigned property,int low,int high,const char* name){auto* row=rows[index].bytes.data();
        std::memset(row,0,32);std::memcpy(row,name,std::strlen(name));put(row,0x24,property);put(row,0x2c,low);put(row,0x30,high);}
    static Contributions::Stats Stats(std::initializer_list<std::pair<unsigned,int>> values){Contributions::Stats result;
        for(auto [stat,value]:values)result.push_back({std::uint64_t{stat}<<32,value});
        std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.key<b.key;});return result;}
    void Bind(){auto bind=[](unsigned char* list,unsigned offset,const Contributions::Stats& values){
        put(list,offset,values.data());put(list,offset+8,static_cast<std::uint64_t>(values.size()));put(list,offset+16,static_cast<std::uint64_t>(values.size()));};
        bind(modifier.data(),0x30,own);bind(root.data(),0xa8,total);bind(socketRoot.data(),0xa8,socket);}
    std::vector<RangeText::Label> Labels(unsigned stat,const char* actual){Bind();
        return AffixLabels(host.data(),{{RangeText::Analyze(actual).key,static_cast<int>(stat),0,false}},actual);}
    std::string Render(unsigned stat,const char* actual){return RangeText::Merge(actual,actual,4096,Labels(stat,actual)).text;}
};
bool __fastcall FixtureScalingSingle(void*,const void* metadata,int value,int layer,bool grouped,char* output,int mode) {
    check(!grouped && mode==1);
    const auto stat=Affixes::Read<std::uint16_t>(static_cast<const unsigned char*>(metadata),0);
    check(!layer || (stat==83 && layer==3));
    if(stat==238)std::snprintf(output,256,"Attacker Takes Damage of %d (Based on Character Level)",value);
    else if(stat==214)std::snprintf(output,256,"+%d Defense (Based on Character Level)",value);
    else if(stat==224)std::snprintf(output,256,"+%d to Attack Rating (Based on Character Level)",value);
    else if(stat==16)std::snprintf(output,256,"+%d%% Enhanced Defense",value);
    else if(stat==34)std::snprintf(output,256,"Physical Damage Reduced by %d",value);
    else if(stat==35)std::snprintf(output,256,"Magic Damage Reduced by %d",value);
    else if(stat==78)std::snprintf(output,256,"Attacker Takes Damage of %d",value);
    else if(stat==85)std::snprintf(output,256,"+%d%% to Experience Gained",value);
    else if(stat==96)std::snprintf(output,256,"+%d%% Faster Run/Walk",value);
    else if(stat==83)std::snprintf(output,256,"+%d to Paladin Skill Levels",value);
    else return false;
    return true;
}
unsigned char fixtureSocketMaximum=4;
const char* fixtureSocketFormat="Max Sockets: %d";
unsigned char __fastcall FixtureSocketCapacity(void*) {return fixtureSocketMaximum;}
const char* __fastcall FixtureSocketLookup(const NativeText*,const NativeText* key,bool) {
    const std::string_view name(key->data,key->size);
    if(name=="ItemStats1p")return "Required Level: %d";
    if(name=="ItemStats1e")return "Required Strength: %d";
    if(name=="ItemStats1f")return "Required Dexterity: %d";
    if(name=="ItemStats1d")return "Durability: %i of %i";
    if(name=="ItemStats1h")return "Defense: %d";
    if(name=="ItemStats1l")return "One-Hand Damage: %d to %d";
    return name=="d2rloader:D2RLoaderMaximumSockets"?fixtureSocketFormat:nullptr;
}
void TestSocketCapacityAdapter(){
    ContributionFixture f;f.own={};f.total=f.Stats({{194,2}});f.Bind();
    auto primary=f.Stats({{194,2}});
    put(f.root.data(),0x30,primary.data());put(f.root.data(),0x38,std::uint64_t{1});put(f.root.data(),0x40,std::uint64_t{1});
    nativeSocketCapacity=FixtureSocketCapacity;lookupHeader=FixtureSocketLookup;fixtureSocketMaximum=4;
    const std::string modifiers="Socketed (2)\n+2% to Experience Gained\n+90 to Life [Combined]\n+94% Enhanced Defense\n";
    const std::string headers="Required Level: 72\nRequired Strength: 43\nDefense: 246 (Base: 127 - 127)";
    const std::string body=modifiers+headers;
    const auto result=AnnotateSocketHeader(f.host.data(),body);
    check(result==modifiers+"\xee\x81\xbe" "0Max Sockets: 4\n"+headers);
    check(result.find("Required Level: 4")==std::string::npos);
    check(AnnotateSocketHeader(f.host.data(),"Max Sockets: 4\n"+body)=="Max Sockets: 4\n"+body);
    // Range publication must retain this header addition even for a weapon
    // whose header has no Defense value; restored TLS cannot recover the item.
    const std::string weaponHeaders="Required Level: 63\nOne-Hand Damage: 15 to 153 (Base: 4 - 28)";
    const std::string weaponPrefix=modifiers+"Very Fast Attack Speed\n";
    const std::string weapon=weaponPrefix+weaponHeaders;
    NativeText receiver{weapon.data(),weapon.size()};originalAssign=RecordHeader;
    pendingHeader={&receiver,weapon,AnnotateSocketHeader(f.host.data(),weapon)};
    PublishHeader(true,&receiver,weapon.data(),weapon.size());
    check(publishedHeader==weaponPrefix+"\xee\x81\xbe" "0Max Sockets: 4\n"+weaponHeaders);
    PublishHeader(true,&receiver,weapon.data(),weapon.size());check(publishedHeader==weapon);
    originalAssign=nullptr;
    fixtureSocketFormat="Sockelmaximum: %u";
    check(AnnotateSocketHeader(f.host.data(),body).find("Sockelmaximum: 4")!=std::string::npos);
    fixtureSocketFormat="Max Sockets: %d %d";check(AnnotateSocketHeader(f.host.data(),body)==body);
    fixtureSocketFormat="";check(AnnotateSocketHeader(f.host.data(),body)==body);
    fixtureSocketFormat="Max Sockets: %d";
    const std::string noLevel="Required Strength: 43\nDurability: 17 of 24\nDefense: 127";
    check(AnnotateSocketHeader(f.host.data(),modifiers+noLevel)==modifiers+"\xee\x81\xbe" "0Max Sockets: 4\n"+noLevel);
    const std::string noRequirements="Durability: 17 of 24\nDefense: 127";
    check(AnnotateSocketHeader(f.host.data(),modifiers+noRequirements)==modifiers+"\xee\x81\xbe" "0Max Sockets: 4\n"+noRequirements);
    check(AnnotateSocketHeader(f.host.data(),"+90 to Life")=="+90 to Life");
    check(AnnotateSocketHeader(f.host.data(),body+"\nRequired Level: 72")==body+"\nRequired Level: 72");
    fixtureSocketMaximum=1;check(AnnotateSocketHeader(f.host.data(),body)==body);
    fixtureSocketMaximum=7;check(AnnotateSocketHeader(f.host.data(),body)==body);
    primary[0].value=0;check(AnnotateSocketHeader(f.host.data(),body)==body);
    primary[0].value=2;nativeSocketCapacity=nullptr;check(AnnotateSocketHeader(f.host.data(),body)==body);
    nativeSocketCapacity=nullptr;lookupHeader=nullptr;
}
void TestScalingAffixAdapter(){
    ContributionFixture f;scalingReady.store(true);originalSingle=FixtureScalingSingle;
    std::array<unsigned char,36> experience{};for(unsigned n=0;n<8;++n)put(experience.data(),n*4,100);
    put(fakeTables,0x14e0,experience.data());put(fakeTables,0x14e8,std::uint64_t{102});
    f.Property(0,17,238);f.Roll(0,0,0,0,"of Thorns");f.Roll(1,0,0,0,"of Thorns");
    put(f.rows[0].bytes.data(),0x28,8);put(f.rows[1].bytes.data(),0x28,12);
    put(f.rows[0].bytes.data(),0x58,40);put(f.rows[1].bytes.data(),0x58,67);
    auto* metadata=f.statRows.data()+238*0x144;metadata[0x50]=2;metadata[0x51]=3;put(metadata,0x52,std::uint16_t{12});
    f.own=f.Stats({{238,8}});f.total=f.own;
    const char* thorns="Attacker Takes Damage of 99 (Based on Character Level)";
    const std::string ranged="Attacker Takes Damage of \xee\x81\xbe" "U(1-99)\xee\x81\xbe" "3 (Based on Character Level)";
    auto labels=f.Labels(238,thorns);check(labels.size()==1 && labels[0].text.find("[T2]")!=std::string::npos);
    auto text=RangeText::Merge(thorns,ranged,4096,labels).text;
    check(text.find("[1 - 100]")!=std::string::npos && text.find(thorns)!=std::string::npos);
    check(text.find("Unavailable")==std::string::npos);
    put(experience.data(),28,99);check(f.Labels(238,thorns)[0].fallbackRange.empty());put(experience.data(),28,100);
    f.total=f.Stats({{238,9}});check(f.Labels(238,thorns)[0].fallbackRange.empty());f.total=f.own;
    scalingReady.store(false);labels=f.Labels(238,thorns);
    check(labels[0].fallbackRange.empty() && labels[0].text.find("[T1]")!=std::string::npos);scalingReady.store(true);
    f.Property(0,17,214);f.Property(1,1,78);f.Roll(1,1,1,3,"of Thorns");
    put(f.rows[0].bytes.data(),0x28,48);metadata=f.statRows.data()+214*0x144;
    metadata[0x50]=4;metadata[0x51]=3;put(metadata,0x52,std::uint16_t{12});
    f.own=f.Stats({{214,48}});f.total=f.own;
    text=f.Render(214,"+594 Defense (Based on Character Level)");check(text.find("[6 - 600]")!=std::string::npos);
    f.Property(0,8,99);f.Roll(0,0,10,10,"of Balance");put(f.rows[0].bytes.data(),0x28,0);
    f.own=f.Stats({{99,10}});f.total=f.own;
    check(f.Render(99,"+10% Faster Hit Recovery").find("[+10%]")!=std::string::npos);
    f.Property(0,11,199);f.Roll(0,0,100,17,"of Chilling Armor");put(f.rows[0].bytes.data(),0x28,60);
    put(fakeTables,0x14d0,6u);put(fakeTables,0x14d4,63u);
    f.own={{(std::uint64_t{199}<<32)|3857u,100}};f.total=f.own;f.Bind();
    const char* proc="100% Chance to cast level 17 Chilling Armor when you Level-Up";
    const std::vector<CapturedLine> procLines={{RangeText::Analyze(proc).key,199,3857,false}};
    labels=AffixLabels(f.host.data(),procLines,proc);
    check(labels.size()==1 && labels[0].fallbackRange=="100%");
    text=RangeText::Merge(proc,proc,4096,labels).text;check(text.find("[100%]")!=std::string::npos);
    check(text.find("100 - 17")==std::string::npos && text.find("level 17")!=std::string::npos);
    f.own[0].value=99;f.total=f.own;f.Bind();check(AffixLabels(f.host.data(),procLines,proc)[0].fallbackRange.empty());
    f.own[0].value=100;f.total=f.own;f.Bind();
    check(AffixLabels(f.host.data(),procLines,"100% Chance to cast level 18 Chilling Armor when you Level-Up")[0].fallbackRange.empty());

    // Bramble Song: Eagleeye16 + Trump's second specification24, stat224.
    put(f.root.data(),0x1c,0xa0000000u);
    f.Property(0,17,224);f.Property(1,17,218);f.Roll(0,0,0,0,"Eagleeye");f.Roll(1,1,0,0,"Trump");
    put(f.rows[0].bytes.data(),0x28,16);put(f.rows[1].bytes.data(),0x28,8);
    put(f.rows[1].bytes.data(),0x34,0);put(f.rows[1].bytes.data(),0x38,24);
    put(f.rows[1].bytes.data(),0x3c,0);put(f.rows[1].bytes.data(),0x40,0);
    put(f.Data()-24,4,2u);metadata=f.statRows.data()+224*0x144;
    metadata[0x50]=2;metadata[0x51]=1;put(metadata,0x52,std::uint16_t{12});
    f.own=f.Stats({{218,8},{224,40}});f.total=f.own;
    const char* rating="+2000 to Attack Rating (Based on Character Level)";
    text=f.Render(224,rating);
    check(text.find("[+20 - +2000]")!=std::string::npos && text.find("[Combined]")!=std::string::npos);
    check(text.find("+800 to Attack Rating")!=std::string::npos && text.find("+1200 to Attack Rating")!=std::string::npos);
    check(text.find("[Eagleeye]")!=std::string::npos && text.find("[Trump]")!=std::string::npos);
    f.total=f.Stats({{224,41}});check(f.Labels(224,rating)[0].ownedParts.empty());f.total=f.own;
    put(f.rows[0].bytes.data(),0x28,1);put(f.rows[1].bytes.data(),0x38,1);
    f.own=f.Stats({{224,2}});f.total=f.own;
    check(f.Labels(224,"+100 to Attack Rating (Based on Character Level)")[0].ownedParts.empty());

    // No range-pass identities: recover exact localized source/stat identities.
    f.Property(0,2,16);f.Property(1,1,73);f.Roll(0,0,201,225,"Godly");f.Roll(1,0,16,20,"Silver-Wrought");
    put(f.rows[0].bytes.data(),0x28,0);put(f.rows[1].bytes.data(),0x28,0);
    put(f.rows[1].bytes.data(),0x34,1);put(f.rows[1].bytes.data(),0x38,0);
    put(f.rows[1].bytes.data(),0x3c,16);put(f.rows[1].bytes.data(),0x40,20);
    f.own=f.Stats({{16,230},{73,17}});f.total={};f.Bind();
    const char* defense="+230% Enhanced Defense";
    labels=AffixLabels(f.host.data(),{},defense);
    text=RangeText::Merge(defense,defense,4096,labels).text;
    check(text.find("[+217% - +245%]")!=std::string::npos && text.find("[Combined]")!=std::string::npos);
    check(text.find("?% Enhanced Defense")!=std::string::npos && text.find("[Silver-Wrought]")!=std::string::npos);
    check(text.find("Unavailable")==std::string::npos);
    check(AffixLabels(f.host.data(),{},"+230% Enhanced Defense\n+230% Enhanced Defense").empty());
    f.own=f.Stats({{16,260}});f.Bind();
    text=RangeText::Merge("+260% Enhanced Defense","+260% Enhanced Defense",4096,AffixLabels(f.host.data(),{},"+260% Enhanced Defense")).text;
    check(text.find("[Combined]")==std::string::npos);
    put(f.Data()-24,4,0u);
    put(f.rows[1].bytes.data(),0x34,-1);
    f.Property(0,1,78);f.Roll(0,0,1,3,"of Thorns");f.own=f.Stats({{78,2}});f.total=f.own;f.Bind();
    const char* flatThorns="Attacker Takes Damage of 2";
    text=RangeText::Merge(flatThorns,flatThorns,4096,AffixLabels(f.host.data(),{},flatThorns)).text;
    check(text.find("[1 - 3]")!=std::string::npos && text.find("Unavailable")==std::string::npos);
    f.Property(0,1,85);f.Roll(0,0,2,2,"Loremaster's1");f.own=f.Stats({{85,2}});f.total=f.own;f.Bind();
    text=RangeText::Merge("+2% to Experience Gained","+2% to Experience Gained",4096,AffixLabels(f.host.data(),{},"+2% to Experience Gained")).text;
    check(text.find("\xee\x81\xbe" "U[S] [Loremaster's1]")!=std::string::npos);
    f.Property(0,8,96);f.Roll(0,0,30,30,"of Speed");f.own=f.Stats({{96,30}});f.total=f.own;
    check(f.Render(96,"+30% Faster Run/Walk").find("\xee\x81\xbe" "U[S] [of Speed]")!=std::string::npos);
    f.Property(0,21,83);put(f.props[0].bytes.data(),0x0a,std::uint16_t{3});
    f.Roll(0,0,1,1,"Monk's");put(f.statRows.data()+83*0x144,0x32,std::uint16_t{13});
    f.own={{(std::uint64_t{83}<<32)|3u,1}};f.total=f.own;f.Bind();
    const char* skill="+1 to Paladin Skill Levels";
    labels=AffixLabels(f.host.data(),{},skill);
    text=RangeText::Merge(skill,skill,4096,labels).text;
    check(text.find("[+1]")!=std::string::npos && text.find("[Monk's]")!=std::string::npos);
    put(f.props[0].bytes.data(),0x0a,std::uint16_t{2});check(AffixLabels(f.host.data(),{},skill).empty());
    scalingReady.store(false);originalSingle=nullptr;
    std::puts("Production fixed FHR/proc operands, active level100 bounds, coefficient tiers and refusal cases pass.");
}
void TestOwnedContributionAdapter(){
    TestScalingAffixAdapter();
    TestSocketCapacityAdapter();
    {
        ContributionFixture armor;
        std::array<unsigned char,0x1c0> armorRow{};put(armorRow.data(),0xd4,111);put(armorRow.data(),0xd8,126);
        auto primary=armor.Stats({{31,127}});put(armor.root.data(),0x30,primary.data());put(armor.root.data(),0x38,std::uint64_t{1});put(armor.root.data(),0x40,std::uint64_t{1});
        armor.own=armor.Stats({{7,33*256},{16,94}});armor.socket=armor.Stats({{7,19*256}});
        armor.total=armor.Stats({{7,90*256},{31,246}});armor.Bind();
        // Beast Shell order: ruby, jewel, then own ED list. Neither inserted
        // source has ED, but their presence used to reject the header outright.
        std::array<unsigned char,0x200> ruby{},rubyAllocation{};
        std::array<unsigned char,0xc0> rubyRoot{};auto* rubyData=rubyAllocation.data()+28;
        auto rubyLife=armor.Stats({{7,38*256}});
        put(ruby.data(),0,4u);put(ruby.data(),4,596u);put(ruby.data(),8,3u);ruby[0x1bd]=2;
        put(ruby.data(),0x10,rubyData);put(ruby.data(),0x88,rubyRoot.data());put(rubyData,0,2u);
        put(rubyRoot.data(),0x1c,0x80000000u);put(rubyRoot.data(),0x78,armor.root.data());put(rubyRoot.data(),0xa0,ruby.data());
        put(rubyRoot.data(),0xa8,rubyLife.data());put(rubyRoot.data(),0xb0,std::uint64_t{1});put(rubyRoot.data(),0xb8,std::uint64_t{1});
        put(armor.root.data(),0x90,rubyRoot.data());put(rubyRoot.data(),0x68,armor.socketRoot.data());
        put(armor.socketRoot.data(),0x68,armor.modifier.data());put(armor.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));
        put(armor.JewelData(),0xb0,ruby.data());
        contributionsReady.store(false);check(!ArmorBaseRange(armor.host.data(),armor.Data(),armorRow.data()));
        contributionsReady.store(true);auto base=ArmorBaseRange(armor.host.data(),armor.Data(),armorRow.data());
        check(base&&*base==std::pair{127,127});
        const auto key=RangeText::Analyze("Defense: 246").key;
        check(HeaderText::Fixed("Defense: 246",key,true,base)=="Defense: 246 \xee\x81\xbe" "U(Base: 127 - 127)\xee\x81\xbe" "3");
        check(ModifierOperand(armor.host.data(),16)==94);
        // Socket ED may alter total Defense but is not a host-generation operand.
        armor.socket=armor.Stats({{7,19*256},{16,10}});armor.Bind();check(ModifierOperand(armor.host.data(),16)==94);
        armor.own=armor.Stats({{7,33*256}});armor.Bind();check(!ArmorBaseRange(armor.host.data(),armor.Data(),armorRow.data()));
        armor.own=armor.Stats({{7,33*256},{16,94}});armor.Bind();
        put(armor.socketRoot.data(),0x78,static_cast<unsigned char*>(nullptr));check(!ArmorBaseRange(armor.host.data(),armor.Data(),armorRow.data()));
        put(armor.socketRoot.data(),0x78,armor.root.data());put(rubyRoot.data(),0x20,1u);check(!ArmorBaseRange(armor.host.data(),armor.Data(),armorRow.data()));
        put(rubyRoot.data(),0x20,0u);put(rubyData,0xb0,armor.jewel.data());check(!ArmorBaseRange(armor.host.data(),armor.Data(),armorRow.data()));
        put(rubyData,0xb0,static_cast<unsigned char*>(nullptr));
        put(armor.Data(),0x18,0x00400010u);primary[0].value=190;
        base=ArmorBaseRange(armor.host.data(),armor.Data(),armorRow.data());check(base&&*base==std::pair{190,190});
        std::puts("Generated ED armor Base header accepts owned socket siblings without counting their ED; malformed ownership/state/order still declines.");
    }
    ContributionFixture f;
    f.Property(0,8,93);f.Property(1,8,93);f.Roll(0,0,20,20,"of Alacrity");f.Roll(1,1,15,15,"of Fervor");
    f.own=f.Stats({{93,20}});f.socket=f.Stats({{93,15}});f.total=f.Stats({{93,35}});
    auto text=f.Render(93,"+35% Increased Attack Speed");
    check(text.find("+20% Increased Attack Speed")!=std::string::npos&&text.find("[Socket 1] [S] [of Fervor]")!=std::string::npos&&text.find("[Combined]")!=std::string::npos);
    auto labels=f.Labels(93,"+35% Increased Attack Speed");check(labels.size()==1&&labels[0].ownedParts.size()==2);
    contributionsReady.store(false);check(f.Labels(93,"+35% Increased Attack Speed")[0].ownedParts.empty());contributionsReady.store(true);
    f.Property(0,1,7);f.Property(1,1,7);f.Roll(0,0,11,20,"of the Wolf");f.Roll(1,1,9,20,"of Hope");
    f.own=f.Stats({{7,13*256}});f.socket=f.Stats({{7,16*256}});f.total=f.Stats({{7,29*256}});
    text=f.Render(7,"+29 to Life");check(text.find("[+20 - +40]")!=std::string::npos&&text.find("+13 to Life")!=std::string::npos&&text.find("+16 to Life")!=std::string::npos);
    check(RangeText::Merge("+29 to Life","+29 to Life",16,f.Labels(7,"+29 to Life")).text=="+29 to Life");
    f.total=f.Stats({{7,30*256}});check(f.Labels(7,"+29 to Life")[0].ownedParts.empty());f.total=f.Stats({{7,29*256}});
    f.own.push_back({std::uint64_t{394}<<32,1});check(f.Labels(7,"+29 to Life")[0].ownedParts.empty());f.own.pop_back();
    put(f.socketRoot.data(),0x20,1u);check(f.Labels(7,"+29 to Life")[0].ownedParts.empty());put(f.socketRoot.data(),0x20,0u);
    f.jewel[0x1bd]=3;check(f.Labels(7,"+29 to Life")[0].ownedParts.empty());f.jewel[0x1bd]=2;
    f.Property(0,6,0xffff);f.Property(1,6,0xffff);f.Roll(0,0,81,100,"of Evisceration");f.Roll(1,1,10,15,"of Carnage");
    f.own=f.Stats({{22,96}});f.socket=f.Stats({{22,11}});f.total=f.Stats({{22,146}});
    text=f.Render(22,"+107 to Maximum Weapon Damage");check(text.find("[+91 - +115]")!=std::string::npos&&text.find("+96 to Maximum Weapon Damage")!=std::string::npos);
    // Computed aggregate damage and consumed ED operands cannot replace leaf operands.
    f.Property(0,7,0xffff);f.Property(1,7,0xffff);f.Roll(0,0,234,266,"Cruel");f.Roll(1,1,31,40,"Ruby");
    put(fakeTables,0x1608,f.rows.data());f.own=f.Stats({{17,249},{18,249}});f.socket=f.Stats({{17,38},{18,38}});f.total={};
    text=f.Render(17,"+287% Enhanced Weapon Damage");check(text.find("[+265% - +306%]")!=std::string::npos&&text.find("+249% Enhanced Weapon Damage")!=std::string::npos);
    f.socket=f.Stats({{17,38},{18,37}});check(f.Labels(17,"+287% Enhanced Weapon Damage")[0].ownedParts.empty());
    // A socket-only line receives a label even with no matching host definition.
    f.Property(1,1,149);f.Roll(1,1,4,7,"of Desire");f.own={};f.socket=f.Stats({{149,7}});f.total=f.socket;
    text=f.Render(149,"+7 Cold Absorb");check(text.find("[+4 - +7]")!=std::string::npos&&text.find("[Socket 1]")!=std::string::npos);

    // Public active table spans drive rune context and completed corruption.
    D2RL::PluginContext context{};D2RL::DataTableService service{};service.getTable=FixtureTable;
    contributionContext=&context;contributionTables=&service;
    auto table=[](D2RL::DataTables::TableId id,const unsigned char* rows,std::size_t count,std::size_t stride){contributionFixtureTables[static_cast<unsigned>(id)]={rows,count,stride};};
    table(D2RL::DataTables::TableId::Properties,reinterpret_cast<const unsigned char*>(f.props.data()),f.props.size(),sizeof(Affixes::Property));
    std::array<unsigned char,448*3> items{};std::array<unsigned char,232*4> types{};std::array<unsigned char,192> gems{};
    put(items.data(),0x12e,std::int16_t{2});put(items.data(),0x130,std::int16_t{-1});put(items.data()+448,0x80,ContributionDefinitions::Code("ka3 "));
    put(types.data(),0,ContributionDefinitions::Code("armo"));put(types.data()+232,0,ContributionDefinitions::Code("weap"));
    put(types.data()+232*2,0,ContributionDefinitions::Code("swor"));put(types.data()+232*2,4,std::int16_t{1});put(types.data()+232*3,0,ContributionDefinitions::Code("shld"));
    table(D2RL::DataTables::TableId::Items,items.data(),3,448);table(D2RL::DataTables::TableId::ItemTypes,types.data(),4,232);
    table(D2RL::DataTables::TableId::Gems,gems.data(),1,192);
    std::memcpy(gems.data(),"El Rune",8);put(gems.data(),0x28,2u);
    for(unsigned c=0;c<3;++c)for(unsigned n=0;n<3;++n)put(gems.data(),0x30+c*0x30+n*16,UINT32_MAX);
    f.Property(1,1,19);f.Property(2,1,89);f.Property(3,1,31);
    put(gems.data(),0x30,1u);put(gems.data(),0x38,50);put(gems.data(),0x3c,50);
    put(gems.data(),0x40,2u);put(gems.data(),0x48,1);put(gems.data(),0x4c,1);
    put(gems.data(),0x60,3u);put(gems.data(),0x68,15);put(gems.data(),0x6c,15);
    put(f.Data()-24,0,0u);put(f.JewelData()-24,0,0u);put(f.JewelData(),0,2u);
    f.own={};f.socket=f.Stats({{19,50},{89,1}});f.total=f.socket;
    text=f.Render(19,"+50 to Attack Rating");check(text.find("[Socket 1] [El Rune]")!=std::string::npos);
    contributionFixtureBusy=true;check(f.Labels(19,"+50 to Attack Rating").empty());contributionFixtureBusy=false;
    put(items.data(),0x12e,std::int16_t{0});f.socket=f.Stats({{31,15}});f.total=f.Stats({{31,140}});
    text=f.Render(31,"+15 Defense");check(text.find("[Socket 1] [El Rune]")!=std::string::npos);
    put(items.data(),0x12e,std::int16_t{2});f.Property(1,1,48);f.Property(2,1,49);
    std::memcpy(gems.data(),"Perfect Ruby",13);put(gems.data(),0x38,15);put(gems.data(),0x3c,15);put(gems.data(),0x48,20);put(gems.data(),0x4c,20);
    f.socket=f.Stats({{48,15},{49,20}});f.total=f.socket;text=f.Render(48,"Adds 15-20 Weapon Fire Damage");
    check(text.find("[Socket 1] [Perfect Ruby]")!=std::string::npos);
    // Clear socket, preserve the magic original and split only the fixed winner.
    put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));put(f.Data(),0,4u);put(f.Data()-24,0,1u);
    f.Property(0,7,0xffff);f.Roll(0,0,267,300,"Cruel");f.Property(1,1,60);f.Property(4,1,361);f.Property(5,1,362);
    std::array<unsigned char,364> cube{};cube[0]=1;cube[2]=0xff;cube[3]=15;cube[12]=2;put(cube.data(),4,362u);put(cube.data(),8,65);
    put(cube.data(),0x14,std::uint16_t{2});put(cube.data(),0x16,1u);put(cube.data(),0x1e,std::uint16_t{1});put(cube.data(),0x20,1u);cube[0x67]=0xfe;
    for(unsigned n=0;n<5;++n)put(cube.data(),0x77+n*12,UINT32_MAX);
    auto spec=[&](unsigned n,unsigned id,int value,unsigned chance){auto* p=cube.data()+0x77+n*12;put(p,0,id);put(p,6,static_cast<std::int16_t>(value));put(p,8,static_cast<std::int16_t>(value));p[10]=static_cast<unsigned char>(chance);};
    spec(0,0,50,100);spec(1,1,8,100);spec(2,4,1,0);spec(3,5,101,0);table(D2RL::DataTables::TableId::CubeMain,cube.data(),1,364);
    f.own=f.Stats({{17,341},{18,341},{60,8},{361,2},{362,168}});f.total=f.Stats({{60,8},{361,2},{362,168}});
    text=f.Render(17,"+341% Enhanced Weapon Damage");check(text.find("[+317% - +350%]")!=std::string::npos&&text.find("+291% Enhanced Weapon Damage")!=std::string::npos&&text.find("+50% Enhanced Weapon Damage [C]")!=std::string::npos);
    text=f.Render(60,"+8% Life Stolen per Hit");check(text.find("[C]")!=std::string::npos);
    contributionFixtureStale=true;check(f.Labels(17,"+341% Enhanced Weapon Damage")[0].ownedParts.empty());contributionFixtureStale=false;
    f.own.back().value=101;check(f.Render(17,"+341% Enhanced Weapon Damage").find("[C]")==std::string::npos);
    f.own.back().value=67;check(f.Render(17,"+341% Enhanced Weapon Damage").find("[C]")==std::string::npos);
    f.own.back().value=168;put(f.Data(),0x18,0x04000000u);check(f.Labels(17,"+341% Enhanced Weapon Damage")[0].ownedParts.empty());
    std::puts("Production owned capture, variable jewel/Life/ED/flat-max, socket-only, El contexts, Ruby pair and corruption splits pass; unsupported sources retain prior output.");
}
}
