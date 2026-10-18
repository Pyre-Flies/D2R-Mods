#pragma once
#include "../src/contribution_model.h"
#include <fstream>
// Private definition input only: no extracted game rows are compiled into tests.
inline void AuditOperationGaps(const char* path){
 std::ifstream in(path,std::ios::binary);char magic[4]{};unsigned count{},references{};
 in.read(magic,4);in.read(reinterpret_cast<char*>(&count),4);in.read(reinterpret_cast<char*>(&references),4);
 check(in.good() && std::memcmp(magic,"GAP1",4)==0 && count>0 && count<32768 && references<1000000);
 std::vector<Affixes::Property> properties(count);in.read(reinterpret_cast<char*>(properties.data()),count*48);check(in.good());
 // Disjoint synthetic stat isolates whole-model poisoning from valid target refusal.
 properties.push_back(property({4000}));const auto sentinel=roll(count,11,20,"[control]");
 unsigned isolation=0,failed=0,ed=0,edFailed=0,durability=0,sockets=0,durabilityMissing=0,socketsMissing=0,packed=0,derived=0,chargeSources=0;
 for(unsigned n=0;n<references;++n){unsigned id{};int parameter{},low{},high{};
  in.read(reinterpret_cast<char*>(&id),4);in.read(reinterpret_cast<char*>(&parameter),4);
  in.read(reinterpret_cast<char*>(&low),4);in.read(reinterpret_cast<char*>(&high),4);check(in.good() && id<count);
  for(const char* tag:{"[U]","[Set]","[RW]"}){
   auto d=roll(id,low,high,tag);put(d.row.bytes.data(),0x28,parameter);
   std::array<Affixes::Rolled,2> defs{d,sentinel};const auto solved=solve(defs,properties,stats({{4000,17}}));
   const auto label=PropertyRolls::Label(solved,"control",4000,17);
   const bool ok=label && label->ownedParts.size()==1 && label->ownedParts[0].low==11 && label->ownedParts[0].high==20 && label->ownedParts[0].value==17;
   ++isolation;if(!ok){++failed;std::printf("ISOLATION FAILURE reference=%u property=%u tag=%s\n",n,id,tag);}
  }
  const auto& p=properties[id].bytes;const unsigned fn=p[0x18];
  if(fn==11 || fn==19){
   if(parameter>=0 && parameter<=32767 && high>0 && high<=63){
    const int layer=(parameter<<6)|high;const unsigned target=Affixes::Read<std::uint16_t>(p.data(),0x20);
    check(Affixes::PropertyContributes(properties[id],parameter,low,high,static_cast<int>(target),layer,{6,63}));
    check(!Affixes::PropertyContributes(properties[id],parameter,low,high,static_cast<int>(target),layer^(1<<6),{6,63}));
    ++packed;
   }else ++derived;
  }
  if(fn==19 && Affixes::Read<std::uint16_t>(p.data(),0x20)==204 && parameter>=0 && parameter<=32767 && high!=0 && high<=63){
   auto d=roll(id,low,high,"[U]");put(d.row.bytes.data(),0x28,parameter);
   for(int level:{high<0?1:high,high<0?63:high}){
    const int layer=(parameter<<6)|level;
    check(Affixes::ChargeSourceLabel({&d,1},properties,layer,{6,63})=="[U]");
    check(Affixes::ChargeSourceLabel({&d,1},properties,layer^(1<<6),{6,63}).empty());
    ++chargeSources;
   }
  }
  if(fn!=2 && fn!=13 && fn!=14)continue;
  const unsigned stat=Affixes::Read<std::uint16_t>(p.data(),0x20);
  if(low>high || low<0 || high>1000000)continue;
  auto d=roll(id,low,high,"[U]");put(d.row.bytes.data(),0x28,parameter);
  for(int value:{low,low+(high-low)/2,high}){
   ContributionModel::Group group{stats({{stat,value}}),{d},""};
   const auto label=ContributionModel::Build("+# modifier",stat,0,{&group,1},properties);
   const bool numeric=label && label->ownedParts.size()==1 && label->ownedParts[0].low==low && label->ownedParts[0].high==high && label->ownedParts[0].value==value;
   if(fn==2){++ed;if(!numeric)++edFailed;}
   if(fn==13){++durability;if(!numeric)++durabilityMissing;}
   if(fn==14){++sockets;if(!numeric)++socketsMissing;}
  }
 }
 check(in.peek()==std::char_traits<char>::eof());
 // Exact combined ED bounds with ambiguous individual contributions retained.
 unsigned edProperty=0;while(edProperty<count && properties[edProperty].bytes[0x18]!=2)++edProperty;check(edProperty<count);
 auto first=roll(edProperty,100,150,"[U]"),second=roll(edProperty,20,40,"[Set]");
 ContributionModel::Group overlapping{stats({{16,145}}),{first,second},""};
 auto combined=ContributionModel::Build("+#% Enhanced Defense",16,0,{&overlapping,1},properties);
 check(combined && combined->ownedParts.size()==2 && !combined->ownedParts[0].value && !combined->ownedParts[1].value);
 check(RangeText::ExpandOwned("+145% Enhanced Defense",*combined).find("120% - +190%")!=std::string::npos);
 std::printf("Operation isolation: %u checks, %u failures. ED bounds: %u checks, %u failures. Durability numeric fallback absent: %u/%u. Socket numeric fallback absent: %u/%u.\n",isolation,failed,ed,edFailed,durabilityMissing,durability,socketsMissing,sockets);
 std::printf("Charge source-only identity: %u matching/mismatching checks pass.\n",chargeSources);
 std::printf("Proc/charge packed identity: %u matching and mismatching pairs pass; %u derived/out-of-domain levels require native evidence.\n",packed,derived);
 check(failed==0 && edFailed==0);
}
