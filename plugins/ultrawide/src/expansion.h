#pragma once
#include "expansion_profile.h"
#include "startup.h"
#include "hash.h"
#include <D2RLPlugin/api.h>
#include <D2RLPlugin/mutations.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace TerrainProbe::Expansion {
inline uintptr_t arena{};
inline bool enabled{};
inline std::array<HANDLE, std::size(ExpansionProfile::Shaders)> files{};
inline PRUNTIME_FUNCTION unwindTable{};

inline bool RegisterUnwind(uintptr_t storage) noexcept {
    using namespace ExpansionProfile;
    static_assert(sizeof(UnwindRegion) == sizeof(RUNTIME_FUNCTION));
    auto table = reinterpret_cast<PRUNTIME_FUNCTION>(storage + PrefixOffset + 0x100);
    std::memcpy(table, UnwindRegions, sizeof(UnwindRegions));
    std::memcpy(reinterpret_cast<void*>(storage + PrefixOffset + 0x200), UnwindInfo, sizeof(UnwindInfo));
    if (!RtlAddFunctionTable(table, static_cast<DWORD>(std::size(UnwindRegions)), storage)) return false;
    unwindTable = table;
    return true;
}

inline void ReleaseUncommitted() noexcept {
    if (unwindTable) { RtlDeleteFunctionTable(unwindTable); unwindTable = nullptr; }
    if (arena) { VirtualFree(reinterpret_cast<void*>(arena), 0, MEM_RELEASE); arena = 0; }
}

inline bool Empty(const Startup::Summary& s) noexcept {
    return s.nonzeroBytes == std::array<uint32_t, 5>{} &&
        s.nonzeroRecords == std::array<uint32_t, 5>{} && !s.cpuMaterialSlots && !s.submittedSlots &&
        !s.maskPointer && !s.defaultMaterial && s.dimensions == std::array<int32_t, 3>{} &&
        s.cpuOrigin == std::array<int32_t, 2>{} && s.gpuState == std::array<int32_t, 6>{} &&
        s.ownerPrefix == std::array<unsigned char, 32>{};
}

inline bool Rebase(const ExpansionProfile::Patch& patch, int64_t delta, std::array<unsigned char, 32>& output) noexcept {
    if (!patch.size || patch.size > output.size() || patch.count > std::size(patch.offsets)) return false;
    std::memcpy(output.data(), patch.replacement, patch.size);
    for (unsigned i = 0; i < patch.count; ++i) {
        const auto offset = patch.offsets[i];
        if (offset + 4 > patch.size || (i && offset < patch.offsets[i - 1] + 4)) return false;
        int32_t old{};
        std::memcpy(&old, output.data() + offset, 4);
        const auto value = static_cast<int64_t>(old) + delta;
        if (value < std::numeric_limits<int32_t>::min() || value > std::numeric_limits<int32_t>::max()) return false;
        const auto replacement = static_cast<int32_t>(value);
        std::memcpy(output.data() + offset, &replacement, 4);
    }
    return true;
}

inline void CloseFiles() noexcept {
    for (auto& file : files) {
        if (file && file != INVALID_HANDLE_VALUE) CloseHandle(file);
        file = nullptr;
    }
}

inline bool CopyBridges(uintptr_t destination, int64_t delta) noexcept {
    for (const auto& blob : ExpansionProfile::Blobs) {
        auto output = reinterpret_cast<unsigned char*>(destination + blob.offset);
        std::memcpy(output, blob.bytes, blob.size);
        if (blob.externalDisplacement) {
            if (blob.externalDisplacement + 4 > blob.size) return false;
            int32_t prior{};
            std::memcpy(&prior, output + blob.externalDisplacement, 4);
            const auto value = static_cast<int64_t>(prior) - delta;
            if (value < INT32_MIN || value > INT32_MAX) return false;
            const auto adjusted = static_cast<int32_t>(value);
            std::memcpy(output + blob.externalDisplacement, &adjusted, 4);
        }
    }
    return true;
}

inline bool Read(uintptr_t address, void* output, size_t size) noexcept {
    SIZE_T copied{};
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), output, size, &copied) && copied == size;
}

inline bool Initial(const D2RL::PluginContext* ctx) noexcept {
    Startup::Summary a{}, b{};
    const auto reader = [&](uintptr_t rva, void* output, size_t size) noexcept { return Read(ctx->exeBase + rva, output, size); };
    uintptr_t buckets{}, renderer{}, stampOwner{}, decalOwner{};
    return Startup::Capture(reader, a) && Startup::Capture(reader, b) && a == b && Empty(a) &&
        reader(0x34f3b90, &buckets, 8) && !buckets && reader(0x34f6480, &renderer, 8) && !renderer &&
        reader(0x34fcb00, &stampOwner, 8) && !stampOwner && reader(0x34fcb08, &decalOwner, 8) && !decalOwner;
}

inline std::wstring WideFromUtf8(const char* utf8) {
    if (!utf8 || !*utf8) return {};
    const int needed = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (needed <= 1) return {};
    std::wstring out(static_cast<size_t>(needed - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, out.data(), needed);
    return out;
}

inline bool FormatUtf8PathPrefix(const std::wstring& dirWithTrailingSlash, std::string& outPrefix) {
    if (dirWithTrailingSlash.empty()) return false;
    const int needed = WideCharToMultiByte(CP_UTF8, 0, dirWithTrailingSlash.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (needed <= 1 || needed > 240) return false;
    std::string utf8(static_cast<size_t>(needed - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, dirWithTrailingSlash.c_str(), -1, utf8.data(), needed, nullptr, nullptr);
    std::replace(utf8.begin(), utf8.end(), '\\', '/');
    if (utf8.empty() || utf8.back() != '/') utf8.push_back('/');
    if (utf8.size() >= 240) return false;
    outPrefix = std::move(utf8);
    return true;
}

struct CandidateDirectory {
    std::wstring path{};
    bool useRelativeMpqPrefix{};
};

inline std::vector<CandidateDirectory> BuildShaderCandidates(const D2RL::PluginContext* ctx, const std::wstring& root) {
    using namespace ExpansionProfile;
    std::vector<CandidateDirectory> candidates{};
    const std::wstring suffix = std::wstring(L"data\\hd\\") + ShaderDirectory + L"\\";
    if (ctx && ctx->activeMod && *ctx->activeMod) {
        const auto modWide = WideFromUtf8(ctx->activeMod);
        if (!modWide.empty()) {
            candidates.push_back({root + L"mods\\" + modWide + L"\\" + modWide + L".mpq\\" + suffix, true});
            candidates.push_back({root + L"mods\\" + modWide + L"\\" + suffix, false});
            candidates.push_back({root + L"mods\\" + modWide + L"\\d2rloader\\" + suffix, false});
        }
    }
    if (ctx && ctx->pluginConfigPath && *ctx->pluginConfigPath) {
        std::wstring cfgDir(ctx->pluginConfigPath);
        const auto slash = cfgDir.find_last_of(L"\\/");
        if (slash != std::wstring::npos) {
            cfgDir.resize(slash + 1);
            // Check sibling ../data/hd/tc22sh/ relative to config/
            std::wstring trimmed = cfgDir.substr(0, cfgDir.size() - 1);
            const auto parentSlash = trimmed.find_last_of(L"\\/");
            if (parentSlash != std::wstring::npos) {
                std::wstring loaderBase = trimmed.substr(0, parentSlash + 1);
                candidates.push_back({loaderBase + suffix, false});
                candidates.push_back({loaderBase + L"plugins\\" + suffix, false});
            }
        }
    }
    HMODULE selfModule{};
    if (GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&CloseFiles),
            &selfModule) && selfModule) {
        wchar_t dllBuf[32768]{};
        const auto dllLen = GetModuleFileNameW(selfModule, dllBuf, 32768);
        if (dllLen > 0 && dllLen < 32768) {
            std::wstring dllDir(dllBuf, dllLen);
            const auto slash = dllDir.find_last_of(L"\\/");
            if (slash != std::wstring::npos) {
                dllDir.resize(slash + 1);
                candidates.push_back({dllDir + suffix, false});
            }
        }
    }
    candidates.push_back({root + L"d2rloader\\" + suffix, false});
    candidates.push_back({root + suffix, false});
    return candidates;
}

inline bool OpenAndVerifyShaders(
    const D2RL::PluginContext* ctx,
    const std::wstring& root,
    std::string& resolvedPrefix)
{
    using namespace ExpansionProfile;
    const auto candidates = BuildShaderCandidates(ctx, root);
    for (const auto& candidate : candidates) {
        CloseFiles();
        bool allValid = true;
        for (size_t i = 0; i < std::size(Shaders); ++i) {
            const auto shader = candidate.path + Shaders[i].name;
            files[i] = CreateFileW(shader.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            LARGE_INTEGER size{};
            if (files[i] == INVALID_HANDLE_VALUE || !GetFileSizeEx(files[i], &size) ||
                static_cast<uint64_t>(size.QuadPart) != Shaders[i].bytes || !Ultrawide::FileHash(shader.c_str(), Shaders[i].hash)) {
                allValid = false;
                break;
            }
        }
        if (!allValid) {
            CloseFiles();
            continue;
        }
        if (candidate.useRelativeMpqPrefix) {
            resolvedPrefix = reinterpret_cast<const char*>(Prefix);
            return true;
        }
        if (FormatUtf8PathPrefix(candidate.path, resolvedPrefix)) {
            return true;
        }
        CloseFiles();
    }
    return false;
}

enum class Publication { Active, Refused, Uncertain };

template<class StillInitial>
Publication Publish(
    const D2RL::PluginContext* ctx,
    const D2RL::MutationService* service,
    int64_t delta,
    StillInitial&& initial) noexcept
{
    using namespace D2RL::Mutations;
    TransactionHandle transaction{};
    if (service->beginTransaction(ctx, &transaction) != Result::Success || !transaction) return Publication::Refused;
    for (const auto& patch : ExpansionProfile::Patches) {
        std::array<unsigned char, 32> bytes{};
        OperationHandle operation{};
        const BytePatchRequest request{
            .structSize = BytePatchRequestSize,
            .rva = patch.rva,
            .expected = patch.expected,
            .expectedSize = patch.size,
            .bytes = bytes.data(),
            .size = patch.size
        };
        if (!Rebase(patch, delta, bytes) || service->stageBytePatch(ctx, transaction, &request, &operation) != Result::Success || !operation) {
            service->cancelTransaction(ctx, transaction);
            return Publication::Refused;
        }
    }
    if (!initial()) {
        service->cancelTransaction(ctx, transaction);
        return Publication::Refused;
    }
    CommitResult result{.structSize = CommitResultSize};
    const auto status = service->commit(ctx, transaction, &result);
    if (status == Result::Success && result.result == Result::Success) return Publication::Active;
    const bool restored = HasFlag(result.flags, CommitFlags::OriginalStateRestored);
    D2RL::LogInfoF(ctx, "Terrain expansion commit failed result=%u reason=%u original_restored=%u.",
        static_cast<unsigned>(status), static_cast<unsigned>(result.failureReason), restored ? 1U : 0U);
    if (restored) service->cancelTransaction(ctx, transaction);
    return restored ? Publication::Refused : Publication::Uncertain;
}

// All files are verified and held read-only before any native mutation. The
// selector and every size/array edit share one provider-owned transaction.
inline bool Try(const D2RL::PluginContext* ctx, bool requested = true) noexcept {
    using namespace ExpansionProfile;
    using namespace D2RL::Mutations;
    if (!requested) return true;
    try {
        wchar_t path[32768]{};
        const auto length = GetModuleFileNameW(reinterpret_cast<HMODULE>(ctx->exeBase), path, 32768);
        if (!length || length >= 32768) return true;
        std::wstring root(path, length);
        const auto slash = root.find_last_of(L"\\/");
        if (slash == std::wstring::npos) return true;
        root.resize(slash + 1);
        if (!Initial(ctx)) {
            ctx->LogWarn("Terrain expansion refused: initial cache, shader cache or renderer already occupied; original profile retained.");
            return true;
        }
        const D2RL::MutationService* service{};
        if (ctx->QueryService(&service) != D2RL::ServiceQueryResult::Success ||
            !D2RL::HasMutationServiceField(service, D2RL::MutationServiceRequiredSize) ||
            !service->beginTransaction || !service->stageBytePatch || !service->commit || !service->cancelTransaction) {
            ctx->LogWarn("Terrain expansion refused: atomic mutation service unavailable.");
            return true;
        }
        for (const auto& guard : Guards) {
            if (!ctx->CheckExpectedBytes(guard.rva, guard.bytes, static_cast<uint32_t>(guard.size))) {
                D2RL::LogInfoF(ctx, "Terrain expansion refused: preserved witness 0x%llx.", static_cast<unsigned long long>(guard.rva));
                return true;
            }
        }
        for (const auto& patch : Patches) {
            if (!ctx->CheckExpectedBytes(patch.rva, patch.expected, patch.size)) {
                D2RL::LogInfoF(ctx, "Terrain expansion refused: patch witness 0x%llx.", static_cast<unsigned long long>(patch.rva));
                return true;
            }
        }
        std::string resolvedPrefix{};
        if (!OpenAndVerifyShaders(ctx, root, resolvedPrefix)) {
            CloseFiles();
            ctx->LogWarn("Terrain expansion refused: paired 22x22 shaders (tc22sh) missing, changed or busy; original profile retained.");
            return true;
        }
        // Positive module-relative placement also satisfies the two native
        // [module + index*8 + disp32] instructions. Never overwrite an allocation.
        for (uintptr_t offset = TemplateArena; offset < 0x40000000; offset += 0x10000) {
            void* block = VirtualAlloc(reinterpret_cast<void*>(ctx->exeBase + offset), ArenaBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (block) {
                arena = reinterpret_cast<uintptr_t>(block);
                break;
            }
        }
        if (!arena) {
            CloseFiles();
            ctx->LogWarn("Terrain expansion refused: no nearby private storage.");
            return true;
        }
        const auto delta = static_cast<int64_t>(arena - ctx->exeBase) - static_cast<int64_t>(TemplateArena);
        for (const auto& constant : Constants) {
            std::memcpy(reinterpret_cast<void*>(arena + constant.offset), constant.bytes, constant.size);
        }
        std::memcpy(reinterpret_cast<void*>(arena + CodeOffset), Selector, sizeof(Selector));
        const bool bridges = CopyBridges(arena, delta);
        std::memset(reinterpret_cast<void*>(arena + PrefixOffset), 0, 240);
        std::memcpy(reinterpret_cast<void*>(arena + PrefixOffset), resolvedPrefix.data(), resolvedPrefix.size() + 1);
        int32_t displacement{};
        std::memcpy(&displacement, reinterpret_cast<void*>(arena + CodeOffset + SelectorFallbackDisplacement), 4);
        const auto relative = static_cast<int64_t>(displacement) - delta;
        bool prepared = bridges && relative >= INT32_MIN && relative <= INT32_MAX && RegisterUnwind(arena);
        displacement = prepared ? static_cast<int32_t>(relative) : 0;
        std::memcpy(reinterpret_cast<void*>(arena + CodeOffset + SelectorFallbackDisplacement), &displacement, 4);
        DWORD old{};
        prepared = prepared &&
            VirtualProtect(reinterpret_cast<void*>(arena + CodeOffset), 4096, PAGE_EXECUTE_READ, &old) &&
            VirtualProtect(reinterpret_cast<void*>(arena + PrefixOffset), 4096, PAGE_READONLY, &old) &&
            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(arena + CodeOffset), 4096);
        const auto publication = prepared ? Publish(ctx, service, delta, [&]() noexcept { return Initial(ctx); }) : Publication::Refused;
        prepared = publication == Publication::Active;
        if (publication == Publication::Uncertain) {
            // Retain every target allocation and shader handle for owner cleanup
            // if provider rollback is uncertain; do not free possibly live data.
            return false;
        }
        if (!prepared) {
            ReleaseUncommitted();
            CloseFiles();
            ctx->LogWarn("Terrain expansion refused during preparation; original cache and shader selection retained.");
            return true;
        }
        enabled = true;
        D2RL::LogInfoF(ctx, "Terrain expansion committed: side=%u cells=%u spans=%u paired_shaders=%u arena_rva=0x%llx prefix=%s.",
            Side, Cells, static_cast<unsigned>(std::size(Patches)), static_cast<unsigned>(std::size(Shaders)),
            static_cast<unsigned long long>(arena - ctx->exeBase), resolvedPrefix.c_str());
        return true;
    } catch (...) {
        // No throwing operations follow allocation or transaction creation.
        CloseFiles();
        ctx->LogWarn("Terrain expansion refused while preparing paths; original profile retained.");
        return true;
    }
}
// The process owns the committed arena until exit. Native shutdown can still
// hold relocated records after plugin callbacks stop; never free it on unload.
}
