#include "../src/contribution_model.h"
#include "../src/contribution_definitions.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <fstream>
#include <filesystem>
#include <source_location>
namespace {
void check(bool ok,const std::source_location location=std::source_location::current()){
    if(!ok){std::fprintf(stderr,"Contribution check failed at line %u\n",location.line());std::exit(1);}
}
template<class T> void put(unsigned char* p,unsigned at,T value){std::memcpy(p+at,&value,sizeof(value));}
Contributions::Stats stats(unsigned stat,std::int64_t value){return {{std::uint64_t{stat}<<32,value}};}
Affixes::Rolled roll(unsigned property,int low,int high,const char* name=nullptr){
    return ContributionDefinitions::Rolled({property,0,low,high},name);
}
Affixes::Property property(unsigned fn,unsigned stat){Affixes::Property p{};p.bytes[0x18]=static_cast<unsigned char>(fn);put(p.bytes.data(),0x20,static_cast<std::uint16_t>(stat));return p;}
void TestSamples(){
    std::vector<Affixes::Property> p={property(8,93),property(1,7),property(6,0xffff),property(7,0xffff),property(1,149),property(1,48),property(1,49)};
    auto run=[&](unsigned stat,unsigned shift,std::vector<ContributionModel::Group> groups,const char* actual,const char* expected){
        auto label=ContributionModel::Build(RangeText::Analyze(actual).key,stat,shift,groups,p);check(label.has_value());
        const auto result=RangeText::Merge(actual,actual,4096,{*label});check(result.text.find(expected)!=std::string::npos);return *label;};
    auto ias=run(93,0,{{stats(93,20),{roll(0,20,20,"[S] [of Alacrity]")},""},{stats(93,15),{roll(0,15,15,"[S] [of Fervor]")},"[Socket 1] "}},
        "+35% Increased Attack Speed","+15% Increased Attack Speed [Socket 1]");
    check(RangeText::ExpandOwned("+36% Increased Attack Speed",ias).empty());
    ias.ownedParts[1].value=14;check(RangeText::ExpandOwned("+35% Increased Attack Speed",ias).empty());
    const auto life=run(7,8,{{stats(7,3328),{roll(1,11,20,"[S] [of the Wolf]")},""},{stats(7,4096),{roll(1,9,20,"[S] [of Hope]")},"[Socket 1] "}},
        "+29 to Life","[+20 - +40]");
    check(RangeText::ExpandOwned("+29 to Life",life).find("+13 to Life")!=std::string::npos);
    run(7,8,{{{}, {},""},{stats(7,38*256),{roll(1,38,38,"[Perfect Ruby]")},"[Socket 2] "}},
        "+38 to Life","[+38]");
    auto malformed=stats(7,4097);check(!ContributionModel::Build("life",7,8,std::vector<ContributionModel::Group>{{malformed,{roll(1,9,20,"[S]")},""}},p));
    run(22,0,{{stats(22,96),{roll(2,81,100,"[S] [of Evisceration]")},""},{stats(22,11),{roll(2,10,15,"[S] [of Carnage]")},"[Socket 2] "}},
        "+107 to Maximum Weapon Damage","[+91 - +115]");
    run(17,0,{{stats(17,249),{roll(3,234,266,"[P] [Cruel]")},""},{stats(17,38),{roll(3,31,40,"[P] [Ruby]")},"[Socket 1] "}},
        "+287% Enhanced Weapon Damage","[+265% - +306%]");
    const auto corruption=run(17,0,{{stats(17,291),{roll(3,267,300,"[P] [Cruel]")},""},{stats(17,50),{roll(3,50,50,"[C]")},""}},
        "+341% Enhanced Weapon Damage","[+317% - +350%]");
    check(RangeText::ExpandOwned("+341% Enhanced Weapon Damage",corruption).find("[+50%]")!=std::string::npos);
    const auto cold=run(149,0,{{{}, {},""},{stats(149,7),{roll(4,4,7,"[S] [of Desire]")},"[Socket 1] "}},
        "+7 Cold Absorb","[Socket 1] [S] [of Desire]");
    check(RangeText::Merge("+7 Cold Absorb","+7 Cold Absorb",16,{cold}).text=="+7 Cold Absorb");
    check(!ContributionModel::Build("cold",149,0,std::vector<ContributionModel::Group>{{stats(149,1),{},""},{stats(149,7),{roll(4,4,7,"[S]")},""}},p));
    Affixes::Rolled gem=roll(5,15,15,"[Perfect Ruby]");put(gem.row.bytes.data(),0x34,6);put(gem.row.bytes.data(),0x38,0);put(gem.row.bytes.data(),0x3c,20);put(gem.row.bytes.data(),0x40,20);
    Contributions::Stats fire={{std::uint64_t{48}<<32,15},{std::uint64_t{49}<<32,20}};
    const char* actual="Adds 15-20 Weapon Fire Damage";
    auto pair=ContributionModel::Build(RangeText::Analyze(actual).key,48,0,std::vector<ContributionModel::Group>{{{},{},""},{fire,{gem},"[Socket 1] "}},p,true);
    check(pair.has_value());check(RangeText::ExpandOwned(actual,*pair).find("[Socket 1] [Perfect Ruby]")!=std::string::npos);
    check(RangeText::ExpandOwned(actual,*pair).find("[15 / 20]")!=std::string::npos);
    check(RangeText::ExpandOwned("Adds 15-21 Weapon Fire Damage",*pair).empty());
    check(RangeText::ExpandOwned("Adds 15-20 Weapon Fire Damage for 3 seconds",*pair).empty());
    check(!ContributionModel::Build("poison",57,0,std::vector<ContributionModel::Group>{{fire,{gem},""}},p,true));
    auto uncertainUnique=roll(1,9,20,"[U]");
    check(!ContributionModel::Build("life",7,8,std::vector<ContributionModel::Group>{{stats(7,4096),{uncertainUnique},""}},p));
    uncertainUnique.uniqueScalar=true;
    check(ContributionModel::Build("life",7,8,std::vector<ContributionModel::Group>{{stats(7,4096),{uncertainUnique},""}},p).has_value());
    auto pairedPhysical=p;pairedPhysical[2].bytes[0x19]=5;
    check(!ContributionModel::Build("max",22,0,std::vector<ContributionModel::Group>{{stats(22,11),{roll(2,10,15)},""}},pairedPhysical));
}

void TestSeparateEndpointSources(){
    for(unsigned minimum:{48u,50u,52u,54u})for(const char* source:{"[U]","[Set]","[RW]"}){
        std::vector<Affixes::Property> props={property(1,minimum),property(1,minimum+1)};
        auto low=roll(0,10,12,source),high=roll(1,15,25,source);low.uniqueScalar=high.uniqueScalar=true;
        Contributions::Stats values={{std::uint64_t{minimum}<<32,12},{std::uint64_t{minimum+1}<<32,15}};
        const std::string actual="Adds 12-15 Element Damage";
        std::vector<ContributionModel::Group> groups={{values,{low,high},""}};
        auto pair=ContributionModel::Build(RangeText::Analyze(actual).key,minimum,0,groups,props,true);
        check(pair && pair->ownedParts.size()==1);
        check(RangeText::ExpandOwned(actual,*pair).find("[10 - 12 / 15 - 25]")!=std::string::npos);
        groups[0].values[1].value=26;check(!ContributionModel::Build("pair",minimum,0,groups,props,true));
        groups[0].values=values;groups[0].definitions[0].unconditionalScalar=false;
        check(!ContributionModel::Build("pair",minimum,0,groups,props,true));
        groups[0].definitions[0].unconditionalScalar=true;props[0].bytes[2]=1;
        check(!ContributionModel::Build("pair",minimum,0,groups,props,true));
        props[0].bytes[2]=0;props[0].bytes[0x18]=3;
        check(!ContributionModel::Build("pair",minimum,0,groups,props,true));
    }
}

void TestDefinitions(){
    std::array<unsigned char,448*3> items{};std::array<unsigned char,232*4> types{};
    put(items.data(),0x12e,std::int16_t{2});put(items.data(),0x130,std::int16_t{-1});put(items.data()+448,0x80,ContributionDefinitions::Code("ka3 "));
    put(types.data(),0,ContributionDefinitions::Code("armo"));put(types.data()+232,0,ContributionDefinitions::Code("weap"));
    put(types.data()+232*2,0,ContributionDefinitions::Code("swor"));put(types.data()+232*2,4,std::int16_t{1});
    put(types.data()+232*3,0,ContributionDefinitions::Code("shld"));
    const ContributionDefinitions::Table itemView{items.data(),3,448},typeView{types.data(),4,232};
    check(ContributionDefinitions::Context(itemView,typeView,0)==0u);
    put(types.data()+232,4,std::int16_t{2});check(!ContributionDefinitions::Context(itemView,typeView,0));
    put(types.data()+232,4,std::int16_t{0});
    // Shared ancestry on two branches is valid, unlike a recursive parent.
    put(items.data(),0x130,std::int16_t{1});check(ContributionDefinitions::Context(itemView,typeView,0)==0u);
    put(items.data(),0x130,std::int16_t{-1});
    std::array<unsigned char,192> gems{};put(gems.data(),0x28,625u);
    for(unsigned context=0;context<3;++context)for(unsigned n=0;n<3;++n)put(gems.data(),0x30+context*0x30+n*16,UINT32_MAX);
    put(gems.data(),0x30,15u);put(gems.data(),0x38,50);put(gems.data(),0x3c,50);
    put(gems.data(),0x60,0u);put(gems.data(),0x68,15);put(gems.data(),0x6c,15);
    auto w=ContributionDefinitions::Gem({gems.data(),1,192},625,0),a=ContributionDefinitions::Gem({gems.data(),1,192},625,1);
    check(w&&a&&w->at(0).low==50&&a->at(0).low==15);check(!ContributionDefinitions::Gem({gems.data(),1,191},625,0));
    std::array<unsigned char,364*2> cube{};
    for(unsigned n=0;n<2;++n){auto* r=cube.data()+364*n;r[0]=1;r[2]=0xff;r[3]=15;r[12]=2;put(r,4,362u);put(r,8,n?21:65);
        put(r,0x14,std::uint16_t{2});put(r,0x16,1u);put(r,0x1e,std::uint16_t{1});put(r,0x20,1u);r[0x67]=0xfe;
        for(unsigned m=0;m<5;++m)put(r,0x77+m*12,UINT32_MAX);
        put(r,0x77+24,374u);put(r,0x77+30,std::int16_t{1});put(r,0x77+32,std::int16_t{1});
        put(r,0x77+36,375u);put(r,0x77+42,std::int16_t{101});put(r,0x77+44,std::int16_t{101});}
    put(cube.data(),0x77,29u);put(cube.data(),0x7d,std::int16_t{50});put(cube.data(),0x7f,std::int16_t{50});cube[0x81]=100;
    put(cube.data(),0x83,66u);put(cube.data(),0x89,std::int16_t{8});put(cube.data(),0x8b,std::int16_t{8});cube[0x8d]=100;
    auto resolve=[&](int dummy){return ContributionDefinitions::ResolveCorruption({cube.data(),2,364},itemView,typeView,0,4,2,dummy,374,375);};
    auto positive=resolve(168);check(positive&&positive->recognized&&!positive->conversion&&positive->specs.size()==2&&positive->specs[0].low==50);
    auto nothing=resolve(140);check(nothing&&nothing->recognized&&nothing->specs.empty());
    auto converted=resolve(101);check(converted&&converted->conversion&&converted->specs.empty());
    check(!ContributionDefinitions::ResolveCorruption({cube.data(),2,364},itemView,typeView,0,4,4,168,374,375));
    check(!resolve(67));check(!resolve(202));cube[0x81]=99;check(!resolve(168));cube[0x81]=100;
    cube[0x1c]=2;check(!resolve(168));
}
struct Memory {
    std::map<std::uintptr_t,unsigned char> bytes;
    template<class T>void Put(std::uintptr_t at,T value){const auto* p=reinterpret_cast<const unsigned char*>(&value);for(std::size_t n=0;n<sizeof(value);++n)bytes[at+n]=p[n];}
    void Zero(std::uintptr_t at,std::size_t length){for(std::size_t n=0;n<length;++n)bytes[at+n]=0;}
    bool Read(std::uintptr_t at,void* out,std::size_t length)const {auto* p=static_cast<unsigned char*>(out);for(std::size_t n=0;n<length;++n){const auto i=bytes.find(at+n);if(i==bytes.end())return false;p[n]=i->second;}return true;}
};
void TestCapture(){
    Contributions::Stats bounded;check(!Contributions::Add(bounded,{{0,std::int64_t{INT32_MAX}+1}}));
    Memory m;constexpr std::uintptr_t host=0x10000,root=0x20000,child=0x30000,data=0x40000,entries=0x50000;
    m.Zero(host,0x200);m.Zero(root,0xc0);m.Zero(child,0xc0);m.Zero(data-28,0x100);
    m.Put(host,4u);m.Put(host+4,31u);m.Put(host+8,1u);m.Put(host+0x10,data);m.Put(host+0x1bd,std::uint8_t{3});m.Put(host+0x88,root);
    m.Put(data,4u);m.Put(data+0x40,std::uint16_t{100});m.Put(root+0x1c,0x80000000u);m.Put(root+0x90,child);
    m.Put(child,host);m.Put(child+0x78,root);m.Put(child+0x1c,0x40u);m.Put(child+0x18,-1);
    m.Put(child+0x30,entries);m.Put(child+0x38,std::uint64_t{1});m.Put(child+0x40,std::uint64_t{1});
    m.Put(entries,std::uint64_t{149}<<32);m.Put(entries+8,std::uint64_t{0xfffffff1});
    auto read=[&](std::uintptr_t at,void* out,std::size_t size){return m.Read(at,out,size);};
    auto capture=Contributions::Capture(host,read);check(capture&&Contributions::Value(capture->own,149)==-15);
    m.Put(root+0x1c,0xa0000000u);check(Contributions::Capture(host,read).has_value());
    m.Put(root+0x1c,0xc0000000u);check(!Contributions::Capture(host,read));
    m.Put(root+0x1c,0x20000000u);check(!Contributions::Capture(host,read));m.Put(root+0x1c,0x80000000u);
    m.Put(child+0x20,1u);check(!Contributions::Capture(host,read));m.Put(child+0x20,0u);
    m.Put(child+0x68,child);check(!Contributions::Capture(host,read));m.Put(child+0x68,std::uintptr_t{});
    m.Put(child+0x38,std::uint64_t{2049});check(!Contributions::Capture(host,read));m.Put(child+0x38,std::uint64_t{1});
    constexpr std::uintptr_t first=0x60000,second=0x70000,firstRoot=0x80000,secondRoot=0x90000,
        firstData=0xa0000,secondData=0xb0000,inventory=0xc0000,values=0xd0000;
    for(auto [unit,list,itemData,id]:std::array{
        std::array<std::uintptr_t,4>{first,firstRoot,firstData,2},std::array<std::uintptr_t,4>{second,secondRoot,secondData,3}}){
        m.Zero(unit,0x200);m.Zero(list,0xc0);m.Zero(itemData-28,0x100);
        m.Put(unit,4u);m.Put(unit+4,658u);m.Put(unit+8,static_cast<unsigned>(id));m.Put(unit+0x10,itemData);
        m.Put(unit+0x1bd,std::uint8_t{3});m.Put(unit+0x88,list);m.Put(itemData,4u);
        m.Put(list+0x1c,0x80000000u);m.Put(list+0x78,root);m.Put(list+0xa0,unit);
        m.Put(list+0xa8,values);m.Put(list+0xb0,std::uint64_t{1});m.Put(list+0xb8,std::uint64_t{1});
    }
    m.Put(values,std::uint64_t{149}<<32);m.Put(values+8,std::uint64_t{7});
    m.Put(child+0x68,secondRoot);m.Put(secondRoot+0x68,firstRoot);
    m.Zero(inventory,0x20);m.Put(host+0x90,inventory);m.Put(inventory,0x01020304u);m.Put(inventory+0x10,first);m.Put(firstData+0xb0,second);
    capture=Contributions::Capture(host,read);check(capture&&capture->sockets.size()==2&&capture->sockets[0].identity.id==2&&capture->sockets[1].identity.id==3);
    m.Put(firstRoot+0x1c,0xa0000000u);check(Contributions::Capture(host,read).has_value());
    m.Put(firstRoot+0x1c,0xc0000000u);check(!Contributions::Capture(host,read));m.Put(firstRoot+0x1c,0x80000000u);
    m.Put(secondRoot+0xa0,first);check(!Contributions::Capture(host,read));m.Put(secondRoot+0xa0,second);
    m.Put(secondData+0xb0,first);check(!Contributions::Capture(host,read));m.Put(secondData+0xb0,std::uintptr_t{});
    m.Put(second+0x1bd,std::uint8_t{2});check(!Contributions::Capture(host,read));m.Put(second+0x1bd,std::uint8_t{3});
    m.Put(inventory+0x10,second);check(!Contributions::Capture(host,read));m.Put(inventory+0x10,first);
    // An unreadable slot cannot yield a partial socket list.
    m.bytes.erase(firstData+0xb0);check(!Contributions::Capture(host,read));
}
}
// Optional local-only replay consumes captured active definitions; it is not a
// CTest prerequisite and no game rows are redistributed with the source/tests.
void TestActiveTables(const std::filesystem::path& directory){
    auto load=[&](const char* name){std::ifstream stream(directory/name,std::ios::binary);check(stream.good());
        return std::vector<unsigned char>(std::istreambuf_iterator<char>(stream),{});};
    const auto cube=load("active-cube-rows.bin"),items=load("active-item-rows.bin"),types=load("active-type-rows.bin"),gems=load("active-gem-rows.bin");
    check(cube.size()%364==0&&items.size()%448==0&&types.size()%232==0&&gems.size()%192==0);
    const ContributionDefinitions::Table c{cube.data(),cube.size()/364,364},i{items.data(),items.size()/448,448},t{types.data(),types.size()/232,232},g{gems.data(),gems.size()/192,192};
    check(ContributionDefinitions::Context(i,t,31)==0u);check(ContributionDefinitions::Context(i,t,360)==1u);
    const auto outcome=ContributionDefinitions::ResolveCorruption(c,i,t,31,4,2,168,374,375);
    check(outcome&&outcome->recognized&&!outcome->conversion&&outcome->specs.size()==2);
    check(outcome->specs[0].property==29&&outcome->specs[0].low==50&&outcome->specs[0].high==50);
    check(outcome->specs[1].property==66&&outcome->specs[1].low==8&&outcome->specs[1].high==8);
    const auto elWeapon=ContributionDefinitions::Gem(g,625,0),elArmor=ContributionDefinitions::Gem(g,625,1);
    check(elWeapon&&elArmor&&elWeapon->size()==2&&elArmor->size()==2);
    check(elWeapon->at(0).property==73&&elWeapon->at(0).low==1&&elArmor->at(0).low==1);
    check(elWeapon->at(1).property==15&&elWeapon->at(1).low==50&&elArmor->at(1).property==0&&elArmor->at(1).low==15);
    std::puts("Local active compiled-table replay: host weapon/armor contexts, completed roll67 ED50/LL8 winner, and El contexts agree.");
}
int main(int argc,char** argv){TestSamples();TestSeparateEndpointSources();TestDefinitions();TestCapture();
    if(argc==2)TestActiveTables(argv[1]);
    std::puts("Owned IAS/Life/flat-max/ED/socket-only/fire sources, corruption outcomes and atomic refusals pass.");}
