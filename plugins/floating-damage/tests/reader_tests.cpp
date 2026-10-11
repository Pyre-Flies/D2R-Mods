#include "native_reader.h"
#include <cstdio>
#include <cstdlib>
using namespace Damage;
void Check(bool ok,const char* text){if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
template<class T> void Put(unsigned char* data,std::size_t at,T value){std::memcpy(data+at,&value,sizeof(value));}
int main(){
    std::array<unsigned char,0x2a30> game{};
    std::array<unsigned char,0x170> unit{};
    std::array<unsigned char,0xc0> stats{};
    std::array<std::uint32_t,2> path{100U*65536,101U*65536};
    struct Stat {std::uint32_t layer,id;std::int64_t value;};
    std::array<Stat,4> values{{{0,6,660},{0,7,3584},{0,172,0},{1,6,9999}}};
    const auto gameAddress=reinterpret_cast<std::uintptr_t>(game.data());
    const auto unitAddress=reinterpret_cast<std::uintptr_t>(unit.data());
    Put(unit.data(),0,1U);Put(unit.data(),4,5U);Put(unit.data(),8,17U);
    Put(unit.data(),0xd8,gameAddress);Put(unit.data(),0x88,reinterpret_cast<std::uintptr_t>(stats.data()));
    Put(unit.data(),0x38,reinterpret_cast<std::uintptr_t>(path.data()));
    Put(stats.data(),0x1c,-1);Put(stats.data(),0xa8,reinterpret_cast<std::uintptr_t>(values.data()));
    Put(stats.data(),0xb0,std::uint64_t{4});Put(stats.data(),0xb8,std::uint64_t{4});
    Put(game.data(),0x2630+17*8,unitAddress);
    std::uintptr_t global=gameAddress;
    const auto base=reinterpret_cast<std::uintptr_t>(&global)-Native::GameGlobalRva;
    Native::Snapshot snapshot{};
    Check(Native::SnapshotGame(base,0,snapshot) && snapshot.count==1,"owned hostile monster is sampled");
    Check(snapshot.monsters[0].hp==660 && snapshot.monsters[0].maximum==3584 && snapshot.monsters[0].position.y==101,"composed 64-bit layer-zero HP and fixed-point position retained");
    values[2].value=2;
    Check(Native::SnapshotGame(base,0,snapshot) && snapshot.count==0,"neutral NPC is excluded");
    values[2].value=1;
    Check(Native::SnapshotGame(base,0,snapshot) && snapshot.count==0,"friendly unit is excluded");
    values[2].value=0;Put(unit.data(),0xd8,gameAddress+8);
    Check(!Native::SnapshotGame(base,0,snapshot),"foreign game ownership refuses snapshot");
    Put(unit.data(),0xd8,gameAddress);Put(unit.data(),0x158,unitAddress);
    Check(!Native::SnapshotGame(base,0,snapshot),"cyclic unit chain refuses snapshot");
    Put(unit.data(),0x158,std::uintptr_t{0});values[0].value=-10;
    Check(Native::SnapshotGame(base,0,snapshot) && snapshot.count==1 && snapshot.monsters[0].hp==0,"negative post-hit HP has a zero health floor");
    Put(stats.data(),0xb0,std::uint64_t{999999});
    Check(Native::SnapshotGame(base,0,snapshot) && snapshot.count==0,"unbounded stat arrays are skipped");
    std::int32_t signedRate{};
    Check(Native::SignedRegen(4294967000LL,signedRate) && signedRate==-296 &&
        Native::SignedRegen(-296,signedRate) && signedRate==-296,"captured zero-extended stat74 and signed cells normalize equally");
    Check(!Native::SignedRegen(1LL<<40,signedRate),"widened out-of-range stat74 is refused rather than truncated");
    std::array<unsigned char,0xa8> effect{};std::array<Stat,1> rate{{{0,74,4294967000LL}}};
    Put(stats.data(),0x90,reinterpret_cast<std::uintptr_t>(effect.data()));
    Put(effect.data(),0x20,2U);Put(effect.data(),0x24,1050.0f);
    Put(effect.data(),0x30,reinterpret_cast<std::uintptr_t>(rate.data()));
    Put(effect.data(),0x38,std::uint64_t{1});Put(effect.data(),0x40,std::uint64_t{1});
    Native::Stats aggregate{};aggregate.regen=4294967000LL;aggregate.hasRegen=true;
    auto poison=Native::ReadPoison(unitAddress,aggregate,1000,true);
    Check(poison.valid && poison.rate==296 && poison.frame==1000 && poison.expires==1050,
        "matching state2, signed composite rate and integer float expiration produce owned evidence");
    aggregate.regen=-200;
    Check(!Native::ReadPoison(unitAddress,aggregate,1000,true).valid,"other regeneration contributions refuse poison prediction");
    aggregate.regen=-296;aggregate.hasRegen=false;
    Check(!Native::ReadPoison(unitAddress,aggregate,1000,true).valid,"absent composite rate refuses poison prediction");
    aggregate.hasRegen=true;Put(effect.data(),0x24,1000.0f);
    Check(!Native::ReadPoison(unitAddress,aggregate,1000,true).valid,"expired state is refused");
    Put(effect.data(),0x24,1050.5f);
    Check(!Native::ReadPoison(unitAddress,aggregate,1000,true).valid,"fractional expiration is refused");
    Put(effect.data(),0x24,1050.0f);
    Check(!Native::ReadPoison(unitAddress,aggregate,1000,false).valid,"missing authoritative frame disables only poison prediction");
    std::array<Stat,1> baseRate{{{0,74,17}}};
    Put(stats.data(),0x30,reinterpret_cast<std::uintptr_t>(baseRate.data()));
    Put(stats.data(),0x38,std::uint64_t{1});Put(stats.data(),0x40,std::uint64_t{1});
    rate[0].value=4294967065LL;aggregate.regen=4294967082LL;
    poison=Native::ReadPoison(unitAddress,aggregate,1000,true);
    Check(poison.valid && poison.rate==231,"captured base17 plus poison negative231 admits full poison rate rather than aggregate214");
    aggregate.regen=4294967083LL;
    Check(!Native::ReadPoison(unitAddress,aggregate,1000,true).valid,"unaccounted extra regeneration contribution is still refused");
    aggregate.regen=4294967082LL;baseRate[0].value=-1;
    Check(!Native::ReadPoison(unitAddress,aggregate,1000,true).valid,"negative base regeneration cannot masquerade as ordinary regeneration");
    baseRate[0].value=17;Put(stats.data(),0x38,std::uint64_t{2049});
    Check(!Native::ReadPoison(unitAddress,aggregate,1000,true).valid,"invalid base array refuses optional prediction");
    std::puts("Server ownership, composed HP, layer filtering, friendliness, cycle and bounds checks pass.");
}
