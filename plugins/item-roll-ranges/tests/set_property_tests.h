// Permanent set properties must not depend on native clone generation.
namespace {
void TestPermanentSetProduction(){
    ContributionFixture f;propertySourcesReady.store(true);
    put(f.Data(),0,5u);put(f.Data(),0x34,0);put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));
    f.Property(0,1,127);f.Property(1,1,80);f.Property(2,1,378);
    std::array<unsigned char,460> row{};
    for(unsigned n=0;n<9;++n)put(row.data(),0x98+n*16,UINT32_MAX);
    auto spec=[&](unsigned n,unsigned property,int low,int high){const auto at=0x98+n*16;
        put(row.data(),at,property);put(row.data(),at+8,low);put(row.data(),at+12,high);};
    spec(0,0,1,2);spec(1,1,10,30);spec(2,2,1,1);row[0x2c]=9;
    // This overlapping conditional set bonus is not a permanent item property.
    put(row.data(),0x128,1u);put(row.data(),0x130,200);put(row.data(),0x134,200);
    put(fakeTables,0x13a8,row.data());put(fakeTables,0x13b0,std::uint64_t{1});
    f.own=f.Stats({{80,26},{127,2},{378,1}});f.total=f.own;
    auto text=f.Render(80,"+26% Chance Items Roll Magic or Better");
    check(text.find("[+10% - +30%]")!=std::string::npos && text.find("[Set]")!=std::string::npos);
    check(text.find("200")==std::string::npos);
    check(f.Render(127,"+2 to All Skills").find("[+1 - +2]")!=std::string::npos);
    check(f.Render(378,"+1 Bone Spear Projectiles").find("[+1]")!=std::string::npos);
    put(f.Data(),0x34,1);check(f.Render(80,"+26% Chance Items Roll Magic or Better").find("[Set]")==std::string::npos);
    put(f.Data(),0x34,0);put(f.modifier.data(),0x20,159u);
    check(f.Render(80,"+26% Chance Items Roll Magic or Better").find("[+10% - +30%]")==std::string::npos);
    put(f.modifier.data(),0x20,0u);
    std::array<unsigned char,0xc0> activated{};
    auto conditional=f.Stats({{127,1}});
    put(activated.data(),0,f.host.data());put(activated.data(),0x18,-1);
    put(activated.data(),0x1c,0x40u);put(activated.data(),0x20,165u);
    put(activated.data(),0x78,f.root.data());put(activated.data(),0x68,f.modifier.data());
    put(activated.data(),0x30,conditional.data());put(activated.data(),0x38,std::uint64_t{1});put(activated.data(),0x40,std::uint64_t{1});
    put(f.root.data(),0x90,activated.data());
    f.total=f.Stats({{80,26},{127,3},{378,1}});
    check(f.Render(80,"+26% Chance Items Roll Magic or Better").find("[+10% - +30%]")!=std::string::npos);
    check(f.Render(127,"+2 to All Skills").find("[+1 - +2]")!=std::string::npos);
    f.Bind();
    for(unsigned state=165;state<=170;++state){
        put(activated.data(),0x20,state);
        const auto snapshot=CaptureItemContributions(f.host.data());
        check(snapshot && Contributions::Value(snapshot->own,127)==2 && Contributions::Value(snapshot->activeSet,127)==1);
        check(Contributions::Value(snapshot->total,127)==3); // Live aggregate remains intact.
        originalProperties=AppendProperties;
        char output[1024]="already formatted [Set]\n";
        check(RenderProperties(f.host.data(),f.host.data(),output,1024,0,static_cast<int>(state),0,64,0,nullptr,nullptr)==42);
        check(std::string(output)=="already formatted [Set]\n+1 to All Skills\n");
    }
    put(activated.data(),0x20,164u);check(!CaptureItemContributions(f.host.data()));
    put(activated.data(),0x20,165u);put(activated.data(),0x18,20);check(!CaptureItemContributions(f.host.data()));
    put(activated.data(),0x18,-1);put(activated.data(),0,f.jewel.data());check(!CaptureItemContributions(f.host.data()));
    put(activated.data(),0,f.host.data());put(f.Data(),0,7u);check(!CaptureItemContributions(f.host.data()));
    put(f.Data(),0,5u);
    put(f.root.data(),0x90,f.modifier.data());
    f.total=f.own;check(f.Render(127,"+2 to All Skills").find("[+1 - +2]")!=std::string::npos);
    propertySourcesReady.store(false);
    std::puts("Permanent set skills/MF/custom stats, clone/Chronicle independence, conditional exclusion and invalid-state refusal pass.");
}
void TestHeaderHitAndOwnerLevelCaching();
void TestPoisonDurationProduction(){
    ContributionFixture f;propertySourcesReady.store(true);poisonDisplayReady.store(true);
    put(f.Data(),0,5u);put(f.Data(),0x34,0);put(f.Data()-24,0,0u);
    put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));
    f.Property(0,15,57);f.props[0].bytes[0x19]=16;f.props[0].bytes[0x1a]=17;
    put(f.props[0].bytes.data(),0x22,std::uint16_t{58});put(f.props[0].bytes.data(),0x24,std::uint16_t{59});
    std::array<unsigned char,460> row{};
    for(unsigned n=0;n<9;++n)put(row.data(),0x98+n*16,UINT32_MAX);
    put(row.data(),0x98,0u);put(row.data(),0x9c,100);put(row.data(),0xa0,10);put(row.data(),0xa4,20);
    put(fakeTables,0x13a8,row.data());put(fakeTables,0x13b0,std::uint64_t{1});
    f.own=f.Stats({{57,10},{58,20},{59,100},{326,1}});f.total=f.own;f.Bind();
    const std::string actual="Adds 4-8 Weapon Poison Damage over 4 Seconds";
    const auto labels=AffixLabels(f.host.data(),{},actual);
    const auto text=RangeText::Merge(actual,actual,4096,labels).text;
    std::printf("Poison duration replay: %s\n",text.c_str());
    check(text.find("[4 / 8]")!=std::string::npos && text.find("[Set]")!=std::string::npos);
    const auto render=[&](const std::string& input){f.Bind();return RangeText::Merge(input,input,4096,AffixLabels(f.host.data(),{},input)).text;};
    check(render("Adds 4-8 Weapon Poison Damage over 5 Seconds").find("[4 / 8]")==std::string::npos);
    check(render("Adds 5-8 Weapon Poison Damage over 4 Seconds").find("[4 / 8]")==std::string::npos);
    for(const int frames:{25,50,75,100,125,150}){
        put(row.data(),0x9c,frames);f.own=f.Stats({{57,10},{58,20},{59,frames},{326,1}});f.total=f.own;
        const auto low=(10*frames+128)/256,high=(20*frames+128)/256;
        const auto input="Adds "+std::to_string(low)+"-"+std::to_string(high)+" Weapon Poison Damage over "+std::to_string(frames/25)+" Seconds";
        check(render(input).find("["+std::to_string(low)+" / "+std::to_string(high)+"]")!=std::string::npos);
    }
    put(row.data(),0x9c,100);put(row.data(),0xa4,10);f.own=f.Stats({{57,10},{58,10},{59,100},{326,1}});f.total=f.own;
    check(render("Adds 4 Weapon Poison Damage over 4 Seconds").find("[4 / 4]")!=std::string::npos);
    put(row.data(),0xa4,20);f.props[0].bytes[0x18]=1;f.own=f.Stats({{57,12},{58,20},{59,100},{326,1}});f.total=f.own;
    check(render("Adds 5-8 Weapon Poison Damage over 4 Seconds").find("[4 - 8 / 8]")!=std::string::npos);
    put(row.data(),0x9c,101);check(render("Adds 5-8 Weapon Poison Damage over 4 Seconds").find("[4 - 8 / 8]")==std::string::npos);
    poisonDisplayReady.store(false);
    propertySourcesReady.store(false);
    TestHeaderHitAndOwnerLevelCaching();
}

unsigned headerHitBuilderCalls{};
int headerHitOwnerLevel=80;
unsigned ownerLevelPropertyCalls{};
std::string headerHitStorage;
NativeText* __fastcall FakeHeaderHitBuilder(void* a,void*,void*,void*,int,unsigned char,unsigned char,void*,void*) {
    ++headerHitBuilderCalls;
    headerHitStorage="Defense: 100\nRequired Level: 20";
    auto* text=static_cast<NativeText*>(a);
    text->data=headerHitStorage.data();
    text->size=headerHitStorage.size();
    return text;
}
void* __fastcall FakeHeaderHitAssign(NativeText* target,const char* data,std::size_t size) {
    headerHitStorage.assign(data,size);
    target->data=headerHitStorage.data();
    target->size=headerHitStorage.size();
    return target;
}
const unsigned char* __fastcall FakeHeaderHitArmor(unsigned char,unsigned) {
    static std::array<unsigned char,0x200> armorRow{};
    armorRow[LevelProfile::QualityLevelOffset]=25;
    return armorRow.data();
}
std::uint64_t __fastcall FakeHeaderHitProperties(void*,char* output,int capacity,int,int,int,int,int,const void*,void*) {
    ++ownerLevelPropertyCalls;
    char line[256]{};
    std::snprintf(line,sizeof(line),"+26%% Chance Items Roll Magic or Better\n+%d Defense (Based on Character Level)\n",headerHitOwnerLevel);
    strcat_s(output,static_cast<std::size_t>(capacity),line);if(captureLines)captureLines->push_back({RangeText::Analyze("+26% Chance Items Roll Magic or Better").key,80,0,false,0});
    return 1;
}
void TestHeaderHitAndOwnerLevelCaching(){
    ContributionFixture f;
    active.store(true);
    scalingReady.store(true);
    propertySourcesReady.store(true);
    headerBuilderCache={};
    propertyRenderCache={};
    put(f.Data(),0,5u);put(f.Data(),0x18,0x10u);put(f.Data(),0x34,0);
    put(f.Data(),LevelProfile::ItemLevelOffset,85);
    put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));
    f.Property(0,1,80);
    std::array<unsigned char,460> row{};
    for(unsigned n=0;n<9;++n)put(row.data(),0x98+n*16,UINT32_MAX);
    put(row.data(),0x98,0u);put(row.data(),0xa0,10);put(row.data(),0xa4,30);row[0x2c]=9;
    put(fakeTables,0x13a8,row.data());put(fakeTables,0x13b0,std::uint64_t{1});
    f.own=f.Stats({{80,26}});f.total=f.own;f.Bind();

    std::array<unsigned char,0x100> playerUnit{};
    std::array<unsigned char,0xc0> playerStats{};
    Contributions::Stats playerStatVec={{std::uint64_t{12}<<32,80}};
    put(playerUnit.data(),0,0u);
    put(playerUnit.data(),0x88,playerStats.data());
    put(playerStats.data(),0x30,playerStatVec.data());
    put(playerStats.data(),0x38,std::uint64_t{1});
    put(f.Data(),0x90,playerUnit.data());

    const auto prevBuilder=originalBuilder;
    const auto prevAssign=originalAssign;
    const auto prevArmor=getArmor;
    const auto prevLookup=lookupHeader;
    const auto prevProperties=originalProperties;
    const auto prevPropsReturn=propertiesReturn;
    originalBuilder=FakeHeaderHitBuilder;
    originalAssign=FakeHeaderHitAssign;
    getArmor=FakeHeaderHitArmor;
    lookupHeader=FixtureSocketLookup;
    originalProperties=FakeHeaderHitProperties;
    propertiesReturn=0x12345678u;
    headerHitBuilderCalls=0;
    ownerLevelPropertyCalls=0;
    headerHitOwnerLevel=80;

    std::array<unsigned char,0x1900> frame{};
    std::array<std::uint64_t,6> headerBuf{};
    // Frame 1: header cache miss with Held()==false, scoped via fixedHeaderRequested.
    fixedHeaderRequested=true;
    auto* hdr1=RunHeaderBuilder(true,frame.data(),headerBuf.data(),nullptr,playerUnit.data(),f.host.data(),0,0,0,nullptr,nullptr);
    check(hdr1!=nullptr && headerHitBuilderCalls==1 && pendingPropertyRequest.held);
    char propOut1[2048]{};
    check(RunPropertiesAdapter(propertiesReturn,true,f.host.data(),propOut1,sizeof(propOut1),0,0,0,64,0,nullptr,nullptr)==1);
    check(ownerLevelPropertyCalls==1);
    check(std::string(propOut1).find("[+10% - +30%]")!=std::string::npos && std::string(propOut1).find("[Set]")!=std::string::npos);
    check(std::string(propOut1).find("+80 Defense (Based on Character Level)")!=std::string::npos);

    // Frame 2: header cache hit must preserve pendingPropertyRequest so PropertiesAdapter stays scoped when Held()==false.
    fixedHeaderRequested=true;
    auto* hdr2=RunHeaderBuilder(true,frame.data(),headerBuf.data(),nullptr,playerUnit.data(),f.host.data(),0,0,0,nullptr,nullptr);
    check(hdr2!=nullptr && headerHitBuilderCalls==1 && pendingPropertyRequest.held);
    char propOut2[2048]{};
    check(RunPropertiesAdapter(propertiesReturn,true,f.host.data(),propOut2,sizeof(propOut2),0,0,0,64,0,nullptr,nullptr)==1);
    check(ownerLevelPropertyCalls==1); // Served from propertyRenderCache at level 80.
    check(std::string(propOut2)==std::string(propOut1));

    // Level up owner from 80 -> 81: PropertyCacheKey.ownerLevel must invalidate propertyRenderCache.
    playerStatVec[0].value=81;
    headerHitOwnerLevel=81;
    fixedHeaderRequested=true;
    auto* hdr3=RunHeaderBuilder(true,frame.data(),headerBuf.data(),nullptr,playerUnit.data(),f.host.data(),0,0,0,nullptr,nullptr);
    check(hdr3!=nullptr && headerHitBuilderCalls==2 && pendingPropertyRequest.held);
    char propOut3[2048]{};
    check(RunPropertiesAdapter(propertiesReturn,true,f.host.data(),propOut3,sizeof(propOut3),0,0,0,64,0,nullptr,nullptr)==1);
    check(ownerLevelPropertyCalls==2);
    check(std::string(propOut3).find("[+10% - +30%]")!=std::string::npos && std::string(propOut3).find("[Set]")!=std::string::npos);
    check(std::string(propOut3).find("+81 Defense (Based on Character Level)")!=std::string::npos);

    originalBuilder=prevBuilder;
    originalAssign=prevAssign;
    getArmor=prevArmor;
    lookupHeader=prevLookup;
    originalProperties=prevProperties;
    propertiesReturn=prevPropsReturn;
    currentTooltipPlayer=nullptr;
    pendingPropertyRequest={};
    headerBuilderCache={};
    propertyRenderCache={};
    scalingReady.store(false);
    propertySourcesReady.store(false);
    active.store(false);
    std::puts("Verified header-cache hit preserves pendingPropertyRequest and owner level invalidates propertyRenderCache.");
}

}
