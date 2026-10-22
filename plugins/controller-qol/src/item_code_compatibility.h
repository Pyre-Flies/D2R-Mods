#pragma once
#include "item_code_contract.h"
#include "item_code_background_contract.h"
#include "item_code_controller_contract.h"
#include "item_code_drop_contract.h"
#include "item_code_socket_contract.h"
#include "item_code_label_only_contract.h"
#include "item_code_visibility_contract.h"
#include "item_code_opacity_contract.h"
#include "item_code_polling_contract.h"
#include "item_code_native_reads_contract.h"
#include "item_code_label_preflight_contract.h"
#include "item_code_resident_pages_contract.h"
#include "item_code_socket_batch_alt_contract.h"
#include "item_code_formatter_observation_contract.h"
#include "item_code_label_work_contract.h"
#include "chat_forwarding_memory.h"
#include "core_compatibility.h"
#include "loot_performance.h"
#include <D2RLPlugin/api.h>
#include <atomic>
#include <mutex>

namespace QolItemCode {
enum class ArtifactProfile {Unknown,Legacy,BackgroundFix,ControllerFix,DropFix,SocketFix,LabelOnly,Visibility,Opacity,Polling,NativeReads,LabelPreflight,LabelWork,FormatterObservation,ResidentPages,SocketBatchAlt};
// Cache only disk identity. Every admission still opens the live image and
// checks the complete matching forwarding graph, page ownership and continuation.
inline ArtifactProfile IdentifyArtifact(HMODULE module) noexcept {
    struct Known {ArtifactProfile profile;unsigned char hash[32];};
    static constexpr Known known[]={
        {ArtifactProfile::SocketBatchAlt,{0xcc,0x9d,0xfc,0x74,0x30,0x08,0x9a,0x74,0xd3,0x73,0x2b,0x2a,0x35,0x51,0xd9,0xce,0xe0,0x5d,0x35,0x17,0x94,0x0d,0x34,0xb4,0x79,0x50,0x58,0xbf,0x44,0x2b,0xd4,0xd7}}, // Reviewed 1.2.16 artifact.
        {ArtifactProfile::ResidentPages,{0x9e,0x7d,0xbf,0xf8,0x40,0x5b,0x38,0x9b,0x74,0x7d,0x88,0x18,0x08,0x75,0x10,0x7f,0x22,0x94,0x70,0x1d,0xee,0x91,0x52,0x1e,0xa9,0xc8,0x47,0x85,0x15,0x76,0x73,0xa8}}, // Reviewed 1.2.15 artifact.
        {ArtifactProfile::FormatterObservation,{0x2e,0xa0,0x89,0xcc,0x7e,0x69,0x82,0x85,0xd3,0x74,0xbe,0xa8,0x59,0x82,0x90,0x53,0xec,0x1c,0x1c,0xe3,0xed,0x69,0xe7,0xb1,0x30,0xc9,0x30,0xb0,0xdf,0x01,0x06,0x6f}}, // Reviewed 1.2.14 artifact.
        {ArtifactProfile::LabelWork,{0x94,0x12,0xc5,0xf6,0x17,0x59,0x18,0xb5,0xb1,0x1b,0xa7,0xde,0x63,0x1d,0x4f,0x79,0x5e,0xe8,0x7c,0xde,0x37,0xca,0xb9,0xb9,0xb5,0xfd,0x35,0x32,0x8a,0xac,0x32,0xf8}}, // Reviewed 1.2.13 artifact.
        {ArtifactProfile::LabelPreflight,{0x2c,0x8a,0xa2,0x35,0xf5,0x59,0x90,0x25,0x0f,0xcf,0x9d,0xa0,0x3e,0x61,0x15,0x91,0xfa,0xd1,0x48,0x52,0x86,0x49,0x4a,0x6a,0x2e,0x90,0x68,0x3a,0x4c,0x9f,0x6f,0x29}}, // Reviewed 1.2.12 artifact.
        {ArtifactProfile::NativeReads,{0xbc,0x98,0xe7,0x0a,0x31,0xc7,0x76,0x5b,0x9e,0x25,0x21,0x98,0x57,0xae,0xd6,0x23,0xec,0xa5,0x07,0xa2,0x7d,0xa3,0xa9,0x5c,0xba,0xfb,0x02,0x41,0xa9,0xce,0x72,0x3c}}, // Reviewed 1.2.11 artifact.
        {ArtifactProfile::Legacy,{0x7a,0xb7,0xf7,0x48,0xc9,0xc1,0x10,0xb3,0xec,0x91,0xe5,0xc6,0x9d,0x9a,0x21,0x12,0x5e,0xbd,0x7b,0xc1,0xe2,0x3d,0xc8,0x2e,0x47,0x5c,0x56,0x3d,0xd9,0x92,0xfd,0xcf}},
        {ArtifactProfile::BackgroundFix,{0xe7,0x60,0x6d,0x9e,0xd5,0xb7,0x9d,0xa8,0x0a,0x50,0xf1,0xef,0x9a,0xd8,0xa9,0x4d,0x9a,0xf8,0x42,0x59,0x24,0x7d,0x08,0xd3,0xd1,0x77,0xb1,0xcf,0xe8,0xf4,0x05,0xf6}},
        {ArtifactProfile::ControllerFix,{0x13,0x0e,0x04,0xf1,0xce,0x7a,0xf2,0x77,0x24,0x07,0x76,0x82,0xde,0x2d,0xc9,0x80,0x67,0x9a,0xce,0x2e,0xcc,0xff,0x62,0x51,0x4f,0x4b,0xee,0x93,0xa6,0xdb,0x61,0x6c}},
        {ArtifactProfile::DropFix,{0x99,0xf4,0x90,0x67,0x8d,0x70,0x0b,0xeb,0xbb,0x03,0xfe,0xba,0x09,0xcf,0x67,0xc8,0x8a,0xe4,0xba,0x94,0xc6,0x4a,0x61,0x81,0xe5,0x38,0x49,0xfb,0x66,0xec,0x20,0x7a}},
        {ArtifactProfile::SocketFix,{0xd6,0xb7,0x1c,0x25,0xbd,0xad,0xa6,0x3f,0x67,0x0e,0x4b,0x8f,0xe5,0x0e,0x50,0xa0,0x00,0x16,0x5f,0xad,0x72,0x1e,0x6c,0x6f,0x39,0x9d,0x01,0x77,0x30,0x30,0x83,0xeb}},
        {ArtifactProfile::LabelOnly,{0x9f,0x4a,0x21,0x9d,0x13,0x4b,0x31,0xff,0x99,0x6a,0xce,0xe6,0xd7,0xaa,0xd0,0x4b,0x10,0x67,0x06,0x0f,0x9c,0x12,0x04,0x20,0x61,0x57,0xbd,0xb6,0x5d,0x88,0x3d,0xa8}},
        {ArtifactProfile::Visibility,{0x98,0x54,0xdf,0xbd,0x08,0x2f,0x1f,0xc1,0x03,0xca,0x30,0xed,0x35,0x5e,0x07,0xa8,0xe5,0xf5,0x74,0x12,0x06,0xf3,0x70,0xd7,0x32,0xe2,0x6e,0x93,0x90,0x05,0xfa,0xe7}},
        {ArtifactProfile::Opacity,{0x1e,0x10,0x2e,0xf3,0x8b,0x1d,0x26,0xc5,0x6b,0xbe,0x12,0xa1,0x18,0xd7,0xf2,0x1e,0xa7,0x5e,0x9c,0xb1,0x0f,0x52,0x7f,0xca,0x70,0xb8,0x56,0xdc,0x34,0xd3,0x93,0xff}},
        {ArtifactProfile::Polling,{0x3e,0x5f,0x3e,0xd2,0xc5,0x81,0x90,0x35,0xab,0x37,0x6d,0xff,0x60,0xb1,0x59,0x7a,0x70,0xa0,0x71,0x04,0x47,0x37,0x0a,0xa3,0x0d,0xab,0x3e,0x5a,0xa2,0x26,0x2e,0x88}},
    };
    static std::mutex mutex;
    static HMODULE checked{};
    static ArtifactProfile identity=ArtifactProfile::Unknown;
    const std::lock_guard lock(mutex);
    if(module!=checked) {
        identity=ArtifactProfile::Unknown;
        unsigned char actual[32]{};bool readable{};
        (void)QolCore::VerifyFileHash(module,known[0].hash,actual,&readable);
        if(readable)for(const auto& candidate:known)
            if(!std::memcmp(actual,candidate.hash,sizeof(actual))) {identity=candidate.profile;break;}
        checked=module;
    }
    return module?identity:ArtifactProfile::Unknown;
}
inline bool Artifact(HMODULE module) noexcept {return IdentifyArtifact(module)==ArtifactProfile::Legacy;}
inline bool BackgroundFixArtifact(HMODULE module) noexcept {return IdentifyArtifact(module)==ArtifactProfile::BackgroundFix;}
inline bool ControllerFixArtifact(HMODULE module) noexcept {return IdentifyArtifact(module)==ArtifactProfile::ControllerFix;}
inline bool DropFixArtifact(HMODULE module) noexcept {return IdentifyArtifact(module)==ArtifactProfile::DropFix;}
inline bool SocketFixArtifact(HMODULE module) noexcept {return IdentifyArtifact(module)==ArtifactProfile::SocketFix;}
inline bool LabelOnlyArtifact(HMODULE module) noexcept {return IdentifyArtifact(module)==ArtifactProfile::LabelOnly;}
inline bool VisibilityArtifact(HMODULE module) noexcept {return IdentifyArtifact(module)==ArtifactProfile::Visibility;}
inline bool OpacityArtifact(HMODULE module) noexcept {return IdentifyArtifact(module)==ArtifactProfile::Opacity;}
inline bool PollingArtifact(HMODULE module) noexcept {return IdentifyArtifact(module)==ArtifactProfile::Polling;}
inline bool NativeReadsArtifact(HMODULE module) noexcept {return IdentifyArtifact(module)==ArtifactProfile::NativeReads;}
inline bool Compatible(uintptr_t game,const unsigned char* expected,size_t size) noexcept {
    QolPerf::Scope timing(QolPerf::Counters[QolPerf::Compatibility]);
    if(!expected || size<5 || size>sizeof(Original) || std::memcmp(expected,Original,size))return false;
    auto module=GetModuleHandleW(L"unhoarder.dll");
    const auto profile=IdentifyArtifact(module);
    QolChatForwarding::ImageMemory image{};
    if(profile==ArtifactProfile::Unknown || !image.Open(module,true))return false;
    const auto companion=reinterpret_cast<uintptr_t>(module);
    switch(profile) {
    case ArtifactProfile::SocketBatchAlt: return QolItemCodeSocketBatchAlt::Validate(image.Reader(),game,companion);
    case ArtifactProfile::ResidentPages: return QolItemCodeResidentPages::Validate(image.Reader(),game,companion);
    case ArtifactProfile::FormatterObservation: return QolItemCodeFormatterObservation::Validate(image.Reader(),game,companion);
    case ArtifactProfile::LabelWork: return QolItemCodeLabelWork::Validate(image.Reader(),game,companion);
    case ArtifactProfile::LabelPreflight: return QolItemCodeLabelPreflight::Validate(image.Reader(),game,companion);
    case ArtifactProfile::Legacy: return Validate(image.Reader(),game,companion);
    case ArtifactProfile::BackgroundFix: return QolItemCodeBackgroundFix::Validate(image.Reader(),game,companion);
    case ArtifactProfile::ControllerFix: return QolItemCodeControllerFix::Validate(image.Reader(),game,companion);
    case ArtifactProfile::DropFix: return QolItemCodeDropFix::Validate(image.Reader(),game,companion);
    case ArtifactProfile::SocketFix: return QolItemCodeSocketFix::Validate(image.Reader(),game,companion);
    case ArtifactProfile::LabelOnly: return QolItemCodeLabelOnly::Validate(image.Reader(),game,companion);
    case ArtifactProfile::Visibility: return QolItemCodeVisibility::Validate(image.Reader(),game,companion);
    case ArtifactProfile::Opacity: return QolItemCodeOpacity::Validate(image.Reader(),game,companion);
    case ArtifactProfile::Polling: return QolItemCodePolling::Validate(image.Reader(),game,companion);
    case ArtifactProfile::NativeReads: return QolItemCodeNativeReads::Validate(image.Reader(),game,companion);
    default:return false;
    }
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
