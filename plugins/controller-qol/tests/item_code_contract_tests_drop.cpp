#include "item_code_drop_contract.h"
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
        using namespace QolItemCodeDropFix;
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
    bool Valid(){return QolItemCodeDropFix::Validate(Reader(),game,companion);}
};
unsigned checks{};
void Check(bool ok,const char* text) {++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
uintptr_t observed{};unsigned calls{};
__declspec(noinline) uint64_t OriginalCall(uintptr_t item) {observed=item;++calls;return 0xfedcba9876543210ULL;}
void NativeForwarding() {
    auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,0xc0000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(memory!=nullptr,"allocate native forwarding fixture");
    std::memcpy(memory+QolItemCodeDropFix::HookRva,QolItemCodeDropFix::Wrapper,sizeof(QolItemCodeDropFix::Wrapper));
    const auto original=reinterpret_cast<uintptr_t>(&OriginalCall);
    std::memcpy(memory+QolItemCodeDropFix::SlotRva,&original,sizeof(original));
    DWORD old{};Check(VirtualProtect(memory+QolItemCodeDropFix::HookRva,sizeof(QolItemCodeDropFix::Wrapper),PAGE_EXECUTE_READ,&old)!=0,"protect wrapper code");
    Check(FlushInstructionCache(GetCurrentProcess(),memory+QolItemCodeDropFix::HookRva,sizeof(QolItemCodeDropFix::Wrapper))!=0,"publish fixture instructions");
    const auto forward=reinterpret_cast<uint64_t(*)(uintptr_t)>(memory+QolItemCodeDropFix::HookRva);
    for(const uintptr_t item:{uintptr_t{0},uintptr_t{0x123456789abcdef0ULL},reinterpret_cast<uintptr_t>(&observed)}) {
        calls=0;observed=0;
        Check(forward(item)==0xfedcba9876543210ULL,"wrapper preserves full return value");
        Check(calls==1 && observed==item,"wrapper calls original exactly once with same item");
    }
    VirtualFree(memory,0,MEM_RELEASE);
}
int main() {
    Fixture live;Check(live.Valid(),"captured UnHoarder graph accepted");
    for(size_t block=0;block<live.blocks.size();++block) {
        for(size_t byte=0;byte<live.blocks[block].bytes.size();++byte) {
            Fixture changed;changed.blocks[block].bytes[byte]^=1;
            Check(!changed.Valid(),"every changed graph byte refused");
        }
        Fixture wrongOwner;wrongOwner.blocks[block].kind=static_cast<Kind>(99);
        Check(!wrongOwner.Valid(),"wrong memory ownership or protection refused");
        Fixture missing;missing.blocks.erase(missing.blocks.begin()+static_cast<ptrdiff_t>(block));
        Check(!missing.Valid(),"unreadable graph node refused");
    }
    Fixture pristine;std::memcpy(pristine.blocks[0].bytes.data(),QolItemCodeDropFix::Original,sizeof(QolItemCodeDropFix::Original));
    Check(!pristine.Valid(),"pristine entry belongs to unchanged original admission");
    Check(!QolItemCodeDropFix::Validate({},live.game,live.companion),"missing reader refused");
    Check(!QolItemCodeDropFix::Validate(live.Reader(),0,live.companion),"null game refused");
    Check(!QolItemCodeDropFix::Validate(live.Reader(),live.game,0),"null owner refused");
    Check(!QolItemCodeDropFix::Validate(live.Reader(),UINTPTR_MAX,live.companion),"game overflow refused");
    Check(!QolItemCodeDropFix::Validate(live.Reader(),live.game,UINTPTR_MAX),"owner overflow refused");
    // Rechecking is required: a previously valid graph does not bless changes.
    Check(live.Valid(),"valid before mutation");live.blocks[4].bytes[11]^=1;
    Check(!live.Valid(),"later continuation mutation refused");
    NativeForwarding();
    std::printf("%u item-code graph checks passed\n",checks);
}
