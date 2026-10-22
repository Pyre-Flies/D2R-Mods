#include <windows.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
unsigned apiReads{};
static BOOL WINAPI CountRead(HANDLE process,LPCVOID source,LPVOID out,SIZE_T size,SIZE_T* copied) {
    ++apiReads;return ReadProcessMemory(process,source,out,size,copied);
}
#define ReadProcessMemory CountRead
#ifdef UNHOARDER_IMAGE_HEADERS
#include "maps_socket_memory.hpp"
using Image=UnHoarder::MapsSocket::ImageMemory;
bool Open(Image& image,void* source){return image.Open(static_cast<HMODULE>(source));}
#else
#include "chat_forwarding_memory.h"
using Image=QolChatForwarding::ImageMemory;
bool Open(Image& image,void* source){return image.Open(static_cast<HMODULE>(source),true);}
#endif
#undef ReadProcessMemory
static void Check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL: %s (API reads=%u)\n",why,apiReads);std::exit(1);}}
int main() {
    auto* source=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x4000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(source!=nullptr,"owned PE fixture");
    auto make=[&](LONG offset=0x80) {
        std::memset(source,0,0x4000);
        auto& dos=*reinterpret_cast<IMAGE_DOS_HEADER*>(source);dos.e_magic=IMAGE_DOS_SIGNATURE;dos.e_lfanew=offset;
        auto& nt=*reinterpret_cast<IMAGE_NT_HEADERS64*>(source+offset);nt.Signature=IMAGE_NT_SIGNATURE;
        nt.FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;nt.FileHeader.NumberOfSections=2;
        nt.FileHeader.SizeOfOptionalHeader=sizeof(IMAGE_OPTIONAL_HEADER64);
        nt.OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;nt.OptionalHeader.SizeOfImage=0x4000;
        auto* sections=reinterpret_cast<IMAGE_SECTION_HEADER*>(&nt+1);
        sections[0].VirtualAddress=0x2000;sections[0].Misc.VirtualSize=0x1000;
        sections[0].Characteristics=IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_EXECUTE;
        sections[1].VirtualAddress=0x3000;sections[1].Misc.VirtualSize=0x1000;
        sections[1].Characteristics=IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE;
    };
    make();Image image{};apiReads=0;
    Check(Open(image,source) && image.count==2,"complete valid current header metadata");
    Check(apiReads==1,"common PE header layout uses one fresh API snapshot");
    for(unsigned field=0;field<10;++field) {
        make();Check(Open(image,source),"valid before live mutation");
        auto& dos=*reinterpret_cast<IMAGE_DOS_HEADER*>(source);
        auto& nt=*reinterpret_cast<IMAGE_NT_HEADERS64*>(source+0x80);
        auto* sections=reinterpret_cast<IMAGE_SECTION_HEADER*>(&nt+1);
        switch(field) {
        case 0:dos.e_magic=0;break;
        case 1:dos.e_lfanew=-1;break;
        case 2:nt.Signature=0;break;
        case 3:nt.FileHeader.Machine=IMAGE_FILE_MACHINE_I386;break;
        case 4:nt.FileHeader.NumberOfSections=97;break;
        case 5:nt.FileHeader.SizeOfOptionalHeader=0;break;
        case 6:nt.OptionalHeader.Magic=0;break;
        case 7:nt.OptionalHeader.SizeOfImage=0x800;break;
        case 8:sections[0].Misc.VirtualSize=0x4000;break;
        case 9:sections[1].VirtualAddress=0x2800;break;
        }
        Check(!Open(image,source),"fresh header mutation refused after warm admission");
    }
    make(0x1000);apiReads=0;
    Check(Open(image,source) && apiReads==3,"large header offset retains bounded fallback reads");
    make();DWORD old{};Check(VirtualProtect(source,0x1000,PAGE_NOACCESS,&old)!=0,"invalidate headers");
    Check(!Open(image,source),"unreadable current headers refused");
    Check(VirtualProtect(source,0x1000,PAGE_READWRITE,&old)!=0,"restore headers");
    // A small valid header at a readable-page boundary must still work when
    // the optional whole-page snapshot spans an inaccessible following page.
    std::array<unsigned char,0x200> small{};std::memcpy(small.data(),source,small.size());
    std::memcpy(source+0xe00,small.data(),small.size());
    Check(VirtualProtect(source+0x1000,0x1000,PAGE_NOACCESS,&old)!=0,"guard optional snapshot tail");
    apiReads=0;Check(Open(image,source+0xe00),"failed optional snapshot preserves legacy narrow-read admission");
    Check(apiReads==4,"fallback performs all three original narrow reads");
    VirtualFree(source,0,MEM_RELEASE);
    std::puts("Fresh PE headers: 3 API reads to 1; mutation, bounds and inaccessible-tail fallback retained.");
}
