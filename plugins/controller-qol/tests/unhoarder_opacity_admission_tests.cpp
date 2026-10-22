#include <windows.h>
static unsigned memoryReads{};
static BOOL WINAPI CountMemoryRead(HANDLE process,LPCVOID at,LPVOID out,SIZE_T bytes,SIZE_T* got) {
 ++memoryReads;return ReadProcessMemory(process,at,out,bytes,got);
}
#define ReadProcessMemory CountMemoryRead
#include "item_code_compatibility.h"
#undef ReadProcessMemory
#include <cstdio>
#include <cstdlib>
static void Check(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
int main(int argc,char** argv){
 Check(argc==2,"actual DLL argument");
 auto module=LoadLibraryExA(argv[1],nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 Check(module!=nullptr,"load companion without initialization");
#if defined(QOL_TEST_SOCKET_BATCH_ALT_PROFILE)
 namespace P=QolItemCodeSocketBatchAlt;
#elif defined(QOL_TEST_RESIDENT_PAGES_PROFILE)
 namespace P=QolItemCodeResidentPages;
#elif defined(QOL_TEST_FORMATTER_OBSERVATION_PROFILE)
 namespace P=QolItemCodeFormatterObservation;
#elif defined(QOL_TEST_LABEL_WORK_PROFILE)
 namespace P=QolItemCodeLabelWork;
#elif defined(QOL_TEST_LABEL_PREFLIGHT_PROFILE)
 namespace P=QolItemCodeLabelPreflight;
#elif defined(QOL_TEST_NATIVE_READ_PROFILE)
 namespace P=QolItemCodeNativeReads;
#elif defined(QOL_TEST_POLLING_PROFILE)
 namespace P=QolItemCodePolling;
#else
 namespace P=QolItemCodeOpacity;
#endif
 auto companion=reinterpret_cast<uintptr_t>(module);
 auto* storage=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x380000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
 Check(storage!=nullptr,"synthetic native image");auto game=reinterpret_cast<uintptr_t>(storage);
 std::memcpy(storage+P::Rva,P::Original,sizeof(P::Original));
 auto entry=storage+P::Rva; entry[0]=0xe9;
 auto delta=static_cast<int32_t>((storage+0x100)-(entry+5));std::memcpy(entry+1,&delta,4);
 std::memcpy(storage+0x100,P::Jump,6);auto hook=companion+P::HookRva;std::memcpy(storage+0x106,&hook,8);
 std::memcpy(storage+0x140,P::Original,5);std::memcpy(storage+0x145,P::Jump,6);
 auto resume=game+P::Rva+5;std::memcpy(storage+0x14b,&resume,8);
 auto trampoline=game+0x140;std::memcpy(reinterpret_cast<void*>(companion+P::SlotRva),&trampoline,8);
 DWORD old{};Check(VirtualProtect(storage,0x380000,PAGE_EXECUTE_READ,&old)!=0,"native image RX");
 Check(QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"actual new artifact and complete item forwarding accepted");
 memoryReads=0;
 Check(QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"warm exact companion remains admitted");
 Check(memoryReads==1,"fresh common-layout header snapshot; page-qualified live graph retains every check");
 auto wrapper=reinterpret_cast<unsigned char*>(hook);auto saved=*wrapper;
 Check(VirtualProtect(wrapper,1,PAGE_EXECUTE_READWRITE,&old)!=0,"test wrapper protection");
 *wrapper^=1;DWORD unused{};Check(VirtualProtect(wrapper,1,old,&unused)!=0,"wrapper RX restored");
 Check(!QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"changed wrapper rejected despite known disk identity");
 Check(VirtualProtect(wrapper,1,PAGE_EXECUTE_READWRITE,&old)!=0,"restore wrapper");*wrapper=saved;
 Check(VirtualProtect(wrapper,1,old,&unused)!=0,"restored RX");
 Check(QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"restored forwarding accepted");
 // Disk identity never substitutes for current live bytes or pointer ownership.
 for(size_t i=0;i<sizeof(P::Wrapper);++i) {
  Check(VirtualProtect(wrapper+i,1,PAGE_EXECUTE_READWRITE,&old)!=0,"mutate private wrapper byte");
  wrapper[i]^=1;Check(VirtualProtect(wrapper+i,1,old,&unused)!=0,"mutated wrapper page restored");
  Check(!QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"every changed wrapper byte refused");
  Check(VirtualProtect(wrapper+i,1,PAGE_EXECUTE_READWRITE,&old)!=0,"restore private wrapper byte");
  wrapper[i]^=1;Check(VirtualProtect(wrapper+i,1,old,&unused)!=0,"restored wrapper page protection");
 }
 uintptr_t zero{};std::memcpy(reinterpret_cast<void*>(companion+P::SlotRva),&zero,8);
 Check(!QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"cleared original slot refused");
 std::memcpy(reinterpret_cast<void*>(companion+P::SlotRva),&trampoline,8);
 for(size_t i=0;i<19;++i) {
  auto* byte=storage+0x140+i;
  Check(VirtualProtect(byte,1,PAGE_EXECUTE_READWRITE,&old)!=0,"mutate private trampoline byte");
  *byte^=1;Check(VirtualProtect(byte,1,old,&unused)!=0,"mutated trampoline page restored");
  Check(!QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"changed trampoline or continuation refused");
  Check(VirtualProtect(byte,1,PAGE_EXECUTE_READWRITE,&old)!=0,"restore private trampoline byte");
  *byte^=1;Check(VirtualProtect(byte,1,old,&unused)!=0,"restored trampoline protection");
 }
 Check(QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"complete restored live graph admitted again");
 Check(VirtualProtect(wrapper,1,PAGE_EXECUTE_READWRITE,&old)!=0,"change live code protection after warmup");
 Check(!QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"writable companion code still refused");
 Check(VirtualProtect(wrapper,1,old,&unused)!=0,"restore code protection");
 auto* slot=reinterpret_cast<void*>(companion+P::SlotRva);
 Check(VirtualProtect(slot,8,PAGE_NOACCESS,&old)!=0,"make live slot unreadable");
 Check(!QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"unreadable slot still refused");
 Check(VirtualProtect(slot,8,old,&unused)!=0,"restore slot page");
 Check(VirtualProtect(slot,8,old|PAGE_GUARD,&unused)!=0,"guard live slot page");
 Check(!QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"guarded slot still refused");
 MEMORY_BASIC_INFORMATION page{};VirtualQuery(slot,&page,sizeof(page));
 Check((page.Protect&PAGE_GUARD)!=0,"kind-qualified reader refuses guard before copying");
 Check(VirtualProtect(slot,8,old,&unused)!=0,"restore guarded slot page");
 auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(module);
 Check(VirtualProtect(dos,sizeof(*dos),PAGE_READWRITE,&old)!=0,"mutate loaded PE metadata");
 const auto magic=dos->e_magic;dos->e_magic=0;
 Check(!QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"loaded PE metadata rechecked despite cached disk identity");
 dos->e_magic=magic;Check(VirtualProtect(dos,sizeof(*dos),old,&unused)!=0,"restore loaded PE metadata");
 Check(QolItemCode::Compatible(game,P::Original,sizeof(P::Original)),"restored metadata and live protections admitted again");
 std::memcpy(reinterpret_cast<void*>(companion+P::SlotRva),&zero,8);
 VirtualFree(storage,0,MEM_RELEASE);FreeLibrary(module);
 std::puts("Exact companion admitted; full wrapper, slot, trampoline and continuation mutations refused; game untouched.");
}
