#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <algorithm>

namespace TerrainProbe::GroundSupply {
inline constexpr unsigned MaxInput=64,MaxNodes=256,Side=22;
struct View {uintptr_t data{},count{};bool operator==(const View&) const=default;};
struct Rect {int32_t x{},y{},w{},h{};bool operator==(const Rect&) const=default;};
struct Node {
    uintptr_t wrapper{},room{},levelPointer{},gate{};
    Rect rect{};uint32_t level{};
    bool operator==(const Node&) const=default;
};
struct Neighbors {
    uintptr_t data{};uint32_t count{};
    std::array<uintptr_t,64> values{};
    bool operator==(const Neighbors&) const=default;
};
struct Snapshot {
    View source{};std::array<int32_t,2> origin{};
    std::array<uintptr_t,MaxNodes> selected{};
    std::array<Node,MaxNodes> nodes{};
    std::array<Neighbors,MaxNodes> neighbors{};
    std::array<uint32_t,MaxInput> levels{};
    unsigned nodeCount{},count{},levelCount{};
    bool operator==(const Snapshot&) const=default;
};
enum class Status {Refused,Complete,Unstable};
inline bool Address(uintptr_t address,size_t size) noexcept {
    return address>=0x10000 && size && size<=512 && address<=0x7fffffffffffULL-size;
}
inline bool Valid(const Rect& r) noexcept {
    return r.x>=-1000000 && r.x<=1000000 && r.y>=-1000000 && r.y<=1000000 &&
        r.w>0 && r.w<=4096 && r.h>0 && r.h<=4096;
}
inline bool Intersects(const Rect& r,const std::array<int32_t,2>& origin) noexcept {
    return r.x<static_cast<int64_t>(origin[0])+Side && static_cast<int64_t>(r.x)+r.w>origin[0] &&
           r.y<static_cast<int64_t>(origin[1])+Side && static_cast<int64_t>(r.y)+r.h>origin[1];
}
template<class Read> Status Build(uintptr_t base,uintptr_t viewAddress,Read&& raw,Snapshot& out) noexcept {
    out={};
    const auto read=[&](uintptr_t address,void* data,size_t size) noexcept {
        return Address(address,size) && raw(address,data,size);
    };
    if(!read(viewAddress,&out.source,sizeof(View)) || !out.source.count || out.source.count>MaxInput ||
       !read(out.source.data,out.selected.data(),static_cast<size_t>(out.source.count)*sizeof(uintptr_t)) ||
       !read(base+0x34ff608,out.origin.data(),sizeof(out.origin))) return Status::Refused;
    for(auto c:out.origin) if(c < -1000000 || c > 1000000) return Status::Refused;
    out.count=static_cast<unsigned>(out.source.count);
    const auto node=[&](uintptr_t wrapper) noexcept -> const Node* {
        for(unsigned i=0;i<out.nodeCount;++i) if(out.nodes[i].wrapper==wrapper) return &out.nodes[i];
        if(out.nodeCount==MaxNodes || !Address(wrapper,0x48)) return nullptr;
        auto& n=out.nodes[out.nodeCount];n.wrapper=wrapper;
        if(!read(wrapper+0x18,&n.room,8) || !Address(n.room,0x98) ||
           !read(n.room+0x60,&n.rect,sizeof(Rect)) || !Valid(n.rect) ||
           !read(n.room+0x88,&n.gate,8) || !read(n.room+0x90,&n.levelPointer,8) ||
           !Address(n.levelPointer,0x200) || !read(n.levelPointer+0x1f8,&n.level,4)) return nullptr;
        ++out.nodeCount;return &n;
    };
    // Admit only levels already represented in the native input. Preserve mixed
    // views and their order without activating or discovering another level.
    for(unsigned i=0;i<out.source.count;++i) {
        const auto n=node(out.selected[i]);
        if(!n) return Status::Refused;
        if(!n->level || n->level>4096) return Status::Refused;
        if(std::find(out.levels.begin(),out.levels.begin()+out.levelCount,n->level)==out.levels.begin()+out.levelCount)
            out.levels[out.levelCount++]=n->level;
    }
    // Expand up to 2 hops from the supplied view, strictly clipped to already-represented
    // levels, existing gate pointers, and the 22x22 cache box.
    const unsigned hop0End=static_cast<unsigned>(out.source.count);
    for(unsigned i=0;i<hop0End;++i) {
        auto& edge=out.neighbors[i];const auto wrapper=out.selected[i];
        if(!read(wrapper,&edge.data,8) || !read(wrapper+0x40,&edge.count,4) || edge.count>64 ||
           (edge.count && !read(edge.data,edge.values.data(),edge.count*8ULL))) return Status::Refused;
        for(unsigned j=0;j<edge.count;++j) {
            const auto candidate=node(edge.values[j]);if(!candidate) return Status::Refused;
            if(std::find(out.levels.begin(),out.levels.begin()+out.levelCount,candidate->level)==out.levels.begin()+out.levelCount ||
               !candidate->gate || !Intersects(candidate->rect,out.origin)) continue;
            if(std::find(out.selected.begin(),out.selected.begin()+out.count,candidate->wrapper)!=out.selected.begin()+out.count) continue;
            if(out.count==MaxNodes) return Status::Refused;
            out.selected[out.count++]=candidate->wrapper;
        }
    }
    const unsigned hop1End=out.count;
    for(unsigned i=hop0End;i<hop1End;++i) {
        auto& edge=out.neighbors[i];const auto wrapper=out.selected[i];
        if(!read(wrapper,&edge.data,8) || !read(wrapper+0x40,&edge.count,4) || edge.count>64 ||
           (edge.count && !read(edge.data,edge.values.data(),edge.count*8ULL))) return Status::Refused;
        for(unsigned j=0;j<edge.count;++j) {
            const auto candidate=node(edge.values[j]);if(!candidate) return Status::Refused;
            if(std::find(out.levels.begin(),out.levels.begin()+out.levelCount,candidate->level)==out.levels.begin()+out.levelCount ||
               !candidate->gate || !Intersects(candidate->rect,out.origin)) continue;
            if(std::find(out.selected.begin(),out.selected.begin()+out.count,candidate->wrapper)!=out.selected.begin()+out.count) continue;
            if(out.count==MaxNodes) return Status::Refused;
            out.selected[out.count++]=candidate->wrapper;
        }
    }
    return Status::Complete;
}

// Integer argument registers and the full RAX are forwarded unchanged, except
// RDX points to our synchronous private view when two complete copies agree.
template<class Read,class Original>
uintptr_t Forward(bool permitted,uintptr_t base,uintptr_t owner,uintptr_t view,
                  uintptr_t arg3,uintptr_t arg4,Read&& read,Original&& original,
                  unsigned& added,Status& status) noexcept {
    added=0;status=Status::Refused;
    if(!permitted) return original(owner,view,arg3,arg4);
    Snapshot a{},b{};
    status=Build(base,view,read,a);
    if(status!=Status::Complete) return original(owner,view,arg3,arg4);
    if(Build(base,view,read,b)!=Status::Complete) {
        status=Status::Refused;return original(owner,view,arg3,arg4);
    }
    if(!(a==b)) {status=Status::Unstable;return original(owner,view,arg3,arg4);}
    added=a.count-static_cast<unsigned>(a.source.count);
    if(!added) return original(owner,view,arg3,arg4);
    const View expanded{reinterpret_cast<uintptr_t>(a.selected.data()),a.count};
    return original(owner,reinterpret_cast<uintptr_t>(&expanded),arg3,arg4);
}
}
