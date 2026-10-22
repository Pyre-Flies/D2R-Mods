#include "compatibility.h"
#include "preview.h"
#include "native_fixture.h"
#include <algorithm>

namespace {
struct Image {
    unsigned char* base{};
    explicit Image(size_t size) {
        base=static_cast<unsigned char*>(VirtualAlloc(nullptr,size,MEM_RESERVE,PAGE_NOACCESS));
        Check(base!=nullptr,"guard fixture image reservation");
    }
    ~Image(){VirtualFree(base,0,MEM_RELEASE);}
    uintptr_t Address() const {return reinterpret_cast<uintptr_t>(base);}
    void Put(uintptr_t rva,const void* bytes,size_t size) {
        const auto page=rva&~uintptr_t(4095);
        Check(VirtualAlloc(base+page,(rva-page+size+4095)&~size_t(4095),MEM_COMMIT,PAGE_READWRITE)!=nullptr,"guard fixture page commitment");
        std::memcpy(base+rva,bytes,size);
    }
    template<size_t N> void Put(uintptr_t rva,const std::array<unsigned char,N>& bytes) {Put(rva,bytes.data(),N);}
    void Pointer(uintptr_t rva,uintptr_t value){Put(rva,&value,sizeof(value));}
};
}
int main() {
    using namespace DayNight;
    using namespace Profile;
    Image host(0x4000000),core(0x800000);
    core.Put(CoreWrapperRva,CoreWrapper);core.Put(NoDebugDescriptorRva,NoDebugDescriptor);core.Put(SetterDescriptorRva,SetterDescriptor);
    host.Put(GameCallerRva,GameCaller);host.Put(GameThunkRva,GameThunk);host.Put(NativeSetterRva,NativeSetter);
    core.Pointer(CoreNoDebugPointerRva,host.Address()+NativeNoDebugRva);
    core.Pointer(CoreSetterPointerRva,host.Address()+NativeSetterRva);
    host.Pointer(GameThunkSlotRva,core.Address()+CoreWrapperRva);
    host.Put(RendererGetterRva,RendererGetter);host.Put(RendererOverrideControlRva,RendererOverrideControl);
    host.Put(ScriptSetterCallRva,ScriptSetterCall);host.Put(NormalCycleTableRva,NormalCycleTable);
    Preview preview;Check(preview.Start(host.Address(),core.Address()),"preview code admission");
    for(const auto [rva,size]:{std::pair{RendererGetterRva,RendererGetter.size()},std::pair{RendererOverrideControlRva,RendererOverrideControl.size()},std::pair{ScriptSetterCallRva,ScriptSetterCall.size()},std::pair{NormalCycleTableRva,NormalCycleTable.size()}}) {
        for(size_t i=0;i<size;++i) {
            auto& byte=host.base[rva+i];byte^=1;
            Preview changed;Check(!changed.Start(host.Address(),core.Address()),"every changed preview witness byte refuses");byte^=1;
        }
    }
    Check(NativeGuards(host.Address(),core.Address()),"complete native admission");
    for(const auto [address,size]:{
            std::pair{core.Address()+CoreWrapperRva,CoreWrapper.size()},
            std::pair{core.Address()+NoDebugDescriptorRva,NoDebugDescriptor.size()},
            std::pair{core.Address()+SetterDescriptorRva,SetterDescriptor.size()},
            std::pair{host.Address()+GameCallerRva,GameCaller.size()},
            std::pair{host.Address()+GameThunkRva,GameThunk.size()},
            std::pair{host.Address()+NativeSetterRva,NativeSetter.size()}}) {
        for(size_t i=0;i<size;++i) {
            auto& byte=*reinterpret_cast<unsigned char*>(address+i);byte^=1;
            Check(!NativeGuards(host.Address(),core.Address()),"every changed witness byte refuses");byte^=1;
        }
    }
    for(const auto address:{core.Address()+CoreNoDebugPointerRva,core.Address()+CoreSetterPointerRva,host.Address()+GameThunkSlotRva}) {
        auto& pointer=*reinterpret_cast<uintptr_t*>(address);++pointer;
        Check(!NativeGuards(host.Address(),core.Address()),"unexpected predecessor or stale binding refuses");--pointer;
    }
    Check(!NativeGuards(0,core.Address()) && !NativeGuards(host.Address(),0) && !NativeGuards(1,2),"missing/unreadable images refuse without fault");

    Fixture::NativePath fixture;
    auto wrong=fixture.before;wrong[0]^=1;
    Check(fixture.patch.Install(fixture.PatchAddress(),wrong,fixture.after)==PatchResult::Refused,"unknown modified block refuses");
    Check(fixture.patch.Install(fixture.PatchAddress()+1,fixture.before,fixture.after)==PatchResult::Refused,"unaligned publication refuses");
    Check(fixture.patch.Install(fixture.PatchAddress(),fixture.before,fixture.after)==PatchResult::Applied,"owned publication");
    Check(fixture.patch.Install(fixture.PatchAddress(),fixture.before,fixture.after)==PatchResult::Refused,"duplicate load refuses");
    MEMORY_BASIC_INFORMATION memory{};VirtualQuery(reinterpret_cast<void*>(fixture.PatchAddress()),&memory,sizeof(memory));
    Check(memory.Protect==PAGE_EXECUTE_READ,"publication restores code protection");
    // Simulate a later owner atomically publishing a different block. Our
    // teardown must not overwrite it, and no plugin callback remains targeted.
    auto later=fixture.after;later[15]=0x91;
    OwnedPatch successor;
    Check(successor.Install(fixture.PatchAddress(),fixture.after,later)==PatchResult::Applied,"later owner publication");
    Check(fixture.patch.Stop()==PatchResult::NotOwned,"later owner restoration refusal");
    Check(Bytes(fixture.PatchAddress(),later),"later owner's bytes preserved");
    Check(successor.Stop()==PatchResult::Restored,"fixture later owner cleanup");
    OwnedPatch cleanup;
    Check(cleanup.Install(fixture.PatchAddress(),fixture.after,fixture.before)==PatchResult::Applied,"fixture original cleanup");
    std::puts("PASS: all code/binding guards, unreadable/unaligned/conflicting admission, code protection, and conditional teardown.");
}
