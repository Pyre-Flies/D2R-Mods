#include "../src/property_rolls.h"
#include "../src/runeword_sources.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace {
void check(bool ok){if(!ok)std::abort();}
template<class T>void put(unsigned char* p,unsigned at,T value){std::memcpy(p+at,&value,sizeof(value));}
Affixes::Property property(std::initializer_list<unsigned> stats){Affixes::Property p{};unsigned n=0;for(auto s:stats){p.bytes[0x18+n]=n?3:1;put(p.bytes.data(),0x20+2*n,std::uint16_t(s));++n;}return p;}
Affixes::Rolled roll(unsigned id,int low,int high,const char* tag=nullptr){Affixes::Rolled r{};r.id=id+1;r.source=tag;r.uniqueScalar=tag!=nullptr;for(unsigned n=0;n<3;++n)put(r.row.bytes.data(),0x24+n*16,-1);put(r.row.bytes.data(),0x24,id);put(r.row.bytes.data(),0x2c,low);put(r.row.bytes.data(),0x30,high);return r;}
Contributions::Stats stats(std::initializer_list<std::pair<unsigned,int>> entries){Contributions::Stats out;for(auto [s,v]:entries)out.push_back({std::uint64_t{s}<<32,v});std::sort(out.begin(),out.end(),[](auto a,auto b){return a.key<b.key;});return out;}
auto solve(std::span<const Affixes::Rolled> d,std::span<const Affixes::Property> p,const Contributions::Stats& a){return PropertyRolls::Solve(PropertyRolls::Compile(d,p),a,[](unsigned,std::int64_t raw){return Contributions::Normalize(raw,0);});}
void Linked(){
 std::vector<Affixes::Property> p={property({39,41,43,45}),property({39}),property({41}),property({45}),property({0,1,2,3}),property({7})};
 std::vector<Affixes::Rolled> d={roll(0,16,20),roll(1,11,20),roll(2,21,30)};
 auto s=solve(d,p,stats({{39,32},{41,50},{43,20},{45,20}}));
 auto fire=PropertyRolls::Label(s,"fire",39,32);check(fire && fire->ownedParts.size()==2 && fire->ownedParts[0].value==20 && fire->ownedParts[1].value==12);
 check(PropertyRolls::Label(s,"light",41,50)->ownedParts[1].value==30);
 check(RangeText::ExpandOwned("Fire Resist +32%",*fire).find("Fire Resist +12%")!=std::string::npos);
 d={roll(0,16,20),roll(3,11,20)};s=solve(d,p,stats({{39,20},{41,20},{43,20},{45,32}}));
 auto poison=PropertyRolls::Label(s,"poison",45,32);check(poison && poison->ownedParts[0].value==20 && poison->ownedParts[1].value==12);
 // Cross-stat evidence works regardless of which resistance is first/overlapped.
 for(unsigned target:{39u,41u,43u,45u}){
  auto q=p; q[1]=property({target});d={roll(0,16,20),roll(1,11,20)};
  auto a=stats({{39,18},{41,18},{43,18},{45,18}});for(auto& v:a)if(v.key==std::uint64_t{target}<<32)v.value=33;
  auto result=PropertyRolls::Label(solve(d,q,a),"res",target,33);check(result && result->ownedParts[0].value==18 && result->ownedParts[1].value==15);
 }
 d={roll(0,16,20),roll(0,16,20)};s=solve(d,p,stats({{39,36},{41,36},{43,36},{45,36}}));
 auto ambiguous=PropertyRolls::Label(s,"res",39,36);check(ambiguous && !ambiguous->ownedParts[0].value && !ambiguous->ownedParts[1].value);
 // Unexplained totals refuse only their connected component.
 d={roll(0,16,20),roll(5,10,20)};s=solve(d,p,stats({{39,20},{41,21},{43,20},{45,20},{7,15}}));check(!PropertyRolls::Label(s,"res",39,20));check(PropertyRolls::Label(s,"life",7,15).has_value());
 d={roll(4,3,8,"[U]")};s=solve(d,p,stats({{0,6},{1,6},{2,6},{3,6}}));const std::array<unsigned,4> members={0,1,2,3};
 auto attributes=PropertyRolls::Label(s,"attributes",0,6,members);check(attributes && RangeText::ExpandOwned("+6 to All Attributes",*attributes).find("[+3 - +8]")!=std::string::npos);
 d={roll(4,5,10,"[U]")};s=solve(d,p,stats({{0,10},{1,10},{2,10},{3,10}}));check(PropertyRolls::Label(s,"attributes",0,10,members)->ownedParts[0].high==10);
 // Differing members cannot get a false grouped range.
 s=solve(d,p,stats({{0,10},{1,9},{2,10},{3,10}}));check(!PropertyRolls::Label(s,"attributes",0,10,members));
 d[0].uniqueScalar=false;s=solve(d,p,stats({{0,10},{1,10},{2,10},{3,10}}));check(!PropertyRolls::Label(s,"attributes",0,10,members));
 d={roll(0,0,20)};s=solve(d,p,stats({{39,0},{41,5},{43,6},{45,7}}));check(!PropertyRolls::Label(s,"res",41,5));
 auto replace=p;replace[0].bytes[2]=1;d={roll(0,16,20)};check(!PropertyRolls::Label(solve(d,replace,stats({{39,20},{41,20},{43,20},{45,20}})),"res",39,20));
 auto unknown=p;unknown[0].bytes[0x19]=22;check(!PropertyRolls::Label(solve(d,unknown,stats({{39,20},{41,20},{43,20},{45,20}})),"res",39,20));
 std::vector<PropertyRolls::Interval> b={{0,1},{0,1},{0,1}};const std::array<PropertyRolls::Equation,3> impossible={{{0,1,{0,1}},{1,1,{1,2}},{2,1,{0,2}}}};unsigned budget=4096;check(!PropertyRolls::Feasible(b,impossible,budget));budget=0;check(!PropertyRolls::Feasible(b,impossible,budget) && budget==0);
 // Exhaustive integer fixtures verify every resolved value against all feasible assignments.
 const std::array<PropertyRolls::Equation,3> shape={{{0,0,{0,1}},{1,0,{1,2}},{2,0,{0,2}}}};
 for(int x=0;x<=6;++x)for(int y=0;y<=6;++y)for(int z=0;z<=6;++z){
  auto e=shape;e[0].actual=x;e[1].actual=y;e[2].actual=z;std::vector<PropertyRolls::Interval> bounds={{0,3},{0,3},{0,3}};budget=4096;
  bool expected=false;for(int a=0;a<=3;++a)for(int c=0;c<=3;++c)for(int v=0;v<=3;++v)expected|=a+c==x && c+v==y && a+v==z;
  const bool found=PropertyRolls::Propagate(bounds,e) && PropertyRolls::Feasible(bounds,e,budget);check(found==expected);
 }
}
void Words(){
 std::array<unsigned char,232*2> words{};std::array<unsigned char,448*4> items{};std::array<unsigned char,232*4> types{};
 put(items.data(),0x12e,std::int16_t{1});put(items.data(),0x130,std::int16_t{0});
 const ContributionDefinitions::Table w{words.data(),2,232},i{items.data(),4,448},t{types.data(),4,232};
 auto* r=words.data();r[0x44]=1;put(r,0x46,std::uint16_t{123});put(r,0x4a,std::int16_t{1});
 for(unsigned n=0;n<6;++n)put(r,0x5c+n*4,-1);for(unsigned n=0;n<7;++n)put(r,0x74+n*16,UINT32_MAX);
 put(r,0x5c,1);put(r,0x60,2);put(r,0x64,2);put(r,0x74,0u);put(r,0x7c,100);put(r,0x80,150);
 std::vector<Contributions::Socket> sockets(3);sockets[0].identity.classId=1;sockets[1].identity.classId=2;sockets[2].identity.classId=2;
 auto resolve=[&](unsigned flags=0x04000000,unsigned quality=2){return RunewordSources::Resolve(w,i,t,0,quality,flags,sockets,10,2);};
 check(resolve() && resolve()->at(0).low==100 && resolve()->at(0).high==150);
 // Changed mod recipe/range uses active rows, with no known name or item database.
 put(r,0x7c,200);put(r,0x80,300);check(resolve()->at(0).low==200);
 check(!resolve(0));check(!resolve(0xc4000000));check(!resolve(0x04000000,6));
 std::swap(sockets[0],sockets[2]);check(!resolve());std::swap(sockets[0],sockets[2]);
 std::memcpy(words.data()+232,words.data(),232);check(!resolve());words[232+0x44]=0;check(resolve().has_value());
 put(r,0x56,std::int16_t{1});check(!resolve());put(r,0x56,std::int16_t{0});
 put(r,0x4a,std::int16_t{3});check(!resolve());put(r,0x4a,std::int16_t{1});
 put(types.data()+232,4,std::int16_t{2});put(types.data()+464,4,std::int16_t{1});check(!resolve());put(types.data()+232,4,std::int16_t{0});
 put(r,0x74,999u);check(!resolve());put(r,0x74,0x10001u);check(resolve().has_value());put(r,0x74,0x10002u);check(!resolve());
}
void LayerSelectors(){
 std::vector<Affixes::Property> p={property({83}),property({97}),property({188}),property({107,97})};
 p[0].bytes[0x18]=21;put(p[0].bytes.data(),0x0a,std::uint16_t{0});
 auto d=roll(0,1,3,"[RW]");check(PropertyRolls::LayerBounds(d,p,83,0)==PropertyRolls::Interval{1,3});check(!PropertyRolls::LayerBounds(d,p,83,1));
 p[1].bytes[0x18]=24;d=roll(1,3,6,"[RW]");put(d.row.bytes.data(),0x28,133);
 check(PropertyRolls::LayerBounds(d,p,97,133)==PropertyRolls::Interval{3,6});check(!PropertyRolls::LayerBounds(d,p,97,132));
 p[2].bytes[0x18]=10;d=roll(2,1,4,"[RW]");put(d.row.bytes.data(),0x28,7);
 check(PropertyRolls::LayerBounds(d,p,188,17)==PropertyRolls::Interval{1,4});check(!PropertyRolls::LayerBounds(d,p,188,7));
 p[3].bytes[0x18]=22;p[3].bytes[0x19]=24;d=roll(3,2,5,"[RW]");put(d.row.bytes.data(),0x28,54);
 check(PropertyRolls::LayerBounds(d,p,107,54)==PropertyRolls::Interval{2,5});check(PropertyRolls::LayerBounds(d,p,97,54)==PropertyRolls::Interval{2,5});
 d.uniqueScalar=false;check(!PropertyRolls::LayerBounds(d,p,107,54));d.uniqueScalar=true;p[3].bytes[2]=1;check(!PropertyRolls::LayerBounds(d,p,107,54));
 p[3].bytes[2]=0;p[3].bytes[0x18]=11;check(!PropertyRolls::LayerBounds(d,p,107,54));
 std::vector<Affixes::PropertyGroup> groups(1);groups[0].bytes[4]=2;
 put(groups[0].bytes.data(),8,0u);put(groups[0].bytes.data(),28,1);put(groups[0].bytes.data(),32,1u);put(groups[0].bytes.data(),52,1);
 auto ambiguous=roll(0x10000,1,1,"[U]");const auto blocked=PropertyRolls::Compile(std::span(&ambiguous,1),p,groups);
 check(blocked.blocked.contains(83) && blocked.blocked.contains(97));
 put(groups[0].bytes.data(),8,0x10000u);check(!PropertyRolls::Compile(std::span(&ambiguous,1),p,groups).complete);
}
void RandomLayers(){
 std::vector<Affixes::Property> p={property({107}),property({107}),property({83}),property({188})};
 p[0].bytes[0x18]=p[2].bytes[0x18]=p[3].bytes[0x18]=12;p[1].bytes[0x18]=22;
 const auto normalized=[](std::int64_t value){return Contributions::Normalize(value,0);};
 const auto owned=[](unsigned stat,std::initializer_list<std::pair<unsigned,int>> layers){Contributions::Stats a;
  for(auto [layer,value]:layers)a.push_back({(std::uint64_t{stat}<<32)|layer,value});
  std::sort(a.begin(),a.end(),[](const auto& x,const auto& y){return x.key<y.key;});return a;};
 bool proved=false;
 // All installed class pools and strengths; endpoint IDs select skills, never
 // the displayed bonus range. No class names/skill IDs exist in production.
 for(const auto& pool:std::array<std::pair<int,int>,8>{{{6,35},{36,65},{66,95},{96,125},{126,155},{221,250},{251,280},{373,402}}})
  for(int bonus=2;bonus<=5;++bonus)for(int layer:{pool.first,(pool.first+pool.second)/2,pool.second}) {
   auto d=roll(0,pool.first,pool.second);put(d.row.bytes.data(),0x28,bonus);
   check(!Affixes::Contributes(d.row,p,107,layer));check(Affixes::Contributes(d.row,p,107,layer,{6,63,true}));
   check(!Affixes::Contributes(d.row,p,107,pool.first-1,{6,63,true}));
   check(PropertyRolls::LayerBounds(d,p,107,static_cast<unsigned>(layer),true)==PropertyRolls::Interval{bonus,bonus});
   auto label=PropertyRolls::LayerLabel(std::span(&d,1),p,owned(107,{{static_cast<unsigned>(layer),bonus}}),107,static_cast<unsigned>(layer),"skill",normalized,true,proved);
   check(proved && label && label->ownedParts.size()==1 && label->ownedParts[0].value==bonus && label->ownedParts[0].low==bonus && label->ownedParts[0].high==bonus);
  }
 auto random=roll(0,251,280);put(random.row.bytes.data(),0x28,5);
 check(!PropertyRolls::LayerLabel(std::span(&random,1),p,owned(107,{{262,5}}),107,262,"skill",normalized,false,proved) && !proved);
 check(!PropertyRolls::LayerLabel(std::span(&random,1),p,owned(107,{{262,6}}),107,262,"skill",normalized,true,proved) && !proved);
 check(!PropertyRolls::LayerLabel(std::span(&random,1),p,owned(107,{{250,5}}),107,250,"skill",normalized,true,proved) && !proved);
 auto fixed=roll(1,2,2);put(fixed.row.bytes.data(),0x28,262);std::vector<Affixes::Rolled> d={random,fixed};
 auto label=PropertyRolls::LayerLabel(d,p,owned(107,{{262,7}}),107,262,"skill",normalized,true,proved);
 check(proved && label && label->ownedParts.size()==2 && label->ownedTotal==7);
 // Random skill selected elsewhere must not be attached to the fixed skill.
 label=PropertyRolls::LayerLabel(d,p,owned(107,{{262,2},{270,5}}),107,262,"skill",normalized,true,proved);
 check(proved && label && label->ownedParts.size()==1 && label->ownedParts[0].low==2);
 label=PropertyRolls::LayerLabel(d,p,owned(107,{{262,2},{270,5}}),107,270,"skill",normalized,true,proved);
 check(proved && label && label->ownedParts.size()==1 && label->ownedParts[0].low==5);
 // Two equal independent random bonuses have ambiguous attribution.
 d={random,random};check(!PropertyRolls::LayerLabel(d,p,owned(107,{{262,5},{270,5}}),107,262,"skill",normalized,true,proved) && !proved);
 label=PropertyRolls::LayerLabel(d,p,owned(107,{{262,10}}),107,262,"skill",normalized,true,proved);
 check(proved && label && label->ownedParts.size()==2 && label->ownedTotal==10);
 // Negative cancellation, replacement, conditional and proof-budget cases
 // cannot manufacture a selected random layer.
 fixed=roll(1,-5,-5);put(fixed.row.bytes.data(),0x28,270);d={random,fixed};
 check(!PropertyRolls::LayerLabel(d,p,owned(107,{{262,5}}),107,262,"skill",normalized,true,proved) && !proved);
 d={fixed};label=PropertyRolls::LayerLabel(d,p,owned(107,{{270,-5}}),107,270,"skill",normalized,true,proved);
 check(proved && label && label->ownedTotal==-5); // Fixed-only behavior is preserved.
 p[0].bytes[2]=1;d={random};check(!PropertyRolls::LayerLabel(d,p,owned(107,{{262,5}}),107,262,"skill",normalized,true,proved) && !proved);p[0].bytes[2]=0;
 random.unconditionalScalar=false;d={random};check(!PropertyRolls::LayerLabel(d,p,owned(107,{{262,5}}),107,262,"skill",normalized,true,proved) && !proved);random.unconditionalScalar=true;
 p[0].bytes[0x19]=24;put(p[0].bytes.data(),0x22,std::uint16_t{97});d={random};
 check(!PropertyRolls::LayerLabel(d,p,owned(107,{{262,5}}),107,262,"skill",normalized,true,proved) && !proved);p[0].bytes[0x19]=0;
 d.assign(6,random);check(!PropertyRolls::LayerLabel(d,p,owned(107,{{251,5},{252,5},{253,5},{254,5},{255,5},{256,5}}),107,251,"skill",normalized,true,proved) && !proved);
 p.push_back(property({65535}));p.back().bytes[0x18]=25;
 std::vector<Affixes::PropertyGroup> unknownGroup(1);put(unknownGroup[0].bytes.data(),8,4u);put(unknownGroup[0].bytes.data(),28,1);
 d={random,roll(0x10000,1,1,"[U]")};
 check(!PropertyRolls::LayerLabel(d,p,owned(107,{{262,5}}),107,262,"skill",normalized,true,proved,unknownGroup) && !proved);
 // Function12 uses raw layers for random class and tab properties as well.
 for(auto [id,stat,layer]:std::array<std::array<unsigned,3>,2>{{{2,83,0},{3,188,17}}}) {
  auto r=roll(id,0,50,"[RW]");put(r.row.bytes.data(),0x28,2);
  label=PropertyRolls::LayerLabel(std::span(&r,1),p,owned(stat,{{layer,2}}),stat,layer,"selector",normalized,true,proved);
  check(proved && label && label->ownedParts[0].low==2);
 }
}
void Choices(){
 const std::array<PropertyRolls::Equation,1> proofEquation={{{0,1,{0}}}};
 unsigned proofBudget=0;bool proofUnknown=false;
 check(!PropertyRolls::Feasible({{0,1}},proofEquation,proofBudget,&proofUnknown) && proofUnknown);
 proofBudget=10;proofUnknown=false;check(!PropertyRolls::Feasible({{2,3}},proofEquation,proofBudget,&proofUnknown) && !proofUnknown);
 std::array<unsigned char,324> metadata{};check(PropertyRolls::ItemScalarOperand(metadata.data()));
 for(unsigned op:{8,9}){metadata[0x50]=static_cast<unsigned char>(op);check(PropertyRolls::ItemScalarOperand(metadata.data()));}
 for(unsigned op:{1,2,3,4,5,6,7,10,11,12,13}){metadata[0x50]=static_cast<unsigned char>(op);check(!PropertyRolls::ItemScalarOperand(metadata.data()));}
 std::vector<Affixes::Property> p={property({0,1,2,3}),property({96}),property({79})};
 auto a=roll(0,3,8,"[U]"),b=roll(1,10,20,"[U]"),c=roll(2,15,30,"[U]");
 a.unconditionalScalar=b.unconditionalScalar=c.unconditionalScalar=false;
 a.choicePool=b.choicePool=c.choicePool=1;a.choiceEntry=1;b.choiceEntry=2;c.choiceEntry=3;
 std::vector<Affixes::Rolled> d={a,b,c,roll(2,45,75,"[U]")};
 const auto normalize=[](unsigned,std::int64_t value){return Contributions::Normalize(value,0);};
 auto s=PropertyRolls::SolveSources(d,p,stats({{0,6},{1,6},{2,6},{3,6},{79,55}}),normalize);
 const std::array<unsigned,4> group={0,1,2,3};auto label=PropertyRolls::Label(s,"attributes",0,6,group);check(label && label->ownedParts.size()==1 && label->ownedParts[0].low==3);
 // Same-stat alternatives cannot be promoted just because a value fits both.
 b=a;b.choiceEntry=2;d={a,b};s=PropertyRolls::SolveSources(d,p,stats({{0,6},{1,6},{2,6},{3,6}}),normalize);
 check(!PropertyRolls::Label(s,"attributes",0,6,group));
 // Contradictory members cannot establish either a selection or shared roll.
 d={a,c};s=PropertyRolls::SolveSources(d,p,stats({{0,6},{1,7},{2,6},{3,6}}),normalize);check(!PropertyRolls::Label(s,"attributes",0,6,group));
 // A bounded refusal leaves the conditional candidates blocked.
 auto unqualified=p;unqualified[1].bytes[0x18]=25;put(unqualified[1].bytes.data(),0x20,std::uint16_t{65535});
 d={a,b};d[1].row=roll(1,1,2,"[U]").row;
 s=PropertyRolls::SolveSources(d,unqualified,stats({{0,6},{1,6},{2,6},{3,6}}),normalize);check(!PropertyRolls::Label(s,"attributes",0,6,group));
 d.clear();for(unsigned entry=1;entry<=129;++entry){auto v=a;v.choiceEntry=entry;d.push_back(v);}
 s=PropertyRolls::SolveSources(d,p,stats({{0,6},{1,6},{2,6},{3,6}}),normalize);check(!PropertyRolls::Label(s,"attributes",0,6,group));
}
}
#include "active_property_audit.h"
int main(int argc,char** argv){Linked();Words();LayerSelectors();RandomLayers();Choices();if(argc==2)AuditActiveWords(argv[1]);std::puts("Linked scalar/shared rolls, random/fixed selector proofs, reverse attribution, integer constraints and active recipe variants pass.");}
