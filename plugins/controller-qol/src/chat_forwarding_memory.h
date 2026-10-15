#pragma once
#include "chat_forwarding_contract.h"
#include <windows.h>

namespace QolChatForwarding {
// Only readable executable image sections belonging to this loaded companion
// supply code. Its object/cookie must live in writable, non-executable image
// data. No scan, guessed RVA, executable-only ownership test or cached proof.
struct ImageMemory {
    uintptr_t base{};
    uint32_t size{};
    IMAGE_SECTION_HEADER sections[96]{};
    unsigned count{};
    static bool Copy(uintptr_t address,void* output,size_t bytes) noexcept {
        SIZE_T got{};
        return address && bytes && address<=UINTPTR_MAX-bytes &&
            ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),output,bytes,&got) && got==bytes;
    }
    bool Open(HMODULE module) noexcept {
        base=reinterpret_cast<uintptr_t>(module);
        IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
        if(!Copy(base,&dos,sizeof(dos)) || dos.e_magic!=IMAGE_DOS_SIGNATURE || dos.e_lfanew<0 || dos.e_lfanew>0x1000 ||
           base>UINTPTR_MAX-static_cast<uintptr_t>(dos.e_lfanew))return false;
        const auto header=base+static_cast<uintptr_t>(dos.e_lfanew);
        if(!Copy(header,&nt,sizeof(nt)) || nt.Signature!=IMAGE_NT_SIGNATURE ||
           nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 || nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
           nt.FileHeader.SizeOfOptionalHeader!=sizeof(IMAGE_OPTIONAL_HEADER64) ||
           !nt.FileHeader.NumberOfSections || nt.FileHeader.NumberOfSections>std::size(sections) ||
           nt.OptionalHeader.SizeOfImage<0x1000 || nt.OptionalHeader.SizeOfImage>0x40000000)return false;
        size=nt.OptionalHeader.SizeOfImage;count=nt.FileHeader.NumberOfSections;
        const auto table=header+sizeof(nt);
        if(base>UINTPTR_MAX-size || table<base || table-base>size || count*sizeof(sections[0])>size-(table-base) ||
           !Copy(table,sections,count*sizeof(sections[0])))return false;
        for(unsigned i=0;i<count;++i) {
            const auto& section=sections[i];
            if(section.VirtualAddress>size || section.Misc.VirtualSize>size-section.VirtualAddress)return false;
            for(unsigned j=0;j<i;++j) {
                const auto& previous=sections[j];
                if(section.VirtualAddress<previous.VirtualAddress+previous.Misc.VirtualSize &&
                   previous.VirtualAddress<section.VirtualAddress+section.Misc.VirtualSize)return false;
            }
        }
        return true;
    }
    bool Read(uintptr_t address,void* output,size_t bytes,Kind kind) const noexcept {
        if(!bytes || address>UINTPTR_MAX-bytes)return false;
        if(kind!=Kind::Trampoline) {
            if(address<base || address-base>size || bytes>size-(address-base))return false;
            bool admitted=false;
            for(unsigned i=0;i<count;++i) {
                const auto& s=sections[i];const auto rva=address-base;
                if(rva<s.VirtualAddress || rva-s.VirtualAddress>s.Misc.VirtualSize || bytes>s.Misc.VirtualSize-(rva-s.VirtualAddress))continue;
                const auto flags=s.Characteristics;
                admitted=(flags&IMAGE_SCN_MEM_READ) && !(flags&IMAGE_SCN_MEM_DISCARDABLE) &&
                    (kind==Kind::Code?((flags&IMAGE_SCN_MEM_EXECUTE) && !(flags&IMAGE_SCN_MEM_WRITE)):
                                     ((flags&IMAGE_SCN_MEM_WRITE) && !(flags&IMAGE_SCN_MEM_EXECUTE)));
            }
            if(!admitted)return false;
        }
        for(auto cursor=address;cursor<address+bytes;) {
            MEMORY_BASIC_INFORMATION region{};
            if(!VirtualQuery(reinterpret_cast<void*>(cursor),&region,sizeof(region)) || region.State!=MEM_COMMIT ||
               (region.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
            const bool executable=(region.Protect&(PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))!=0;
            const bool writable=(region.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))!=0;
            if(kind==Kind::Data?(executable || !writable):!executable)return false;
            if(kind!=Kind::Trampoline && (region.Type!=MEM_IMAGE || reinterpret_cast<uintptr_t>(region.AllocationBase)!=base))return false;
            if(kind==Kind::Code && (region.Protect&(PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return false;
            const auto start=reinterpret_cast<uintptr_t>(region.BaseAddress);
            if(region.RegionSize>UINTPTR_MAX-start || start+region.RegionSize<=cursor)return false;
            cursor=start+region.RegionSize;
        }
        return Copy(address,output,bytes);
    }
    Memory Reader() noexcept {
        return {this,[](void* user,uintptr_t address,void* output,size_t bytes,Kind kind) noexcept {
            return static_cast<ImageMemory*>(user)->Read(address,output,bytes,kind);
        }};
    }
};
} // namespace QolChatForwarding
