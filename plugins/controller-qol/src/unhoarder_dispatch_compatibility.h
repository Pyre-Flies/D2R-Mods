#pragma once
#include "unhoarder_dispatch_contract.h"
#include "unhoarder_dispatch_background_contract.h"
#include "unhoarder_dispatch_controller_contract.h"
#include "unhoarder_dispatch_drop_contract.h"
#include "unhoarder_dispatch_socket_contract.h"
#include "unhoarder_dispatch_socket_batch_alt_contract.h"
#include "item_code_compatibility.h"
#include "chat_forwarding_memory.h"
#include "core_compatibility.h"
#include <D2RLPlugin/api.h>
#include <mutex>
#include <vector>
namespace QolUnhoarderDispatch {
inline bool Artifact(HMODULE module) noexcept {
    static std::mutex mutex;
    static HMODULE checked{};
    static bool accepted{};
    const std::lock_guard lock(mutex);
    if(module!=checked) {
        constexpr unsigned char hash[]={0x7a,0xb7,0xf7,0x48,0xc9,0xc1,0x10,0xb3,0xec,0x91,0xe5,0xc6,0x9d,0x9a,0x21,0x12,0x5e,0xbd,0x7b,0xc1,0xe2,0x3d,0xc8,0x2e,0x47,0x5c,0x56,0x3d,0xd9,0x92,0xfd,0xcf};
        accepted=QolCore::VerifyFileHash(module,hash);checked=module;
    }
    return module && accepted;
}
// The reviewed artifact has one executable section and no relocations there.
// Recheck ALL loaded executable bytes each use, including filtering helpers;
// a known on-disk hash never admits changed live behavior.
inline bool ExecutableImageFor(QolChatForwarding::ImageMemory& image,uintptr_t textRva,size_t textSize,const unsigned char (&textHash)[32]) noexcept {
    unsigned executableSections=0;
    for(unsigned i=0;i<image.count;++i) {
        const auto& section=image.sections[i];
        if(!(section.Characteristics&IMAGE_SCN_MEM_EXECUTE))continue;
        if(section.VirtualAddress!=textRva || section.Misc.VirtualSize!=textSize)return false;
        ++executableSections;
    }
    if(executableSections!=1)return false;
    try {
        thread_local std::vector<unsigned char> bytes;
        bytes.resize(textSize);
        if(!image.Read(image.base+textRva,bytes.data(),bytes.size(),QolChatForwarding::Kind::Code))return false;
        BCRYPT_ALG_HANDLE algorithm{};unsigned char digest[32]{};
        if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
        const auto result=BCryptHash(algorithm,nullptr,0,bytes.data(),static_cast<ULONG>(bytes.size()),digest,sizeof(digest));
        BCryptCloseAlgorithmProvider(algorithm,0);
        return result>=0 && std::memcmp(digest,textHash,sizeof(digest))==0;
    } catch(...) {return false;}
}
inline bool ValidateStructural(const QolChatForwarding::Memory& memory,uintptr_t game,uintptr_t companion,
    const unsigned char* original,size_t size) noexcept {
    using namespace QolChatForwarding;
    if(!memory.read || !original || size<32 || size>4096 || !game || !companion ||
       game>UINTPTR_MAX-Rva-size)return false;
    unsigned char entry[4096]{},relay[14]{},wrapper[160]{},trampoline[19]{};
    uintptr_t relayAddress{},destination{},slotAddress{},previous{},resume{};int32_t displacement{};
    const auto address=game+Rva;
    if(!memory.read(memory.user,address,entry,size,Kind::Trampoline) || entry[0]!=0xe9 ||
       std::memcmp(entry+5,original+5,size-5))return false;
    std::memcpy(&displacement,entry+1,4);
    if(!Relative(address+5,displacement,relayAddress) ||
       !memory.read(memory.user,relayAddress,relay,sizeof(relay),Kind::Trampoline) ||
       std::memcmp(relay,QolItemCode::Jump,6))return false;
    std::memcpy(&destination,relay+6,8);
    if(!memory.read(memory.user,destination,wrapper,sizeof(wrapper),Kind::Code))return false;
    // Locate RIP-indirect call (FF 15 disp32) or RIP-relative load (48 8B 05 disp32) for the original trampoline slot
    bool matchedTrampoline=false;
    for(size_t i=0;i+7<=sizeof(wrapper);++i) {
        slotAddress=0;
        if(wrapper[i]==0xff && wrapper[i+1]==0x15) {
            int32_t slotDisp{};std::memcpy(&slotDisp,wrapper+i+2,4);
            (void)Relative(destination+i+6,slotDisp,slotAddress);
        } else if(wrapper[i]==0x48 && wrapper[i+1]==0x8b && wrapper[i+2]==0x05) {
            int32_t slotDisp{};std::memcpy(&slotDisp,wrapper+i+3,4);
            (void)Relative(destination+i+7,slotDisp,slotAddress);
        }
        if(slotAddress && memory.read(memory.user,slotAddress,&previous,8,Kind::Data) && previous &&
           memory.read(memory.user,previous,trampoline,sizeof(trampoline),Kind::Trampoline) &&
           !std::memcmp(trampoline,original,5) && !std::memcmp(trampoline+5,QolItemCode::Jump,6)) {
            std::memcpy(&resume,trampoline+11,8);
            if(resume==address+5) {matchedTrampoline=true;break;}
        }
    }
    return matchedTrampoline && QolItemCode::ValidateStructural(memory,game,companion);
}
inline bool Check(const D2RL::PluginContext* context,const unsigned char* original,size_t size) noexcept {
    if(!context || !original || size>UINT32_MAX)return false;
    if(context->CheckExpectedBytes(Rva,original,static_cast<uint32_t>(size)))return true;
    const auto module=GetModuleHandleW(L"unhoarder.dll");
    QolChatForwarding::ImageMemory image{};
    if(!module || !image.Open(module))return false;
    if(QolItemCode::IdentifyArtifact(module)==QolItemCode::ArtifactProfile::Structural)return
        ValidateStructural(image.Reader(),context->exeBase,reinterpret_cast<uintptr_t>(module),original,size);
    namespace SocketBatchAlt=QolUnhoarderDispatchSocketBatchAlt;
    if(QolItemCode::IdentifyArtifact(module)==QolItemCode::ArtifactProfile::SocketBatchAlt)return
        ExecutableImageFor(image,SocketBatchAlt::TextRva,SocketBatchAlt::TextSize,SocketBatchAlt::TextHash) &&
        SocketBatchAlt::Validate(image.Reader(),context->exeBase,reinterpret_cast<uintptr_t>(module),original,size);
    if(Artifact(module))return ExecutableImageFor(image,TextRva,TextSize,TextHash) &&
        Validate(image.Reader(),context->exeBase,reinterpret_cast<uintptr_t>(module),original,size);
    namespace Fixed=QolUnhoarderDispatchBackgroundFix;
    if(QolItemCode::BackgroundFixArtifact(module)) return
        ExecutableImageFor(image,Fixed::TextRva,Fixed::TextSize,Fixed::TextHash) &&
        Fixed::Validate(image.Reader(),context->exeBase,reinterpret_cast<uintptr_t>(module),original,size);
    namespace Controller=QolUnhoarderDispatchControllerFix;
    if(QolItemCode::ControllerFixArtifact(module)) return
        ExecutableImageFor(image,Controller::TextRva,Controller::TextSize,Controller::TextHash) &&
        Controller::Validate(image.Reader(),context->exeBase,reinterpret_cast<uintptr_t>(module),original,size);
    namespace Drop=QolUnhoarderDispatchDropFix;
    if(QolItemCode::DropFixArtifact(module)) return
        ExecutableImageFor(image,Drop::TextRva,Drop::TextSize,Drop::TextHash) &&
        Drop::Validate(image.Reader(),context->exeBase,reinterpret_cast<uintptr_t>(module),original,size);
    namespace Socket=QolUnhoarderDispatchSocketFix;
    return QolItemCode::SocketFixArtifact(module) &&
        ExecutableImageFor(image,Socket::TextRva,Socket::TextSize,Socket::TextHash) &&
        Socket::Validate(image.Reader(),context->exeBase,reinterpret_cast<uintptr_t>(module),original,size);
}
}
