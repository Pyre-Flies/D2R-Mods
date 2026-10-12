#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <vector>
namespace SocketIAS {
struct Item {std::uint32_t id{},classId{};int value{};};
struct Snapshot {int host{},total{};std::vector<Item> items;};
// Read-only, exact-profile capture. Return no partial proof on malformed lists.
template<class Read> std::optional<Snapshot> Capture(std::uintptr_t host,Read read) {
    auto load=[&](std::uintptr_t at,auto& out) {return at>=0x10000 && read(at,&out,sizeof(out));};
    auto value=[&](std::uintptr_t list,std::size_t offset)->std::optional<int> {
        std::array<std::uint64_t,3> vector{};
        if (!load(list+offset,vector) || vector[1]>2048 || vector[1]>vector[2] ||
            (vector[1] && (vector[0]<0x10000 || vector[0]>0x7fffffffffffULL-vector[1]*16))) return {};
        struct Entry {std::uint64_t key;std::int64_t amount;};
        std::uint64_t previous=0;int result=0;
        for (std::uint64_t n=0;n<vector[1];++n) {
            Entry entry{};
            if (!load(static_cast<std::uintptr_t>(vector[0])+n*16,entry) || (n && entry.key<=previous)) return {};
            previous=entry.key;
            if (entry.key==(std::uint64_t{93}<<32)) {
                if (entry.amount<0 || entry.amount>1000000) return {};
                result=static_cast<int>(entry.amount);
            }
        }
        return result;
    };
    std::uint32_t type{},rootFlags{};std::uintptr_t root{},child{};
    if (!load(host,type) || type!=4 || !load(host+0x88,root) ||
        !load(root+0x1c,rootFlags) || !(rootFlags&0x80000000u) || !load(root+0x90,child)) return {};
    Snapshot result;const auto total=value(root,0xa8);if (!total) return {};result.total=*total;
    struct Owned {std::uintptr_t unit;Item item;};
    std::vector<Owned> owned;std::vector<std::uintptr_t> seen;
    while (child) {
        if (seen.size()>=64 || std::find(seen.begin(),seen.end(),child)!=seen.end()) return {};
        seen.push_back(child);
        std::uintptr_t parent{},owner{};std::uint32_t flags{},state{};
        if (!load(child+0x78,parent) || parent!=root || !load(child+0x1c,flags) ||
            !load(child+0x20,state) || state!=0) return {};
        if (flags&0x80000000u) {
            std::array<std::uint32_t,3> identity{};std::uintptr_t ownerRoot{};
            if (owned.size()>=6 || !load(child+0xa0,owner) || owner==host ||
                !load(owner,identity) || identity[0]!=4 || !load(owner+0x88,ownerRoot) || ownerRoot!=child) return {};
            const auto amount=value(child,0xa8);if (!amount) return {};
            if (std::any_of(owned.begin(),owned.end(),[&](const Owned& item) {return item.unit==owner || item.item.id==identity[2];})) return {};
            owned.push_back({owner,{identity[2],identity[1],*amount}});
        } else {
            int expires{};
            if (!load(child,owner) || owner!=host || !load(child+0x18,expires) || expires!=-1) return {};
            const auto amount=value(child,0x30);if (!amount || result.host>1000000-*amount) return {};
            result.host+=*amount;
        }
        if (!load(child+0x68,child)) return {};
    }
    if (owned.empty()) return {};
    std::uintptr_t control{},inventory{};std::uint32_t refs{},magic{};
    if (!load(host+0xa0,control) || (control && !load(control+8,refs)) ||
        !load(host+(control && refs?0x98:0x90),inventory) || !load(inventory,magic) ||
        magic!=0x01020304 || !load(inventory+0x10,child)) return {};
    seen.clear();long long sum=result.host;
    while (child) {
        if (child==host || seen.size()>=6 || std::find(seen.begin(),seen.end(),child)!=seen.end()) return {};
        seen.push_back(child);std::array<std::uint32_t,3> identity{};std::uintptr_t data{};
        if (!load(child,identity) || identity[0]!=4 || !load(child+0x10,data)) return {};
        auto found=std::find_if(owned.begin(),owned.end(),[&](const Owned& item) {
            return item.unit==child && item.item.classId==identity[1] && item.item.id==identity[2];
        });
        if (found==owned.end()) return {};
        result.items.push_back(found->item);sum+=found->item.value;
        if (!load(data+0xb0,child)) return {};
    }
    if (result.items.size()!=owned.size() || sum!=result.total) return {};
    return result;
}
}
