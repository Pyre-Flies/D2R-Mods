// Full production source capture -> active definitions -> labels -> merge.
#include <fstream>
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
    std::array<Affixes::Property,8> props{};
    std::vector<unsigned char> statRows=std::vector<unsigned char>(450*0x144);
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
        for(unsigned stat=0;stat<450;++stat){put(statRows.data()+stat*0x144,0,static_cast<std::uint16_t>(stat));
            put(statRows.data()+stat*0x144,0x32,std::uint16_t{19});}
        put(statRows.data()+7*0x144,0x14,std::uint8_t{8});
        put(fakeTables,0x15e8,rows.data());put(fakeTables,0x15f0,std::uint64_t{2});
        put(fakeTables,0x1600,rows.data());put(fakeTables,0x1608,rows.data()+2);put(fakeTables,0x1610,rows.data()+2);
        put(fakeTables,0x240,props.data());put(fakeTables,0x248,std::uint64_t{props.size()});
        put(fakeTables,0x1258,statRows.data());put(fakeTables,0x1260,std::uint64_t{450});
        getTables=FakeTables;eligibleAffix=FakeEligible;contributionsReady.store(true);
        contributionContext=nullptr;contributionTables=nullptr;contributionFixtureTables={};
        contributionFixtureBusy=contributionFixtureStale=false;
    }
    ~ContributionFixture(){contributionsReady.store(false);contributionContext=nullptr;contributionTables=nullptr;
        getTables=nullptr;eligibleAffix=nullptr;std::memset(fakeTables,0,sizeof(fakeTables));}
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
    if(grouped && Affixes::Read<std::uint16_t>(static_cast<const unsigned char*>(metadata),0)>=329)return false;
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
    // Unique and Set items clamp Max Sockets to their innate sock(194) property.
    nativeSocketCapacity=FixtureSocketCapacity;lookupHeader=FixtureSocketLookup;fixtureSocketMaximum=6;
    f.Property(0,14,194);
    std::array<unsigned char,348> uniqueRow{};for(unsigned k=0;k<12;++k)put(uniqueRow.data(),0x98+k*16,UINT32_MAX);
    put(uniqueRow.data(),0x98,0u);put(uniqueRow.data(),0xa0,1);put(uniqueRow.data(),0xa4,2);
    put(fakeTables,0x13c8,uniqueRow.data());put(fakeTables,0x13d0,std::uint64_t{1});
    put(f.Data(),0,7u);put(f.Data(),0x34,0);
    check(ItemSocketCapacity(f.host.data(),1)==2);
    check(AnnotateSocketHeader(f.host.data(),body)==modifiers+"\xee\x81\xbe" "0Max Sockets: 2\n"+headers);
    // Unsocketed Set/Unique items replace Core's base Max Sockets line via PublishSocketOverride.
    originalAssign=RecordHeader;
    const std::string unsocketed="Max Sockets: 6\n"+headers;
    NativeText unsocketedReceiver{unsocketed.data(),unsocketed.size()};
    std::string capKey,capLabel;
    check(FormatSocketCapacityLabel(ItemSocketCapacity(f.host.data(),0),capKey,capLabel));
    pendingSocketOverride={&unsocketedReceiver,capKey,capLabel};
    PublishSocketOverride(true,&unsocketedReceiver,unsocketed.data(),unsocketed.size());
    check(publishedHeader=="\xee\x81\xbe" "0Max Sockets: 2\n"+headers);
    originalAssign=nullptr;
    put(f.Data(),0,6u);fixtureSocketMaximum=4;
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
    const std::string procMarker="\xee\x81\xbe";
    const std::string nativeProc="100% Chance to cast level "+procMarker+"U(12-17)"+procMarker+"3 Chilling Armor when you Level-Up";
    text=RangeText::Merge(proc,nativeProc,4096,labels).text;
    check(text.find("[100%]")!=std::string::npos && text.find("12 - 17")==std::string::npos);
    // The reported death-proc uses a different stat and a native skill-level range.
    f.Property(0,11,197);put(f.rows[0].bytes.data(),0x28,48);put(f.rows[0].bytes.data(),0x30,48);
    f.own={{(std::uint64_t{197}<<32)|3120u,100}};f.total=f.own;f.Bind();
    const char* nova="100% Chance to cast level 48 Nova when you Die";
    const std::vector<CapturedLine> novaLines={{RangeText::Analyze(nova).key,197,3120,false}};
    const auto novaLabels=AffixLabels(f.host.data(),novaLines,nova);
    const std::string nativeNova="100% Chance to cast level "+procMarker+"U(36-48)"+procMarker+"3 Nova when you Die";
    text=RangeText::Merge(nova,nativeNova,4096,novaLabels).text;
    check(text.find("[100%]")!=std::string::npos && text.find("36 - 48")==std::string::npos && text.find("level 48 Nova")!=std::string::npos);
    f.Property(0,11,199);put(f.rows[0].bytes.data(),0x28,60);put(f.rows[0].bytes.data(),0x30,17);
    f.own={{(std::uint64_t{199}<<32)|3857u,100}};f.total=f.own;f.Bind();

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
void TestLevelHeaderAdapter(){
    ContributionFixture f;std::array<unsigned char,448> row{};
    put(f.Data(),LevelProfile::ItemLevelOffset,99);row[LevelProfile::QualityLevelOffset]=20;
    levelsReady.store(true);lookupHeader=FixtureSocketLookup;
    const auto allocation=f.allocation;const auto originalRow=row;
    const std::string label="\xee\x81\xbe" "0(Ilvl: 99, Qlvl: 20)";
    const std::string prefix="Socketed (2)\n+90 to Life [Combined]\nVery Fast Attack Speed\nMax Sockets: 4\n";
    const std::string headers="Required Level: 63\nRequired Strength: 55\nOne-Hand Damage: 15 to 153 (Base: 4 - 28)";
    const auto actual=prefix+headers;
    const auto annotated=AnnotateLevelHeader(actual,f.Data(),row.data());
    check(annotated==prefix+label+'\n'+headers);
    check(f.allocation==allocation && row==originalRow);
    check(AnnotateLevelHeader(annotated,f.Data(),row.data())==annotated);
    const std::string noLevel="Required Strength: 55\nOne-Hand Damage: 3 to 19";
    check(AnnotateLevelHeader(prefix+noLevel,f.Data(),row.data())==prefix+label+'\n'+noLevel);
    const std::string belt="Defense: 2 \xee\x81\xbe" "U(Base: 2 - 2)\xee\x81\xbe" "3";
    check(AnnotateLevelHeader(belt,f.Data(),row.data())==label+'\n'+belt);
    check(AnnotateLevelHeader("+5 to Life",f.Data(),row.data())=="+5 to Life");
    check(AnnotateLevelHeader("+5 to Life\n",f.Data(),row.data())=="+5 to Life\n");
    check(AnnotateLevelHeader("",f.Data(),row.data()).empty());
    put(f.Data(),0,4u);row[LevelProfile::QualityLevelOffset]=1;
    const std::string amulet="3% Chance to cast level 3 Inner Sight on striking\n\xee\x81\xbe" "3Amulet of Inner Sight (99)";
    const std::string expected="3% Chance to cast level 3 Inner Sight on striking\n\xee\x81\xbe" "0(Ilvl: 99, Qlvl: 1)\n\xee\x81\xbe" "3Amulet of Inner Sight (99)";
    check(AnnotateLevelHeader(amulet,f.Data(),row.data())==expected);
    check(AnnotateLevelHeader(amulet+'\n',f.Data(),row.data())==expected+'\n');
    check(AnnotateLevelHeader(expected,f.Data(),row.data())==expected);
    put(f.Data(),0,6u);check(AnnotateLevelHeader(amulet,f.Data(),row.data())==amulet);
    put(f.Data(),0,4u);put(f.Data(),LevelProfile::ItemLevelOffset,98);
    check(AnnotateLevelHeader(amulet,f.Data(),row.data())==amulet);
    put(f.Data(),0,6u);put(f.Data(),LevelProfile::ItemLevelOffset,99);row[LevelProfile::QualityLevelOffset]=20;
    const auto ambiguous=actual+"\nRequired Level: 63";
    check(AnnotateLevelHeader(ambiguous,f.Data(),row.data())==ambiguous);
    originalAssign=RecordHeader;NativeText receiver{actual.data(),actual.size()};
    pendingHeader={&receiver,actual,annotated};
    PublishHeader(true,&receiver,actual.data(),actual.size());check(publishedHeader==annotated);
    PublishHeader(true,&receiver,actual.data(),actual.size());check(publishedHeader==actual);
    NativeText empty{};pendingHeader={&empty,"",label};
    PublishHeader(true,&empty,nullptr,0);check(publishedHeader==label);
    pendingHeader={};originalAssign=nullptr;
    put(f.Data(),LevelProfile::ItemLevelOffset,0);
    check(AnnotateLevelHeader(actual,f.Data(),row.data())==actual);
    check(Affixes::Read<int>(f.Data(),LevelProfile::ItemLevelOffset)==0); // Never call the repairing getter.
    put(f.Data(),LevelProfile::ItemLevelOffset,-1);
    check(AnnotateLevelHeader(actual,f.Data(),row.data())==actual);
    put(f.Data(),LevelProfile::ItemLevelOffset,300);row[LevelProfile::QualityLevelOffset]=255;
    check(AnnotateLevelHeader(actual,f.Data(),row.data()).find("(Ilvl: 300, Qlvl: 255)")!=std::string::npos);
    row[LevelProfile::QualityLevelOffset]=0;
    check(AnnotateLevelHeader(actual,f.Data(),row.data()).find("Qlvl: 0)")!=std::string::npos);
    check(AnnotateLevelHeader(actual,nullptr,row.data())==actual);
    check(AnnotateLevelHeader(actual,f.Data(),nullptr)==actual);
    lookupHeader=nullptr;check(AnnotateLevelHeader(actual,f.Data(),row.data())==actual);
    lookupHeader=FixtureSocketLookup;levelsReady.store(false);
    check(AnnotateLevelHeader(actual,f.Data(),row.data())==actual);
    lookupHeader=nullptr;
    std::puts("Level header fields, complete weapon/armor order, no-requirement/empty bodies, publication and refusal pass.");
}
void TestRandomizedItemAffixCombinations(){
    // Comprehensive validation of newly fixed rule gaps + 2,500 randomized multi-affix items
    // covering combining/non-combining damage endpoints, dgrp grouping/splitting, op=11 stats,
    // fn=[21,3] elemental skill stats, layered skills, procs, per-level stats, sockets,
    // corruptions, crafts, enchantments, and runewords.
    ContributionFixture f;
    f.statRows.assign(440*0x144,0);
    for(unsigned stat=0;stat<440;++stat){
        put(f.statRows.data()+stat*0x144,0,static_cast<std::uint16_t>(stat));
        put(f.statRows.data()+stat*0x144,0x32,std::uint16_t{19});
        put(f.statRows.data()+stat*0x144,0x30,static_cast<std::uint16_t>(10+stat));
    }
    // Distinct priorities for composite damage families:
    put(f.statRows.data()+21*0x144,0x30,std::uint16_t{120});
    put(f.statRows.data()+23*0x144,0x30,std::uint16_t{118});
    put(f.statRows.data()+48*0x144,0x30,std::uint16_t{102});
    put(f.statRows.data()+50*0x144,0x30,std::uint16_t{99});
    put(f.statRows.data()+52*0x144,0x30,std::uint16_t{97});
    put(f.statRows.data()+54*0x144,0x30,std::uint16_t{96});
    put(f.statRows.data()+57*0x144,0x30,std::uint16_t{92});
    // ValShift = 8 on Life (7) and Mana (9)
    put(f.statRows.data()+7*0x144,0x14,std::uint8_t{8});
    put(f.statRows.data()+9*0x144,0x14,std::uint8_t{8});
    // op = 8/9 on attributes (0..3), op = 11 on hp%/mana%/str%/dex%/vit%/enr% (76,77,413..416), op = 13 on 16,17,18,75
    f.statRows[1*0x144+0x50]=8;f.statRows[3*0x144+0x50]=9;
    for(unsigned s:{76u,77u,413u,414u,415u,416u})f.statRows[s*0x144+0x50]=11;
    for(unsigned s:{16u,17u,18u,75u})f.statRows[s*0x144+0x50]=13;
    // dgrp groups: 1 (0..3), 2 (39,41,43,45), 3 (329..332), 4 (333..336)
    for(unsigned s:{0u,1u,2u,3u})put(f.statRows.data()+s*0x144,0x3a,std::uint16_t{1});
    for(unsigned s:{39u,41u,43u,45u})put(f.statRows.data()+s*0x144,0x3a,std::uint16_t{2});
    for(unsigned s:{329u,330u,331u,332u})put(f.statRows.data()+s*0x144,0x3a,std::uint16_t{3});
    for(unsigned s:{333u,334u,335u,336u})put(f.statRows.data()+s*0x144,0x3a,std::uint16_t{4});
    // Layered skill descfuncs:
    put(f.statRows.data()+83*0x144,0x32,std::uint16_t{13});
    put(f.statRows.data()+188*0x144,0x32,std::uint16_t{14});
    put(f.statRows.data()+107*0x144,0x32,std::uint16_t{27});
    put(f.statRows.data()+387*0x144,0x32,std::uint16_t{28});
    put(f.statRows.data()+97*0x144,0x32,std::uint16_t{0});
    put(f.statRows.data()+126*0x144,0x32,std::uint16_t{0});
    put(fakeTables,0x1258,f.statRows.data());put(fakeTables,0x1260,std::uint64_t{440});
    put(fakeTables,0x14d0,6u);put(fakeTables,0x14d4,63u);

    // Build a comprehensive property table (54 properties)
    std::vector<Affixes::Property> props(54);
    auto setProp1=[&](unsigned id,unsigned fn,unsigned stat,unsigned val=0){
        props[id]={};props[id].bytes[0x18]=static_cast<unsigned char>(fn);
        put(props[id].bytes.data(),0x20,static_cast<std::uint16_t>(stat));
        if(val)put(props[id].bytes.data(),0x0a,static_cast<std::uint16_t>(val));
    };
    setProp1(0,1,0);   // str
    setProp1(1,1,1);   // enr
    setProp1(2,1,2);   // dex
    setProp1(3,1,3);   // vit
    // 4: all-stats (0,1,2,3)
    setProp1(4,1,0);for(unsigned k=1;k<4;++k){props[4].bytes[0x18+k]=3;put(props[4].bytes.data(),0x20+k*2,static_cast<std::uint16_t>(k));}
    setProp1(5,1,7);   // hp (shift 8)
    setProp1(6,1,9);   // mana (shift 8)
    setProp1(7,1,76);  // hp% (op 11)
    setProp1(8,1,77);  // mana% (op 11)
    setProp1(9,1,413); // str% (op 11)
    setProp1(10,1,414);// dex% (op 11)
    setProp1(11,2,16); // ac% (op 13)
    setProp1(12,7,17); // dmg% (17,18 op 13)
    setProp1(13,1,19); // att
    setProp1(14,5,21); // dmg-min (21/23)
    setProp1(15,6,22); // dmg-max (22/24)
    // 16: dmg-norm (fn 15 stat 21, fn 16 stat 22)
    setProp1(16,15,21);props[16].bytes[0x19]=16;put(props[16].bytes.data(),0x22,std::uint16_t{22});
    setProp1(17,1,39); // res-fire
    setProp1(18,1,41); // res-ltng
    setProp1(19,1,43); // res-cold
    setProp1(20,1,45); // res-pois
    // 21: res-all (39,41,43,45)
    setProp1(21,1,39);
    props[21].bytes[0x19]=3;put(props[21].bytes.data(),0x22,std::uint16_t{41});
    props[21].bytes[0x1a]=3;put(props[21].bytes.data(),0x24,std::uint16_t{43});
    props[21].bytes[0x1b]=3;put(props[21].bytes.data(),0x26,std::uint16_t{45});
    // 22: dmg-fire (fn 15 stat 48, fn 16 stat 49), 23: fire-min (48), 24: fire-max (49)
    setProp1(22,15,48);props[22].bytes[0x19]=16;put(props[22].bytes.data(),0x22,std::uint16_t{49});
    setProp1(23,1,48);
    setProp1(24,1,49);
    // 25: dmg-cold (fn 15 stat 54, fn 16 stat 55), 26: cold-min (54), 27: cold-max (55)
    setProp1(25,15,54);props[25].bytes[0x19]=16;put(props[25].bytes.data(),0x22,std::uint16_t{55});
    setProp1(26,1,54);
    setProp1(27,1,55);
    // 28..31: extra-fire/ltng/cold/pois (329..332), 32: extra-elem (329..332)
    for(unsigned k=0;k<4;++k)setProp1(28+k,1,329+k);
    setProp1(32,1,329);for(unsigned k=1;k<4;++k){props[32].bytes[0x18+k]=3;put(props[32].bytes.data(),0x20+k*2,static_cast<std::uint16_t>(329+k));}
    // 33..36: pierce-fire/ltng/cold/pois (333..336), 37: pierce-elem (333..336)
    for(unsigned k=0;k<4;++k)setProp1(33+k,1,333+k);
    setProp1(37,1,333);for(unsigned k=1;k<4;++k){props[37].bytes[0x18+k]=3;put(props[37].bytes.data(),0x20+k*2,static_cast<std::uint16_t>(333+k));}
    // 38: fireskill (fn 21 val 1 stat 126, fn 3 stat 399), 39: coldskill (fn 21 val 2 stat 126, fn 3 stat 400)
    setProp1(38,21,126,1);props[38].bytes[0x19]=3;put(props[38].bytes.data(),0x22,std::uint16_t{399});
    setProp1(39,21,126,2);props[39].bytes[0x19]=3;put(props[39].bytes.data(),0x22,std::uint16_t{400});
    // 40: allskills (127), 41: paladin skills (fn 21 val 3 stat 83), 42: oskill (fn 22 stat 387, fn 24 stat 97)
    setProp1(40,1,127);
    setProp1(41,21,83,3);
    setProp1(42,22,387);props[42].bytes[0x19]=24;put(props[42].bytes.data(),0x22,std::uint16_t{97});
    // 43: swing1 (93), 44: cast1 (102), 45: mag% (80), 46: upgrade/minor (fn 17 stat 392), 47: skill (fn 22 stat 107)
    setProp1(43,8,93);
    setProp1(44,8,102);
    setProp1(45,1,80);
    setProp1(46,17,392);
    setProp1(47,22,107);
    setProp1(48,1,79);    // gold% (79)
    setProp1(49,20,0xffff); // indestruct (fn 20, stat 0xffff -> sets stat 152)
    // 50: dmg-elem (48/49, 50/51, 54/55, plus slot 6 fn 17 stat 56 coldlength)
    setProp1(50,15,48);props[50].bytes[0x19]=16;put(props[50].bytes.data(),0x22,std::uint16_t{49});
    props[50].bytes[0x1a]=15;put(props[50].bytes.data(),0x24,std::uint16_t{50});
    props[50].bytes[0x1b]=16;put(props[50].bytes.data(),0x26,std::uint16_t{51});
    props[50].bytes[0x1c]=15;put(props[50].bytes.data(),0x28,std::uint16_t{54});
    props[50].bytes[0x1d]=16;put(props[50].bytes.data(),0x2a,std::uint16_t{55});
    props[50].bytes[0x1e]=17;put(props[50].bytes.data(),0x2c,std::uint16_t{56});
    setProp1(51,12,107);  // skill-rand (fn 12 stat 107)
    setProp1(52,1,20);    // block (stat 20)
    setProp1(53,1,74);    // regen (stat 74)

    put(fakeTables,0x240,props.data());put(fakeTables,0x248,static_cast<std::uint64_t>(props.size()));
    propertySourcesReady.store(true);scalingReady.store(true);randomSkillsReady.store(true);

    D2RL::DataTableService service{.serviceSize=D2RL::DataTableServiceRequiredSize,.serviceVersion=D2RL::DataTableService::AbiVersion,.getTable=FixtureTable};
    D2RL::PluginContext context{};contributionContext=&context;contributionTables=&service;
    std::array<unsigned char,448*2> items{};std::array<unsigned char,232*2> types{};
    put(items.data(),0x12e,std::int16_t{0});put(items.data(),0x130,std::int16_t{-1});
    put(types.data(),0,ContributionDefinitions::Code("weap"));put(types.data()+232,0,ContributionDefinitions::Code("armo"));
    contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::Properties)]={reinterpret_cast<const unsigned char*>(props.data()),props.size(),sizeof(Affixes::Property)};
    contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::Items)]={items.data(),2,448};
    contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::ItemTypes)]={types.data(),2,232};

    // 1. Verify Real CubeMain Enchantment format (param=0, min=1, max=1) + op=11 (hp%) + fn=[21,3] (fireskill):
    std::array<unsigned char,364> enchCube{};
    enchCube[0]=1;enchCube[2]=0xff;enchCube[3]=16;enchCube[12]=2;put(enchCube.data(),4,392u);enchCube[0x67]=0xfe;
    put(enchCube.data(),0x14,std::uint16_t{2});put(enchCube.data(),0x16,0u);
    put(enchCube.data(),0x1e,std::uint16_t{1});put(enchCube.data(),0x20,1u);
    for(unsigned n=0;n<5;++n)put(enchCube.data(),0x77+n*12,UINT32_MAX);
    // mod 1: upgrade/minor (prop 46) with param=0, min=1, max=1 (real Reimagined CubeMain format)
    put(enchCube.data()+0x77,0,46u);put(enchCube.data()+0x77,4,std::int16_t{0});
    put(enchCube.data()+0x77,6,std::int16_t{1});put(enchCube.data()+0x77,8,std::int16_t{1});enchCube[0x77+10]=100;
    // mod 2: hp% (prop 7, stat 76, op 11) 5..10
    put(enchCube.data()+0x77+12,0,7u);put(enchCube.data()+0x77+12,6,std::int16_t{5});put(enchCube.data()+0x77+12,8,std::int16_t{10});enchCube[0x77+12+10]=100;
    // mod 3: fireskill (prop 38, fn=[21,3] stat 399) 1..2
    put(enchCube.data()+0x77+24,0,38u);put(enchCube.data()+0x77+24,6,std::int16_t{1});put(enchCube.data()+0x77+24,8,std::int16_t{2});enchCube[0x77+24+10]=100;
    contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::CubeMain)]={enchCube.data(),1,364};

    // Build affix pool for randomized item generation (16 suffixes, 16 prefixes, 2 automagic [Base])
    std::vector<Affixes::Row> affixPool(34);
    auto defAffix=[&](unsigned idx,const char* name,unsigned p1,int l1,int h1,int p2=-1,int l2=0,int h2=0){
        auto* r=affixPool[idx].bytes.data();std::memset(r,0,sizeof(Affixes::Row));
        std::memcpy(r,name,std::strlen(name));
        r[0x54]=r[0x64]=r[0x82]=1;put(r,0x58,static_cast<int>(idx+1));put(r,0x5c,1);
        put(r,0x24,static_cast<int>(p1));put(r,0x2c,l1);put(r,0x30,h1);
        put(r,0x34,p2);put(r,0x3c,l2);put(r,0x40,h2);
        put(r,0x44,-1);
    };
    // Suffixes (0..15):
    defAffix(0,"of the Titan",0,15,20);          // str 15-20
    defAffix(1,"of Precision",2,10,15);          // dex 10-15
    defAffix(2,"of the Whale",5,80,100);         // hp 80-100 (ValShift 8)
    defAffix(3,"of Vitality",7,5,10);            // hp% 5-10 (op 11)
    defAffix(4,"of Maiming",15,10,15);           // dmg-max 10-15 (stat 22/24)
    defAffix(5,"of Craftsmanship",15,2,4);       // dmg-max 2-4
    defAffix(6,"of Transcendence",14,12,16);     // dmg-min 12-16 (stat 21/23)
    defAffix(7,"of Incineration",24,25,40);      // fire-max 25-40 (stat 49)
    defAffix(8,"of Burning",23,15,20);           // fire-min 15-20 (stat 48)
    defAffix(9,"of the Glacier",25,10,30);       // dmg-cold 10-30 (stat 54=10, 55=30)
    defAffix(10,"of Readiness",43,10,20);        // ias 10-20 (stat 93)
    defAffix(11,"of the Magus",44,15,25);        // fcr 15-25 (stat 102)
    defAffix(12,"of Good Luck",45,20,35);        // mf 20-35 (stat 80)
    defAffix(13,"of Perfection",4,5,10);         // all-stats 5-10 (dgrp 1)
    defAffix(14,"of StrengthPercent",9,5,12);    // str% 5-12 (stat 413, op 11)
    defAffix(15,"of Frostbite",27,20,35);        // cold-max 20-35 (stat 55)
    // Prefixes (16..31):
    defAffix(16,"Merciless",12,100,150);         // dmg% 100-150 (17,18)
    defAffix(17,"Saintly",11,80,120);            // ac% 80-120 (16)
    defAffix(18,"Strange",13,150,250);           // att 150-250 (19)
    defAffix(19,"Sharp",16,5,12);                // dmg-norm 5-12 (21=5, 22=12)
    defAffix(20,"Ruby",17,31,40);                // res-fire 31-40 (39)
    defAffix(21,"Amber",18,31,40);               // res-ltng 31-40 (41)
    defAffix(22,"Sapphire",19,31,40);            // res-cold 31-40 (43)
    defAffix(23,"Emerald",20,31,40);             // res-pois 31-40 (45)
    defAffix(24,"Scintillating",21,12,15);       // res-all 12-15 (dgrp 2)
    defAffix(25,"Fiery",22,8,20);                // dmg-fire 8-20 (48=8, 49=20)
    defAffix(26,"Volcanic",38,1,3);              // fireskill 1-3 (stat 399, fn=[21,3])
    defAffix(27,"Glacial",39,1,3);               // coldskill 1-3 (stat 400, fn=[21,3])
    defAffix(28,"Priest's",41,1,2);              // paladin skills 1-2 (stat 83, layer 3)
    defAffix(29,"Arch-Angel's",40,1,2);          // allskills 1-2 (stat 127)
    defAffix(30,"Pyromancer's",28,5,10,33,5,10); // extra-fire 5-10 (329) + pierce-fire 5-10 (333)
    defAffix(31,"Elementalist's",32,5,8,37,5,8); // extra-elem 5-8 (329..332) + pierce-elem 5-8 (333..336)
    // Automagic [Base] (32..33):
    defAffix(32,"PaladinRes",21,20,30);          // [Base] res-all 20-30
    defAffix(33,"OrbFireSkill",38,1,2);          // [Base] fireskill 1-2

    put(fakeTables,0x15e8,affixPool.data());put(fakeTables,0x15f0,static_cast<std::uint64_t>(affixPool.size()));
    put(fakeTables,0x1600,affixPool.data());
    put(fakeTables,0x1608,affixPool.data()+16);
    put(fakeTables,0x1610,affixPool.data()+32);

    // Quick check of Enchanted item with CubeMain recipe (param=0, min=1, max=1) + op=11 (hp%) + fn=[21,3] (fireskill):
    put(f.Data(),0,6u);put(f.Data()-28,0,0u);
    for(unsigned i=0;i<6;++i)put(f.Data()-24+i*4,0,0u);
    put(f.Data()-24,0,4u); // suffix 3 ("of Vitality", hp% 5-10)
    put(f.Data()-20,0,27u); // prefix 26 ("Volcanic", fireskill 1-3)
    put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));
    f.own={
        {std::uint64_t{76}<<32,16},  // 8 from suffix + 8 from [E]
        {(std::uint64_t{126}<<32)|1,4},
        {std::uint64_t{392}<<32,1},
        {std::uint64_t{399}<<32,4}   // 2 from prefix + 2 from [E]
    };
    f.total=f.own;f.Bind();
    const auto hpPctOut=f.Render(76,"Increase Maximum Life 16%");
    check(hpPctOut.find("[5% - 10%]")!=std::string::npos && hpPctOut.find("[S] [of Vitality]")!=std::string::npos);
    check(hpPctOut.find("[5% - 10%]")!=std::string::npos && hpPctOut.find("[E]")!=std::string::npos);
    check(hpPctOut.find("[10% - 20%]")!=std::string::npos && hpPctOut.find("[Combined]")!=std::string::npos);
    const auto fireSkillOut=f.Render(399,"+4 to Fire Skills");
    check(fireSkillOut.find("[+1 - +3]")!=std::string::npos && fireSkillOut.find("[P] [Volcanic]")!=std::string::npos);
    check(fireSkillOut.find("[+1 - +2]")!=std::string::npos && fireSkillOut.find("[E]")!=std::string::npos);
    check(fireSkillOut.find("[+2 - +5]")!=std::string::npos && fireSkillOut.find("[Combined]")!=std::string::npos);

    // 2. Check residual [E] synthesis on multi-upgraded item with oskill (387), skill (107), hp (7, shift 8), dmg% (17):
    f.own={
        {std::uint64_t{7}<<32,50*256},
        {std::uint64_t{17}<<32,30},{std::uint64_t{18}<<32,30},
        {(std::uint64_t{97}<<32)|429,1},
        {(std::uint64_t{107}<<32)|262,2},
        {(std::uint64_t{387}<<32)|429,1},
        {std::uint64_t{392}<<32,3}
    };
    for(unsigned i=0;i<6;++i)put(f.Data()-24+i*4,0,0u);
    f.total=f.own;f.Bind();
    check(f.Render(7,"+50 to Life").find("[+50]")!=std::string::npos);
    check(f.Render(7,"+50 to Life").find("[E]")!=std::string::npos);
    check(f.Render(17,"+30% Enhanced Damage").find("[+30%]")!=std::string::npos);
    const char* oskillTxt="+1 to Teleport";
    const auto oskillLbl=AffixLabels(f.host.data(),{{RangeText::Analyze(oskillTxt).key,387,429,false}},oskillTxt);
    check(RangeText::Merge(oskillTxt,oskillTxt,4096,oskillLbl).text.find("[+1]")!=std::string::npos);
    const char* sskillTxt="+2 to Wake of Fire (Assassin Only)";
    const auto sskillLbl=AffixLabels(f.host.data(),{{RangeText::Analyze(sskillTxt).key,107,262,false}},sskillTxt);
    check(RangeText::Merge(sskillTxt,sskillTxt,4096,sskillLbl).text.find("[+2]")!=std::string::npos);

    // 2b. Ladder 1-Enchant-Per-Item Multi-Tier Disambiguation, Deduplication, fn=20/12/17, 2H Damage & Shield Block:
    {
        std::array<unsigned char,364*10> ladderCube{};
        auto initEnchRow=[&](unsigned idx)->unsigned char*{
            auto* r=ladderCube.data()+idx*364;
            r[0]=1;r[2]=0xff;r[3]=16;r[12]=2;put(r,4,392u);r[0x67]=0xfe;
            put(r,0x14,std::uint16_t{2});put(r,0x16,0u);
            put(r,0x1e,std::uint16_t{1});put(r,0x20,1u);
            for(unsigned n=0;n<5;++n)put(r,0x77+n*12,UINT32_MAX);
            put(r+0x77,0,46u);put(r+0x77,4,std::int16_t{0});put(r+0x77,6,std::int16_t{1});put(r+0x77,8,std::int16_t{1});r[0x77+10]=100;
            return r;
        };
        auto setMod=[&](unsigned char* r,unsigned slot,unsigned propId,int param,int lo,int hi){
            auto* m=r+0x77+slot*12;
            put(m,0,propId);put(m,4,static_cast<std::int16_t>(param));put(m,6,static_cast<std::int16_t>(lo));put(m,8,static_cast<std::int16_t>(hi));m[10]=100;
        };
        // Row 0 & Row 1: Duplicate specs (e.g. Sol+Amethyst vs Sol+Chaos Onyx -> block +10%)
        setMod(initEnchRow(0),1,52,0,10,10);
        setMod(initEnchRow(1),1,52,0,10,10);
        // Row 2: Tier 1 MF+GF (+10% MF, +20% GF)
        {auto* r=initEnchRow(2);setMod(r,1,45,0,10,10);setMod(r,2,48,0,20,20);}
        // Row 3: Tier 2 MF+GF (+25% MF, +50% GF)
        {auto* r=initEnchRow(3);setMod(r,1,45,0,25,25);setMod(r,2,48,0,50,50);}
        // Row 4: Tier 1 Min Dmg (+15), Row 5: Tier 2 Min Dmg (+30), Row 6: Combo Min+Max (+30 Min, +50 Max)
        setMod(initEnchRow(4),1,14,0,15,15);
        setMod(initEnchRow(5),1,14,0,30,30);
        {auto* r=initEnchRow(6);setMod(r,1,14,0,30,30);setMod(r,2,15,0,50,50);}
        // Row 7: +2 Paladin Skills & Indestructible (fn=20)
        {auto* r=initEnchRow(7);setMod(r,1,41,0,2,2);setMod(r,2,49,0,1,1);}
        // Row 8: +20-200 Elemental Damage (dmg-elem with fn=17 stat 56 param=0)
        setMod(initEnchRow(8),1,50,0,20,200);
        // Row 9: +2 Random Assassin Skills (fn=12, param=2, lo=251, hi=280)
        setMod(initEnchRow(9),1,51,2,251,280);
        contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::CubeMain)]={ladderCube.data(),10,364};

        // (a) Multi-tier MF+GF disambiguation with overlapping variable base suffix ("of Good Luck", 20-35% MF):
        put(f.Data()-24,0,13u); // suffix 12 ("of Good Luck", mf 20-35)
        f.own={{std::uint64_t{79}<<32,20},{std::uint64_t{80}<<32,38},{std::uint64_t{392}<<32,1}};
        f.total=f.own;f.Bind();
        const auto mfTier1=f.Render(80,"38% Better Chance of Getting Magic Items");
        check(mfTier1.find("[20% - 35%]")!=std::string::npos && mfTier1.find("28% Better Chance of Getting Magic Items [S] [of Good Luck]")!=std::string::npos);
        check(mfTier1.find("[10%]")!=std::string::npos && mfTier1.find("10% Better Chance of Getting Magic Items [E]")!=std::string::npos);
        check(mfTier1.find("[30% - 45%]")!=std::string::npos && mfTier1.find("[Combined]")!=std::string::npos);
        check(f.Render(79,"20% Extra Gold from Monsters").find("[20%]")!=std::string::npos);

        // Tier 2 MF+GF (+25% MF, +50% GF) on the same variable base suffix:
        f.own={{std::uint64_t{79}<<32,50},{std::uint64_t{80}<<32,53},{std::uint64_t{392}<<32,1}};
        f.total=f.own;f.Bind();
        const auto mfTier2=f.Render(80,"53% Better Chance of Getting Magic Items");
        check(mfTier2.find("[25%]")!=std::string::npos && mfTier2.find("25% Better Chance of Getting Magic Items [E]")!=std::string::npos);
        check(mfTier2.find("[45% - 60%]")!=std::string::npos && mfTier2.find("[Combined]")!=std::string::npos);

        // (b) 2H Weapon (+30 Min, +50 Max stored in stats 23/24) disambiguates Combo row 6 over single-stat row 5:
        put(f.Data()-24,0,0u);
        f.own={{std::uint64_t{23}<<32,30},{std::uint64_t{24}<<32,50},{std::uint64_t{392}<<32,1}};
        f.total={{std::uint64_t{23}<<32,80},{std::uint64_t{24}<<32,150},{std::uint64_t{392}<<32,1}};
        f.Bind();
        const auto twoHandDmg=f.Render(23,"Adds 30-50 Damage");
        check(twoHandDmg.find("[30 / 50]")!=std::string::npos && twoHandDmg.find("[E]")!=std::string::npos);

        // (b2) 2H Sword with both 1H (21/22) and 2H (23/24) stats, base suffix dmg-max 10..15 (rolled 14) + [E] +30 Min/+50 Max -> Adds 30-64 Damage,
        // even when the range clone only observed "+# to Maximum Damage" (stat 22):
        put(f.Data()-24,0,5u); // Suffix 4 ("of Maiming"): dmg-max 10..15
        f.own={{std::uint64_t{21}<<32,30},{std::uint64_t{22}<<32,64},
               {std::uint64_t{23}<<32,30},{std::uint64_t{24}<<32,64},
               {std::uint64_t{392}<<32,1},{std::uint64_t{443}<<32,1}};
        f.total=f.own;f.Bind();
        {
            const char* actual2H="Adds 30-64 Damage";
            const char* cloneMax="+\xee\x81\xbe" "U(10-15)\xee\x81\xbe" "3 to Maximum Damage";
            const auto lbl2H=AffixLabels(f.host.data(),{{RangeText::Analyze(cloneMax).key,22,0,false,0}},actual2H);
            const auto out2H=RangeText::Merge(actual2H,cloneMax,4096,lbl2H).text;
            check(out2H.find("[30 / 50] Adds 30-50 Damage [E]")!=std::string::npos);
            check(out2H.find("[0 / 10 - 15] Adds 0-14 Damage [S]")!=std::string::npos);
            check(out2H.find("[30 / 60 - 65]")!=std::string::npos && out2H.find("[Combined]")!=std::string::npos);
        }
        put(f.Data()-24,0,0u);

        // (c) Indestructible (fn=20 -> stat 152) + +2 Paladin Skills:
        f.own={{(std::uint64_t{83}<<32)|3,2},{std::uint64_t{152}<<32,1},{std::uint64_t{392}<<32,1}};
        f.total=f.own;f.Bind();
        const char* pal2="+2 to Paladin Skill Levels";
        const auto pal2Lbl=AffixLabels(f.host.data(),{{RangeText::Analyze(pal2).key,83,3,false}},pal2);
        check(RangeText::Merge(pal2,pal2,4096,pal2Lbl).text.find("[+2]")!=std::string::npos);

        // (d) Elemental damage (dmg-elem with fn=17 stat 56 param=0):
        f.own={{std::uint64_t{48}<<32,20},{std::uint64_t{49}<<32,200},
               {std::uint64_t{50}<<32,20},{std::uint64_t{51}<<32,200},
               {std::uint64_t{54}<<32,20},{std::uint64_t{55}<<32,200},{std::uint64_t{392}<<32,1}};
        f.total=f.own;f.Bind();
        const auto elemCold=f.Render(54,"Adds 20-200 Cold Damage");
        check(elemCold.find("[20 / 200]")!=std::string::npos && elemCold.find("[E]")!=std::string::npos);

        // (e) Random Class Skill enchantment (fn=12) on Amulet:
        f.own={{(std::uint64_t{107}<<32)|262,2},{std::uint64_t{392}<<32,1}};
        f.total=f.own;f.Bind();
        const auto randSkillLbl=AffixLabels(f.host.data(),{{RangeText::Analyze(sskillTxt).key,107,262,false}},sskillTxt);
        const auto randSkillOut=RangeText::Merge(sskillTxt,sskillTxt,4096,randSkillLbl).text;
        check(randSkillOut.find("[+2]")!=std::string::npos && randSkillOut.find("[E]")!=std::string::npos);

        // (f) Deduplicated Shield Block (+10% block, stat 20) when total includes intrinsic base shield block (55%):
        f.own={{std::uint64_t{20}<<32,10},{std::uint64_t{392}<<32,1}};
        f.total={{std::uint64_t{20}<<32,55},{std::uint64_t{392}<<32,1}};
        f.Bind();
        const auto blkOut=f.Render(20,"+10% Increased Chance of Blocking");
        check(blkOut.find("[+10%]")!=std::string::npos && blkOut.find("[E]")!=std::string::npos);

        // (g) Mara's Kaleidoscope (Unique quality=7: allskills 2..2, res-all 20..30, all-stats 5..10)
        // with overlapping amulet enchantments (+5 Dex/+5 Vit, +15 Fire Res, +5 All Res, +15 All Res, +1 All Skills):
        {
            std::array<unsigned char,364*5> maraCube{};
            auto initMaraRow=[&](unsigned idx)->unsigned char*{
                auto* r=maraCube.data()+idx*364;
                r[0]=1;r[2]=0xff;r[3]=16;r[12]=2;put(r,4,392u);r[0x67]=0xfe;
                put(r,0x14,std::uint16_t{2});put(r,0x16,0u);
                put(r,0x1e,std::uint16_t{1});put(r,0x20,1u);
                for(unsigned n=0;n<5;++n)put(r,0x77+n*12,UINT32_MAX);
                put(r+0x77,0,46u);put(r+0x77,4,std::int16_t{0});put(r+0x77,6,std::int16_t{1});put(r+0x77,8,std::int16_t{1});r[0x77+10]=100;
                return r;
            };
            // Row 0: +5 Dex & +5 Vit (props 2 & 3)
            {auto* r=initMaraRow(0);setMod(r,1,2,0,5,5);setMod(r,2,3,0,5,5);}
            // Row 1: +15 Fire Resist (prop 17)
            setMod(initMaraRow(1),1,17,0,15,15);
            // Row 2: +5 All Resistances (prop 21)
            setMod(initMaraRow(2),1,21,0,5,5);
            // Row 3: +15 All Resistances (prop 21)
            setMod(initMaraRow(3),1,21,0,15,15);
            // Row 4: +1 All Skills (prop 40)
            setMod(initMaraRow(4),1,40,0,1,1);
            contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::CubeMain)]={maraCube.data(),5,364};

            std::array<unsigned char,348> maraUnique{};
            for(unsigned k=0;k<12;++k)put(maraUnique.data(),0x98+k*16,UINT32_MAX);
            put(maraUnique.data(),0x98,40u);put(maraUnique.data(),0xa0,2);put(maraUnique.data(),0xa4,2);    // allskills 2..2
            put(maraUnique.data(),0xa8,21u);put(maraUnique.data(),0xb0,20);put(maraUnique.data(),0xb4,30);  // res-all 20..30
            put(maraUnique.data(),0xb8,4u);put(maraUnique.data(),0xc0,5);put(maraUnique.data(),0xc4,10);    // all-stats 5..10
            put(fakeTables,0x13c8,maraUnique.data());put(fakeTables,0x13d0,std::uint64_t{1});
            put(f.Data(),0,7u);put(f.Data()+0x34,0,0);

            // Case 1: All Resistances +27 (22 [U] + 5 [E], inside 20..30 base range!) & All Attributes +8:
            f.own={{std::uint64_t{0}<<32,8},{std::uint64_t{1}<<32,8},{std::uint64_t{2}<<32,8},{std::uint64_t{3}<<32,8},
                   {std::uint64_t{39}<<32,27},{std::uint64_t{41}<<32,27},{std::uint64_t{43}<<32,27},{std::uint64_t{45}<<32,27},
                   {std::uint64_t{127}<<32,2},{std::uint64_t{392}<<32,1}};
            f.total=f.own;f.Bind();
            const char* maraRes27="All Resistances +27";
            const auto maraLbl27=AffixLabels(f.host.data(),{{RangeText::Analyze(maraRes27).key,39,0,true,2}},maraRes27);
            const auto maraOut27=RangeText::Merge(maraRes27,maraRes27,4096,maraLbl27).text;
            check(maraOut27.find("[+20 - +30]")!=std::string::npos && maraOut27.find("All Resistances +22 [U]")!=std::string::npos);
            check(maraOut27.find("[+5]")!=std::string::npos && maraOut27.find("All Resistances +5 [E]")!=std::string::npos);
            check(maraOut27.find("[+25 - +35]")!=std::string::npos && maraOut27.find("[Combined]")!=std::string::npos);

            // Case 2: All Resistances +32 (27 [U] + 5 [E]):
            for(unsigned s:{39u,41u,43u,45u})for(auto& e:f.own)if((e.key>>32)==s)e.value=32;
            f.total=f.own;f.Bind();
            const char* maraRes32="All Resistances +32";
            const auto maraLbl32=AffixLabels(f.host.data(),{{RangeText::Analyze(maraRes32).key,39,0,true,2}},maraRes32);
            const auto maraOut32=RangeText::Merge(maraRes32,maraRes32,4096,maraLbl32).text;
            check(maraOut32.find("All Resistances +27 [U]")!=std::string::npos && maraOut32.find("All Resistances +5 [E]")!=std::string::npos);

            // Case 3: All Resistances +35 (30+5 vs 20+15 tie -> merged [5 - 15] [E] and [20 - 30] [U] with +?):
            for(unsigned s:{39u,41u,43u,45u})for(auto& e:f.own)if((e.key>>32)==s)e.value=35;
            f.total=f.own;f.Bind();
            const char* maraRes35="All Resistances +35";
            const auto maraLbl35=AffixLabels(f.host.data(),{{RangeText::Analyze(maraRes35).key,39,0,true,2}},maraRes35);
            const auto maraOut35=RangeText::Merge(maraRes35,maraRes35,4096,maraLbl35).text;
            check(maraOut35.find("[+20 - +30]")!=std::string::npos && maraOut35.find("All Resistances ? [U]")!=std::string::npos);
            check(maraOut35.find("[+5 - +15]")!=std::string::npos && maraOut35.find("All Resistances ? [E]")!=std::string::npos);
            check(maraOut35.find("[+25 - +45]")!=std::string::npos && maraOut35.find("[Combined]")!=std::string::npos);

            // Case 4: All Resistances +40 (25 [U] + 15 [E]):
            for(unsigned s:{39u,41u,43u,45u})for(auto& e:f.own)if((e.key>>32)==s)e.value=40;
            f.total=f.own;f.Bind();
            const char* maraRes40="All Resistances +40";
            const auto maraLbl40=AffixLabels(f.host.data(),{{RangeText::Analyze(maraRes40).key,39,0,true,2}},maraRes40);
            const auto maraOut40=RangeText::Merge(maraRes40,maraRes40,4096,maraLbl40).text;
            check(maraOut40.find("All Resistances +25 [U]")!=std::string::npos && maraOut40.find("All Resistances +15 [E]")!=std::string::npos);

            // Case 5: Mara's enchanted with +5 Dex & +5 Vit when base all-stats rolled 5 (Str=5, Enr=5, Dex=10, Vit=10, All Res=27):
            f.own={{std::uint64_t{0}<<32,5},{std::uint64_t{1}<<32,5},{std::uint64_t{2}<<32,10},{std::uint64_t{3}<<32,10},
                   {std::uint64_t{39}<<32,27},{std::uint64_t{41}<<32,27},{std::uint64_t{43}<<32,27},{std::uint64_t{45}<<32,27},
                   {std::uint64_t{127}<<32,2},{std::uint64_t{392}<<32,1}};
            f.total=f.own;f.Bind();
            const auto maraDexOut=f.Render(2,"+10 to Dexterity");
            check(maraDexOut.find("+5 to Dexterity [U]")!=std::string::npos && maraDexOut.find("+5 to Dexterity [E]")!=std::string::npos);

            // Verify AddOwnedScalarIdentities upgrades ungrouped RangeHelperAdapter identity (stat 45, grouped=false) to grouped=true, group=2:
            {
                for(unsigned s:{0u,1u,2u,3u})for(auto& e:f.own)if((e.key>>32)==s)e.value=8;f.total=f.own;f.Bind(); std::vector<CapturedLine> obs={{RangeText::Analyze(maraRes27).key,45,0,false,0}};
                originalSingle=[](void*,const void* row,int val,int,bool grp,char* out,int)->bool{
                    const auto id=Affixes::Read<std::uint16_t>(static_cast<const unsigned char*>(row),0);
                    if(grp && (id==39||id==41||id==43||id==45)){std::snprintf(out,256,"All Resistances +%d",val);return true;}
                    return false;
                };
                const auto upgradedLbl=AffixLabels(f.host.data(),obs,maraRes27);
                originalSingle=nullptr;
                const auto upgradedOut=RangeText::Merge(maraRes27,maraRes27,4096,upgradedLbl).text;
                check(upgradedOut.find("All Resistances +22 [U]")!=std::string::npos && upgradedOut.find("All Resistances +5 [E]")!=std::string::npos);
            }
            put(fakeTables,0x13c8,static_cast<const unsigned char*>(nullptr));put(fakeTables,0x13d0,std::uint64_t{0});
        }

        contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::CubeMain)]={enchCube.data(),1,364};
    }

    // 3. Deterministic PRNG Fuzz Suite: 2,500 randomly generated multi-affix / multi-source items!
    std::uint64_t rngState=0xD2B00B1E55A17EULL;
    auto nextRand=[&]()->std::uint32_t{
        rngState ^= rngState >> 12;
        rngState ^= rngState << 25;
        rngState ^= rngState >> 27;
        return static_cast<std::uint32_t>((rngState * 0x2545F4914F6CDD1DULL) >> 32);
    };
    auto randRange=[&](int lo,int hi)->int{
        return lo + static_cast<int>(nextRand() % static_cast<unsigned>(hi - lo + 1));
    };

    for(unsigned iter=0;iter<2500;++iter){
        const bool isRare=(nextRand()&1)!=0;
        put(f.Data(),0,isRare?6u:4u);
        for(unsigned i=0;i<6;++i)put(f.Data()-24+i*4,0,0u);
        const bool hasBase=(nextRand()%4)==0;
        const unsigned baseIdx=hasBase?(32u+(nextRand()%2)):0u;
        put(f.Data()-28,0,hasBase?(baseIdx+1):0u);

        // Pick 1..3 unique suffixes (0..15) and 1..3 unique prefixes (16..31)
        const unsigned numSuf=isRare?static_cast<unsigned>(randRange(1,3)):1u;
        const unsigned numPre=isRare?static_cast<unsigned>(randRange(1,3)):1u;
        std::vector<unsigned> chosen;
        while(chosen.size()<numSuf){
            const unsigned c=nextRand()%16;
            if(std::find(chosen.begin(),chosen.end(),c)==chosen.end())chosen.push_back(c);
        }
        while(chosen.size()<numSuf+numPre){
            const unsigned c=16+(nextRand()%16);
            if(std::find(chosen.begin(),chosen.end(),c)==chosen.end())chosen.push_back(c);
        }
        for(std::size_t i=0;i<chosen.size();++i)put(f.Data()-24+i*4,0,chosen[i]+1);

        // Attach a socketed jewel (75% chance) or multi-enchantment residual [E] (remaining 25%)
        // so GatherSourceGroups + AddLinkedSourceLabels + AddOwnedSourceLabels are exercised on every combination,
        // and ALSO test clone-ranged Merge when neither socket nor enchant is active!
        const unsigned mode=iter%4;
        const bool hasSocket=(mode==0 || mode==1);
        const bool hasEnchant=(mode==1 || mode==2);
        const unsigned jewelAffix=nextRand()%32;
        if(hasSocket){
            put(f.modifier.data(),0x68,f.socketRoot.data());
            for(unsigned i=0;i<6;++i)put(f.JewelData()-24+i*4,0,0u);
            put(f.JewelData()-24,0,jewelAffix+1);
        }else{
            put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));
        }

        // Simulate exact stat rolls and bounds for host (own) and socket (socket), then sum into total:
        std::map<std::uint64_t,int> ownMap,sockMap,totalMap,minBoundMap,maxBoundMap;
        auto applyAffix=[&](unsigned idx,std::map<std::uint64_t,int>& target,bool addBounds){
            const auto& row=affixPool[idx];
            for(unsigned slotBlock=0;slotBlock<2;++slotBlock){
                const int pid=row.Number(0x24+slotBlock*16);
                if(pid<0)continue;
                const int lo=row.Number(0x2c+slotBlock*16),hi=row.Number(0x30+slotBlock*16);
                const int rollVal=randRange(lo,hi);
                const auto& p=props[static_cast<std::size_t>(pid)].bytes;
                for(unsigned s=0;s<7;++s){
                    const unsigned fn=p[0x18+s];if(!fn)continue;
                    const unsigned st=Affixes::Read<std::uint16_t>(p.data(),0x20+s*2);
                    if(fn==7){
                        target[std::uint64_t{17}<<32]+=rollVal;target[std::uint64_t{18}<<32]+=rollVal;
                        if(addBounds){minBoundMap[std::uint64_t{17}<<32]+=lo;maxBoundMap[std::uint64_t{17}<<32]+=hi;}
                    }else if(fn==5){
                        target[std::uint64_t{21}<<32]+=rollVal;
                        if(addBounds){minBoundMap[std::uint64_t{21}<<32]+=lo;maxBoundMap[std::uint64_t{21}<<32]+=hi;}
                    }else if(fn==6){
                        target[std::uint64_t{22}<<32]+=rollVal;
                        if(addBounds){minBoundMap[std::uint64_t{22}<<32]+=lo;maxBoundMap[std::uint64_t{22}<<32]+=hi;}
                    }else if(fn==15){
                        target[std::uint64_t{st}<<32]+=lo;
                        if(addBounds){minBoundMap[std::uint64_t{st}<<32]+=lo;maxBoundMap[std::uint64_t{st}<<32]+=lo;}
                    }else if(fn==16){
                        target[std::uint64_t{st}<<32]+=hi;
                        if(addBounds){minBoundMap[std::uint64_t{st}<<32]+=hi;maxBoundMap[std::uint64_t{st}<<32]+=hi;}
                    }else if(fn==21){
                        const unsigned lay=Affixes::Read<std::uint16_t>(p.data(),0x0a+s*2);
                        target[(std::uint64_t{st}<<32)|lay]+=rollVal;
                        if(addBounds){minBoundMap[(std::uint64_t{st}<<32)|lay]+=lo;maxBoundMap[(std::uint64_t{st}<<32)|lay]+=hi;}
                    }else{
                        const unsigned shift=(st==7||st==9)?8u:0u;
                        target[std::uint64_t{st}<<32]+=(rollVal<<shift);
                        if(addBounds){minBoundMap[std::uint64_t{st}<<32]+=lo;maxBoundMap[std::uint64_t{st}<<32]+=hi;}
                    }
                }
            }
        };
        for(unsigned idx:chosen)applyAffix(idx,ownMap,true);
        if(hasBase)applyAffix(baseIdx,ownMap,true);
        if(hasSocket)applyAffix(jewelAffix,sockMap,true);
        if(hasEnchant){
            ownMap[std::uint64_t{392}<<32]=2;
            ownMap[std::uint64_t{127}<<32]+=2;
            ownMap[std::uint64_t{45}<<32]+=15;
            minBoundMap[std::uint64_t{127}<<32]+=2;maxBoundMap[std::uint64_t{127}<<32]+=2;
            minBoundMap[std::uint64_t{45}<<32]+=15;maxBoundMap[std::uint64_t{45}<<32]+=15;
        }
        totalMap=ownMap;
        for(const auto& [k,v]:sockMap)totalMap[k]+=v;

        f.own.clear();for(const auto& [k,v]:ownMap)f.own.push_back({k,v});
        f.socket.clear();for(const auto& [k,v]:sockMap)f.socket.push_back({k,v});
        f.total.clear();for(const auto& [k,v]:totalMap)f.total.push_back({k,v});
        f.Bind();

        // Build the native tooltip lines (actual & ranged) and observed CapturedLines:
        std::vector<std::string> actualLines,rangedLines;
        std::vector<CapturedLine> observedAll,observedNoComposite;
        const std::string m="\xee\x81\xbe";
        auto span=[&](unsigned st,unsigned lay=0)->std::string{
            const auto key=(std::uint64_t{st}<<32)|lay;
            const int lo=minBoundMap[key],hi=maxBoundMap[key];
            if(lo==hi)return std::to_string(lo);
            return m+"U("+std::to_string(lo)+"-"+std::to_string(hi)+")"+m+"3";
        };
        auto addLine=[&](std::string act,std::string rng,int stat,int layer,bool grouped,std::uint16_t grp,bool isComposite=false){
            const auto key=RangeText::Analyze(act).key;
            actualLines.push_back(std::move(act));
            rangedLines.push_back(std::move(rng));
            observedAll.push_back({key,stat,layer,grouped,grp});
            if(!isComposite)observedNoComposite.push_back({key,stat,layer,grouped,grp});
        };

        auto getTot=[&](unsigned st,unsigned lay=0)->int{
            const auto it=totalMap.find((std::uint64_t{st}<<32)|lay);
            if(it==totalMap.end())return 0;
            return (st==7||st==9)?(it->second>>8):it->second;
        };

        // 1. Check dgrp=1 (0,1,2,3)
        if(getTot(0)>0 && getTot(0)==getTot(1) && getTot(0)==getTot(2) && getTot(0)==getTot(3) &&
           minBoundMap[0]==minBoundMap[std::uint64_t{1}<<32] && maxBoundMap[0]==maxBoundMap[std::uint64_t{1}<<32]){
            addLine("+"+std::to_string(getTot(0))+" to All Attributes","+"+span(0)+" to All Attributes",0,0,true,1);
        }else{
            if(getTot(0)>0)addLine("+"+std::to_string(getTot(0))+" to Strength","+"+span(0)+" to Strength",0,0,false,0);
            if(getTot(1)>0)addLine("+"+std::to_string(getTot(1))+" to Energy","+"+span(1)+" to Energy",1,0,false,0);
            if(getTot(2)>0)addLine("+"+std::to_string(getTot(2))+" to Dexterity","+"+span(2)+" to Dexterity",2,0,false,0);
            if(getTot(3)>0)addLine("+"+std::to_string(getTot(3))+" to Vitality","+"+span(3)+" to Vitality",3,0,false,0);
        }
        // 2. Check dgrp=2 (39,41,43,45)
        if(getTot(39)>0 && getTot(39)==getTot(41) && getTot(39)==getTot(43) && getTot(39)==getTot(45) &&
           minBoundMap[std::uint64_t{39}<<32]==minBoundMap[std::uint64_t{41}<<32] &&
           maxBoundMap[std::uint64_t{39}<<32]==maxBoundMap[std::uint64_t{41}<<32]){
            addLine("All Resistances +"+std::to_string(getTot(39)),"All Resistances +"+span(39),39,0,true,2);
        }else{
            if(getTot(39)>0)addLine("Fire Resist +"+std::to_string(getTot(39))+"%","Fire Resist +"+span(39)+"%",39,0,false,0);
            if(getTot(41)>0)addLine("Lightning Resist +"+std::to_string(getTot(41))+"%","Lightning Resist +"+span(41)+"%",41,0,false,0);
            if(getTot(43)>0)addLine("Cold Resist +"+std::to_string(getTot(43))+"%","Cold Resist +"+span(43)+"%",43,0,false,0);
            if(getTot(45)>0)addLine("Poison Resist +"+std::to_string(getTot(45))+"%","Poison Resist +"+span(45)+"%",45,0,false,0);
        }
        // 3. Check dgrp=3 (329..332) and dgrp=4 (333..336)
        const char* elemNames[4]={"Fire","Lightning","Cold","Poison"};
        for(unsigned k=0;k<4;++k){
            if(getTot(329+k)>0)addLine("+"+std::to_string(getTot(329+k))+"% to "+elemNames[k]+" Skill Damage",
                "+"+span(329+k)+"% to "+elemNames[k]+" Skill Damage",static_cast<int>(329+k),0,false,0);
            if(getTot(333+k)>0)addLine("-"+std::to_string(getTot(333+k))+"% to Enemy "+elemNames[k]+" Resistance",
                "-"+span(333+k)+"% to Enemy "+elemNames[k]+" Resistance",static_cast<int>(333+k),0,false,0);
        }
        // 4. Composite & single-endpoint damage in ascending descpriority order (Cold 94, Fire 102, Physical 126):
        const int pMin=getTot(21),pMax=getTot(22);
        const bool DropComposite=(iter%2)==1 && !(pMin>0 && pMax>0 && pMin>=pMax);
        const int cMin=getTot(54),cMax=getTot(55);
        if(cMin>0 && cMax>0 && cMin<cMax){
            addLine("Adds "+std::to_string(cMin)+"-"+std::to_string(cMax)+" Cold Damage",
                "Adds "+span(54)+"-"+span(55)+" Cold Damage",54,0,false,0,DropComposite);
        }else{
            if(cMin>0)addLine("+"+std::to_string(cMin)+" to Minimum Cold Damage","+"+span(54)+" to Minimum Cold Damage",54,0,false,0);
            if(cMax>0)addLine("+"+std::to_string(cMax)+" to Maximum Cold Damage","+"+span(55)+" to Maximum Cold Damage",55,0,false,0);
        }
        const int fMin=getTot(48),fMax=getTot(49);
        if(fMin>0 && fMax>0 && fMin<fMax){
            addLine("Adds "+std::to_string(fMin)+"-"+std::to_string(fMax)+" Fire Damage",
                "Adds "+span(48)+"-"+span(49)+" Fire Damage",48,0,false,0,DropComposite);
        }else{
            if(fMin>0)addLine("+"+std::to_string(fMin)+" to Minimum Fire Damage","+"+span(48)+" to Minimum Fire Damage",48,0,false,0);
            if(fMax>0)addLine("+"+std::to_string(fMax)+" to Maximum Fire Damage","+"+span(49)+" to Maximum Fire Damage",49,0,false,0);
        }
        if(pMin>0 && pMax>0 && pMin<pMax){
            addLine("Adds "+std::to_string(pMin)+"-"+std::to_string(pMax)+" Damage",
                "Adds "+span(21)+"-"+span(22)+" Damage",21,0,false,0,DropComposite);
        }else{
            if(pMin>0)addLine("+"+std::to_string(pMin)+" to Minimum Damage","+"+span(21)+" to Minimum Damage",21,0,false,0);
            if(pMax>0)addLine("+"+std::to_string(pMax)+" to Maximum Damage","+"+span(22)+" to Maximum Damage",22,0,false,0);
        }
        // 6. Scalar & layered stats:
        if(getTot(7)>0)addLine("+"+std::to_string(getTot(7))+" to Life","+"+span(7)+" to Life",7,0,false,0);
        if(getTot(76)>0)addLine("Increase Maximum Life "+std::to_string(getTot(76))+"%","Increase Maximum Life "+span(76)+"%",76,0,false,0);
        if(getTot(413)>0)addLine("+"+std::to_string(getTot(413))+"% to Strength","+"+span(413)+"% to Strength",413,0,false,0);
        if(getTot(16)>0)addLine("+"+std::to_string(getTot(16))+"% Enhanced Defense","+"+span(16)+"% Enhanced Defense",16,0,false,0);
        if(getTot(17)>0)addLine("+"+std::to_string(getTot(17))+"% Enhanced Damage","+"+span(17)+"% Enhanced Damage",17,0,false,0);
        if(getTot(19)>0)addLine("+"+std::to_string(getTot(19))+" to Attack Rating","+"+span(19)+" to Attack Rating",19,0,false,0);
        if(getTot(80)>0)addLine(std::to_string(getTot(80))+"% Better Chance of Getting Magic Items",span(80)+"% Better Chance of Getting Magic Items",80,0,false,0);
        if(getTot(93)>0)addLine("+"+std::to_string(getTot(93))+"% Increased Attack Speed","+"+span(93)+"% Increased Attack Speed",93,0,false,0);
        if(getTot(102)>0)addLine("+"+std::to_string(getTot(102))+"% Faster Cast Rate","+"+span(102)+"% Faster Cast Rate",102,0,false,0);
        if(getTot(127)>0)addLine("+"+std::to_string(getTot(127))+" to All Skills","+"+span(127)+" to All Skills",127,0,false,0);
        if(getTot(83,3)>0)addLine("+"+std::to_string(getTot(83,3))+" to Paladin Skill Levels","+"+span(83,3)+" to Paladin Skill Levels",83,3,false,0);
        if(getTot(399)>0)addLine("+"+std::to_string(getTot(399))+" to Fire Skills","+"+span(399)+" to Fire Skills",399,0,false,0);
        if(getTot(400)>0)addLine("+"+std::to_string(getTot(400))+" to Cold Skills","+"+span(400)+" to Cold Skills",400,0,false,0);

        std::string actualTooltip,rangedTooltip;
        for(std::size_t i=0;i<actualLines.size();++i){
            if(i){actualTooltip+='\n';rangedTooltip+='\n';}
            actualTooltip+=actualLines[i];
            rangedTooltip+=(hasSocket || hasEnchant)?actualLines[i]:rangedLines[i];
        }
        const auto& obs=DropComposite?observedNoComposite:observedAll;
        const auto labels=AffixLabels(f.host.data(),obs,actualTooltip);
        const auto merged=RangeText::Merge(actualTooltip,rangedTooltip,8192,labels);

        check(merged.labeled==actualLines.size());
        check(merged.text.find("(?)")==std::string::npos);
        check(merged.text.find("Range Unavailable")==std::string::npos);
        for(const auto& line:actualLines){
            check(merged.text.find(line)!=std::string::npos);
        }
        // Also verify tight buffer capacity fallback never truncates original lines or crashes:
        const auto compactMerged=RangeText::Merge(actualTooltip,rangedTooltip,actualTooltip.size()+48,labels);
        for(const auto& line:actualLines){
            check(compactMerged.text.find(line)!=std::string::npos);
        }
    }

    propertySourcesReady.store(false);scalingReady.store(false);randomSkillsReady.store(false);
    contributionContext=nullptr;contributionTables=nullptr;contributionFixtureTables={};
    std::puts("Passed 2,500 randomized multi-affix / multi-source item combinations and all 6 newly validated rule families.");
}

void TestOwnedContributionAdapter(){
    TestScalingAffixAdapter();
    TestSocketCapacityAdapter();
    TestLevelHeaderAdapter();
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

namespace {
void TestLinkedProduction(){
    ContributionFixture f;propertySourcesReady.store(true);quantityRateReady.store(true);
    put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));
    f.Property(0,1,39);
    for(unsigned k=1;k<4;++k){f.props[0].bytes[0x18+k]=3;put(f.props[0].bytes.data(),0x20+k*2,std::uint16_t(39+k*2));}
    f.Property(1,1,45);f.Roll(0,0,16,20,"Prismatic");f.Roll(1,1,11,20,"Viridian");put(f.Data()-20,0,2u);
    f.own=f.Stats({{39,20},{41,20},{43,20},{45,32}});f.total=f.own;
    auto text=f.Render(45,"Poison Resist +32%");check(text.find("Poison Resist +20%")!=std::string::npos && text.find("Poison Resist +12%")!=std::string::npos);
    f.own.back().value=42;f.total=f.own;check(f.Render(45,"Poison Resist +42%").find("[Combined]")==std::string::npos);
    // Hidden quantity parameter and variable source bounds use the same path.
    f.Property(0,17,253);f.Roll(0,0,0,0,"of Replenishing");put(f.rows[0].bytes.data(),0x28,5);put(f.Data()-20,0,0u);
    f.own=f.Stats({{253,5}});f.total=f.own;text=f.Render(253,"Replenishes Quantity");check(text.find("[1 per 20.04s]")!=std::string::npos);
    put(f.rows[0].bytes.data(),0x28,0);put(f.rows[0].bytes.data(),0x2c,20);put(f.rows[0].bytes.data(),0x30,25);
    f.own=f.Stats({{253,25}});f.total=f.own;text=f.Render(253,"Replenishes Quantity");check(text.find("[1 per 5 - 5.04s]")!=std::string::npos && text.find("(1 per 5s)")!=std::string::npos);
    quantityRateReady.store(false);check(f.Render(253,"Replenishes Quantity").find("1 per")==std::string::npos);quantityRateReady.store(true);
    // Unique grouped attributes retain equal ranges and source labels.
    put(f.Data(),0,7u);put(f.Data()+0x34,0,0);put(f.Data()-24,0,0u);f.Property(2,1,0);
    for(unsigned k=1;k<4;++k){f.props[2].bytes[0x18+k]=3;put(f.props[2].bytes.data(),0x20+k*2,std::uint16_t(k));}
    std::array<unsigned char,348> unique{};for(unsigned k=0;k<12;++k)put(unique.data(),0x98+k*16,UINT32_MAX);
    put(unique.data(),0x98,2u);put(unique.data(),0xa0,3);put(unique.data(),0xa4,8);
    std::array<Affixes::PropertyGroup,1> groups{};put(fakeTables,0x258,groups.data());put(fakeTables,0x260,std::uint64_t{1});
    put(fakeTables,0x13c8,unique.data());put(fakeTables,0x13d0,std::uint64_t{1});
    for(unsigned k=0;k<4;++k)put(f.statRows.data()+k*0x144,0x3a,std::uint16_t{1});
    f.statRows[1*0x144+0x50]=8;f.statRows[3*0x144+0x50]=9;
    f.own=f.Stats({{0,6},{1,6},{2,6},{3,6}});f.total=f.own;f.Bind();
    const std::string attrs="+6 to All Attributes";
    auto labels=AffixLabels(f.host.data(),{{RangeText::Analyze(attrs).key,0,0,true,1}},attrs);
    text=RangeText::Merge(attrs,attrs,4096,labels).text;check(text.find("[+3 - +8]")!=std::string::npos && text.find("[U]")!=std::string::npos && text.find("(?)")==std::string::npos);
    // Ordinary direct unique properties also work without a PropertyGroups table.
    put(fakeTables,0x260,std::uint64_t{0});
    labels=AffixLabels(f.host.data(),{{RangeText::Analyze(attrs).key,0,0,true,1}},attrs);
    text=RangeText::Merge(attrs,attrs,4096,labels).text;check(text.find("[+3 - +8]")!=std::string::npos);
    // A one-choice weighted pool can supply an observed stat's marginal bounds,
    // but its alternative candidates cannot be independent additive variables.
    unsigned char spec[16]{};put(spec,0,0x10000u);put(spec,8,1);put(spec,12,1);
    std::array<Affixes::PropertyGroup,1> pool{};pool[0].bytes[4]=2;
    put(pool[0].bytes.data(),8,0u);put(pool[0].bytes.data(),20,5);put(pool[0].bytes.data(),24,10);put(pool[0].bytes.data(),28,1);
    put(pool[0].bytes.data(),32,1u);put(pool[0].bytes.data(),44,11);put(pool[0].bytes.data(),48,20);put(pool[0].bytes.data(),52,1);
    std::vector<Affixes::Rolled> alternatives;AddUniqueSource(spec,reinterpret_cast<const unsigned char*>(pool.data()),1,f.props.size(),alternatives);
    check(alternatives.size()==2 && alternatives[0].uniqueScalar && !alternatives[0].unconditionalScalar);
    f.Property(0,1,7);int lo{},hi{};check(Affixes::UniqueScalarBounds(alternatives[0],f.props,7,0,lo,hi) && lo==5 && hi==10);
    const auto conditional=PropertyRolls::Compile(alternatives,f.props);check(conditional.variables.empty() && conditional.blocked.contains(7));
    // Synthetic weighted Attributes/FRW/MF pool, with a separate MF roll.
    pool={};pool[0].bytes[4]=2;f.Property(1,8,79);f.Property(3,8,96);
    for(unsigned k=0;k<3;++k){auto* entry=pool[0].bytes.data()+8+k*24;
        put(entry,0,k==0?2u:k==1?3u:1u);put(entry,12,k==0?3:k==1?10:15);put(entry,16,k==0?8:k==1?20:30);put(entry,20,1);}
    std::memcpy(unique.data()+0x98,spec,16);put(unique.data(),0xa8,1u);put(unique.data(),0xb0,45);put(unique.data(),0xb4,75);
    put(fakeTables,0x258,pool.data());put(fakeTables,0x260,std::uint64_t{1});
    f.own=f.Stats({{0,6},{1,6},{2,6},{3,6},{79,55}});f.total=f.own;f.Bind();
    labels=AffixLabels(f.host.data(),{{RangeText::Analyze(attrs).key,0,0,true,1}},attrs);
    text=RangeText::Merge(attrs,attrs,4096,labels).text;check(text.find("[+3 - +8]")!=std::string::npos && text.find("(?)")==std::string::npos);
    // Separate independently rolled elemental endpoints on a single affix.
    put(f.Data(),0,6u);put(f.Data()-24,0,1u);put(f.Data()-20,0,0u);f.Property(0,1,48);f.Property(1,1,49);
    f.Roll(0,0,121,170,"Scorching");put(f.rows[0].bytes.data(),0x34,1);put(f.rows[0].bytes.data(),0x3c,181);put(f.rows[0].bytes.data(),0x40,240);
    f.own=f.Stats({{48,156},{49,181}});f.total=f.own;text=f.Render(48,"Adds 156-181 Weapon Fire Damage");
    check(text.find("[121 - 170 / 181 - 240]")!=std::string::npos && text.find("Adds 156-181")!=std::string::npos && text.find("(?)")==std::string::npos);
    f.total.back().value=241;check(f.Render(48,"Adds 156-241 Weapon Fire Damage").find("[121 - 170 / 181 - 240]")==std::string::npos);
    put(f.rows[0].bytes.data(),0x34,-1);
    // Active compiled runeword with a reciprocal owned socket, empty property
    // groups (vanilla), mana and a class-skill layer; no source-name database.
    put(fakeTables,0x258,static_cast<const Affixes::PropertyGroup*>(nullptr));put(fakeTables,0x260,std::uint64_t{0});
    put(f.Data(),0,2u);put(f.Data()+0x18,0,0x04000010u);put(f.modifier.data(),0x20,171u);
    put(f.modifier.data(),0x68,f.socketRoot.data());put(f.JewelData(),0,2u);put(f.JewelData()-24,0,0u);
    f.Property(4,1,9);f.Property(5,21,83);put(f.props[5].bytes.data(),0x0a,std::uint16_t{2});
    f.own={{std::uint64_t{9}<<32,119*256},{(std::uint64_t{83}<<32)|2,2}};f.socket={};f.total=f.own;f.total.push_back({std::uint64_t{194}<<32,1});f.Bind();
    f.statRows[9*0x144+0x14]=8;put(f.statRows.data()+83*0x144,0x32,std::uint16_t{13});
    std::array<unsigned char,448*3> items{};std::array<unsigned char,232*3> types{};std::array<unsigned char,192> gems{};std::array<unsigned char,232> runes{};
    put(items.data(),0x12e,std::int16_t{1});put(items.data(),0x130,std::int16_t{-1});
    put(types.data(),0,ContributionDefinitions::Code("armo"));put(types.data()+232,0,ContributionDefinitions::Code("weap"));put(types.data()+464,0,ContributionDefinitions::Code("shld"));
    std::memcpy(gems.data(),"Fixture Rune",13);put(gems.data(),0x28,2u);for(unsigned k=0;k<9;++k)put(gems.data(),0x30+k*16,UINT32_MAX);
    runes[0x44]=1;put(runes.data(),0x46,std::uint16_t{10});put(runes.data(),0x4a,std::int16_t{1});
    for(unsigned k=0;k<6;++k)put(runes.data(),0x5c+k*4,-1);put(runes.data(),0x5c,2);
    for(unsigned k=0;k<7;++k)put(runes.data(),0x74+k*16,UINT32_MAX);
    put(runes.data(),0x74,4u);put(runes.data(),0x7c,100);put(runes.data(),0x80,150);
    put(runes.data(),0x84,5u);put(runes.data(),0x8c,2);put(runes.data(),0x90,2);
    put(fakeTables,0x1860,runes.data());put(fakeTables,0x1868,std::uint64_t{1});
    D2RL::DataTableService service{.serviceSize=D2RL::DataTableServiceRequiredSize,.serviceVersion=D2RL::DataTableService::AbiVersion,.getTable=FixtureTable};D2RL::PluginContext context{};
    contributionContext=&context;contributionTables=&service;
    contributionFixtureTables[16]={reinterpret_cast<const unsigned char*>(f.props.data()),f.props.size(),48};
    contributionFixtureTables[59]={items.data(),3,448};contributionFixtureTables[41]={types.data(),3,232};contributionFixtureTables[62]={gems.data(),1,192};contributionFixtureTables[66]={runes.data(),1,232};
    auto snapshot=CaptureItemContributions(f.host.data());check(snapshot && snapshot->word.size()==2);
    check(ReadWordDefinitions(*snapshot,f.props,{}).has_value());
    text=f.Render(9,"+119 to Mana");check(text.find("[+100 - +150]")!=std::string::npos && text.find("[RW]")!=std::string::npos);
    const std::string skill="+2 to Necromancer Skill Levels";
    labels=AffixLabels(f.host.data(),{{RangeText::Analyze(skill).key,83,2,false}},skill);text=RangeText::Merge(skill,skill,4096,labels).text;
    check(text.find("[+2]")!=std::string::npos && text.find("[RW]")!=std::string::npos);
    // Selector zero is a real layer (Amazon), not the absence of a selector.
    put(f.props[5].bytes.data(),0x0a,std::uint16_t{0});f.own[1].key=std::uint64_t{83}<<32;f.total=f.own;f.total.push_back({std::uint64_t{194}<<32,1});f.Bind();
    text=f.Render(83,"+2 to Amazon Skill Levels");check(text.find("[+2]")!=std::string::npos && text.find("[RW]")!=std::string::npos);
    f.Property(5,24,97);put(runes.data(),0x88,133);put(runes.data(),0x8c,3);put(runes.data(),0x90,6);
    f.own[1]={(std::uint64_t{97}<<32)|133,5};f.total=f.own;f.total.push_back({std::uint64_t{194}<<32,1});f.Bind();
    const std::string oskill="+5 to Teleport";
    labels=AffixLabels(f.host.data(),{{RangeText::Analyze(oskill).key,97,133,false}},oskill);text=RangeText::Merge(oskill,oskill,4096,labels).text;
    check(text.find("[+3 - +6]")!=std::string::npos && text.find("+5 to Teleport")!=std::string::npos && text.find("[RW]")!=std::string::npos);
    contributionFixtureBusy=true;check(!ReadWordDefinitions(*snapshot,f.props,{}));contributionFixtureBusy=false;
    put(fakeTables,0x1868,std::uint64_t{2});check(!ReadWordDefinitions(*snapshot,f.props,{}));put(fakeTables,0x1868,std::uint64_t{1});
    put(f.Data()+0x18,0,0u);
    check(!CaptureItemContributions(f.host.data()));propertySourcesReady.store(false);quantityRateReady.store(false);
    contributionContext=nullptr;contributionTables=nullptr;contributionFixtureTables={};
    std::puts("Production shared-resist reverse inference, grouped unique attributes, quantity rates and active-table runeword sources pass.");
}
bool __fastcall FixtureRandomSkillSingle(void*,const void* metadata,int value,int layer,bool grouped,char* output,int mode) {
    if(Affixes::Read<std::uint16_t>(static_cast<const unsigned char*>(metadata),0)!=107 || layer!=262)return false;
    if(grouped && Affixes::Read<std::uint16_t>(static_cast<const unsigned char*>(metadata),0)>=329)return false;
    check(!grouped && mode==1);
    std::snprintf(output,256,"+%d to Wake of Fire (Assassin Only)",value);return true;
}
void TestRandomSkillProduction(){
    ContributionFixture f;propertySourcesReady.store(true);randomSkillsReady.store(true);
    put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));put(fakeTables,0x1608,f.rows.data());
    f.Property(0,12,107);f.Roll(0,0,251,280,"Gnostics");put(f.rows[0].bytes.data(),0x28,5);
    f.own={{(std::uint64_t{107}<<32)|262,5}};f.total=f.own;f.Bind();
    originalSingle=FixtureRandomSkillSingle;const std::string actual="+5 to Wake of Fire (Assassin Only)";
    // No captured identity/clone: recover through the existing native formatter.
    auto labels=AffixLabels(f.host.data(),{},actual);auto text=RangeText::Merge(actual,actual,4096,labels).text;
    check(text.find("[+5]")!=std::string::npos && text.find("[P] [Gnostics]")!=std::string::npos && text.find("251") ==std::string::npos);
    randomSkillsReady.store(false);labels=AffixLabels(f.host.data(),{},actual);
    check(RangeText::Merge(actual,actual,4096,labels).text==actual);randomSkillsReady.store(true);
    f.own[0].value=6;f.total=f.own;f.Bind();labels=AffixLabels(f.host.data(),{},"+6 to Wake of Fire (Assassin Only)");
    text=RangeText::Merge("+6 to Wake of Fire (Assassin Only)","+6 to Wake of Fire (Assassin Only)",4096,labels).text;
    check(text.find("[+5]")==std::string::npos && text.find("Gnostics")==std::string::npos);
    originalSingle=nullptr;randomSkillsReady.store(false);propertySourcesReady.store(false);
    std::puts("Production random-skill clone-less identity, fixed bonus/source display, mismatch and optional guard refusal pass.");
}
void TestLiveRandomSkillCapture(const char* path){
    std::ifstream input(path,std::ios::binary);char magic[4]{};input.read(magic,4);check(std::memcmp(magic,"IRRS",4)==0);
    unsigned itemClass{},quality{},flags{},version{},prefix{},automagic{},size{};std::array<unsigned,6> ids{};
    for(auto* field:{&itemClass,&quality,&flags,&version,&prefix,&automagic,&size})input.read(reinterpret_cast<char*>(field),4);
    input.read(reinterpret_cast<char*>(ids.data()),24);check(input.good() && size<=256 && version<=65535);
    Contributions::Stats actual(size);for(auto& value:actual){input.read(reinterpret_cast<char*>(&value.key),8);input.read(reinterpret_cast<char*>(&value.value),8);}
    std::vector<std::vector<unsigned char>> bytes(4);const unsigned strides[]={48,200,140,324};std::array<unsigned,4> counts{};
    for(unsigned n=0;n<4;++n){unsigned stride{};input.read(reinterpret_cast<char*>(&counts[n]),4);input.read(reinterpret_cast<char*>(&stride),4);
        check(input.good() && counts[n]<=32768 && stride==strides[n]);bytes[n].resize(static_cast<std::size_t>(counts[n])*stride);
        input.read(reinterpret_cast<char*>(bytes[n].data()),static_cast<std::streamsize>(bytes[n].size()));check(input.good());}
    check(input.peek()==std::char_traits<char>::eof() && prefix<=automagic && automagic<=counts[2]);
    ContributionFixture f;propertySourcesReady.store(true);randomSkillsReady.store(true);
    put(f.host.data(),4,itemClass);put(f.Data(),0,quality);put(f.Data()+0x18,0,flags);put(f.Data()+0x40,0,static_cast<std::uint16_t>(version));
    std::memcpy(f.Data()-24,ids.data(),24);put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));f.own=actual;f.total=actual;f.Bind();
    put(fakeTables,0x240,bytes[0].data());put(fakeTables,0x248,std::uint64_t{counts[0]});put(fakeTables,0x258,bytes[1].data());put(fakeTables,0x260,std::uint64_t{counts[1]});
    put(fakeTables,0x15e8,bytes[2].data());put(fakeTables,0x15f0,std::uint64_t{counts[2]});put(fakeTables,0x1600,bytes[2].data());
    put(fakeTables,0x1608,bytes[2].data()+prefix*140);put(fakeTables,0x1610,bytes[2].data()+automagic*140);
    put(fakeTables,0x1258,bytes[3].data());put(fakeTables,0x1260,std::uint64_t{counts[3]});
    originalSingle=FixtureRandomSkillSingle;const std::string text="+5 to Wake of Fire (Assassin Only)";
    const auto labels=AffixLabels(f.host.data(),{},text);const auto rendered=RangeText::Merge(text,text,4096,labels).text;
    std::printf("Captured random-skill amulet: %s\n",rendered.c_str());
    check(rendered.find("[+5]")!=std::string::npos && rendered.find("[P] [Gnostics] [T1]")!=std::string::npos && rendered.find("(?)")==std::string::npos);
    originalSingle=nullptr;propertySourcesReady.store(false);randomSkillsReady.store(false);
}
void TestLiveUniqueCapture(const char* path){
    std::ifstream input(path,std::ios::binary);char magic[4]{};input.read(magic,4);check(std::memcmp(magic,"IRRU",4)==0);
    int record{};unsigned itemClass{},flags{},size{};
    input.read(reinterpret_cast<char*>(&record),4);input.read(reinterpret_cast<char*>(&itemClass),4);input.read(reinterpret_cast<char*>(&flags),4);input.read(reinterpret_cast<char*>(&size),4);check(input.good() && size<=256);
    Contributions::Stats actual(size);for(auto& value:actual){input.read(reinterpret_cast<char*>(&value.key),8);input.read(reinterpret_cast<char*>(&value.value),8);}
    std::vector<std::vector<unsigned char>> bytes(4);const unsigned strides[]={48,200,348,324};std::array<unsigned,4> counts{};
    for(unsigned n=0;n<4;++n){unsigned stride{};input.read(reinterpret_cast<char*>(&counts[n]),4);input.read(reinterpret_cast<char*>(&stride),4);
        check(input.good() && counts[n]>0 && counts[n]<=32768 && stride==strides[n]);bytes[n].resize(static_cast<std::size_t>(counts[n])*stride);
        input.read(reinterpret_cast<char*>(bytes[n].data()),static_cast<std::streamsize>(bytes[n].size()));check(input.good());}
    check(input.peek()==std::char_traits<char>::eof());ContributionFixture f;propertySourcesReady.store(true);
    put(f.host.data(),4,itemClass);put(f.Data(),0,7u);put(f.Data()+0x18,0,flags);put(f.Data()+0x34,0,record);put(f.Data()-24,0,0u);
    put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));f.own=actual;f.total=actual;f.Bind();
    put(fakeTables,0x240,bytes[0].data());put(fakeTables,0x248,std::uint64_t{counts[0]});
    put(fakeTables,0x258,bytes[1].data());put(fakeTables,0x260,std::uint64_t{counts[1]});
    put(fakeTables,0x13c8,bytes[2].data());put(fakeTables,0x13d0,std::uint64_t{counts[2]});
    put(fakeTables,0x1258,bytes[3].data());put(fakeTables,0x1260,std::uint64_t{counts[3]});
    originalSingle=nullptr;const std::string text="+10 to All Attributes";
    auto labels=AffixLabels(f.host.data(),{{RangeText::Analyze(text).key,0,0,true,1}},text);
    const auto rendered=RangeText::Merge(text,text,4096,labels).text;
    if(rendered.find("[+5 - +10]")==std::string::npos){
        const auto properties=std::span(reinterpret_cast<const Affixes::Property*>(bytes[0].data()),counts[0]);
        const auto groups=std::span(reinterpret_cast<const Affixes::PropertyGroup*>(bytes[1].data()),counts[1]);
        std::vector<Affixes::Rolled> defs;
        for(unsigned n=0;n<12;++n)AddUniqueSource(bytes[2].data()+static_cast<std::size_t>(record)*348+0x98+n*16,bytes[1].data(),counts[1],counts[0],defs);
        const auto normalize=[&](unsigned stat,std::int64_t raw)->std::optional<int>{if(stat>=counts[3])return {};const auto* row=bytes[3].data()+stat*324;if(!PropertyRolls::ItemScalarOperand(row))return {};return Contributions::Normalize(raw,row[0x14]);};
        const auto solved=PropertyRolls::SolveSources(defs,properties,actual,normalize,groups);
        std::fprintf(stderr,"Loaded replay: defs=%zu complete=%d stat0 verified=%d blocked=%d rendered=%s\n",defs.size(),solved.model.complete,solved.verified.contains(0),solved.model.blocked.contains(0),rendered.c_str());
        for(const auto& d:defs)std::fprintf(stderr,"property=%d bounds=%d..%d pool=%u entry=%u unconditional=%d\n",d.row.Number(0x24),d.row.Number(0x2c),d.row.Number(0x30),d.choicePool,d.choiceEntry,d.unconditionalScalar);
    }
    check(rendered.find("[+5 - +10]")!=std::string::npos && rendered.find("[U]")!=std::string::npos && rendered.find("(?)")==std::string::npos);
    propertySourcesReady.store(false);std::puts("Current loaded unique/group/property/stat metadata and owned stat replay: selected Attributes5..10 pool renders correctly.");
}
}

namespace {
void TestOwnedDurabilityPhysical(){
    ContributionFixture f;
    f.Property(0,13,75);f.Roll(0,0,25,25,"Durable");f.statRows[75*0x144+0x50]=13;
    SourceGroups sources;sources.groups.push_back({f.Stats({{75,25}}),{{1,f.rows[0],true,1}},""});
    sources.capture.total=f.Stats({{73,40}}); // Capacity is not the percent.
    auto render=[&](int stat,const char* text){
        std::vector<RangeText::Label> labels;
        const std::array<CapturedLine,1> observed={CapturedLine{RangeText::Analyze(text).key,stat,0,false}};
        AddLinkedSourceLabels(sources,observed,text,f.props,f.statRows.data(),400,labels);
        return RangeText::Merge(text,text,4096,labels).text;
    };
    check(render(75,"Increase Maximum Durability 25%").find("[25%]")!=std::string::npos);
    check(render(75,"Increase Maximum Durability 26%").find("[25%]")==std::string::npos);
    f.props[0].bytes[2]=1;
    check(render(75,"Increase Maximum Durability 25%").find("[25%]")==std::string::npos);
    f.props[0].bytes[2]=0;f.statRows[75*0x144+0x50]=12;
    check(render(75,"Increase Maximum Durability 25%").find("[25%]")==std::string::npos);
    f.Property(0,15,21);f.props[0].bytes[0x19]=16;put(f.props[0].bytes.data(),0x22,std::uint16_t{22});
    f.Roll(0,0,5,10,"Damage");sources.groups[0]={f.Stats({{21,5},{22,10}}),{{1,f.rows[0],true,1}},""};
    sources.capture.total=f.Stats({{21,8},{22,29}}); // Base weapon damage must not leak into the modifier.
    check(render(21,"Adds 5-10 Weapon Damage").find("[5 / 10]")!=std::string::npos);
    check(render(22,"Adds 5-10 Weapon Damage").find("[5 / 10]")!=std::string::npos);
    check(render(21,"Adds 8-29 Weapon Damage").find("[5 / 10]")==std::string::npos);
    f.props[0].bytes[0x19]=25;
    check(render(21,"Adds 5-10 Weapon Damage").find("[5 / 10]")==std::string::npos);
    f.props[0].bytes[0x19]=16;f.props[0].bytes[2]=1;
    check(render(21,"Adds 5-10 Weapon Damage").find("[5 / 10]")==std::string::npos);
    // Corrupted + Enchanted Unique/Set items: Frost Wyrm / Knight's Dawn / Ragnarok patterns.
    ContributionFixture u;propertySourcesReady.store(true);scalingReady.store(true);originalSingle=FixtureScalingSingle;
    put(u.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));
    put(u.Data(),0,7u);put(u.Data(),0x34,0);put(u.Data()-24,0,0u);
    u.Property(0,7,0xffff); // dmg% (stat 17/18, op 13)
    u.Property(1,11,198);   // proc on striking
    u.Property(2,1,91);     // Requirements % (descfunc 20)
    u.Property(3,1,39);     // Fire resist
    u.Property(4,17,394);   // Enchantment marker (+1 to stat 394)
    u.statRows[17*0x144+0x50]=13;u.statRows[18*0x144+0x50]=13;
    put(u.statRows.data()+91*0x144,0x32,std::uint16_t{20});
    put(fakeTables,0x14d0,6u);put(fakeTables,0x14d4,63u);
    std::array<unsigned char,348> uRow{};for(unsigned k=0;k<12;++k)put(uRow.data(),0x98+k*16,UINT32_MAX);
    put(uRow.data(),0x98,0u);put(uRow.data(),0xa0,300);put(uRow.data(),0xa4,350);
    put(uRow.data(),0xa8,1u);put(uRow.data(),0xac,64);put(uRow.data(),0xb0,15);put(uRow.data(),0xb4,20); // 15% lvl 20 Frozen Orb
    put(uRow.data(),0xb8,2u);put(uRow.data(),0xc0,50);put(uRow.data(),0xc4,100);
    put(fakeTables,0x13c8,uRow.data());put(fakeTables,0x13d0,std::uint64_t{1});
    const int orbLayer=(64<<6)|20;
    // Corrupted (361=2, 362=168) and multi-enchanted (392=2, 393=3, 394=5):
    // host stats (dmg% 344%, proc 15%, Requirements +52%) still resolve cleanly without (?) warnings.
    u.own={{std::uint64_t{17}<<32,344},{std::uint64_t{18}<<32,344},{std::uint64_t{91}<<32,52},
           {(std::uint64_t{198}<<32)|static_cast<unsigned>(orbLayer),15},
           {std::uint64_t{361}<<32,2},{std::uint64_t{362}<<32,168},
           {std::uint64_t{392}<<32,2},{std::uint64_t{393}<<32,3},{std::uint64_t{394}<<32,5}};
    u.total={{std::uint64_t{91}<<32,52},{(std::uint64_t{198}<<32)|static_cast<unsigned>(orbLayer),15},
             {std::uint64_t{361}<<32,2},{std::uint64_t{362}<<32,168},
             {std::uint64_t{392}<<32,2},{std::uint64_t{393}<<32,3},{std::uint64_t{394}<<32,5}};
    u.Bind();
    const auto edText=u.Render(17,"+344% Enhanced Weapon Damage");
    check(edText.find("[+300% - +350%]")!=std::string::npos && edText.find("[U]")!=std::string::npos && edText.find("(?)")==std::string::npos);
    const auto reqText=u.Render(91,"Requirements +52%");
    check(reqText.find("[+50% - +100%]")!=std::string::npos && reqText.find("[U]")!=std::string::npos && reqText.find("(?)")==std::string::npos);
    const char* orbProc="15% Chance to cast level 20 Frozen Orb on striking";
    const auto orbLabels=AffixLabels(u.host.data(),{{RangeText::Analyze(orbProc).key,198,orbLayer,false}},orbProc);
    const auto orbRendered=RangeText::Merge(orbProc,orbProc,4096,orbLabels).text;
    check(orbRendered.find("[15%]")!=std::string::npos && orbRendered.find("[U]")!=std::string::npos);
    // No-clone path (corrupted/enchanted item with observed={}): proc lines and 2-number
    // composite damage (Cold Damage) both recover without proc lines polluting composite candidates.
    u.Property(5,15,54);u.props[5].bytes[0x19]=16;put(u.props[5].bytes.data(),0x22,std::uint16_t{55});
    put(uRow.data(),0xc8,5u);put(uRow.data(),0xd0,200);put(uRow.data(),0xd4,300);
    u.own.insert(u.own.begin()+2,{std::uint64_t{54}<<32,200});
    u.own.insert(u.own.begin()+3,{std::uint64_t{55}<<32,300});
    u.total.insert(u.total.begin(),{std::uint64_t{54}<<32,200});
    u.total.insert(u.total.begin()+1,{std::uint64_t{55}<<32,300});
    u.Bind();
    const std::string noCloneFrost=std::string(orbProc)+"\nAdds 200-300 Cold Damage";
    const auto noCloneLabels=AffixLabels(u.host.data(),{},noCloneFrost);
    const auto noCloneRendered=RangeText::Merge(noCloneFrost,noCloneFrost,4096,noCloneLabels).text;
    check(noCloneRendered.find("[15%]")!=std::string::npos && noCloneRendered.find(orbProc)!=std::string::npos);
    check(noCloneRendered.find("[200 / 300]")!=std::string::npos && noCloneRendered.find("Adds 200-300 Cold Damage")!=std::string::npos);
    // Singular CubeMain enchantment candidate resolves as [E] when present in CubeMain.
    D2RL::DataTableService service{.serviceSize=D2RL::DataTableServiceRequiredSize,.serviceVersion=D2RL::DataTableService::AbiVersion,.getTable=FixtureTable};
    D2RL::PluginContext context{};contributionContext=&context;contributionTables=&service;
    std::array<unsigned char,448*2> items{};std::array<unsigned char,232*2> types{};std::array<unsigned char,364> cube{};
    put(items.data(),0x12e,std::int16_t{0});put(items.data(),0x130,std::int16_t{-1});
    put(types.data(),0,ContributionDefinitions::Code("weap"));put(types.data()+232,0,ContributionDefinitions::Code("orb "));
    cube[0]=1;cube[2]=0xff;cube[3]=18;cube[12]=2;put(cube.data(),4,394u);cube[0x67]=0xfe;
    put(cube.data(),0x14,std::uint16_t{2});put(cube.data(),0x16,0u);
    put(cube.data(),0x1e,std::uint16_t{1});put(cube.data(),0x20,1u);
    for(unsigned n=0;n<5;++n)put(cube.data(),0x77+n*12,UINT32_MAX);
    put(cube.data()+0x77,0,3u);put(cube.data()+0x77,6,std::int16_t{10});put(cube.data()+0x77,8,std::int16_t{15});cube[0x77+10]=100;
    put(cube.data()+0x77+12,0,4u);put(cube.data()+0x77+12,4,std::int16_t{1});
    contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::Properties)]={reinterpret_cast<const unsigned char*>(u.props.data()),u.props.size(),sizeof(Affixes::Property)};
    contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::Items)]={items.data(),2,448};
    contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::ItemTypes)]={types.data(),2,232};
    contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::CubeMain)]={cube.data(),1,364};
    u.own={{std::uint64_t{39}<<32,12},{std::uint64_t{394}<<32,1}};u.total=u.own;u.Bind();
    const auto enchText=u.Render(39,"Fire Resist +12%");
    check(enchText.find("[+10% - +15%]")!=std::string::npos && enchText.find("[E]")!=std::string::npos);
    // Status headers (Enchantments stat 394, Upgrades stat 419, Corrupted stat 361) never receive a spurious [E] tag:
    check(u.Render(394,"Enchantments: 1 / 1").find("[E]")==std::string::npos);
    // Multi-step Unique charm upgrades (e.g. Hellfire Torch / Annihilus stat 419, steps 0..2 with specific unique input flag 0x41):
    {
        std::array<unsigned char,364*3> torchCube{};
        u.Property(4,17,419);
        for(unsigned step=0;step<3;++step){
            auto* r=torchCube.data()+step*364;
            r[0]=1;r[2]=0xff;r[3]=18;r[12]=2;put(r,4,419u);put(r,8,static_cast<int>(step));r[0x67]=0xfe;
            put(r,0x14,std::uint16_t{0x41});put(r,0x16,std::uint16_t{0});put(r,0x18,std::uint16_t{1});r[0x1a]=7;
            put(r,0x1e,std::uint16_t{1});put(r,0x20,std::uint16_t{1});
            for(unsigned n=0;n<5;++n)put(r,0x77+n*12,UINT32_MAX);
            put(r+0x77,0,4u);put(r+0x77,6,std::int16_t{1});put(r+0x77,8,std::int16_t{1});r[0x77+10]=100;
            put(r+0x77+12,0,3u);put(r+0x77+12,6,std::int16_t{2});put(r+0x77+12,8,std::int16_t{2});r[0x77+12+10]=100;
        }
        contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::CubeMain)]={torchCube.data(),3,364};
        u.Property(7,1,397);
        put(uRow.data(),0xc8,3u);put(uRow.data(),0xd0,5);put(uRow.data(),0xd4,10); // Unique base Fire Resist 5-10%
        put(uRow.data(),0xd8,7u);put(uRow.data(),0xe0,1);put(uRow.data(),0xe4,1);  // Unique torch_weight (stat 397 = "The weight of evil...")
        u.own={{std::uint64_t{39}<<32,14},{std::uint64_t{397}<<32,1},{std::uint64_t{419}<<32,3}};u.total=u.own;u.Bind();
        const auto torchRes=u.Render(39,"Fire Resist +14%");
        check(torchRes.find("[U]")!=std::string::npos && torchRes.find("[E]")!=std::string::npos);
        check(torchRes.find("+8%")!=std::string::npos && torchRes.find("+6%")!=std::string::npos);
        check(u.Render(419,"Upgrades: 3 / 5").find("[E]")==std::string::npos);
        check(u.Render(397,"The weight of evil...").find("[U]")==std::string::npos);
        u.Property(4,17,394);
        u.Property(7,1,329);
        put(uRow.data(),0xc8,5u);put(uRow.data(),0xd0,200);put(uRow.data(),0xd4,300);
        put(uRow.data(),0xd8,6u);put(uRow.data(),0xe0,5);put(uRow.data(),0xe4,5);
        contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::CubeMain)]={cube.data(),1,364};
    }
    // Multi-enchanted residual [E] attribution and fixed fn=3 (extra-elem) independence:
    u.Property(6,1,329);
    for(unsigned k=1;k<4;++k){u.props[6].bytes[0x18+k]=3;put(u.props[6].bytes.data(),0x20+k*2,std::uint16_t(329+k));}
    u.Property(7,1,329);
    put(uRow.data(),0xd8,6u);put(uRow.data(),0xe0,5);put(uRow.data(),0xe4,5); // fixed +5% all elem
    u.own={{std::uint64_t{48}<<32,63},{std::uint64_t{49}<<32,511},
           {std::uint64_t{329}<<32,13},{std::uint64_t{330}<<32,5},{std::uint64_t{331}<<32,5},{std::uint64_t{332}<<32,5},
           {std::uint64_t{394}<<32,5}};
    u.total=u.own;u.Bind();
    check(u.Render(329,"+13% to Fire Skill Damage").find("[Combined]")!=std::string::npos);
    check(u.Render(330,"+5% to Cold Skill Damage").find("[+5%]")!=std::string::npos);
    // Crafted items (quality=8): Shadow Gorget Amulet (+2 Paladin Skills [E], +2 All Skills [Craft], +10% Skill Damage [Craft])
    // and Havoc Mark Vortex Shield (+10-15% Skill Damage [Craft], -10-15% Enemy Resistance [Craft] with dgrp=3/4 individual lines).
    put(u.Data(),0,8u);
    u.Property(0,1,127); // allskills
    u.Property(1,1,333); // pierce-elem
    for(unsigned k=1;k<4;++k){u.props[1].bytes[0x18+k]=3;put(u.props[1].bytes.data(),0x20+k*2,std::uint16_t(333+k));}
    u.Property(2,21,83);put(u.props[2].bytes.data(),0x0a,std::uint16_t{3}); // pal (stat 83, layer 3)
    put(u.statRows.data()+83*0x144,0x32,std::uint16_t{13});
    for(unsigned k=0;k<4;++k){
        put(u.statRows.data()+(329+k)*0x144,0x3a,std::uint16_t{3});
        put(u.statRows.data()+(333+k)*0x144,0x3a,std::uint16_t{4});
    }
    std::array<unsigned char,364*2> craftCube{};
    // Row 0: Crafted Amulet / Weapon recipe: allskills 2..2, extra-elem 10..10
    craftCube[0]=1;craftCube[2]=0xff;craftCube[3]=0;craftCube[12]=2;craftCube[0x64]=8;craftCube[0x67]=0xff;
    put(craftCube.data(),0x14,std::uint16_t{2});put(craftCube.data(),0x16,0u);
    for(unsigned n=0;n<5;++n)put(craftCube.data(),0x77+n*12,UINT32_MAX);
    put(craftCube.data()+0x77,0,0u);put(craftCube.data()+0x77,6,std::int16_t{2});put(craftCube.data()+0x77,8,std::int16_t{2});craftCube[0x77+10]=100;
    put(craftCube.data()+0x77+12,0,6u);put(craftCube.data()+0x77+12,6,std::int16_t{10});put(craftCube.data()+0x77+12,8,std::int16_t{10});craftCube[0x77+12+10]=100;
    // Row 1: Crafted Shield recipe: allskills 2..2, extra-elem 10..15, pierce-elem 10..15
    auto* craftRow2=craftCube.data()+364;
    craftRow2[0]=1;craftRow2[2]=0xff;craftRow2[3]=0;craftRow2[12]=2;craftRow2[0x64]=8;craftRow2[0x67]=0xff;
    put(craftRow2,0x14,std::uint16_t{2});put(craftRow2,0x16,0u);
    for(unsigned n=0;n<5;++n)put(craftRow2,0x77+n*12,UINT32_MAX);
    put(craftRow2+0x77,0,0u);put(craftRow2+0x77,6,std::int16_t{2});put(craftRow2+0x77,8,std::int16_t{2});craftRow2[0x77+10]=100;
    put(craftRow2+0x77+12,0,6u);put(craftRow2+0x77+12,6,std::int16_t{10});put(craftRow2+0x77+12,8,std::int16_t{15});craftRow2[0x77+12+10]=100;
    put(craftRow2+0x77+24,0,1u);put(craftRow2+0x77+24,6,std::int16_t{10});put(craftRow2+0x77+24,8,std::int16_t{15});craftRow2[0x77+24+10]=100;
    contributionFixtureTables[static_cast<unsigned>(D2RL::DataTables::TableId::CubeMain)]={craftCube.data(),2,364};
    // Shadow Gorget Amulet state:
    u.own={{(std::uint64_t{83}<<32)|3,2},{std::uint64_t{127}<<32,2},
           {std::uint64_t{329}<<32,10},{std::uint64_t{330}<<32,10},{std::uint64_t{331}<<32,10},{std::uint64_t{332}<<32,10},
           {std::uint64_t{392}<<32,2}};
    u.total=u.own;u.Bind();
    const char* palSkills="+2 to Paladin Skill Levels";
    const auto palLabels=AffixLabels(u.host.data(),{{RangeText::Analyze(palSkills).key,83,3,false}},palSkills);
    const auto palRendered=RangeText::Merge(palSkills,palSkills,4096,palLabels).text;
    check(palRendered.find("[+2]")!=std::string::npos && palRendered.find("[E]")!=std::string::npos);
    check(u.Render(127,"+2 to All Skills").find("[Craft]")!=std::string::npos);
    check(u.Render(329,"+10% to Fire Skill Damage").find("[Craft]")!=std::string::npos);
    // Havoc Mark Vortex Shield state:
    u.own={{std::uint64_t{127}<<32,2},
           {std::uint64_t{329}<<32,15},{std::uint64_t{330}<<32,15},{std::uint64_t{331}<<32,15},{std::uint64_t{332}<<32,15},
           {std::uint64_t{333}<<32,10},{std::uint64_t{334}<<32,10},{std::uint64_t{335}<<32,10},{std::uint64_t{336}<<32,10}};
    u.total=u.own;u.Bind();
    const auto coldDmg=u.Render(331,"+15% to Cold Skill Damage");
    check(coldDmg.find("[+10% - +15%]")!=std::string::npos && coldDmg.find("[Craft]")!=std::string::npos);
    const auto coldPierce=u.Render(335,"-10% to Enemy Cold Resistance");
    check(coldPierce.find("[-10% - -15%]")!=std::string::npos && coldPierce.find("[Craft]")!=std::string::npos);
    propertySourcesReady.store(false);scalingReady.store(false);originalSingle=nullptr;
    contributionContext=nullptr;contributionTables=nullptr;contributionFixtureTables={};
    std::puts("Verified owned durability and physical endpoint rendering, base exclusion, mismatches and operation guards.");
    TestRandomizedItemAffixCombinations();
}
}

namespace {
void TestChargeSourceOnly(){
    ContributionFixture f;f.Property(0,19,204);f.Roll(0,0,-60,-10,"of Miasma Bolt");
    put(f.rows[0].bytes.data(),0x28,395);put(fakeTables,0x14d0,6u);put(fakeTables,0x14d4,63u);
    const int layer=(395<<6)|11;
    f.own={{(std::uint64_t{204}<<32)|static_cast<unsigned>(layer),36486}};f.total=f.own;f.Bind();
    const char* actual="Level 11 Miasma Bolt (134/142 Charges)";
    const auto key=RangeText::Analyze(actual).key;
    const std::vector<CapturedLine> observed={CapturedLine{key,204,layer,false}};
    const auto labels=AffixLabels(f.host.data(),observed,actual);
    const auto rendered=RangeText::Merge(actual,actual,4096,labels);
    check(rendered.text.find("[S] [of Miasma Bolt] [T1]")!=std::string::npos);
    check(rendered.text.starts_with(actual) && rendered.annotated==0 && rendered.text.find('?')==std::string::npos);
    const std::string marker="\xee\x81\xbe";
    const std::string native="Level "+marker+"U(1-20)"+marker+"3 Miasma Bolt (134/142 Charges)";
    const auto suppressed=RangeText::Merge(actual,native,4096,labels);
    check(suppressed.text==rendered.text && suppressed.annotated==0);
    check(RangeText::Merge(actual,native,10,labels).text==actual);
    // RangeHelperAdapter wraps the charge fraction in one U span, emitting key
    // "Level#IronGolem#Charges" while actual has "Level#IronGolem(#/#Charges)".
    const char* golemActual="Level 2 Iron Golem (34/35 Charges)";
    const std::string golemRanged="Level 35 Iron Golem "+marker+"U(5/35)"+marker+"3 Charges";
    put(f.rows[0].bytes.data(),0x28,94);
    const int golemLayer=(94<<6)|2;
    f.own={{(std::uint64_t{204}<<32)|static_cast<unsigned>(golemLayer),(35<<8)|34}};f.total=f.own;f.Bind();
    const std::vector<CapturedLine> golemObserved={CapturedLine{RangeText::Analyze(golemRanged).key,204,golemLayer,false}};
    const auto golemLabels=AffixLabels(f.host.data(),golemObserved,golemActual);
    check(golemLabels.size()==1 && golemLabels[0].key==RangeText::Analyze(golemActual).key && golemLabels[0].sourceOnly);
    const auto golemMerged=RangeText::Merge(golemActual,golemRanged,4096,golemLabels);
    check(golemMerged.text.starts_with(golemActual) && golemMerged.text.find("Level 35")==std::string::npos);
    check(golemMerged.text.find("[S] [of Miasma Bolt] [T1]")!=std::string::npos);
    // No-clone recovery also identifies stat204 directly from owned charge state.
    scalingReady.store(true);
    const auto golemNoClone=AffixLabels(f.host.data(),{},golemActual);
    scalingReady.store(false);
    check(golemNoClone.size()==1 && golemNoClone[0].key==RangeText::Analyze(golemActual).key && golemNoClone[0].sourceOnly);
    put(f.rows[0].bytes.data(),0x28,395);
    const Affixes::Rolled affix{1,f.rows[0],false,2};const Affixes::LayerEncoding encoding{6,63};
    check(!Affixes::Contributes(affix.row,f.props,204,layer,encoding)); // Numeric matching not relaxed.
    check(Affixes::ChargeSourceLabel({&affix,1},f.props,layer,encoding)=="[S] [of Miasma Bolt] [T2]");
    check(Affixes::ChargeSourceLabel({&affix,1},f.props,(396<<6)|11,encoding).empty());
    check(Affixes::ChargeSourceLabel({&affix,1},f.props,395<<6,encoding).empty());
    check(Affixes::ChargeSourceLabel({&affix,1},f.props,layer,{6,31}).empty());
    const std::array duplicate{affix,affix};check(Affixes::ChargeSourceLabel(duplicate,f.props,layer,encoding).empty());
    auto fixed=affix;put(fixed.row.bytes.data(),0x30,11);
    check(!Affixes::ChargeSourceLabel({&fixed,1},f.props,layer,encoding).empty());
    check(Affixes::ChargeSourceLabel({&fixed,1},f.props,(395<<6)|10,encoding).empty());
    for(const char* source:{"[U]","[Set]","[RW]"}){auto named=affix;named.source=source;named.tier=0;
        check(Affixes::ChargeSourceLabel({&named,1},f.props,layer,encoding)==source);}
    f.props[0].bytes[2]=1;check(Affixes::ChargeSourceLabel({&affix,1},f.props,layer,encoding).empty());
    std::puts("Verified charge source-only labels, calculated/fixed levels, unchanged remaining/capacity, ambiguity and native-range suppression.");
}
}
