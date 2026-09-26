#include "affixes.h"
#include "range_text.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
void check(bool b) { if (!b) std::exit(1); }
void number(Affixes::Row& r,std::size_t at,int n) { std::memcpy(r.bytes.data()+at,&n,4); }
void groupNumber(Affixes::PropertyGroup& r,std::size_t at,int n) { std::memcpy(r.bytes.data()+at,&n,4); }
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
    check(Affixes::NamedLabel(savage)=="[P] [Savage] [T3]");
    check(Affixes::ScalarBounds(savage,{&physical,1},17,0,low,high) && low==66 && high==80);
    check(!Affixes::ScalarBounds(savage,{&physical,1},17,1,low,high));
    check(!Affixes::ScalarBounds(savage,{&physical,1},39,0,low,high));
    std::vector<Affixes::Property> damageProperties(3);
    damageProperties[0].bytes[0x18]=15; damageProperties[0].bytes[0x20]=48;
    damageProperties[0].bytes[0x19]=16; damageProperties[0].bytes[0x22]=49;
    damageProperties[1].bytes[0x18]=1; damageProperties[1].bytes[0x20]=48;
    damageProperties[2].bytes[0x18]=1; damageProperties[2].bytes[0x20]=49;
    auto elemental=row(1); number(elemental,0x24,0); number(elemental,0x2c,21); number(elemental,0x30,50);
    Affixes::Rolled elementalRoll{1,elemental,true,1};
    int minLow{},minHigh{},maxLow{},maxHigh{};
    check(Affixes::DamageEndpointBounds(elementalRoll,damageProperties,48,49,
        minLow,minHigh,maxLow,maxHigh));
    check(minLow==21 && minHigh==21 && maxLow==50 && maxHigh==50);
    auto flame=row(1); number(flame,0x24,1); number(flame,0x2c,1); number(flame,0x30,1);
    number(flame,0x34,2); number(flame,0x3c,2); number(flame,0x40,5);
    Affixes::Rolled flameRoll{2,flame,false,6};
    check(Affixes::DamageEndpointBounds(flameRoll,damageProperties,48,49,
        minLow,minHigh,maxLow,maxHigh));
    check(minLow==1 && minHigh==1 && maxLow==2 && maxHigh==5);
    savage.source="[Unique]";
    check(Affixes::NamedLabel(savage)=="[Unique] [T3]");
    check(!Affixes::ScalarBounds(savage,{&physical,1},17,0,low,high));
    savage.source="[Base]"; savage.tier=0;
    check(Affixes::NamedLabel(savage)=="[Base]");
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
    std::vector<Affixes::PropertyGroup> groups(3);
    // One fixed, positive-weight choice is deterministic and may be followed.
    groupNumber(groups[0],8,0); groupNumber(groups[0],12,0); groupNumber(groups[0],16,0);
    groupNumber(groups[0],24,1); groupNumber(groups[0],28,1);
    auto groupedRow=defense; number(groupedRow,0x24,0x10000);
    check(Affixes::Contributes(groupedRow,props,31,0,{},groups));
    // A second viable choice makes the selected source unknowable.
    groupNumber(groups[0],32,0); groupNumber(groups[0],36,0); groupNumber(groups[0],40,0);
    groupNumber(groups[0],48,1); groupNumber(groups[0],52,1);
    check(!Affixes::Contributes(groupedRow,props,31,0,{},groups));
    // Nested deterministic groups work; cycles and out-of-range IDs fail open.
    groups[1]={}; groupNumber(groups[1],8,0); groupNumber(groups[1],12,0); groupNumber(groups[1],16,0);
    groupNumber(groups[1],24,1); groupNumber(groups[1],28,1);
    groups[2]={}; groupNumber(groups[2],8,0x10001); groupNumber(groups[2],12,0); groupNumber(groups[2],16,0);
    groupNumber(groups[2],24,1); groupNumber(groups[2],28,1);
    number(groupedRow,0x24,0x10002);
    check(Affixes::Contributes(groupedRow,props,31,0,{},groups));
    groupNumber(groups[1],8,0x10002);
    check(!Affixes::Contributes(groupedRow,props,31,0,{},groups));
    number(groupedRow,0x24,0x10003);
    check(!Affixes::Contributes(groupedRow,props,31,0,{},groups));
    std::vector<Affixes::Rolled> rolled={{1,defense,false,2},{2,defense,true,1}};
    check(Affixes::Label(rolled,props,31,0)=="[S] [T2] [P] [T1]");
    const int complete[]={31}, incomplete[]={31,39};
    check(Affixes::GroupLabel(rolled,props,complete,0)=="[S] [T2] [P] [T1]");
    check(Affixes::GroupLabel(rolled,props,incomplete,0).empty());
    const std::vector<Affixes::Rolled> unique={{1,defense,false,0,"[Unique]"},{2,defense,false,0,"[Unique]"}};
    check(Affixes::Label(unique,props,31,0)=="[Unique]");
    std::vector<RangeText::Label> labels={{RangeText::Analyze("+118 Defense").key,"[S] [T2]"}};
    const std::string mark="\xee\x81\xbe";
    const std::string ranged="+"+mark+"U(80-120)"+mark+"3 Defense";
    auto merged=RangeText::Merge("+118 Defense",ranged,1024,labels);
    check(merged.labeled==1 && merged.annotated==1 && merged.text.find("+118 Defense")!=std::string::npos);
    check(merged.text.ends_with(mark+"U[S] [T2]"+mark+"3"));
    merged=RangeText::Merge("+118 Defense","+118 Defense",1024,labels);
    check(merged.labeled==1 && merged.annotated==0);
    labels.push_back(labels[0]);
    check(RangeText::Merge("+118 Defense",ranged,1024,labels).labeled==0);
    labels.pop_back();
    merged=RangeText::Merge("+118 Defense",ranged,20,labels);
    check(merged.text=="+118 Defense" && !merged.labeled && !merged.annotated);
    std::puts("Passed affix tiers, duplicate levels, eligibility, properties, layers, actual values, colors, ambiguity and capacity.");
}
