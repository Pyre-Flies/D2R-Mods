#include "maps_quantity_contract.h"
#include "chat_forwarding_memory.h"
#include <windows.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace QolMapsQuantity;
static unsigned checks{},calls{};
static int64_t nativeValue{};
static const void* nativeUnit{};
static void Check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
static int64_t __fastcall Original(const void* unit,int32_t stat,uint16_t layer) {
    Check(unit==nativeUnit && stat==70 && layer==0,"native reader receives exact unit/stat/layer");++calls;return nativeValue;
}
struct Span {size_t offset,size;Kind kind;};
struct Fixture {
    unsigned char* image{};
    uintptr_t base{},hook{},helper{},ending{},negative{},originalSlot{},levelSlot{},byteTable{},jumpTable{};
    std::vector<Span> spans;
    explicit Fixture(unsigned shift=0) {
        image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x10000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
        Check(image!=nullptr,"private executable fixture");base=reinterpret_cast<uintptr_t>(image);
        hook=base+0x1000+shift;helper=base+0x3000+shift;ending=base+0x5000+shift;negative=base+0x6000+shift;
        originalSlot=base+0x8000;levelSlot=base+0x8010;byteTable=base+0xa000;jumpTable=base+0xb000;
        Put(hook,Profile::Head);Put(hook+0x83,Profile::Gate);Put(helper,Profile::Index);Put(ending,Profile::Return);Put(negative,Profile::Negative);
        Rel(hook+0x19,hook+0x1d,originalSlot);Rel(hook+0x83+0x1a,hook+0x83+0x1f,levelSlot);
        Rel(hook+0x83+0x27,hook+0x83+0x2b,ending);Rel(hook+0x83+0x48,hook+0x83+0x4c,ending);
        Rel(hook+0x83+0x33,hook+0x83+0x37,helper);Rel(helper+0x25,helper+0x29,base);
        const int32_t bt=static_cast<int32_t>(byteTable-base),jt=static_cast<int32_t>(jumpTable-base);
        std::memcpy(reinterpret_cast<void*>(helper+0x2d),&bt,4);std::memcpy(reinterpret_cast<void*>(helper+0x34),&jt,4);
        *reinterpret_cast<uintptr_t*>(originalSlot)=reinterpret_cast<uintptr_t>(&Original);
        *reinterpret_cast<uintptr_t*>(levelSlot)=1;
        *reinterpret_cast<unsigned char*>(byteTable+53)=8;
        *reinterpret_cast<uint32_t*>(jumpTable+32)=static_cast<uint32_t>(negative-base);
        spans.push_back({static_cast<size_t>(originalSlot-base),8,Kind::Data});spans.push_back({static_cast<size_t>(levelSlot-base),8,Kind::Data});
        spans.push_back({static_cast<size_t>(byteTable-base)+53,1,Kind::Code});spans.push_back({static_cast<size_t>(jumpTable-base)+32,4,Kind::Code});
        FlushInstructionCache(GetCurrentProcess(),image,0x10000);
    }
    ~Fixture(){VirtualFree(image,0,MEM_RELEASE);}
    template<size_t N>void Put(uintptr_t address,const unsigned char (&data)[N]){std::memcpy(reinterpret_cast<void*>(address),data,N);spans.push_back({static_cast<size_t>(address-base),N,Kind::Code});}
    static void Rel(uintptr_t operand,uintptr_t end,uintptr_t target){const auto displacement=static_cast<int32_t>(target-end);std::memcpy(reinterpret_cast<void*>(operand),&displacement,4);}
    Memory Reader(){return {this,[](void* user,uintptr_t address,void* out,size_t size,Kind kind) noexcept {
        const auto& f=*static_cast<Fixture*>(user);
        if(address<f.base || address-f.base>0x10000 || size>0x10000-(address-f.base))return false;
        for(const auto& s:f.spans)if(kind==s.kind && address-f.base>=s.offset && address-f.base-s.offset<=s.size && size<=s.size-(address-f.base-s.offset)){
            std::memcpy(out,reinterpret_cast<void*>(address),size);return true;
        }
        return false;
    }};}
    bool Valid(){return Validate(Reader(),base,hook,reinterpret_cast<uintptr_t>(&Original));}
};
static void Artifact(const wchar_t* path,uintptr_t rva) {
    const auto module=LoadLibraryExW(path,nullptr,DONT_RESOLVE_DLL_REFERENCES);
    Check(module!=nullptr,"private companion mapped without running initialization");
    QolChatForwarding::ImageMemory image{};Check(image.Open(module),"mapped companion image admitted");
    const auto base=reinterpret_cast<uintptr_t>(module),hook=base+rva;
    unsigned char head[sizeof(Profile::Head)]{};uintptr_t slot{};
    Check(Block(image.Reader(),hook,Profile::Head,head,{0x19}) && Ref(hook,head,0x19,0x1d,slot),"artifact original slot resolved from instructions");
    // Only this test process's uninitialized private mapping is modified.
    const auto saved=*reinterpret_cast<uintptr_t*>(slot);*reinterpret_cast<uintptr_t*>(slot)=reinterpret_cast<uintptr_t>(&Original);
    Check(Validate(image.Reader(),base,hook,reinterpret_cast<uintptr_t>(&Original)),"production image reader admits artifact quantity path");
    DWORD protection{};Check(VirtualProtect(reinterpret_cast<void*>(hook),sizeof(Profile::Head),PAGE_EXECUTE_READWRITE,&protection)!=0,"fixture protection changed");
    Check(!Validate(image.Reader(),base,hook,reinterpret_cast<uintptr_t>(&Original)),"writable companion code refused despite identical bytes");
    DWORD unused{};VirtualProtect(reinterpret_cast<void*>(hook),sizeof(Profile::Head),protection,&unused);
    *reinterpret_cast<uintptr_t*>(slot)=saved;FreeLibrary(module);
}
int wmain(int argc,wchar_t** argv) {
    Fixture f;Check(f.Valid(),"complete quantity path admitted without file hash or helper RVA");
    const auto read=reinterpret_cast<int64_t(__fastcall*)(const void*,int32_t,uint16_t)>(f.hook);
    for(auto level:{uintptr_t{0},uintptr_t{1}})for(auto unit:{static_cast<const void*>(nullptr),static_cast<const void*>(&f)})for(auto value:{int64_t{0},int64_t{20},int64_t{-1},INT64_MAX}) {
        *reinterpret_cast<uintptr_t*>(f.levelSlot)=level;nativeUnit=unit;nativeValue=value;const auto before=calls;
        Check(f.Valid() && read(unit,70,0)==value && calls==before+1,"executed guarded route preserves original result and calls once");
    }
    for(const auto& s:f.spans) {
        if(s.offset==f.levelSlot-f.base)continue; // Either null or non-null is legitimate; never called.
        for(size_t offset=0;offset<s.size;++offset)for(unsigned bit=0;bit<8;++bit) {
            auto& byte=f.image[s.offset+offset];byte^=1u<<bit;
            Check(!f.Valid(),"mutated operative instruction/reference/table/continuation refused");byte^=1u<<bit;
        }
    }
    f.image[(f.hook-f.base)+0x60]=0xcc;Check(f.Valid(),"unreachable stat-127 body may change independently");
    f.image[0xe000]=0xcc;Check(f.Valid(),"unrelated companion bytes do not affect quantity admission");
    for(unsigned shift=16;shift<256;shift+=16){Fixture relocated(shift);Check(relocated.Valid(),"hook/helper/return locations may relocate with checked references");}
    for(auto& s:f.spans) {const auto kind=s.kind;s.kind=kind==Kind::Code?Kind::Data:Kind::Code;Check(!f.Valid(),"every reachable node requires its original memory role");s.kind=kind;}
    Check(!Validate({},f.base,f.hook,1) && !Validate(f.Reader(),f.base,UINTPTR_MAX-8,1),"missing reader and overflowing addresses refused");
    if(argc==3)Artifact(argv[1],static_cast<uintptr_t>(std::wcstoull(argv[2],nullptr,0)));
    std::printf("Maps quantity contract: %u checks passed; %u native forwarding calls.\n",checks,calls);
}
