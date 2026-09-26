#include "affixes.h"
#include "range_text.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
void check(bool b) { if (!b) std::exit(1); }
void number(Affixes::Row& r,std::size_t at,int n) { std::memcpy(r.bytes.data()+at,&n,4); }
Affixes::Row row(int level,int group=1) {
    Affixes::Row r; r.bytes[0x54]=r.bytes[0x64]=r.bytes[0x82]=1;
    number(r,0x58,level); number(r,0x5c,group);
    number(r,0x34,-1); number(r,0x44,-1); return r;
}
int main() {
    auto named=row(1); std::memcpy(named.bytes.data(),"Savage",7);
    number(named,0x2c,66); number(named,0x30,80);
    Affixes::Property physical{}; physical.bytes[0x18]=7;
    Affixes::Rolled savage{976,named,true,3};
    int low{},high{};
    check(Affixes::NamedLabel(savage)=="[Prefix] [T3]");
    check(Affixes::ScalarBounds(savage,{&physical,1},17,0,low,high) && low==66 && high==80);
    check(!Affixes::ScalarBounds(savage,{&physical,1},17,1,low,high));
    check(!Affixes::ScalarBounds(savage,{&physical,1},39,0,low,high));
    savage.source="[Unique]"; check(!Affixes::ScalarBounds(savage,{&physical,1},17,0,low,high));
    std::vector<Affixes::Row> family={row(1),row(10),row(10),row(30),row(50,2)};
    auto yes=[](const Affixes::Row&){return true;};
    check(Affixes::Tier(family[0],family,true,100,yes)==3);
    check(Affixes::Tier(family[3],family,true,100,yes)==1);
    family[3].bytes[0x64]=0;
    check(Affixes::Tier(family[0],family,true,100,yes)==2);
    check(Affixes::Tier(family[0],family,false,100,yes)==3);
    check(Affixes::Tier(family[0],family,false,100,[](const Affixes::Row& r){return r.Number(0x58)<30;})==2);
    family[3].bytes[0x54]=0;
    check(Affixes::Tier(family[0],family,false,100,yes)==2);
    std::vector<Affixes::Property> props(1);
    props[0].bytes[0x18]=1; props[0].bytes[0x20]=31;
    auto defense=row(1);
    check(Affixes::Contributes(defense,props,31,0));
    check(!Affixes::Contributes(defense,props,39,0));
    props[0].bytes[0x18]=22; number(defense,0x28,7);
    check(Affixes::Contributes(defense,props,31,7));
    check(!Affixes::Contributes(defense,props,31,8));
    // Class selectors come from valN, not the affix parameter or zero layer.
    props[0].bytes[0x18]=21; props[0].bytes[0x0a]=4;
    check(Affixes::Contributes(defense,props,31,4));
    check(!Affixes::Contributes(defense,props,31,0));
    check(!Affixes::Contributes(defense,props,31,3));
    props[0].bytes[0x18]=10; number(defense,0x28,12);
    check(Affixes::Contributes(defense,props,31,32));
    check(!Affixes::Contributes(defense,props,31,12));
    number(defense,0x28,23); check(Affixes::Contributes(defense,props,31,58));
    number(defense,0x28,24); check(!Affixes::Contributes(defense,props,31,64));
    props[0].bytes[0x18]=24; number(defense,0x28,7);
    check(Affixes::Contributes(defense,props,31,7));
    check(!Affixes::Contributes(defense,props,31,0));
    for (auto fn:{11,19}) {
        props[0].bytes[0x18]=static_cast<unsigned char>(fn); number(defense,0x30,3);
        check(Affixes::Contributes(defense,props,31,(7<<6)|3));
        check(!Affixes::Contributes(defense,props,31,(8<<6)|3));
        check(!Affixes::Contributes(defense,props,31,(7<<6)|4));
        check(!Affixes::Contributes(defense,props,31,(7<<6)|3,{0,0}));
        number(defense,0x30,-1); check(!Affixes::Contributes(defense,props,31,(7<<6)|3));
    }
    props[0].bytes[0x18]=5;
    check(Affixes::Contributes(defense,props,21,0));
    check(!Affixes::Contributes(defense,props,22,0));
    props[0].bytes[0x18]=1;
    std::vector<Affixes::Rolled> rolled={{1,defense,false,2},{2,defense,true,1}};
    check(Affixes::Label(rolled,props,31,0)=="[Suffix] [T2] [Prefix] [T1]");
    const int complete[]={31}, incomplete[]={31,39};
    check(Affixes::GroupLabel(rolled,props,complete,0)=="[Suffix] [T2] [Prefix] [T1]");
    check(Affixes::GroupLabel(rolled,props,incomplete,0).empty());
    const std::vector<Affixes::Rolled> unique={{1,defense,false,0,"[Unique]"},{2,defense,false,0,"[Unique]"}};
    check(Affixes::Label(unique,props,31,0)=="[Unique]");
    std::vector<RangeText::Label> labels={{RangeText::Analyze("+118 Defense").key,"[Suffix] [T2]"}};
    const std::string mark="\xee\x81\xbe";
    const std::string ranged="+"+mark+"U(80-120)"+mark+"3 Defense";
    auto merged=RangeText::Merge("+118 Defense",ranged,1024,labels);
    check(merged.labeled==1 && merged.annotated==1 && merged.text.find("+118 Defense")!=std::string::npos);
    check(merged.text.ends_with(mark+"U[Suffix] [T2]"+mark+"3"));
    merged=RangeText::Merge("+118 Defense","+118 Defense",1024,labels);
    check(merged.labeled==1 && merged.annotated==0);
    labels.push_back(labels[0]);
    check(RangeText::Merge("+118 Defense",ranged,1024,labels).labeled==0);
    labels.pop_back();
    merged=RangeText::Merge("+118 Defense",ranged,20,labels);
    check(merged.text=="+118 Defense" && !merged.labeled && !merged.annotated);
    std::puts("Passed affix tiers, duplicate levels, eligibility, properties, layers, actual values, colors, ambiguity and capacity.");
}
