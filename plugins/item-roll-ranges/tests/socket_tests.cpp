#include "../src/socket_ias.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
namespace {
void check(bool value) {if (!value) std::exit(1);}
struct Memory {
    std::map<std::uintptr_t,std::vector<unsigned char>> blocks;
    bool Read(std::uintptr_t at,void* out,std::size_t size) const {
        auto found=blocks.upper_bound(at);if (found==blocks.begin()) return false;--found;
        const auto offset=at-found->first;
        if (offset>found->second.size() || size>found->second.size()-offset) return false;
        std::memcpy(out,found->second.data()+offset,size);return true;
    }
    template<class T> void Put(std::uintptr_t at,T value) {
        auto found=blocks.upper_bound(at);check(found!=blocks.begin());--found;
        const auto offset=at-found->first;check(offset+sizeof(T)<=found->second.size());
        std::memcpy(found->second.data()+offset,&value,sizeof(value));
    }
};
constexpr std::uintptr_t host=0x10000,RootAddress=0x20000,mod=0x30000,jewel=0x40000,
    jewelRoot=0x50000,DataAddress=0x60000,InventoryAddress=0x70000,totalEntry=0x80000,
    hostEntry=0x90000,jewelEntry=0xa0000,ControlAddress=0xb0000;
Memory Fixture() {
    Memory m;
    for (const auto p:{host,RootAddress,mod,jewel,jewelRoot,DataAddress,InventoryAddress,totalEntry,hostEntry,jewelEntry,ControlAddress}) m.blocks[p].resize(0x200);
    m.Put(host,4u);m.Put(host+0x88,RootAddress);m.Put(host+0x90,InventoryAddress);
    m.Put(RootAddress+0x1c,0x80000000u);m.Put(RootAddress+0x90,mod);
    m.Put(RootAddress+0xa8,totalEntry);m.Put(RootAddress+0xb0,std::uint64_t{1});m.Put(RootAddress+0xb8,std::uint64_t{1});
    m.Put(mod,host);m.Put(mod+0x1c,0x40u);m.Put(mod+0x18,-1);m.Put(mod+0x78,RootAddress);m.Put(mod+0x68,jewelRoot);
    m.Put(mod+0x30,hostEntry);m.Put(mod+0x38,std::uint64_t{1});m.Put(mod+0x40,std::uint64_t{1});
    m.Put(jewel,4u);m.Put(jewel+4,658u);m.Put(jewel+8,129u);m.Put(jewel+0x10,DataAddress);m.Put(jewel+0x88,jewelRoot);
    m.Put(jewelRoot+0x1c,0x80000000u);m.Put(jewelRoot+0x78,RootAddress);m.Put(jewelRoot+0xa0,jewel);
    m.Put(jewelRoot+0xa8,jewelEntry);m.Put(jewelRoot+0xb0,std::uint64_t{1});m.Put(jewelRoot+0xb8,std::uint64_t{1});
    m.Put(InventoryAddress,0x01020304u);m.Put(InventoryAddress+0x10,jewel);
    for (const auto p:{totalEntry,hostEntry,jewelEntry}) m.Put(p,std::uint64_t{93}<<32);
    m.Put(totalEntry+8,std::int64_t{35});m.Put(hostEntry+8,std::int64_t{20});m.Put(jewelEntry+8,std::int64_t{15});
    return m;
}
auto Capture(const Memory& m) {return SocketIAS::Capture(host,[&](std::uintptr_t at,void* out,std::size_t size) {return m.Read(at,out,size);});}
}
int main() {
    auto m=Fixture();auto result=Capture(m);
    check(result && result->host==20 && result->total==35 && result->items.size()==1 &&
        result->items[0].value==15 && result->items[0].id==129);
    // Variable actual socket values remain separate ownership evidence.
    m.Put(jewelEntry+8,std::int64_t{12});m.Put(totalEntry+8,std::int64_t{32});
    check(Capture(m)->items[0].value==12);
    m=Fixture();m.Put(host+0xa0,ControlAddress);m.Put(ControlAddress+8,1u);m.Put(host+0x98,InventoryAddress);
    m.Put(host+0x90,std::uintptr_t{});check(Capture(m).has_value());
    m.Put(ControlAddress+8,0u);check(!Capture(m)); // Must use the native-selected InventoryAddress.
    m=Fixture();m.Put(jewelRoot+0xa0,host);check(!Capture(m));
    m=Fixture();m.Put(jewel+0x88,RootAddress);check(!Capture(m));
    m=Fixture();m.Put(jewelRoot+0x78,mod);check(!Capture(m));
    m=Fixture();m.Put(jewelRoot+0x68,mod);check(!Capture(m));
    m=Fixture();m.Put(DataAddress+0xb0,jewel);check(!Capture(m));
    m=Fixture();m.Put(InventoryAddress+0x10,host);check(!Capture(m));
    m=Fixture();m.Put(InventoryAddress,0u);check(!Capture(m));
    m=Fixture();m.Put(InventoryAddress+0x10,std::uintptr_t{});check(!Capture(m));
    m=Fixture();m.Put(RootAddress+0xb0,std::uint64_t{2049});check(!Capture(m));
    m=Fixture();m.Put(jewelRoot+0xb8,std::uint64_t{});check(!Capture(m));
    m=Fixture();m.Put(mod+0x20,12u);check(!Capture(m));
    m=Fixture();m.Put(mod+0x18,10);check(!Capture(m));
    m=Fixture();m.Put(totalEntry+8,std::int64_t{36});check(!Capture(m));
    m=Fixture();m.Put(jewelEntry+8,std::int64_t{-15});check(!Capture(m));
    std::puts("Socket IAS ownership, native InventoryAddress selection, total reconciliation and atomic refusal pass.");
}
