#pragma once
#include "item_code_contract.h"
#include "item_code_background_contract.h"
#include "item_code_controller_contract.h"
#include "item_code_drop_contract.h"
#include "item_code_socket_contract.h"
#include "item_code_label_only_contract.h"
#include "item_code_visibility_contract.h"
#include "item_code_opacity_contract.h"
#include "chat_forwarding_memory.h"
#include "core_compatibility.h"
#include <D2RLPlugin/api.h>
#include <atomic>
#include <mutex>

namespace QolItemCode {
inline bool Artifact(HMODULE module) noexcept {
    static std::mutex mutex;
    static HMODULE checked{};
    static bool accepted{};
    const std::lock_guard lock(mutex);
    if(module!=checked) {
        constexpr unsigned char hash[]={0x7a,0xb7,0xf7,0x48,0xc9,0xc1,0x10,0xb3,0xec,0x91,0xe5,0xc6,0x9d,0x9a,0x21,0x12,0x5e,0xbd,0x7b,0xc1,0xe2,0x3d,0xc8,0x2e,0x47,0x5c,0x56,0x3d,0xd9,0x92,0xfd,0xcf};
        accepted=QolCore::VerifyFileHash(module,hash);checked=module;
    }
    // The disk identity is cached; the entire live graph is rechecked each use.
    return module && accepted;
}
inline bool BackgroundFixArtifact(HMODULE module) noexcept {
    static std::mutex mutex;
    static HMODULE checked{};
    static bool accepted{};
    const std::lock_guard lock(mutex);
    if(module!=checked) {
        constexpr unsigned char hash[]={0xe7,0x60,0x6d,0x9e,0xd5,0xb7,0x9d,0xa8,0x0a,0x50,0xf1,0xef,0x9a,0xd8,0xa9,0x4d,0x9a,0xf8,0x42,0x59,0x24,0x7d,0x08,0xd3,0xd1,0x77,0xb1,0xcf,0xe8,0xf4,0x05,0xf6};
        accepted=QolCore::VerifyFileHash(module,hash);checked=module;
    }
    // The disk identity is cached; the entire live graph is rechecked each use.
    return module && accepted;
}
inline bool ControllerFixArtifact(HMODULE module) noexcept {
    static std::mutex mutex;
    static HMODULE checked{};
    static bool accepted{};
    const std::lock_guard lock(mutex);
    if(module!=checked) {
        constexpr unsigned char hash[]={0x13,0x0e,0x04,0xf1,0xce,0x7a,0xf2,0x77,0x24,0x07,0x76,0x82,0xde,0x2d,0xc9,0x80,0x67,0x9a,0xce,0x2e,0xcc,0xff,0x62,0x51,0x4f,0x4b,0xee,0x93,0xa6,0xdb,0x61,0x6c};
        accepted=QolCore::VerifyFileHash(module,hash);checked=module;
    }
    // The disk identity is cached; the entire live graph is rechecked each use.
    return module && accepted;
}
inline bool DropFixArtifact(HMODULE module) noexcept {
    static std::mutex mutex;
    static HMODULE checked{};
    static bool accepted{};
    const std::lock_guard lock(mutex);
    if(module!=checked) {
        constexpr unsigned char hash[]={0x99,0xf4,0x90,0x67,0x8d,0x70,0x0b,0xeb,0xbb,0x03,0xfe,0xba,0x09,0xcf,0x67,0xc8,0x8a,0xe4,0xba,0x94,0xc6,0x4a,0x61,0x81,0xe5,0x38,0x49,0xfb,0x66,0xec,0x20,0x7a};
        accepted=QolCore::VerifyFileHash(module,hash);checked=module;
    }
    // The disk identity is cached; the entire live graph is rechecked each use.
    return module && accepted;
}
inline bool SocketFixArtifact(HMODULE module) noexcept {
    static std::mutex mutex;
    static HMODULE checked{};
    static bool accepted{};
    const std::lock_guard lock(mutex);
    if(module!=checked) {
        constexpr unsigned char hash[]={0xd6,0xb7,0x1c,0x25,0xbd,0xad,0xa6,0x3f,0x67,0x0e,0x4b,0x8f,0xe5,0x0e,0x50,0xa0,0x00,0x16,0x5f,0xad,0x72,0x1e,0x6c,0x6f,0x39,0x9d,0x01,0x77,0x30,0x30,0x83,0xeb};
        accepted=QolCore::VerifyFileHash(module,hash);checked=module;
    }
    // The disk identity is cached; the entire live graph is rechecked each use.
    return module && accepted;
}
inline bool LabelOnlyArtifact(HMODULE module) noexcept {
    static std::mutex mutex;
    static HMODULE checked{};
    static bool accepted{};
    const std::lock_guard lock(mutex);
    if(module!=checked) {
        constexpr unsigned char hash[]={0x9f,0x4a,0x21,0x9d,0x13,0x4b,0x31,0xff,0x99,0x6a,0xce,0xe6,0xd7,0xaa,0xd0,0x4b,0x10,0x67,0x06,0x0f,0x9c,0x12,0x04,0x20,0x61,0x57,0xbd,0xb6,0x5d,0x88,0x3d,0xa8};
        accepted=QolCore::VerifyFileHash(module,hash);checked=module;
    }
    // The disk identity is cached; the entire live graph is rechecked each use.
    return module && accepted;
}
inline bool VisibilityArtifact(HMODULE module) noexcept {
    static std::mutex mutex;
    static HMODULE checked{};
    static bool accepted{};
    const std::lock_guard lock(mutex);
    if(module!=checked) {
        constexpr unsigned char hash[]={0x98,0x54,0xdf,0xbd,0x08,0x2f,0x1f,0xc1,0x03,0xca,0x30,0xed,0x35,0x5e,0x07,0xa8,0xe5,0xf5,0x74,0x12,0x06,0xf3,0x70,0xd7,0x32,0xe2,0x6e,0x93,0x90,0x05,0xfa,0xe7};
        accepted=QolCore::VerifyFileHash(module,hash);checked=module;
    }
    // The disk identity is cached; the entire live graph is rechecked each use.
    return module && accepted;
}
inline bool OpacityArtifact(HMODULE module) noexcept {
    static std::mutex mutex;
    static HMODULE checked{};
    static bool accepted{};
    const std::lock_guard lock(mutex);
    if(module!=checked) {
        constexpr unsigned char hash[]={0x1e,0x10,0x2e,0xf3,0x8b,0x1d,0x26,0xc5,0x6b,0xbe,0x12,0xa1,0x18,0xd7,0xf2,0x1e,0xa7,0x5e,0x9c,0xb1,0x0f,0x52,0x7f,0xca,0x70,0xb8,0x56,0xdc,0x34,0xd3,0x93,0xff};
        accepted=QolCore::VerifyFileHash(module,hash);checked=module;
    }
    // The disk identity is cached; the entire live graph is rechecked each use.
    return module && accepted;
}
inline bool Compatible(uintptr_t game,const unsigned char* expected,size_t size) noexcept {
    if(!expected || size<5 || size>sizeof(Original) || std::memcmp(expected,Original,size))return false;
    auto module=GetModuleHandleW(L"unhoarder.dll");
    QolChatForwarding::ImageMemory image{};
    return module && image.Open(module) &&
        ((Validate(image.Reader(),game,reinterpret_cast<uintptr_t>(module)) && Artifact(module)) ||
         (QolItemCodeBackgroundFix::Validate(image.Reader(),game,reinterpret_cast<uintptr_t>(module)) && BackgroundFixArtifact(module)) ||
         (QolItemCodeControllerFix::Validate(image.Reader(),game,reinterpret_cast<uintptr_t>(module)) && ControllerFixArtifact(module)) ||
         (QolItemCodeDropFix::Validate(image.Reader(),game,reinterpret_cast<uintptr_t>(module)) && DropFixArtifact(module)) ||
         (QolItemCodeSocketFix::Validate(image.Reader(),game,reinterpret_cast<uintptr_t>(module)) && SocketFixArtifact(module)) ||
         (QolItemCodeLabelOnly::Validate(image.Reader(),game,reinterpret_cast<uintptr_t>(module)) && LabelOnlyArtifact(module)) ||
         (QolItemCodeVisibility::Validate(image.Reader(),game,reinterpret_cast<uintptr_t>(module)) && VisibilityArtifact(module)) ||
         (QolItemCodeOpacity::Validate(image.Reader(),game,reinterpret_cast<uintptr_t>(module)) && OpacityArtifact(module)));
}
// Keep existing direct/native and SDK diagnostic paths intact. Only the item
// code site gains this additional admission; all other guards stay exact.
inline bool Check(const D2RL::PluginContext* ctx,uintptr_t rva,const unsigned char* expected,size_t size) noexcept {
    if(!ctx)return false;
    if(rva==Rva && Compatible(ctx->exeBase,expected,size)) {
        static std::atomic<bool> logged{};
        if(!logged.exchange(true))ctx->LogInfo("[QOL/ItemCode] Verified admitted UnHoarder forwarding hook; native item-code calls preserved.");
        return true;
    }
    return size<=UINT32_MAX && ctx->CheckExpectedBytes(rva,expected,static_cast<uint32_t>(size));
}
inline bool Match(uintptr_t base,uintptr_t rva,const unsigned char* expected,size_t size) noexcept {
    if(rva==Rva && Compatible(base,expected,size))return true;
    __try {return base && base<=UINTPTR_MAX-rva && !std::memcmp(reinterpret_cast<const void*>(base+rva),expected,size);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
}
