#include "ground_supply_model.h"
#include "ground_supply_budget.h"
#include <map>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace TerrainProbe::GroundSupply;
namespace {
void Check(bool ok,const char* message) {if(!ok){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
struct Memory {
    std::map<uintptr_t,std::vector<unsigned char>> fields;
    unsigned viewReads{};bool moving{};
    template<class T> void Put(uintptr_t address,const T& value) {
        const auto begin=reinterpret_cast<const unsigned char*>(&value);
        fields[address]={begin,begin+sizeof(T)};
    }
    bool Read(uintptr_t address,void* output,size_t size) noexcept {
        if(address==0x10000 && ++viewReads==2 && moving) Put(0x30000+0x1060,Rect{912,1113,8,8});
        const auto it=fields.find(address);if(it==fields.end() || it->second.size()!=size) return false;
        std::memcpy(output,it->second.data(),size);return true;
    }
    void Node(uintptr_t wrapper,Rect rect,uint32_t level=2,uintptr_t gate=0x90000) {
        Put(wrapper,uintptr_t{0});Put(wrapper+0x40,uint32_t{0});
        Put(wrapper+0x18,wrapper+0x1000);Put(wrapper+0x1060,rect);
        Put(wrapper+0x1088,gate);Put(wrapper+0x1090,wrapper+0x2000);Put(wrapper+0x21f8,level);
    }
    Memory() {
        Put(0x10000,View{0x11000,1});Put(0x11000,uintptr_t{0x20000});
        Put(0x140000000ULL+0x34ff608,std::array<int32_t,2>{907,1119});
        Node(0x20000,{912,1120,8,8});Node(0x30000,{912,1112,8,8});
        Node(0x40000,{920,1112,8,8});Node(0x50000,{896,1112,8,8},1);
        Node(0x60000,{912,1104,8,8});Node(0x70000,{928,1120,8,8},2,0);
        Put(0x20000,uintptr_t{0x22000});Put(0x20040,uint32_t{7});
        Put(0x22000,std::array<uintptr_t,7>{0x20000,0x30000,0x40000,0x30000,0x50000,0x60000,0x70000});
    }
};
void Run(Memory& memory,bool permitted,unsigned expectedAdded,Status expectedStatus) {
    unsigned calls{},added{};Status status{};
    const auto original=[&](uintptr_t owner,uintptr_t view,uintptr_t third,uintptr_t fourth) noexcept -> uintptr_t {
        ++calls;Check(owner==0x123456789abcdef0ULL && third==0xfedcba9876543210ULL && fourth==0x8877665544332211ULL,"integer arguments");
        if(expectedAdded) {
            Check(view!=0x10000,"private view used");
            const auto& v=*reinterpret_cast<const View*>(view);
            Check(v.count==1+expectedAdded,"expanded count");
            const auto entries=reinterpret_cast<const uintptr_t*>(v.data);
            Check(entries[0]==0x20000 && entries[1]==0x30000 && entries[2]==0x40000,"original order plus unique same-level overlaps");
        } else Check(view==0x10000,"original view forwarded exactly");
        return 0xf1e2d3c4b5a69788ULL;
    };
    const auto result=Forward(permitted,0x140000000ULL,0x123456789abcdef0ULL,0x10000,
        0xfedcba9876543210ULL,0x8877665544332211ULL,
        [&](uintptr_t address,void* data,size_t size) noexcept {return memory.Read(address,data,size);},original,added,status);
    Check(calls==1 && result==0xf1e2d3c4b5a69788ULL,"one original call and complete return");
    Check(added==expectedAdded && status==expectedStatus,"selection status");
    Check(memory.fields[0x10000].size()==sizeof(View),"source view retained");
    View source{};std::memcpy(&source,memory.fields[0x10000].data(),sizeof(source));
    Check(source==View{0x11000,1},"source view not modified");
}
void TimedForward(bool expireSecondSnapshot) {
    Memory memory;int64_t now=15900;unsigned calls{};
    const auto clock=[&](int64_t& value) noexcept {value=now;return true;};
    const auto raw=[&](uintptr_t address,void* data,size_t size) noexcept {
        const bool ok=memory.Read(address,data,size);
        // Crossing a coarse 16ms boundary in 200us must not expire an 8ms budget.
        // A genuinely slow read in the second snapshot must still refuse.
        if(address==0x10000) now=expireSecondSnapshot && memory.viewReads==2 ? 23901 : 16100;
        return ok;
    };
    BudgetReader read{clock,raw,int64_t{15900},int64_t{8000},int64_t{15900}};
    unsigned added{};Status status{};
    const auto result=Forward(true,0x140000000ULL,0,0x10000,0,0,read,
        [&](uintptr_t,uintptr_t view,uintptr_t,uintptr_t) noexcept -> uintptr_t {
            ++calls;Check(expireSecondSnapshot ? view==0x10000 : view!=0x10000,"timed forwarding view");return 0xfeed;
        },added,status);
    Check(calls==1 && result==0xfeed,"timed forwarding exactly once");
    Check(added==(expireSecondSnapshot ? 0u : 2u),"timed selection");
    Check(status==(expireSecondSnapshot ? Status::Refused : Status::Complete),"timed status");
    Check(read.failure==(expireSecondSnapshot ? ReadFailure::Budget : ReadFailure::None),"timed refusal reason");
}
void BudgetChecks() {
    int64_t now=100;bool available=true,readable=true;unsigned reads{};
    const auto clock=[&](int64_t& value) noexcept {value=now;return available;};
    const auto raw=[&](uintptr_t,void*,size_t) noexcept {++reads;return readable;};
    BudgetReader reader{clock,raw,int64_t{100},int64_t{80},int64_t{100}};
    now=180;Check(reader(0,nullptr,1),"exact budget boundary admitted");
    now=181;Check(!reader(0,nullptr,1) && reads==1 && reader.failure==ReadFailure::Budget,"expiry prevents read");
    now=100;Check(!reader(0,nullptr,1) && reads==1,"failure remains sticky");
    BudgetReader backwards{clock,raw,int64_t{100},int64_t{80},int64_t{100}};
    now=99;Check(!backwards(0,nullptr,1) && backwards.failure==ReadFailure::Clock,"backward counter refused");
    BudgetReader missing{clock,raw,int64_t{100},int64_t{80},int64_t{100}};
    available=false;Check(!missing(0,nullptr,1) && missing.failure==ReadFailure::Clock,"failed counter refused");
    BudgetReader memory{clock,raw,int64_t{100},int64_t{80},int64_t{100}};
    available=true;now=100;readable=false;
    Check(!memory(0,nullptr,1) && memory.failure==ReadFailure::Memory,"failed OS read classified");
}
void MixedLevels() {
    Memory memory;
    const std::array<uintptr_t,3> input{0x20000,0x80000,0x20000};
    memory.Put(0x10000,View{0x11000,input.size()});memory.Put(0x11000,input);
    memory.Put(0x221f8,uint32_t{8});memory.Put(0x321f8,uint32_t{40});memory.Put(0x421f8,uint32_t{8});
    memory.Node(0x80000,{912,1120,8,8},40);
    memory.Node(0x90000,{912,1112,8,8},109);
    memory.Put(0x80000,uintptr_t{0x82000});memory.Put(0x80040,uint32_t{2});
    memory.Put(0x82000,std::array<uintptr_t,2>{0x40000,0x90000});
    unsigned calls{},added{};Status status{};
    Forward(true,0x140000000ULL,0,0x10000,0,0,
        [&](uintptr_t address,void* data,size_t size) noexcept {return memory.Read(address,data,size);},
        [&](uintptr_t,uintptr_t view,uintptr_t,uintptr_t) noexcept -> uintptr_t {
            ++calls;Check(view!=0x10000,"mixed view extended");
            const auto& v=*reinterpret_cast<const View*>(view);Check(v.count==5,"mixed count");
            const auto entries=reinterpret_cast<const uintptr_t*>(v.data);
            Check(std::equal(input.begin(),input.end(),entries),"original mixed order and duplicates preserved");
            Check(entries[3]==0x30000 && entries[4]==0x40000,"only already represented levels admitted");
            return 0;
        },added,status);
    Check(calls==1 && added==2 && status==Status::Complete,"mixed-level forwarding");
}
void SecondHopExpansion() {
    Memory memory;
    // 0xA0000 is a 2-hop neighbor (connected via 0x40000) that still intersects the 22x22 cache box
    memory.Node(0xa0000,{920,1120,8,8},2);
    memory.Put(0x40000,uintptr_t{0x42000});memory.Put(0x40040,uint32_t{2});
    memory.Put(0x42000,std::array<uintptr_t,2>{0x40000,0xa0000});
    unsigned calls{},added{};Status status{};
    Forward(true,0x140000000ULL,0,0x10000,0,0,
        [&](uintptr_t address,void* data,size_t size) noexcept {return memory.Read(address,data,size);},
        [&](uintptr_t,uintptr_t view,uintptr_t,uintptr_t) noexcept -> uintptr_t {
            ++calls;
            const auto& v=*reinterpret_cast<const View*>(view);Check(v.count==4,"2-hop count");
            const auto entries=reinterpret_cast<const uintptr_t*>(v.data);
            Check(entries[0]==0x20000 && entries[1]==0x30000 && entries[2]==0x40000 && entries[3]==0xa0000,"2-hop neighbor added");
            return 0;
        },added,status);
    Check(calls==1 && added==3 && status==Status::Complete,"2-hop expansion inside 22x22 box");
}
}
int main() {
    Memory normal;Run(normal,true,2,Status::Complete);
    Memory inactive;Run(inactive,false,0,Status::Refused);Check(inactive.viewReads==0,"inactive makes no reads");
    Memory changed;changed.moving=true;Run(changed,true,0,Status::Unstable);
    Memory other;other.Put(0x221f8,uint32_t{1});Run(other,true,0,Status::Complete);
    for(const uint32_t level:{1u,3u,8u,40u,74u,109u,264u}) {
        Memory area;area.Put(0x221f8,level);area.Put(0x321f8,level);area.Put(0x421f8,level);
        Run(area,true,2,Status::Complete);
    }
    Memory badLevel;badLevel.Put(0x221f8,uint32_t{0});Run(badLevel,true,0,Status::Refused);
    Memory hugeLevel;hugeLevel.Put(0x221f8,uint32_t{4097});Run(hugeLevel,true,0,Status::Refused);
    Memory oversized;oversized.Put(0x20040,uint32_t{65});Run(oversized,true,0,Status::Refused);
    Memory invalid;invalid.Put(0x31060,Rect{912,1112,-8,8});Run(invalid,true,0,Status::Refused);
    Memory unreadable;unreadable.fields.erase(0x31018);unreadable.fields.erase(0x30018);Run(unreadable,true,0,Status::Refused);
    Check(!Intersects({929,1119,8,8},{907,1119}),"half-open far edge excluded");
    Check(Intersects({928,1119,8,8},{907,1119}),"partial overlap included");
    Check(!Address(UINTPTR_MAX-4,8),"overflow refused");
    TimedForward(false);TimedForward(true);BudgetChecks();MixedLevels();SecondHopExpansion();
    std::puts("Ground supply: selection, stability, failure forwarding and ABI fixtures passed.");
}
