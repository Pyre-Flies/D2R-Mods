#include "admission.h"
#include "native.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace {
std::vector<std::string> messages;
unsigned sdkChecks{};
bool sdkAccept{};
void Check(bool value,const char* name){if(!value){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}}
void __cdecl Log(const D2RL::PluginContext*,const char* text) noexcept {messages.emplace_back(text);}
bool __cdecl Expected(const D2RL::PluginContext*,uint64_t,const void*,uint32_t) noexcept {++sdkChecks;return sdkAccept;}
const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize,.logInfo=Log,.logWarn=Log,.checkExpectedBytes=Expected};
const D2RL::PluginContext context{.contextSize=sizeof(D2RL::PluginContext),.abiVersion=D2RL_PLUGIN_ABI_VERSION,.api=&api};
bool Has(const char* text){return std::any_of(messages.begin(),messages.end(),[&](const auto& line){return line.find(text)!=std::string::npos;});}
constexpr char Abc[]="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
void Put(const std::filesystem::path& path,const char* text){std::ofstream file(path,std::ios::binary);file<<text;Check(static_cast<bool>(file),"write owned diagnostic fixture");}
}
int wmain(int argc,wchar_t** argv) {
    using namespace GemBag::Admission;
    Check(argc==2,"fixture directory argument");
    const std::filesystem::path root(argv[1]);std::filesystem::create_directories(root);
    const auto fixture=root/L"owned-admission.txt",missing=root/L"missing-admission.txt";
    Check(!std::filesystem::exists(missing),"missing-file fixture is absent");
    Put(fixture,"abc");Report matching(&context);
    Check(matching.File(fixture,"cubemain.txt",Abc)&&!matching.Failures()&&messages.empty(),"known SHA-256 vector passes without a warning");

    Put(fixture,"abc\n");Report changed(&context);
    Check(!changed.File(fixture,"cubemain.txt",Abc),"newline-only content change still refuses");
    Check(Has("file=cubemain.txt SHA-256 mismatch")&&Has(Abc)&&Has("actual="),"mismatch names file and includes expected/actual digests");
    Check(!Has("hash unavailable")&&!Has(root.string().c_str()),"content mismatch is distinct from unreadable file; log omits absolute user paths");
    messages.clear();Report absent(&context);
    Check(!absent.File(missing,"misc.txt",Abc)&&Has("stage=open-file")&&Has("win32=2")&&Has("hash unavailable"),"missing file reports the failing I/O stage and Windows error");

    const auto lock=CreateFileW(fixture.c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(lock!=INVALID_HANDLE_VALUE,"owned exclusive file handle");messages.clear();Report locked(&context);
    Check(!locked.File(fixture,"properties.txt",Abc)&&Has("win32=32")&&Has("hash unavailable"),"sharing refusal is reported instead of a fake digest");CloseHandle(lock);

    messages.clear();Report allFiles(&context);
    allFiles.File(fixture,"cubemain.txt",Abc);allFiles.File(fixture,"itemstatcost.txt",Abc);
    Check(allFiles.Failures()==2&&messages.size()==2&&Has("cubemain.txt")&&Has("itemstatcost.txt"),"independent table failures are retained in one startup report");

    messages.clear();Report module(&context);const unsigned char wrongHash[32]{};std::filesystem::path path;
    Check(!module.Module(nullptr,"D2RCore.dll",wrongHash)&&Has("module=D2RCore.dll unavailable"),"missing provider module is named");
    Check(!module.Module(GetModuleHandleW(nullptr),"D2RLoader.exe",wrongHash,&path)&&!path.empty()&&Has("file=D2RLoader.exe SHA-256 mismatch"),"real loaded-module hash failure identifies the artifact");

    unsigned char memory[32]{};const unsigned char expected[20]{};
    const auto base=reinterpret_cast<uintptr_t>(memory);Report bytes(&context);messages.clear();
    Check(bytes.Bytes(base,0,expected,"Core/stat-list-selector")&&messages.empty(),"matching native bytes remain admitted");
    memory[17]=0x7f;
    Check(!bytes.Bytes(base,0,expected,"Core/stat-list-selector")&&Has("first_offset=0x11")&&Has("expected=")&&Has("actual="),"native refusal identifies the check and changed byte preview");
    messages.clear();Report unreadable(&context);
    Check(!unreadable.Bytes(0,0,expected,"Core/SetWideListStat")&&Has("memory unavailable")&&Has("win32="),"unreadable native memory is distinguished from changed bytes");
    memory[17]=0;messages.clear();Report provider(&context);sdkAccept=false;
    Check(!provider.Bytes(base,0,expected,"executable/cursor-item",true)&&sdkChecks==1&&Has("mapped bytes match")&&Has("guard retained"),"SDK refusal cannot be bypassed by matching raw bytes");
    messages.clear();Report admitted(&context);sdkAccept=true;
    Check(admitted.Bytes(base,0,expected,"executable/cursor-item",true)&&sdkChecks==2&&messages.empty(),"successful SDK guard retains its verdict");

    messages.clear();
    auto real=context;real.exeBase=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));real.activeMod="ReimaginedLadder";
    Check(!GemBag::Native::Initialize(&real),"real native startup refuses an unsupported executable/provider");
    Check(Has("D2RLoader.exe SHA-256 mismatch")&&Has("D2RCore.dll unavailable")&&!Has("active-mod"),"native admission remains guarded without a named-mod dependency");
    messages.clear();real.activeMod=nullptr;Check(!GemBag::Native::Initialize(&real)&&!Has("active-mod"),"generic installs use the same native checks");
    Check(!GemBag::Native::Initialize(nullptr),"null startup remains inert");
    std::filesystem::remove(fixture);
    std::puts("Admission diagnostics: real file/module hashes, missing/locked files, native checks without mod-name dependency, byte witnesses and retained SDK refusal verified. No game mutation.");
}
