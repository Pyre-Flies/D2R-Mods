// Exercise the actual adapter functions and pointer ownership without a game.
#include "../src/plugin.cpp"
#include <cstdio>
#include <source_location>
#include <cstdlib>
namespace {
std::uint64_t __fastcall FakeKey() { return 7; }
std::uint64_t __fastcall OtherKey() { return 9; }
void check(bool value,std::source_location at=std::source_location::current()) { if (!value) {std::fprintf(stderr,"Check failed: %s:%u\n",at.file_name(),at.line());std::exit(1);} }
void* testSlot{};
void TestPropertyPasses();
void TestAffixRead();
void TestOwnedContributionAdapter();
void TestRangeHelper();
void TestSpecial();
void TestUniqueRange();
void TestHeaderDelegation();
void TestCalculatedHeaders();
void TestAlwaysAdapters();
void TestLinkedProduction();
void TestRandomSkillProduction();
void TestLiveRandomSkillCapture(const char* path);
void TestLiveUniqueCapture(const char* path);
void TestReportedCapture(const char* path,const char* which);
void TestOwnedDurabilityPhysical();
void TestChargeSourceOnly();
void TestIntrinsicReportedBase();
void TestPermanentSetProduction();
void TestPoisonDurationProduction();

}
namespace {
unsigned modifierCalls{},buttonCalls{},widgetCalls{};
void* observedDevice{};unsigned observedUser{},observedMask{};
std::uint64_t __fastcall FakeAlwaysModifiers(){++modifierCalls;return 0x12340005;}
std::uint64_t __fastcall FakeAlwaysButtons(void* device,unsigned user,unsigned mask){
    ++buttonCalls;observedDevice=device;observedUser=user;observedMask=mask;return 0xfedcba9876543210ull;
}
std::uint64_t __fastcall FakeAlwaysWidget(const char* text,void* position,void* layout,const char* footer){
    ++widgetCalls;check(std::strcmp(text,"actual tooltip")==0 && position==reinterpret_cast<void*>(2) &&
        layout==reinterpret_cast<void*>(3) && std::strcmp(footer,"native footer")==0);return 0x87654321;
}
void TestAlwaysAdapters(){
    active.store(false);alwaysReady.store(true);
    originalModifiers=FakeAlwaysModifiers;originalButtons=FakeAlwaysButtons;originalWidget=FakeAlwaysWidget;
    modifierReturn=buttonReturn=widgetReturn=0;
    check(AlwaysModifierAdapter()==0x12340005 && modifierCalls==1);
    check(AlwaysButtonAdapter(reinterpret_cast<void*>(1),3,0x800)==0xfedcba9876543210ull);
    check(buttonCalls==1 && observedDevice==reinterpret_cast<void*>(1) && observedUser==3 && observedMask==0x800);
    check(AlwaysWidgetAdapter("actual tooltip",reinterpret_cast<void*>(2),reinterpret_cast<void*>(3),"native footer")==0x87654321 && widgetCalls==1);
    void* keys=reinterpret_cast<void*>(&AlwaysModifierAdapter);
    void* buttons=reinterpret_cast<void*>(&FakeKey); // Later owner must survive.
    void* widget=reinterpret_cast<void*>(&AlwaysWidgetAdapter);
    modifierSlot=&keys;buttonSlot=&buttons;widgetSlot=&widget;
    RestoreAlways();check(!alwaysReady.load());
    check(keys==reinterpret_cast<void*>(&FakeAlwaysModifiers) && buttons==reinterpret_cast<void*>(&FakeKey));
    check(widget==reinterpret_cast<void*>(&FakeAlwaysWidget));
    modifierSlot=buttonSlot=widgetSlot=nullptr;
    // A short string ending at a readable page boundary must not require a
    // whole 16KiB contiguous region; a missing terminator must refuse safely.
    SYSTEM_INFO info{};GetSystemInfo(&info);const auto page=static_cast<std::size_t>(info.dwPageSize);
    auto* memory=static_cast<char*>(VirtualAlloc(nullptr,page*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(memory!=nullptr);
    DWORD old{};check(VirtualProtect(memory+page,page,PAGE_NOACCESS,&old)!=FALSE);
    std::memcpy(memory+page-6,"hello",6);
    check(CopyNativeString(memory+page-6,16384)==std::optional<std::string>{"hello"});
    memory[page-1]='x';check(!CopyNativeString(memory+page-6,16384));
    check(!CopyNativeString(nullptr,16384));VirtualFree(memory,0,MEM_RELEASE);
}
std::string publishedHeader;
void* __fastcall RecordHeader(NativeText* receiver,const char* data,std::size_t size) {
    publishedHeader.assign(data,size);return receiver;
}
void TestRestoredHeaderContext() {
    NativeText text{"Defense: 164",12};
    originalAssign=RecordHeader;
    pendingHeader={&text,"Defense: 164","Defense: 164 (Base: 96 - 96)"};
    check(!RangeFromOverlay(nullptr)); // Native range TLS already restored.
    check(PublishHeader(true,&text,"Defense: (163-172)",18)==&text);
    check(publishedHeader=="Defense: 164 (Base: 96 - 96)");
    pendingHeader={&text,"changed original","wrong"};
    PublishHeader(true,&text,"native fallback",15);
    check(publishedHeader=="native fallback");
    pendingHeader={&text,"Defense: 164","wrong"};
    PublishHeader(false,&text,"native fallback",15);
    check(publishedHeader=="native fallback");pendingHeader={};
}
}
namespace {
void TestTooltipVisibilitySnapshots() {
    std::array<unsigned char,0x15c> row{},copy{};row[0x2c]=11;row[0x98]=37;
    const auto original=row;
    check(VisibleUniqueRow(row.data(),copy)==copy.data());check(copy[0x2c]==3 && copy[0x98]==37 && row==original);
    row[0x2c]=3;check(VisibleUniqueRow(row.data(),copy)==row.data());
    row[0x2c]=15;check(VisibleUniqueRow(row.data(),copy)==row.data());
    std::array<unsigned char,0x13b8> bank{};
    std::array<unsigned char,0x1cc*2> rows{};
    unsigned char unit[0x18]{},data[0x38]{};
    const unsigned type=4,quality=5;const int record=1;
    const auto* itemData=data;const auto* tableRows=rows.data();const std::uint64_t count=2;
    std::memcpy(unit,&type,4);std::memcpy(unit+0x10,&itemData,8);
    std::memcpy(data,&quality,4);std::memcpy(data+0x34,&record,4);
    std::memcpy(bank.data()+0x13a8,&tableRows,8);std::memcpy(bank.data()+0x13b0,&count,8);
    rows[0x98]=99;rows[0x1cc+0x2c]=9;rows[0x1cc+0x98]=27;const auto originalRows=rows;
    SetVisibilityView view;auto* result=VisibleSetTables(bank.data(),unit,view);
    check(result!=bank.data() && rows==originalRows);
    auto* copied=Affixes::Read<const unsigned char*>(result,0x13a8);
    check(copied[0x98]==99 && copied[0x1cc+0x2c]==1 && copied[0x1cc+0x98]==27);
    data[0]=7;check(VisibleSetTables(bank.data(),unit,view)==bank.data());data[0]=5;
    const int invalid=2;std::memcpy(data+0x34,&invalid,4);check(VisibleSetTables(bank.data(),unit,view)==bank.data());
    unsigned char stack[0x138]{};const auto* oldBase=coreBase;coreBase=reinterpret_cast<const unsigned char*>(0x10000000);
    const std::uintptr_t parent=0x10000000+0x376e3c,outer=0x10000000+0x81977c;
    std::memcpy(stack+0x50,&parent,8);std::memcpy(stack+0x130,&outer,8);
    check(TooltipSelectorScope(stack,0x10000000+0x377cfc,true));
    check(!TooltipSelectorScope(stack,0x10000000+0x377cfc,false));stack[0x130]^=1;
    check(!TooltipSelectorScope(stack,0x10000000+0x377cfc,true));coreBase=oldBase;
}
}
namespace {
std::string rangeConfig="# keep\r\n[display]\r\nalways_show_ranges = false # startup\r\n[other]\r\nkeep = 8\r\n";
unsigned rangeReads{},rangeWrites{};bool rangeReadFail{},rangeWriteFail{};
bool __cdecl ReadRangeConfig(const D2RL::PluginContext*,char* output,unsigned size,unsigned* required) noexcept {
    ++rangeReads;*required=static_cast<unsigned>(rangeConfig.size()+1);
    if(rangeReadFail || size<*required)return false;
    std::memcpy(output,rangeConfig.c_str(),*required);return true;
}
bool __cdecl WriteRangeConfig(const D2RL::PluginContext*,const char* text) noexcept {
    ++rangeWrites;if(rangeWriteFail)return false;rangeConfig=text;return true;
}
}
int main(int argc,char** argv) {
    if(argc==2 && std::strcmp(argv[1],"--intrinsic-base")==0){TestIntrinsicReportedBase();return 0;}
    if(argc==4 && std::strcmp(argv[1],"--reported-capture")==0){TestReportedCapture(argv[2],argv[3]);return 0;}
    TestOwnedDurabilityPhysical();
    TestChargeSourceOnly();
    const D2RL::PluginApi rangeApi{.apiSize=D2RL::PluginApiSize,.readConfig=ReadRangeConfig,.writeConfig=WriteRangeConfig};
    const D2RL::PluginContext rangeContext{.contextSize=sizeof(D2RL::PluginContext),.api=&rangeApi};
    contributionContext=&rangeContext;
    active.store(true);alwaysReady.store(true);alwaysEnabled.store(false);
    check(ApplyRollCommand("") && alwaysEnabled.load());
    check(ApplyRollCommand("off") && !alwaysEnabled.load());
    check(ApplyRollCommand("status") && !alwaysEnabled.load());
    check(!ApplyRollCommand("invalid") && !alwaysEnabled.load());
    auto reads=rangeReads,writes=rangeWrites;
    RollChatCommand("status",nullptr);RollChatCommand("invalid",nullptr);
    check(rangeReads==reads && rangeWrites==writes);
    RollChatCommand("on",nullptr);check(alwaysEnabled.load());
    LoadDisplaySettings(&rangeContext);check(alwaysConfigured);
    check(rangeConfig.starts_with("# keep\r\n") && rangeConfig.find("# startup")!=rangeConfig.npos && rangeConfig.ends_with("[other]\r\nkeep = 8\r\n"));
    const auto retained=rangeConfig;rangeWriteFail=true;
    RollChatCommand("off",nullptr);check(alwaysEnabled.load() && rangeConfig==retained);
    rangeWriteFail=false;rangeReadFail=true;writes=rangeWrites;
    check(!ApplyRollCommand("off") && alwaysEnabled.load() && rangeWrites==writes);
    rangeReadFail=false;rangeConfig=std::string(65537,'#');
    check(!ApplyRollCommand("off") && rangeWrites==writes);rangeConfig=retained;
    RollChatCommand("off",nullptr);LoadDisplaySettings(&rangeContext);check(!alwaysConfigured && !alwaysEnabled.load());
    RollChatCommand("toggle",nullptr);check(alwaysEnabled.load());
    alwaysReady.store(false);writes=rangeWrites;check(!ApplyRollCommand("on") && rangeWrites==writes);
    active.store(false);contributionContext=nullptr;
    {
        std::vector<unsigned char> game(0x2dc4c0);
        const unsigned char entry[]={0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
        std::memcpy(game.data()+0x2dc4b0,entry,sizeof(entry));
        check(AdmitPropertyFormatter(game.data(),game.data()+0x2dc4b0));
        check(!AdmitPropertyFormatter(game.data(),reinterpret_cast<void*>(&AdmitPropertyFormatter)));
        game[0x2dc4b0]=0xe9;
        check(!AdmitPropertyFormatter(game.data(),game.data()+0x2dc4b0));
    }
    {
        unsigned char nativeFrame[0x18f0]{};
        const unsigned contaminatedKeyboard=0xa1b2c301;
        std::memcpy(nativeFrame+0x18ec,&contaminatedKeyboard,4);
        const auto keyboard=Affixes::Read<unsigned char>(nativeFrame,0x18ec);
        check(keyboard==1 && FixedHeaderRequest(keyboard!=0,2,0));
        check(!FixedHeaderRequest(keyboard!=0,0,0));
    }
    tooltipModifiers=2;
    const auto heldKeys=std::exchange(tooltipModifiers,std::nullopt);
    check(heldKeys && FixedHeaderRequest(true,*heldKeys,0) && !tooltipModifiers);
    tooltipModifiers=0;
    const auto releasedKeys=std::exchange(tooltipModifiers,std::nullopt);
    check(releasedKeys && !FixedHeaderRequest(true,*releasedKeys,0) && !tooltipModifiers);
    TestAlwaysAdapters();
    TestLinkedProduction();
    TestRandomSkillProduction();
    TestIntrinsicReportedBase();
    TestPermanentSetProduction();
    TestPoisonDurationProduction();
    if(argc==2)TestLiveUniqueCapture(argv[1]);
    if(argc==3 && std::strcmp(argv[1],"--random-skills")==0)TestLiveRandomSkillCapture(argv[2]);
    TestTooltipVisibilitySnapshots();
    check(FixedHeaderRequest(true,2,0));check(!FixedHeaderRequest(true,0,1));
    check(FixedHeaderRequest(false,0,1));check(!FixedHeaderRequest(false,2,0));
    std::array<unsigned char,12> requestSource{};requestSource[0]=4;requestSource[8]=55;
    PropertyRequest request{requestSource.data(),55,true};
    check(MatchingPropertyRequest(request,requestSource.data()));
    request.held=false;check(!MatchingPropertyRequest(request,requestSource.data()));request.held=true;
    request.id=54;check(!MatchingPropertyRequest(request,requestSource.data()));request.id=55;
    check(!MatchingPropertyRequest(request,nullptr));
    BeginPropertyRequest(requestSource.data(),true);
    check(!TakePropertyRequest(false,false).held && pendingPropertyRequest.held);
    check(!TakePropertyRequest(true,true).held && pendingPropertyRequest.held);
    check(MatchingPropertyRequest(TakePropertyRequest(true,false),requestSource.data()));
    check(!TakePropertyRequest(true,false).held);
    BeginPropertyRequest(requestSource.data(),true);BeginPropertyRequest(requestSource.data(),false);
    check(!TakePropertyRequest(true,false).held); // Next tooltip observes release.

    TestRestoredHeaderContext();
    unsigned char sourceUnit[0x90]{},cloneUnit[0x90]{};
    const unsigned itemType=4;
    std::memcpy(sourceUnit,&itemType,4);std::memcpy(cloneUnit,&itemType,4);
    const void* nativeOverlay[2]={sourceUnit,cloneUnit};
    check(RangeFromOverlay(nativeOverlay));
    nativeOverlay[1]=nullptr;check(!RangeFromOverlay(nativeOverlay)); // Native off path.
    nativeOverlay[1]=sourceUnit;check(!RangeFromOverlay(nativeOverlay));
    nativeOverlay[1]=reinterpret_cast<void*>(1);check(!RangeFromOverlay(nativeOverlay));
    nativeOverlay[1]=cloneUnit;cloneUnit[0]=1;check(!RangeFromOverlay(nativeOverlay));
    check(!RangeFromOverlay(nullptr));
    TestCalculatedHeaders();
    // Unique group IDs are not Properties indices. Expand only validated groups.
    unsigned char group[0xc8]{}, spec[16]{};
    std::uint32_t groupId=0x10000; std::memcpy(spec,&groupId,4);
    group[4]=2;
    const int property=0,param=4,lo=1,hi=1,weight=1;
    std::memcpy(group+8,&property,4); std::memcpy(group+12,&param,4); std::memcpy(group+16,&param,4);
    std::memcpy(group+20,&lo,4); std::memcpy(group+24,&hi,4); std::memcpy(group+28,&weight,4);
    std::vector<Affixes::Rolled> expanded;
    AddUniqueSource(spec,group,1,1,expanded);
    check(expanded.size()==1 && expanded[0].row.Number(0x28)==4 && expanded[0].source);
    check(!expanded[0].uniqueScalar); // No verified exactly-one selection yet.
    std::memcpy(spec+8,&lo,4); std::memcpy(spec+12,&hi,4); expanded.clear();
    AddUniqueSource(spec,group,1,1,expanded); check(expanded.size()==1 && expanded[0].uniqueScalar);
    group[4]=0; expanded.clear(); AddUniqueSource(spec,group,1,1,expanded);
    check(expanded.size()==1 && !expanded[0].uniqueScalar); group[4]=2;
    std::memcpy(group+8,&groupId,4); expanded.clear();
    AddUniqueSource(spec,group,1,1,expanded); check(expanded.empty()); // cycle bounded
    groupId=0x10001; std::memcpy(spec,&groupId,4);
    AddUniqueSource(spec,group,1,1,expanded); check(expanded.empty()); // out of bounds
    Affixes::Row composite{};
    std::memcpy(composite.bytes.data(),"Elemental1",11);
    const int propertyZero=0, absent=-1;
    std::memcpy(composite.bytes.data()+0x24,&propertyZero,4);
    std::memcpy(composite.bytes.data()+0x34,&absent,4);
    std::memcpy(composite.bytes.data()+0x44,&absent,4);
    Affixes::Property compositeProperty{};
    const std::uint16_t fire=48,lightning=50,cold=54;
    compositeProperty.bytes[0x18]=15; std::memcpy(compositeProperty.bytes.data()+0x20,&fire,2);
    compositeProperty.bytes[0x19]=15; std::memcpy(compositeProperty.bytes.data()+0x22,&lightning,2);
    compositeProperty.bytes[0x1a]=15; std::memcpy(compositeProperty.bytes.data()+0x24,&cold,2);
    const std::vector<Affixes::Rolled> compositeRolled={{1,composite,true,2}};
    std::vector<unsigned char> compositeStats(60*0x144);
    const std::uint16_t firePriority=102,lightningPriority=99,coldPriority=96;
    std::memcpy(compositeStats.data()+48*0x144+0x30,&firePriority,2);
    std::memcpy(compositeStats.data()+50*0x144+0x30,&lightningPriority,2);
    std::memcpy(compositeStats.data()+54*0x144+0x30,&coldPriority,2);
    std::vector<CapturedLine> compositeLines;
    const bool compositeAdded=AddUnobservedCompositeIdentities(compositeLines,
        "Adds 5-11 Cold Damage\nAdds 4-9 Lightning Damage\nAdds 3-7 Fire Damage",
        compositeRolled,{&compositeProperty,1},{},{},compositeStats.data(),60);
    if (!compositeAdded) std::puts("Composite identity fixture was not admitted.");
    check(compositeAdded);
    check(compositeLines.size()==3 && compositeLines[0].stat==54 &&
          compositeLines[1].stat==50 && compositeLines[2].stat==48);
    compositeLines={{RangeText::Analyze("Adds 3-7 Fire Damage").key,48,0,false,0}};
    const bool unrelatedAdded=AddUnobservedCompositeIdentities(compositeLines,
        "Adds 3-7 Fire Damage\nUnidentified fixed line",compositeRolled,
        {&compositeProperty,1},{},{},compositeStats.data(),60);
    if (unrelatedAdded) std::puts("Observed family incorrectly claimed an unrelated line.");
    check(!unrelatedAdded);
    Affixes::Row poison{};
    std::memcpy(poison.bytes.data(),"of Blight",10);
    const int propertyOne=1;
    std::memcpy(poison.bytes.data()+0x24,&propertyOne,4);
    std::memcpy(poison.bytes.data()+0x34,&absent,4);
    std::memcpy(poison.bytes.data()+0x44,&absent,4);
    std::vector<Affixes::Property> mixedProperties(2);
    mixedProperties[0].bytes[0x18]=1; std::memcpy(mixedProperties[0].bytes.data()+0x20,&fire,2);
    const std::uint16_t poisonStat=57,poisonPriority=92;
    mixedProperties[1].bytes[0x18]=15; std::memcpy(mixedProperties[1].bytes.data()+0x20,&poisonStat,2);
    std::memcpy(compositeStats.data()+57*0x144+0x30,&poisonPriority,2);
    const std::vector<Affixes::Rolled> mixedRolled={{1,composite,true,2},{2,poison,false,3}};
    compositeLines.clear();
    check(AddUnobservedCompositeIdentities(compositeLines,
        "+9 Poison Damage over 3 seconds\nAdds 3-7 Fire Damage",mixedRolled,mixedProperties,
        {},{},compositeStats.data(),60));
    check(compositeLines.size()==2 && compositeLines[0].stat==57 && compositeLines[1].stat==48);
    TestPropertyPasses();
    TestAffixRead();
    TestOwnedContributionAdapter();
    TestRangeHelper();
    TestSpecial();
    TestUniqueRange();
    TestHeaderDelegation();
    testSlot=reinterpret_cast<void*>(&FakeKey);
    check(Exchange(&testSlot,reinterpret_cast<void*>(&FakeKey),reinterpret_cast<void*>(&OtherKey)));
    check(!Exchange(&testSlot,reinterpret_cast<void*>(&FakeKey),reinterpret_cast<void*>(&OtherKey)));
    check(testSlot==reinterpret_cast<void*>(&OtherKey));
    check(Exchange(&testSlot,reinterpret_cast<void*>(&OtherKey),reinterpret_cast<void*>(&FakeKey)));
    check(testSlot==reinterpret_cast<void*>(&FakeKey));
    check(!D2RLoaderLoadPlugin(nullptr));
    auto info=D2RLoaderGetPluginInfo();
    check(std::strcmp(info->id,"item-roll-ranges")==0);
    check(std::strcmp(info->version,"1.3.1+rev.55")==0);
    std::puts("Passed real adapter passthrough/ABI, inactive late calls, atomic slot conflict/restore, exported identity.");
}

namespace {
NativeText headerFixture{"Defense: 2",10};
NativeText* __fastcall FakeBuilder(void* a,void* b,void* c,void* item,int flags,
    unsigned char d,unsigned char e,void* f,void* g) {
    check(a==reinterpret_cast<void*>(1) && b==reinterpret_cast<void*>(2) &&
        c==reinterpret_cast<void*>(3) && item==reinterpret_cast<void*>(4) && flags==5 &&
        d==6 && e==7 && f==reinterpret_cast<void*>(8) && g==reinterpret_cast<void*>(9));
    return &headerFixture;
}
void* __fastcall FakeAssign(NativeText* text,const char* data,std::size_t size) {
    check(text==&headerFixture && data==headerFixture.data && size==headerFixture.size);
    return reinterpret_cast<void*>(0x1234);
}
std::uint64_t __fastcall FakeDamage(unsigned char mode,void* item,char* output,void* context) {
    check(mode==7 && item==reinterpret_cast<void*>(1) && output==reinterpret_cast<char*>(2) && context==reinterpret_cast<void*>(3));
    return 0xfedcba9876543210ull;
}
const char* __fastcall FakeHeaderLookup(const NativeText*,const NativeText*,bool);
void TestHeaderDelegation() {
    originalDamage=FakeDamage; damageReturn=0;
    check(DamageAdapter(7,reinterpret_cast<void*>(1),reinterpret_cast<char*>(2),reinterpret_cast<void*>(3))==0xfedcba9876543210ull);
    lookupHeader=FakeHeaderLookup;
    check(DefenseHeaderKey()==RangeText::Analyze("Defense: 2").key);
    originalBuilder=FakeBuilder; builderReturn=0;
    check(reinterpret_cast<BuilderFn>(&HeaderBuilderEntry)(reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),
        reinterpret_cast<void*>(3),reinterpret_cast<void*>(4),5,6,7,
        reinterpret_cast<void*>(8),reinterpret_cast<void*>(9))==&headerFixture);
    originalAssign=FakeAssign; assignReturn=0;
    check(HeaderAssignAdapter(&headerFixture,headerFixture.data,headerFixture.size)==reinterpret_cast<void*>(0x1234));
    std::puts("Verified all nine native tooltip arguments and unrelated header publication delegation.");
}
const char* __fastcall FakeHeaderLookup(const NativeText* resource,const NativeText* key,bool required) {
    check(std::string_view(resource->data,resource->size)=="d2r");
    check(std::string_view(key->data,key->size)=="ItemStats1h" && required);
    return "Defense: %d";
}
unsigned char fakeTables[0x1870]{};
const unsigned char* __fastcall FakeTables(unsigned char bank) { check(bank==2); return fakeTables; }
int __fastcall FakeEligible(void*,const void*) { return 1; }
template<class T> void put(unsigned char* p,std::size_t at,T value) { std::memcpy(p+at,&value,sizeof(value)); }
void TestCalculatedHeaders() {
    unsigned char unit[0x1be]{},data[0x80]{},stats[0xb8]{},row[0x1c0]{},primary[32]{},child[0x80]{},modifiers[16]{};
    put(unit,0x88,stats+0); put(stats,0x1c,-1); put(stats,0x30,primary+0); put(stats,0x38,std::uint64_t{1});
    put(primary,0,std::uint64_t{31}<<32); put(primary,8,2); put(row,0xd4,2); put(row,0xd8,2);
    auto base=ArmorBaseRange(unit,data,row); check(base && *base==std::pair{2,2}); // Bloodrune, actual25.
    put(primary,8,3); put(stats,0x90,child+0); put(child,0,unit+0); put(child,0x78,stats+0);
    put(child,0x18,-1); put(child,0x1c,0x40); put(child,0x30,modifiers+0); put(child,0x38,std::uint64_t{1});
    put(modifiers,0,std::uint64_t{16}<<32); put(modifiers,8,18);
    base=ArmorBaseRange(unit,data,row); check(base && *base==std::pair{3,3}); // Generated Sturdy base.
    put(data,0x18,0x400010u); put(primary,8,4);
    base=ArmorBaseRange(unit,data,row); check(base && *base==std::pair{4,4}); // Ethereal Sturdy, actual5.
    put(child,0x68,child+0); check(!ArmorBaseRange(unit,data,row)); // Cyclic ED provenance rejected.
    put(child,0x68,static_cast<unsigned char*>(nullptr)); put(child,0x20,165); check(!ArmorBaseRange(unit,data,row));
    put(child,0x20,0); put(data,0x18,0x10u); put(row,0xd4,29); put(row,0xd8,34); put(primary,8,30);
    base=ArmorBaseRange(unit,data,row); check(base && *base==std::pair{29,34}); // Variable intrinsic armor rolls.
    put(primary,8,40); check(!ArmorBaseRange(unit,data,row)); // No guessed custom base.
    put(row,0xd4,111); put(row,0xd8,126); put(primary,8,121); put(data,0,2u);
    base=ArmorBaseRange(unit,data,row); check(base && *base==std::pair{111,126});
    put(data,0,3u); put(primary,8,118); put(modifiers,0,std::uint64_t{75}<<32); put(modifiers,8,11);
    base=ArmorBaseRange(unit,data,row); check(base && *base==std::pair{111,126}); // Superior durability only.
    put(primary,8,127); put(modifiers,0,std::uint64_t{16}<<32); put(modifiers,8,8);
    base=ArmorBaseRange(unit,data,row); check(base && *base==std::pair{127,127}); // Superior ED.
    put(data,0,1u); put(primary,8,87); put(child,0x38,std::uint64_t{0});
    base=ArmorBaseRange(unit,data,row); check(base && *base==std::pair{83,94}); // Live crude control.
    put(primary,8,83); check(ArmorBaseRange(unit,data,row)==std::pair{83,94});
    put(primary,8,94); check(ArmorBaseRange(unit,data,row)==std::pair{83,94});
    put(primary,8,95); check(!ArmorBaseRange(unit,data,row));
    put(primary,8,121); check(!ArmorBaseRange(unit,data,row)); // Do not fall through to normal range.
    put(primary,8,87); put(data,0x18,0x400010u); check(!ArmorBaseRange(unit,data,row));
    put(data,0x18,0x10u); put(row,0xd4,1); put(row,0xd8,2); put(primary,8,0);
    check(ArmorBaseRange(unit,data,row)==std::pair{0,1}); // Armor has no weapon minimum-one clamp.
    check(!BaseRanges::LowArmor(-1)); check(BaseRanges::LowArmor(INT_MAX)==1610612735);
    put(data,0,2u);
    row[0x113]=9; row[0x114]=19; put(primary,0,std::uint64_t{23}<<32); put(primary,8,9);
    put(primary,16,std::uint64_t{24}<<32); put(primary,24,19); put(stats,0x38,std::uint64_t{2});
    base=WeaponBaseRange(unit,data,row,23,24,0x113,0x114); check(base && *base==std::pair{9,19});
    put(data,0,1u); put(primary,8,6); put(primary,24,14);
    base=WeaponBaseRange(unit,data,row,23,24,0x113,0x114); check(base && *base==std::pair{6,14});
    put(primary,24,15); check(!WeaponBaseRange(unit,data,row,23,24,0x113,0x114));
    put(primary,24,14); put(data,0x18,0x400010u); check(!WeaponBaseRange(unit,data,row,23,24,0x113,0x114));
    put(data,0x18,0x10u); row[0x113]=1; row[0x114]=5; put(primary,8,1); put(primary,24,3);
    check(WeaponBaseRange(unit,data,row,23,24,0x113,0x114)==std::pair{1,3});
    // Live Long Sword controls use one-hand stats21/22. Superior ED stays in own lists.
    row[0x10f]=3; row[0x110]=19; put(primary,0,std::uint64_t{21}<<32); put(primary,16,std::uint64_t{22}<<32);
    put(primary,8,2); put(primary,24,14);
    check(WeaponBaseRange(unit,data,row,21,22,0x10f,0x110)==std::pair{2,14});
    put(data,0,3u); put(primary,8,3); put(primary,24,19);
    check(WeaponBaseRange(unit,data,row,21,22,0x10f,0x110)==std::pair{3,19});
    check(!BaseRanges::LowWeapon(-1)); check(BaseRanges::LowWeapon(0)==1);
    row[0x113]=9; row[0x114]=19; put(primary,0,std::uint64_t{23}<<32); put(primary,16,std::uint64_t{24}<<32);
    put(primary,8,9); put(primary,24,19);
    // Base provenance is independent of proc, per-level, socket or ED sources.
    put(child,0x20,165); put(data,0,7);
    base=WeaponBaseRange(unit,data,row,23,24,0x113,0x114); check(base && *base==std::pair{9,19});
    put(primary,24,20); check(!WeaponBaseRange(unit,data,row,23,24,0x113,0x114));
    put(primary,8,13); put(primary,24,28); put(data,0x18,0x400010u);
    base=WeaponBaseRange(unit,data,row,23,24,0x113,0x114); check(base && *base==std::pair{13,28});
    std::puts("Verified intrinsic armor/weapon base ranges, ED generation, ethereal staging and unrelated-source independence.");
}
unsigned noCloneCalls{};
std::uint64_t __fastcall FakeNoCloneProperties(void*,char* output,int cap,int,int,int,int,int,const void*,void*) {
    ++noCloneCalls;
    strcpy_s(output,static_cast<std::size_t>(cap),"+118 Defense");
    if (captureLines) captureLines->push_back({RangeText::Analyze(output).key,31,0,false,0});
    return 0xfedcba9876543210ull;
}
void TestAffixRead() {
    unsigned char unit[0x200]{}, allocation[0x100]{};
    auto* data=allocation+28;
    put(unit,0,4u); put(unit,0x10,data); unit[0x1bd]=2;
    put(data,0,4); put(data,0x40,static_cast<std::uint16_t>(100)); put(allocation,4,1u);
    Affixes::Row rows[2]{};
    for (unsigned i=0;i<2;++i) {
        auto* r=rows[i].bytes.data(); r[0x54]=r[0x64]=r[0x82]=1;
        put(r,0x34,-1); put(r,0x44,-1); put(r,0x58,static_cast<int>(i*10+1)); put(r,0x5c,1);
    }
    Affixes::Property props[1]{}; props[0].bytes[0x18]=1; props[0].bytes[0x20]=31;
    std::vector<unsigned char> itemStats(60*0x144);
    const std::uint16_t poisonPriority=92;
    std::memcpy(itemStats.data()+57*0x144+0x30,&poisonPriority,2);
    put(fakeTables,0x15e8,rows+0); put(fakeTables,0x15f0,std::uint64_t{2});
    put(fakeTables,0x1600,rows+0); put(fakeTables,0x1608,rows+2); put(fakeTables,0x1610,rows+2);
    put(fakeTables,0x240,props+0); put(fakeTables,0x248,std::uint64_t{1});
    put(fakeTables,0x1258,itemStats.data()); put(fakeTables,0x1260,std::uint64_t{60});
    getTables=FakeTables; eligibleAffix=FakeEligible;
    const std::vector<CapturedLine> lines={{RangeText::Analyze("+118 Defense").key,31,0,false}};
    auto labels=AffixLabels(unit,lines);
    check(labels.size()==1 && labels[0].text=="[S] [T2]");
    std::memcpy(rows[0].bytes.data(),"of Blight",10);
    props[0].bytes[0x18]=15; props[0].bytes[0x20]=57;
    labels=AffixLabels(unit,{},"+7 Weapon Poison Damage over 3 seconds");
    check(labels.size()==1 && labels[0].text=="[S] [of Blight] [T2]" &&
          labels[0].key==RangeText::Analyze("+7 Weapon Poison Damage over 3 seconds").key);
    check(AffixLabels(unit,{},"Unidentified fixed line\n+7 Weapon Poison Damage over 3 seconds").size()==1);
    check(AffixLabels(unit,{},"+50% Damage to Undead\n+7 Weapon Poison Damage over 3 seconds").size()==1);
    // An extra two-number candidate is still ambiguous and must refuse.
    check(AffixLabels(unit,{},"5% Chance to cast level 10 Nova\n+7 Weapon Poison Damage over 3 seconds").empty());
    props[0].bytes[0x18]=1; props[0].bytes[0x20]=31;
    put(data,0,7); check(AffixLabels(unit,lines).empty()); // No unique definition yet.
    // Renewed Sunder regression: flag8 and no affix IDs still have unique sources.
    unsigned char unique[0x15c]{}, groups[0xc8]{};
    unique[0x2c]=8; groups[4]=2;
    for (unsigned n=0;n<12;++n) put(unique,0x98+n*16,-1);
    put(unique,0x98,0x10000); put(groups,8,0); put(groups,28,1);
    put(data,0x34,0);
    put(fakeTables,0x13c8,unique+0); put(fakeTables,0x13d0,std::uint64_t{1});
    put(fakeTables,0x258,groups+0); put(fakeTables,0x260,std::uint64_t{1});
    labels=AffixLabels(unit,lines);
    check(labels.size()==1 && labels[0].text=="[U]");
    originalProperties=FakeNoCloneProperties;
    char noCloneOutput[256]{};
    check(RenderProperties(unit,unit,noCloneOutput,256,1,0,0,0,0,nullptr,nullptr)==0xfedcba9876543210ull);
    check(noCloneCalls==1);
    check(std::strstr(noCloneOutput,"+118 Defense") && std::strstr(noCloneOutput,"[U]"));
    put(data,0,4); auto grouped=lines; grouped[0].grouped=true;
    check(AffixLabels(unit,grouped).empty());
    put(fakeTables,0x1608,rows+3); check(AffixLabels(unit,lines).empty());
    getTables=nullptr; eligibleAffix=nullptr;
    std::puts("Verified native source metadata layout, tier read, quality/group exclusions and malformed partition fallback.");
}
unsigned propertyCalls{};
void* sourceExpected{}; void* cloneExpected{}; const void* defsExpected{}; void* countExpected{};
std::uint64_t __fastcall FakeProperties(void* item,char* output,int cap,int mode,int s1,int s2,int flags,int extra,const void* defs,void* count) {
    check(cap==1024 && mode==1 && s1==2 && s2==3 && flags==4 && extra==5);
    ++propertyCalls;
    if (item==sourceExpected) {
        check(!defs && count && !static_cast<const unsigned char*>(count)[8]);
        std::snprintf(output,cap,"+118 Defense\n+38%% Enhanced Weapon Damage\n");
    } else {
        check(item==cloneExpected && defs==defsExpected && count==countExpected);
        std::snprintf(output,cap,"+\xff" "cU(80-120)\xff" "c3 Defense\n+\xff" "cU(25-50)\xff" "c3%% Enhanced Weapon Damage\n");
        check(captureLines!=nullptr); // Actual-only capture caused the 0.4.3 ED regression.
        char damage[256]="+\xff" "cU(25-50)\xff" "c3% Enhanced Weapon Damage";
        CaptureRangeIdentity(17,0,damage);
    }
    return 0x12345678;
}
std::uint64_t __fastcall AppendProperties(void*,char* output,int cap,int,int,int,int,int,const void*,void*) {
    const auto used=std::strlen(output);
    std::snprintf(output+used,cap-used,"+1 to All Skills\n");
    CaptureRangeIdentity(127,0,"+1 to All Skills");
    return 42;
}
void TestPropertyPasses() {
    unsigned char source[0x90]{},clone[0x90]{};
    void* overlay[]={source,clone};
    check(SourceFromOverlay(overlay,clone)==source);
    check(!SourceFromOverlay(overlay,source));
    check(!SourceFromOverlay(nullptr,clone));
    void* noClone[]={source,nullptr};
    check(SourceFromOverlay(noClone,source)==source);
    check(!SourceFromOverlay(noClone,clone));
    void* badOverlay[]={nullptr,clone}; check(!SourceFromOverlay(badOverlay,clone));
    sourceExpected=source; cloneExpected=clone; defsExpected=reinterpret_cast<void*>(0x1234); countExpected=reinterpret_cast<void*>(0x5678);
    originalProperties=FakeProperties;
    char output[1024]{};
    check(RenderProperties(source,clone,output,1024,1,2,3,4,5,defsExpected,countExpected)==0x12345678);
    check(propertyCalls==2 && !rendering);
    check(std::strstr(output,"[+80 - +120]\xff" "c3 +118 Defense"));
    check(std::strstr(output,"[+25 - +50]\xff" "c3 +38% Enhanced Weapon Damage"));
    originalProperties=AppendProperties;
    const std::string prefix="\xff" "cU[+1]\xff" "c3 +1 to All Skills \xff" "cU[Set]\xff" "c3\n";
    strcpy_s(output,prefix.c_str());
    for(unsigned n=0;n<6;++n){
        const std::string before=output;
        check(RenderProperties(source,source,output,1024,0,165,0,0,0,nullptr,nullptr)==42);
        check(std::string_view(output).starts_with(before));
        check(std::string(output)==before+"+1 to All Skills\n");
    }
    std::puts("Verified original source pass, cleared range args, clone range pass, returned ABI, exact actual values and overlay identity.");
}
int __fastcall FakeRangeHelper(void* unit,void* stats,int stat,int layer,int low,int high,char* output,int mode) {
    check(unit==reinterpret_cast<void*>(0x1234) && stats==reinterpret_cast<void*>(0x5678));
    check(stat==17 && layer==0 && low==75 && high==75 && mode==1);
    strcpy_s(output,256,"+75% Enhanced Damage");
    return 0x12345678;
}
void TestRangeHelper() {
    originalRangeHelper=FakeRangeHelper;
    rangeHelperReturn=rangeHelperActualReturn=0;
    std::vector<CapturedLine> captured;
    captureLines=&captured;
    char output[256]{};
    check(RangeHelperAdapter(reinterpret_cast<void*>(0x1234),reinterpret_cast<void*>(0x5678),17,0,75,75,output,1)==0x12345678);
    check(captured.empty()); // Unrelated call must remain a pure passthrough.
    CaptureRangeIdentity(17,0,output);
    check(captured.size()==1 && captured[0].stat==17 && !captured[0].grouped);
    Affixes::Row row{}; put(row.bytes.data(),0x24,0); put(row.bytes.data(),0x34,-1); put(row.bytes.data(),0x44,-1);
    Affixes::Property property{}; property.bytes[0x18]=7;
    const std::vector<Affixes::Rolled> rolled={{1,row,true,3}};
    const std::vector<RangeText::Label> labels={{captured[0].key,Affixes::Label(rolled,{&property,1},17,0)}};
    auto merged=RangeText::Merge(output,"+75% Enhanced Damage",256,labels);
    check(merged.labeled==1 && merged.text.find("[P] [T3]")!=std::string::npos);
    CaptureRangeIdentity(54,0,output);
    check(captured.size()==2 && captured[1].stat==54);
    captureLines=nullptr;
    check(RangeHelperAdapter(reinterpret_cast<void*>(0x1234),reinterpret_cast<void*>(0x5678),17,0,75,75,output,1)==0x12345678);
    std::puts("Verified paired-helper ABI, unrelated/late calls, generic damage provenance and original values.");
}
unsigned specialCalls{}; bool emitSpecial=true;
unsigned uniqueEndpointCalls{};
bool __fastcall FakeUniqueEndpoint(void* source,const void*,int value,int layer,bool grouped,char* output,int mode) {
    check(source==reinterpret_cast<void*>(0x1234) && (value==5 || value==10) && !layer && !grouped && mode==1);
    ++uniqueEndpointCalls;
    std::snprintf(output,256,"+%d%% Faster Run/Walk",value);
    return true;
}
void TestUniqueRange() {
    originalSingle=FakeUniqueEndpoint;
    unsigned char tables[0x1618]{};
    std::vector<unsigned char> stats(97*0x144);
    stats[96*0x144+0x32]=19;
    put(tables,0x1258,stats.data()); put(tables,0x1260,std::uint64_t{97});
    Affixes::Property property{}; property.bytes[0x18]=8; property.bytes[0x20]=96;
    Affixes::Row row{}; put(row.bytes.data(),0x34,-1); put(row.bytes.data(),0x44,-1);
    put(row.bytes.data(),0x2c,5); put(row.bytes.data(),0x30,10);
    std::vector<Affixes::Rolled> sources={{0,row,false,0,"[U]",true}};
    const CapturedLine line{RangeText::Analyze("+6% Faster Run/Walk").key,96,0,false,0};
    auto source=reinterpret_cast<void*>(0x1234);
    check(UniqueRange(source,line,sources,{&property,1},tables,"+6% Faster Run/Walk")=="5% - 10%");
    check(uniqueEndpointCalls==2);
    sources[0].uniqueScalar=false;
    check(UniqueRange(source,line,sources,{&property,1},tables,"+6% Faster Run/Walk").empty());
    sources[0].uniqueScalar=true; sources.push_back(sources[0]);
    check(UniqueRange(source,line,sources,{&property,1},tables,"+6% Faster Run/Walk").empty());
    check(uniqueEndpointCalls==2);
    originalSingle=nullptr;
    std::puts("Verified unique source bounds use native endpoint formatter; unsupported and overlapping sources are excluded.");
}
int __fastcall FakeSpecial(void* context,int stat,char* output,std::size_t capacity,unsigned char flags) {
    check(context==reinterpret_cast<void*>(0xabcd) && stat==18 && capacity==1024 && flags==3);
    ++specialCalls;
    if (emitSpecial) strcat_s(output,capacity,"+75% Enhanced Damage\n");
    return 7; // Preserve the full int, not a converted bool.
}
void TestSpecial() {
    originalSpecial=FakeSpecial;
    std::vector<CapturedLine> captured;
    captureLines=&captured;
    char output[1024]="+1 to Strength\n";
    check(ObserveSpecial(true,reinterpret_cast<void*>(0xabcd),18,output,1024,3)==7);
    check(specialCalls==1 && captured.size()==1 && captured[0].stat==18);
    check(captured[0].key==RangeText::Analyze("+75% Enhanced Damage").key);
    const std::vector<RangeText::Label> labels={{captured[0].key,"[P] [T3]",{{66,80,"[P] [T3]"}}}};
    const auto merged=RangeText::Merge(output,"+1 to Strength\n+\xee\x81\xbe" "U(66-80)\xee\x81\xbe" "3% Enhanced Damage\n",1024,labels);
    check(merged.text.find("[66%-80%]")!=std::string::npos && merged.text.find("+75% Enhanced Damage")!=std::string::npos);
    check(merged.text.find("[P] [T3]")!=std::string::npos);
    emitSpecial=false;
    check(ObserveSpecial(true,reinterpret_cast<void*>(0xabcd),18,output,1024,3)==7);
    check(specialCalls==2 && captured.size()==1); // Handled with no appended text.
    CaptureSpecialDamage(18,"line one\nline two\n"); check(captured.size()==1);
    CaptureSpecialDamage(39,"+75% Enhanced Damage\n"); check(captured.size()==1);
    emitSpecial=true; specialReturn=0;
    check(SpecialAdapter(reinterpret_cast<void*>(0xabcd),18,output,1024,3)==7);
    check(captured.size()==1 && specialCalls==3); // Unrelated caller.
    captureLines=nullptr;
    check(ObserveSpecial(true,reinterpret_cast<void*>(0xabcd),18,output,1024,3)==7);
    check(specialCalls==4); // Late passthrough without active capture.
    std::puts("Verified special-formatter ABI, append-only ED capture, no-output/multiline exclusions and combined-line labeling.");
}

}

#include "contribution_adapter_tests.h"


#include "definition_catalog_tests.h"
#include "reported_capture_tests.h"

#include "intrinsic_base_tests.h"

#include "set_property_tests.h"
