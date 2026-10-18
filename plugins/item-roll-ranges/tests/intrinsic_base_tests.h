// Production-bound regressions for an implicit formatter-only base stat.
namespace {
std::array<unsigned char,448> intrinsicFixtureRow{};
const unsigned char* __fastcall IntrinsicFixtureRow(unsigned char bank,unsigned) {check(bank==2);return intrinsicFixtureRow.data();}
bool __fastcall IntrinsicFixtureSingle(void*,const void* metadata,int value,int layer,bool grouped,char* output,int mode) {
    if(Affixes::Read<std::uint16_t>(static_cast<const unsigned char*>(metadata),0)!=122)return false;
    check(!layer && !grouped && mode==1);std::snprintf(output,256,"+%d%% Damage to Undead",value);return true;
}
void TestIntrinsicReportedBase(){
    const auto savedArmor=getArmor;const auto savedSingle=originalSingle;
    ContributionFixture f;propertySourcesReady.store(true);getArmor=IntrinsicFixtureRow;originalSingle=IntrinsicFixtureSingle;
    put(f.Data(),0,7u);put(f.Data(),0x34,0);put(f.modifier.data(),0x68,static_cast<unsigned char*>(nullptr));
    f.Property(2,1,122);std::array<unsigned char,348> unique{};
    for(unsigned k=0;k<12;++k)put(unique.data(),0x98+k*16,UINT32_MAX);
    put(fakeTables,0x13c8,unique.data());put(fakeTables,0x13d0,std::uint64_t{1});
    std::array<unsigned char,60*232> types{};intrinsicFixtureRow={};
    put(types.data()+57*232,0,ContributionDefinitions::Code("blun"));
    put(types.data()+58*232,4,std::int16_t{57});
    put(intrinsicFixtureRow.data(),0x12e,std::int16_t{58});put(intrinsicFixtureRow.data(),0x130,std::int16_t{-1});
    put(fakeTables,0x1348,types.data());put(fakeTables,0x1350,std::uint64_t{60});f.own={};f.total={};
    auto text=f.Render(122,"+50% Damage to Undead");std::printf("Intrinsic replay: %s\n",text.c_str());
    check(text.find("[+50%]")!=std::string::npos && text.find("[Base]")!=std::string::npos);
    check(f.Render(122,"+51% Damage to Undead").find("[Base]")==std::string::npos);
    // A rolled damage-to-undead property and the implicit bonus stay separate.
    put(unique.data(),0x98,2u);put(unique.data(),0xa0,100);put(unique.data(),0xa4,200);
    f.own=f.Stats({{122,150}});f.total=f.own;text=f.Render(122,"+200% Damage to Undead");
    check(text.find("[+150% - +250%]")!=std::string::npos && text.find("+150% Damage to Undead")!=std::string::npos && text.find("[Base]")!=std::string::npos);
    put(types.data()+58*232,4,std::int16_t{58});check(f.Render(122,"+200% Damage to Undead").find("[Base]")==std::string::npos);
    put(types.data()+58*232,4,std::int16_t{57});put(fakeTables,0x1350,std::uint64_t{57});
    check(f.Render(122,"+200% Damage to Undead").find("[Base]")==std::string::npos);
    put(fakeTables,0x1350,std::uint64_t{60});put(types.data()+57*232,0,ContributionDefinitions::Code("xxxx"));
    check(f.Render(122,"+200% Damage to Undead").find("[Base]")==std::string::npos);
    put(types.data()+57*232,0,ContributionDefinitions::Code("blun"));f.total=f.Stats({{122,151}});
    check(f.Render(122,"+200% Damage to Undead").find("[Base]")==std::string::npos);
    originalSingle=savedSingle;getArmor=savedArmor;propertySourcesReady.store(false);
    std::puts("Implicit base-only/rolled undead labels, native totals, malformed types and mismatch refusal pass.");
}
}
