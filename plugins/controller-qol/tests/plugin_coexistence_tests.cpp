#include "plugin_coexistence_policy.h"
#include <windows.h>
#include <cwchar>
// Resolve only the companion name to this test image while exercising the
// production guard. The PE reader, VirtualQuery, graph and sender checks stay
// unchanged; the DLL build never sees this test-only module-lookup substitution.
static HMODULE testCompanion{};
static HMODULE CoexistenceTestModule(LPCWSTR name) noexcept {
    if(testCompanion && name && std::wcscmp(name,L"d2rl-global-chat.dll")==0)return testCompanion;
    return ::GetModuleHandleW(name);
}
#define GetModuleHandleW CoexistenceTestModule
#include "client_request_profile.h"
#undef GetModuleHandleW
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <climits>
#pragma section(".qolcode",read,execute)
#pragma section(".qoldata",read,write)
__declspec(allocate(".qolcode")) __declspec(align(16)) unsigned char companionCode[0x2000]{};
__declspec(allocate(".qoldata")) __declspec(align(16)) unsigned char companionData[0x100]{};
void Check(bool ok,const char* message) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
namespace {
using namespace QolChatForwarding;
struct ChatFixture {
    uintptr_t base=0x20000000,core=0x10000000,trampoline=0x50000000;
    std::vector<unsigned char> image=std::vector<unsigned char>(0x20000);
    std::array<uintptr_t,static_cast<size_t>(Node::Count)> addresses{};
    std::array<unsigned char,sizeof(QolClientRequest::CoreEntry)> entry{};
    std::array<unsigned char,28> relocated{};
    bool readable=true,codeOwner=true,dataOwner=true,trampolineExecutable=true;
    static void Jump(unsigned char* output,uintptr_t target) {
        constexpr unsigned char prefix[]={0xff,0x25,0,0,0,0};
        std::memcpy(output,prefix,6);std::memcpy(output+6,&target,8);
    }
    unsigned char* At(uintptr_t address) {return image.data()+address-base;}
    explicit ChatFixture(bool moved=false,uintptr_t mappedBase=0) {
        if(mappedBase)base=mappedBase;
        else if(moved){base+=0x1000000;trampoline+=0x2000000;}
        for(size_t i=0;i<std::size(Profile::Blocks);++i)addresses[i]=base+0x1000+(moved?(13-i):i)*0x200;
        addresses[static_cast<size_t>(Node::HookObject)]=base+(moved?0x18000:0x16000);
        addresses[static_cast<size_t>(Node::Cookie)]=base+(moved?0x17000:0x19000);
        for(size_t i=0;i<std::size(Profile::Blocks);++i) {
            const auto& block=Profile::Blocks[i];auto* bytes=At(addresses[i]);
            std::memcpy(bytes,block.bytes,block.size);
            for(size_t r=0;r<block.count;++r) {
                const auto& ref=block.refs[r];
                const auto delta=static_cast<int64_t>(addresses[static_cast<size_t>(ref.target)])-
                    static_cast<int64_t>(addresses[i]+ref.displacement+4);
                Check(delta>=INT32_MIN && delta<=INT32_MAX,"fixture relative reference fits");
                const auto displacement=static_cast<int32_t>(delta);std::memcpy(bytes+ref.displacement,&displacement,4);
            }
        }
        const uintptr_t cookie=0x123456789abc;
        std::memcpy(At(addresses[static_cast<size_t>(Node::Cookie)]),&cookie,8);
        std::memcpy(At(addresses[static_cast<size_t>(Node::HookObject)])+8,&trampoline,8);
        std::memcpy(entry.data(),QolClientRequest::CoreEntry,entry.size());
        Jump(entry.data(),addresses[0]);
        std::memcpy(relocated.data(),QolClientRequest::CoreEntry,14);Jump(relocated.data()+14,core+14);
    }
    bool Read(uintptr_t address,void* output,size_t size,Kind kind) noexcept {
        if(!readable)return false;
        if(kind==Kind::Trampoline) {
            if(!trampolineExecutable || address!=trampoline || size!=relocated.size())return false;
            std::memcpy(output,relocated.data(),size);return true;
        }
        if(address<base || address-base>image.size() || size>image.size()-(address-base))return false;
        const auto offset=address-base;
        if(kind==Kind::Code?(!codeOwner || offset<0x100 || offset+size>0x10000):(!dataOwner || offset<0x10000))return false;
        std::memcpy(output,At(address),size);return true;
    }
    Result Validate() noexcept {
        const Memory memory{this,[](void* user,uintptr_t address,void* output,size_t size,Kind kind) noexcept {
            return static_cast<ChatFixture*>(user)->Read(address,output,size,kind);
        }};
        return QolChatForwarding::Validate(memory,addresses[0],entry.data(),QolClientRequest::CoreEntry,entry.size(),core+14);
    }
};
const unsigned char* expectedPacket{};
int32_t expectedLength{};
uint32_t* inFlight{};
unsigned forwarded{};
uint64_t __fastcall OriginalSender(const unsigned char* packet,int32_t length) {
    Check(packet==expectedPacket && length==expectedLength,"forwarded packet pointer and byte count preserved");
    Check(inFlight && *inFlight==1,"forwarding retains companion in-flight accounting");
    ++forwarded;return 0xfedcba9876543210ULL;
}
void ExecuteForwarding() {
    // Execute only synthetic private code/data. This tests ABI, one forwarding
    // call, return preservation and balanced counter lifetime, without loading
    // a plugin or issuing any game/network request.
    auto* pages=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x20000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    Check(pages!=nullptr,"allocate private executable forwarding fixture");
    ChatFixture fixture(false,reinterpret_cast<uintptr_t>(pages));
    fixture.core=fixture.base+0x10000;fixture.trampoline=fixture.base+0x1c000;
    std::array<unsigned char,sizeof(QolClientRequest::CoreEntry)> original{};
    original.fill(0x90);std::memcpy(original.data(),QolClientRequest::CoreEntry,14);
    // Exact stolen prologue, a standalone test callback, then its matching
    // epilogue. No game code or native sender implementation is executed.
    size_t position=14;original[position++]=0x48;original[position++]=0xb8;
    const auto callback=reinterpret_cast<uintptr_t>(&OriginalSender);
    std::memcpy(original.data()+position,&callback,8);position+=8;
    constexpr unsigned char suffix[]={0xff,0xd0,0x48,0x83,0xc4,0x50,0x5b,0x5f,0x5e,0x41,0x5c,0x41,0x5e,0x41,0x5f,0x5d,0xc3};
    std::memcpy(original.data()+position,suffix,sizeof(suffix));
    fixture.entry=original;ChatFixture::Jump(fixture.entry.data(),fixture.addresses[0]);
    std::memcpy(fixture.relocated.data(),original.data(),14);ChatFixture::Jump(fixture.relocated.data()+14,fixture.core+14);
    std::memcpy(fixture.At(fixture.addresses[static_cast<size_t>(Node::HookObject)])+8,&fixture.trampoline,8);
    const Memory reader{&fixture,[](void* user,uintptr_t address,void* output,size_t size,Kind kind) noexcept {
        return static_cast<ChatFixture*>(user)->Read(address,output,size,kind);
    }};
    Check(static_cast<bool>(QolChatForwarding::Validate(reader,fixture.addresses[0],fixture.entry.data(),original.data(),original.size(),fixture.core+14)),"executable fixture meets same forwarding proof");
    std::memcpy(pages,fixture.image.data(),fixture.image.size());
    std::memcpy(reinterpret_cast<void*>(fixture.core),original.data(),original.size());
    std::memcpy(reinterpret_cast<void*>(fixture.trampoline),fixture.relocated.data(),fixture.relocated.size());
    DWORD previous{};
    Check(VirtualProtect(pages,0x11000,PAGE_EXECUTE_READ,&previous)!=0 &&
        VirtualProtect(reinterpret_cast<void*>(fixture.trampoline),4096,PAGE_EXECUTE_READ,&previous)!=0,"execute private fixture code with data separate");
    FlushInstructionCache(GetCurrentProcess(),pages,fixture.image.size());
    inFlight=reinterpret_cast<uint32_t*>(fixture.addresses[static_cast<size_t>(Node::HookObject)]+0x40);
    auto send=reinterpret_cast<uint64_t(__fastcall*)(const unsigned char*,int32_t)>(fixture.addresses[0]);
    unsigned cases=0;
    for(const int32_t length:{-1,0,1,24,512}) for(unsigned opcode=0;opcode<256;++opcode) {
        if(opcode==0x15)continue; // Chat handling is deliberately outside this contract.
        unsigned char packet[512]={static_cast<unsigned char>(opcode),0x12,0x34,0x56};
        std::array<unsigned char,sizeof(packet)> saved{};std::memcpy(saved.data(),packet,sizeof(packet));
        const unsigned char* request=length<=0?nullptr:packet;
        expectedPacket=request;expectedLength=length;const auto before=forwarded;
        Check(send(request,expectedLength)==0xfedcba9876543210ULL,"original 64-bit return preserved");
        Check(forwarded==before+1 && *inFlight==0,"item request forwarded once; counter restored");
        Check(!std::memcmp(saved.data(),packet,sizeof(packet)),"forwarder preserves packet contents");++cases;
    }
    inFlight=nullptr;VirtualFree(pages,0,MEM_RELEASE);
    std::printf("Synthetic forwarding: %u opcode/length cases passed.\n",cases);
}
void ChatContracts() {
    ChatFixture fixture;
    Check(static_cast<bool>(fixture.Validate()),"complete non-chat forwarding graph admitted");
    ChatFixture moved(true);
    Check(static_cast<bool>(moved.Validate()),"independently relocated and reordered helpers/data admitted");
    // Every instruction byte and every resolved operand is covered. Relocation
    // fields are constrained graph edges, never unchecked wildcard bytes.
    size_t mutations=0;
    for(size_t i=0;i<std::size(Profile::Blocks);++i) {
        const auto& block=Profile::Blocks[i];auto* bytes=fixture.At(fixture.addresses[i]);
        for(size_t byte=0;byte<block.size;++byte) {
            const auto original=bytes[byte];
            for(unsigned replacement=0;replacement<256;++replacement) {
                if(replacement==original)continue;
                bytes[byte]=static_cast<unsigned char>(replacement);
                Check(!fixture.Validate(),"changed forwarding instruction/reference refused");++mutations;
            }
            bytes[byte]=original;
        }
    }
    fixture.entry[20]^=1;Check(!fixture.Validate(),"Core tail still exact with relocated Chat");fixture.entry[20]^=1;
    fixture.entry[6]^=1;Check(!fixture.Validate(),"sender must target checked graph");fixture.entry[6]^=1;
    fixture.relocated[1]^=1;Check(!fixture.Validate(),"trampoline stolen instructions exact");fixture.relocated[1]^=1;
    fixture.relocated[20]^=1;Check(!fixture.Validate(),"trampoline must return to Core entry+14");fixture.relocated[20]^=1;
    fixture.readable=false;Check(!fixture.Validate(),"unreadable graph refused");fixture.readable=true;
    fixture.codeOwner=false;Check(!fixture.Validate(),"foreign or writable code refused");fixture.codeOwner=true;
    fixture.dataOwner=false;Check(!fixture.Validate(),"foreign or executable state refused");fixture.dataOwner=true;
    fixture.trampolineExecutable=false;Check(!fixture.Validate(),"nonexecutable continuation refused");fixture.trampolineExecutable=true;
    const auto cookie=fixture.addresses[static_cast<size_t>(Node::Cookie)];
    uintptr_t saved{};std::memcpy(&saved,fixture.At(cookie),8);
    std::memset(fixture.At(cookie),0,8);Check(!fixture.Validate(),"uninitialized /GS cookie refused");
    std::memset(fixture.At(cookie),0xff,8);Check(!fixture.Validate(),"cookie with nonzero high bits refused");
    std::memcpy(fixture.At(cookie),&saved,8);
    // The guarded non-chat branch skips chat parsing. Changes there and in
    // unrelated image regions must not introduce a whole-DLL dependency.
    fixture.At(fixture.addresses[static_cast<size_t>(Node::Dispatch)])[sizeof(Profile::Dispatch)]^=0xff;
    fixture.image[0x1f000]^=1;
    Check(static_cast<bool>(fixture.Validate()),"unreachable chat-only and unrelated data changes do not reject item requests");
    uintptr_t target{};
    Check(!Relative(UINTPTR_MAX,1,target) && !Relative(1,-2,target),"relative-reference overflow/underflow refused");
    ImageMemory own{};
    Check(own.Open(GetModuleHandleW(nullptr)),"production image reader accepts valid PE ownership");
    unsigned char byte{};
    Check(own.Read(reinterpret_cast<uintptr_t>(&ChatContracts),&byte,1,Kind::Code),"production reader admits own read-only executable image code");
    Check(!own.Read(reinterpret_cast<uintptr_t>(&ChatContracts),&byte,1,Kind::Data),"code cannot be used as hook object data");
    std::printf("Chat forwarding graph: %zu instruction/reference mutations rejected.\n",mutations);
}
void ProductionImageContracts() {
    // A real MEM_IMAGE fixture exercises production ownership and page checks
    // in normal CTest runs, without distributing or initializing a companion.
    std::array<uintptr_t,static_cast<size_t>(Node::Count)> nodes{};
    for(size_t i=0;i<std::size(Profile::Blocks);++i)nodes[i]=reinterpret_cast<uintptr_t>(companionCode)+i*0x200;
    const auto object=reinterpret_cast<uintptr_t>(companionData),cookie=object+0x80;
    nodes[static_cast<size_t>(Node::HookObject)]=object;
    nodes[static_cast<size_t>(Node::Cookie)]=cookie;
    DWORD oldCode{},ignored{};
    Check(VirtualProtect(companionCode,sizeof(companionCode),PAGE_READWRITE,&oldCode)!=0,"prepare private image fixture");
    for(size_t i=0;i<std::size(Profile::Blocks);++i) {
        const auto& block=Profile::Blocks[i];auto* output=reinterpret_cast<unsigned char*>(nodes[i]);
        std::memcpy(output,block.bytes,block.size);
        for(size_t r=0;r<block.count;++r) {
            const auto& ref=block.refs[r];
            const auto delta=static_cast<int64_t>(nodes[static_cast<size_t>(ref.target)])-static_cast<int64_t>(nodes[i]+ref.displacement+4);
            Check(delta>=INT32_MIN && delta<=INT32_MAX,"production fixture reference fits");
            const auto displacement=static_cast<int32_t>(delta);std::memcpy(output+ref.displacement,&displacement,4);
        }
    }
    Check(VirtualProtect(companionCode,sizeof(companionCode),oldCode,&ignored)!=0,"restore read-only image code");
    const uintptr_t cookieValue=0x123456789abc;
    std::memcpy(reinterpret_cast<void*>(cookie),&cookieValue,8);
    auto* core=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    auto* trampoline=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    Check(core && trampoline,"production fixture sender allocations");
    auto* entry=core+0x822dd0;
    std::memcpy(entry,QolClientRequest::CoreEntry,sizeof(QolClientRequest::CoreEntry));ChatFixture::Jump(entry,nodes[0]);
    std::memcpy(trampoline,QolClientRequest::CoreEntry,14);ChatFixture::Jump(trampoline+14,reinterpret_cast<uintptr_t>(entry)+14);
    Check(VirtualProtect(trampoline,4096,PAGE_EXECUTE_READ,&ignored)!=0,"production fixture continuation executable");
    std::memcpy(companionData+8,&trampoline,8);
    testCompanion=GetModuleHandleW(nullptr);
    const char* reason{};
    auto admit=[&]{return QolCoexistence::Send(reinterpret_cast<uintptr_t>(core),QolClientRequest::CoreEntry,sizeof(QolClientRequest::CoreEntry),&reason);};
    Check(admit() && !reason,"production Send accepts owned image graph without a file-hash exception");
    auto toggle=[&](uintptr_t address) {
        DWORD before{};Check(VirtualProtect(reinterpret_cast<void*>(address),1,PAGE_READWRITE,&before)!=0,"mutate private image witness");
        *reinterpret_cast<unsigned char*>(address)^=1;
        Check(VirtualProtect(reinterpret_cast<void*>(address),1,before,&ignored)!=0,"restore witness page protection");
    };
    const auto helper=nodes[static_cast<size_t>(Node::Enter)];
    toggle(helper);Check(!admit() && reason && std::strcmp(reason,"Enter")==0,"production helper refusal is final");toggle(helper);
    const auto chatOnly=nodes[static_cast<size_t>(Node::Dispatch)]+sizeof(Profile::Dispatch);
    toggle(chatOnly);Check(admit(),"production chat-only changes remain compatible");toggle(chatOnly);
    Check(VirtualProtect(companionCode,sizeof(companionCode),PAGE_EXECUTE_READWRITE,&ignored)!=0,"change actual code protection");
    Check(!admit(),"production writable image code refused");
    Check(VirtualProtect(companionCode,sizeof(companionCode),oldCode,&ignored)!=0,"restore actual code protection");
    DWORD oldData{};Check(VirtualProtect(companionData,sizeof(companionData),PAGE_READONLY,&oldData)!=0,"change actual data protection");
    Check(!admit(),"production nonwritable state refused");
    Check(VirtualProtect(companionData,sizeof(companionData),oldData,&ignored)!=0,"restore actual data protection");
    Check(VirtualProtect(trampoline,4096,PAGE_READONLY,&ignored)!=0,"disable continuation execution");
    Check(!admit(),"production nonexecutable continuation refused");
    Check(VirtualProtect(trampoline,4096,PAGE_EXECUTE_READ,&ignored)!=0,"restore continuation execution");
    std::memset(companionData+8,0,8);Check(!admit(),"production missing continuation refused");std::memcpy(companionData+8,&trampoline,8);
    entry[20]^=1;Check(!admit(),"production changed Core tail refused");entry[20]^=1;
    Check(admit(),"production complete restored graph remains usable");
    testCompanion=nullptr;
    Check(!admit(),"production missing companion owner refused");
    VirtualFree(core,0,MEM_RELEASE);VirtualFree(trampoline,0,MEM_RELEASE);
    std::puts("Production image ownership, protection, helper and continuation refusals passed.");
}
// Optional private artifact check. Maps a companion without running DllMain or
// any of its code; all patched state belongs to this standalone test process.
void RealChatImage(const char* path,const char* hookRva) {
    auto module=LoadLibraryExA(path,nullptr,DONT_RESOLVE_DLL_REFERENCES);
    Check(module!=nullptr,"map current Chat image without executing plugin code");
    const auto base=reinterpret_cast<uintptr_t>(module);
    const auto hook=base+std::strtoull(hookRva,nullptr,16);
    QolChatForwarding::ImageMemory image{};Check(image.Open(module),"real companion PE section layout");
    int32_t delta{};Check(image.Read(hook+0x10,&delta,4,Kind::Code),"read reviewed object operand");
    uintptr_t object{};Check(Relative(hook+0x14,delta,object),"resolve companion object");
    auto* core=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    auto* trampoline=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    Check(core && trampoline,"allocate private Core/trampoline fixtures");
    auto* sender=core+0x822dd0;
    std::memcpy(sender,QolClientRequest::CoreEntry,sizeof(QolClientRequest::CoreEntry));ChatFixture::Jump(sender,hook);
    std::memcpy(trampoline,QolClientRequest::CoreEntry,14);ChatFixture::Jump(trampoline+14,reinterpret_cast<uintptr_t>(sender)+14);
    DWORD previous{};Check(VirtualProtect(trampoline,4096,PAGE_EXECUTE_READ,&previous)!=0,"private trampoline executable");
    uintptr_t saved{};std::memcpy(&saved,reinterpret_cast<void*>(object+8),8);
    std::memcpy(reinterpret_cast<void*>(object+8),&trampoline,8);
    const char* reason{};
    const auto admitted=QolCoexistence::Send(reinterpret_cast<uintptr_t>(core),QolClientRequest::CoreEntry,sizeof(QolClientRequest::CoreEntry),&reason);
    if(!admitted)std::fprintf(stderr,"Real image refused: %s\n",reason?reason:"unknown");
    Check(admitted,"production Send accepts the current actual Chat image forwarding contract");
    std::memcpy(reinterpret_cast<void*>(object+8),&saved,8);
    VirtualFree(core,0,MEM_RELEASE);VirtualFree(trampoline,0,MEM_RELEASE);FreeLibrary(module);
    std::puts("Current companion mapped-image production guard passed; no companion code executed.");
}
}
int main(int argc,char** argv) {
    ChatContracts();
    ExecuteForwarding();
    ProductionImageContracts();
    using namespace QolCoexistence;
    constexpr uintptr_t core=0x10000000,hook=0x200538b0;
    std::array<unsigned char,68> original{},entry{};
    std::array<unsigned char,28> trampoline{};
    for(size_t i=0;i<original.size();++i)original[i]=static_cast<unsigned char>(i+1);
    entry=original;
    auto jump=[](unsigned char* out,uintptr_t target) {
        const unsigned char prefix[]={0xff,0x25,0,0,0,0};
        std::memcpy(out,prefix,6);std::memcpy(out+6,&target,8);
    };
    jump(entry.data(),hook);
    std::memcpy(trampoline.data(),original.data(),14);jump(trampoline.data()+14,core+14);
    auto valid=[&](bool owner=true){return SendChain(entry.data(),original.data(),original.size(),hook,trampoline.data(),core+14,owner);};
    Check(valid(),"known hook forwards through exact relocated original prologue");
    Check(!valid(false),"unknown owner cannot use known hook shape");
    entry[6]^=1;Check(!valid(),"different hook rejected");entry[6]^=1;
    entry[20]^=1;Check(!valid(),"changed Core tail rejected");entry[20]^=1;
    trampoline[1]^=1;Check(!valid(),"changed relocated instruction rejected");trampoline[1]^=1;
    trampoline[20]^=1;Check(!valid(),"wrong trampoline continuation rejected");trampoline[20]^=1;
    Check(!SendChain(entry.data(),original.data(),original.size(),hook,nullptr,core+14,true),"null trampoline rejected");
    Check(StatRoute(core,core,0,0,false),"unhooked offline and non-Ladder reader requires no Maps");
    Check(StatRoute(hook,core,hook,core,true),"known Maps forwarding reader admitted");
    Check(!StatRoute(hook,core,hook,core,false),"unverified Maps refused");
    Check(!StatRoute(hook,core,hook,hook,true),"recursive Maps continuation refused");
    Check(!StatRoute(hook+1,core,hook,core,true),"unrelated stat hook refused");
    Check(!StatRoute(hook,core,hook,0,true),"missing original reader refused");
    // Exercise the production guards in a standalone process with no Ladder
    // plugins loaded. Only the referenced pages of these fake images are used.
    const auto gameImage=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x4000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    const auto coreImage=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    Check(gameImage && coreImage,"allocate isolated image fixtures");
    std::memcpy(reinterpret_cast<void*>(gameImage+0x3e2b73c),QolClientRequest::Thunk,sizeof(QolClientRequest::Thunk));
    *reinterpret_cast<uintptr_t*>(gameImage+0x3e2ae20)=coreImage+0x822dd0;
    std::memcpy(reinterpret_cast<void*>(coreImage+0x822dd0),QolClientRequest::CoreEntry,sizeof(QolClientRequest::CoreEntry));
    *reinterpret_cast<uintptr_t*>(gameImage+0x3e2a218)=coreImage+0x831de0;
    Check(QolClientRequest::Guard(gameImage,coreImage),"production unhooked request guard needs no Ladder plugin");
    Check(StatSlot(gameImage,coreImage),"production unhooked stat slot needs no Ladder plugin");
    *reinterpret_cast<unsigned char*>(coreImage+0x822dd0)=0xff;
    Check(!QolClientRequest::Guard(gameImage,coreImage),"unknown sender replacement still refused");
    *reinterpret_cast<uintptr_t*>(gameImage+0x3e2a218)=hook;
    Check(!StatSlot(gameImage,coreImage),"unknown stat replacement still refused");
    VirtualFree(reinterpret_cast<void*>(gameImage),0,MEM_RELEASE);
    VirtualFree(reinterpret_cast<void*>(coreImage),0,MEM_RELEASE);
    if(argc==3)RealChatImage(argv[1],argv[2]);
    std::puts("Plugin forwarding chains and original-path isolation passed.");
}
