#include "cube_native_calls.h"
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <array>
#include <intrin.h>
using QolCubeNativeCalls::Kind;
unsigned checks{};
void Check(bool ok,const char* why) {++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
struct Capture {uintptr_t first{},second{},third{},fourth{};unsigned calls{};} capture;
bool aligned{};
uintptr_t __fastcall Action(uintptr_t a,uintptr_t b,uintptr_t original) noexcept {
    aligned=(reinterpret_cast<uintptr_t>(_AddressOfReturnAddress())&15)==8;
    capture={a,b,original,0,capture.calls+1};return 0xdeadbeef;
}
uintptr_t __fastcall Prompt(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t widget) noexcept {
    aligned=(reinterpret_cast<uintptr_t>(_AddressOfReturnAddress())&15)==8;
    capture={a,b,c,widget,capture.calls+1};return 0xdeadbeef;
}
void Append(std::vector<unsigned char>& out,std::initializer_list<unsigned char> bytes) {out.insert(out.end(),bytes);}
void Imm(std::vector<unsigned char>& out,uintptr_t value) {const auto p=reinterpret_cast<const unsigned char*>(&value);out.insert(out.end(),p,p+8);}
void* Executable(const unsigned char* bytes,size_t size) {
    auto memory=VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);Check(memory!=nullptr,"test code allocation");
    std::memcpy(memory,bytes,size);DWORD old{};Check(VirtualProtect(memory,size,PAGE_EXECUTE_READ,&old)!=0,"test code finalized RX");
    FlushInstructionCache(GetCurrentProcess(),memory,size);return memory;
}
int main() {
    for(auto kind:{Kind::Tap,Kind::Hold,Kind::Prompt,Kind::ItemPrompt}) {
        unsigned char relay[32]{};
        const auto callback=kind==Kind::Prompt?reinterpret_cast<uintptr_t>(&Prompt):reinterpret_cast<uintptr_t>(&Action);
        const auto length=QolCubeNativeCalls::Relay(kind,callback,relay);
        Check(length>14 && length<=sizeof(relay),"bounded relay size");
        auto executable=Executable(relay,length);
        std::array<uintptr_t,0x108/8> table{};table[0x100/8]=0x1122334455667788;
        // The fixture recreates the real call-site registers and shadow space.
        // RBX/RDI/R14 are nonvolatile and must survive the callback.
        std::vector<unsigned char> runner;
        Append(runner,{0x53,0x57,0x41,0x56,0x48,0x83,0xec,0x20}); // save 3; aligned caller and 32-byte home area
        Append(runner,{0x48,0xbb});Imm(runner,0x1122334455667788);
        Append(runner,{0x48,0xbf});Imm(runner,0x1020304050607080);
        Append(runner,{0x49,0xbe});Imm(runner,0x8877665544332211);
        if(kind==Kind::Hold) {
            Append(runner,{0x48,0xb8});Imm(runner,0x123456789abcdef0); // original lookup return
        } else if(kind==Kind::Tap) {
            Append(runner,{0x48,0xb8});Imm(runner,reinterpret_cast<uintptr_t>(table.data()));
        }
        Append(runner,{0x49,0xbb});Imm(runner,reinterpret_cast<uintptr_t>(executable));
        Append(runner,{0x41,0xff,0xd3}); // CALL relay, just like the patched native call
        // Capture nonvolatile values before restoring the fixture caller's own.
        std::array<uintptr_t,3> preserved{};
        Append(runner,{0x49,0xbb});Imm(runner,reinterpret_cast<uintptr_t>(preserved.data()));
        Append(runner,{0x49,0x89,0x1b,0x49,0x89,0x7b,0x08,0x4d,0x89,0x73,0x10});
        Append(runner,{0x48,0x83,0xc4,0x20,0x41,0x5e,0x5f,0x5b,0xc3});
        auto run=Executable(runner.data(),runner.size());
        using Fn=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
        capture={};const auto result=reinterpret_cast<Fn>(run)(0x11,0x22,0x33);
        Check(result==0xdeadbeef && capture.calls==1,"callback once, return and original continuation preserved");
        Check(aligned,"callback has Windows x64 entry alignment and caller shadow space");
        Check(preserved==std::array<uintptr_t,3>{0x1122334455667788,0x1020304050607080,0x8877665544332211},"RBX RDI R14 preserved");
        if(kind==Kind::Tap)Check(capture.first==0x11 && capture.second==0x22 && capture.third==table[0x100/8],"tap keeps grid/cell and captures original virtual callee");
        if(kind==Kind::Hold)Check(capture.first==0x1020304050607080 && capture.second==0x123456789abcdef0 && capture.third==0x1122334455667788,"hold reconstructs grid/item and original callee");
        if(kind==Kind::ItemPrompt)Check(capture.first==0x11 && capture.second==0x1122334455667788,"item description keeps localization id and supplies RBX item");
        if(kind==Kind::Prompt)Check(capture.first==0x11 && capture.second==0x22 && capture.third==0x33 && capture.fourth==0x8877665544332211,"prompt keeps all formatter arguments and supplies owning grid");
        VirtualFree(run,0,MEM_RELEASE);VirtualFree(executable,0,MEM_RELEASE);
    }
    for(const auto& site:QolCubeNativeCalls::Sites) {
        unsigned char patch[8]{};Check(QolCubeNativeCalls::Patch(0x140000000+site.rva,0x150000000,site.size,patch),"native site is rel32 reachable");
        int32_t relative{};std::memcpy(&relative,patch+1,4);
        Check(patch[0]==0xe8 && static_cast<uintptr_t>(static_cast<int64_t>(0x140000000+site.rva+5)+relative)==0x150000000,"patch reaches exact relay");
        for(size_t i=5;i<site.size;++i)Check(patch[i]==0x90,"whole displaced instructions replaced with NOP padding");
    }
    std::printf("%u native Cube ABI checks passed\n",checks);
}
