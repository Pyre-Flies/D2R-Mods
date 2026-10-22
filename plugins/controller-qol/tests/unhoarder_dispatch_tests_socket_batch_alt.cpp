#include "unhoarder_dispatch_socket_batch_alt_contract.h"
#include "loot_approach_profile.h"
#include "loot_run_policy.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <windows.h>
using QolChatForwarding::Kind;
struct Block {uintptr_t address;Kind kind;std::vector<unsigned char> bytes;};
struct Fixture {
    uintptr_t game=0x140000000, companion=0x7ff98b8d0000, relay=0x13fff0113, trampoline=0x13fff0100;
    std::vector<Block> blocks;
    Fixture() {
        using namespace QolItemCodeSocketBatchAlt;
        blocks={{game+Rva,Kind::Trampoline,{Original,Original+sizeof(Original)}},
                {relay,Kind::Trampoline,std::vector<unsigned char>(14)},
                {companion+HookRva,Kind::Code,{Wrapper,Wrapper+sizeof(Wrapper)}},
                {companion+SlotRva,Kind::Data,std::vector<unsigned char>(8)},
                {trampoline,Kind::Trampoline,std::vector<unsigned char>(19)}};
        blocks[0].bytes[0]=0xe9;
        const int32_t rel=static_cast<int32_t>(static_cast<int64_t>(relay)-static_cast<int64_t>(game+Rva+5));
        std::memcpy(blocks[0].bytes.data()+1,&rel,4);
        std::memcpy(blocks[1].bytes.data(),Jump,6);const auto hook=companion+HookRva;
        std::memcpy(blocks[1].bytes.data()+6,&hook,8);
        std::memcpy(blocks[3].bytes.data(),&trampoline,8);
        std::memcpy(blocks[4].bytes.data(),Original,5);std::memcpy(blocks[4].bytes.data()+5,Jump,6);
        const auto resume=game+Rva+5;std::memcpy(blocks[4].bytes.data()+11,&resume,8);
    }
    QolChatForwarding::Memory Reader() {
        return {this,[](void* user,uintptr_t address,void* out,size_t count,Kind kind) noexcept {
            for(const auto& b:static_cast<Fixture*>(user)->blocks)
                if(b.kind==kind && address==b.address && count==b.bytes.size()) {
                    std::memcpy(out,b.bytes.data(),count);return true;
                }
            return false;
        }};
    }
    bool Valid(){return QolItemCodeSocketBatchAlt::Validate(Reader(),game,companion);}
};

struct DispatchFixture:Fixture {
    uintptr_t dispatchRelay=0x13ffe0113,dispatchTrampoline=0x13ffe0100;
    DispatchFixture() {
        using namespace QolUnhoarderDispatchSocketBatchAlt;
        const auto* original=QolClientLoot::ApproachProfile::Dispatch;
        const auto size=sizeof(QolClientLoot::ApproachProfile::Dispatch);
        blocks.push_back({game+Rva,Kind::Trampoline,{original,original+size}});
        blocks.push_back({dispatchRelay,Kind::Trampoline,std::vector<unsigned char>(14)});
        blocks.push_back({companion+HookRva,Kind::Code,{Wrapper,Wrapper+sizeof(Wrapper)}});
        blocks.push_back({companion+SlotRva,Kind::Data,std::vector<unsigned char>(8)});
        blocks.push_back({dispatchTrampoline,Kind::Trampoline,std::vector<unsigned char>(19)});
        blocks.push_back({companion+BaseRva,Kind::Data,std::vector<unsigned char>(8)});
        blocks[5].bytes[0]=0xe9;
        const int32_t rel=static_cast<int32_t>(dispatchRelay-(game+Rva+5));
        std::memcpy(blocks[5].bytes.data()+1,&rel,4);
        std::memcpy(blocks[6].bytes.data(),QolItemCodeSocketBatchAlt::Jump,6);
        const auto hook=companion+HookRva;
        std::memcpy(blocks[6].bytes.data()+6,&hook,8);
        std::memcpy(blocks[8].bytes.data(),&dispatchTrampoline,8);
        std::memcpy(blocks[9].bytes.data(),original,5);
        std::memcpy(blocks[9].bytes.data()+5,QolItemCodeSocketBatchAlt::Jump,6);
        const auto resume=game+Rva+5;
        std::memcpy(blocks[9].bytes.data()+11,&resume,8);
        std::memcpy(blocks[10].bytes.data(),&game,8);
    }
    bool Valid(){return QolUnhoarderDispatchSocketBatchAlt::Validate(Reader(),game,companion,
        QolClientLoot::ApproachProfile::Dispatch,sizeof(QolClientLoot::ApproachProfile::Dispatch));}
};
unsigned checks{},calls{},helpers{},forgets{};
void* actor{};unsigned seenAction{},seenType{},seenGuid{};bool allow{};
void Check(bool ok,const char* message){++checks;if(!ok){std::printf("FAIL %s\n",message);std::exit(1);}}
void Original(unsigned action,void* player,unsigned type,unsigned guid){++calls;seenAction=action;actor=player;seenType=type;seenGuid=guid;}
bool Helper(unsigned,void*,unsigned,unsigned){++helpers;return allow;}
void Forget(unsigned){++forgets;}
void NativeWrapper() {
    auto* memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,0xec000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(memory!=nullptr,"native wrapper allocation");
    using namespace QolUnhoarderDispatchSocketBatchAlt;
    std::memcpy(memory+HookRva,Wrapper,sizeof(Wrapper));
    const auto original=reinterpret_cast<uintptr_t>(&Original);
    std::memcpy(memory+SlotRva,&original,8);
    for(const auto item:{std::pair<uintptr_t,uintptr_t>{0x295d0,reinterpret_cast<uintptr_t>(&Helper)},
                        std::pair<uintptr_t,uintptr_t>{0x1ebb0,reinterpret_cast<uintptr_t>(&Forget)}}) {
        std::memcpy(memory+item.first,QolItemCodeSocketBatchAlt::Jump,6);std::memcpy(memory+item.first+6,&item.second,8);
        DWORD old{};Check(VirtualProtect(memory+(item.first&~uintptr_t(0xfff)),0x1000,PAGE_EXECUTE_READ,&old)!=0,"helper RX");
    }
    DWORD old{};Check(VirtualProtect(memory+HookRva,sizeof(Wrapper),PAGE_EXECUTE_READ,&old)!=0,"wrapper RX");
    Check(FlushInstructionCache(GetCurrentProcess(),memory,0xec000)!=0,"fixture cache flush");
    const auto wrapper=reinterpret_cast<void(*)(unsigned,void*,unsigned,unsigned)>(memory+HookRva);
    int unit{};
    for(const unsigned action:{2u,4u}) {
        calls=helpers=forgets=0;wrapper(action,&unit,4,42);
        Check(calls==1 && helpers==0 && forgets==0,"movement forwards once without filtering");
        Check(actor==&unit && seenAction==action && seenType==4 && seenGuid==42,"all movement arguments preserved");
    }
    calls=helpers=forgets=0;allow=false;wrapper(22,&unit,4,42);
    Check(calls==0 && helpers==1 && forgets==0,"hidden item is still blocked by companion");
    allow=true;wrapper(22,&unit,4,42);
    Check(calls==1 && helpers==2 && forgets==1,"visible item forwards once after companion helper");
    Check(actor==&unit && seenAction==22 && seenType==4 && seenGuid==42,"pickup arguments preserved");
    VirtualFree(memory,0,MEM_RELEASE);
}
int main(){
    DispatchFixture live;Check(live.Valid(),"complete dispatch and item-code graph accepted");
    for(size_t block=0;block<live.blocks.size();++block) {
        for(size_t byte=0;byte<live.blocks[block].bytes.size();++byte) {
            DispatchFixture changed;changed.blocks[block].bytes[byte]^=1;
            Check(!changed.Valid(),"every changed graph byte refused");
        }
        DispatchFixture wrong;wrong.blocks[block].kind=static_cast<Kind>(99);
        Check(!wrong.Valid(),"foreign ownership/protection refused");
        DispatchFixture missing;missing.blocks.erase(missing.blocks.begin()+static_cast<ptrdiff_t>(block));
        Check(!missing.Valid(),"unreadable graph node refused");
    }
    for(uint64_t low=0;low<256;++low) {
        const auto packed=0x123456789abc0000ULL|low;
        Check(QolLootRun::Packed(packed,2)==packed,"native walk preserved");
        Check(QolLootRun::Packed(packed,4)==(packed|0x100),"only high-byte run choice changes");
        Check(QolLootRun::Packed(packed|0x8000,4)==(packed|0x8000),"native nonzero run flag preserved");
    }
    NativeWrapper();
    std::printf("%u dispatch graph, ownership, packed flag and native forwarding checks passed.\n",checks);
}
