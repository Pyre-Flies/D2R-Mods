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
}

}
