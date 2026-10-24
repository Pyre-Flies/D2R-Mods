#include "expansion.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>

namespace {
using namespace D2RL;
using namespace D2RL::Mutations;
using namespace TerrainProbe;
using Bytes = std::vector<unsigned char>;
struct Staged {
    uintptr_t rva;
    Bytes expected, replacement;
};
std::vector<Staged> staged;
std::map<uintptr_t, Bytes> memory;
unsigned failStage{}, commitCalls{}, cancels{};
bool failBegin{}, failCommit{}, rollback{true};

void Check(bool value, const char* name) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", name);
        std::exit(1);
    }
}

Result Begin(const PluginContext*, TransactionHandle* handle) noexcept {
    if (failBegin) return Result::Unavailable;
    *handle = 1;
    return Result::Success;
}

Result Stage(const PluginContext*, TransactionHandle, const BytePatchRequest* request, OperationHandle* operation) noexcept {
    if (staged.size() + 1 == failStage) return Result::Conflict;
    const auto old = static_cast<const unsigned char*>(request->expected);
    const auto next = static_cast<const unsigned char*>(request->bytes);
    staged.push_back({request->rva, Bytes(old, old + request->expectedSize), Bytes(next, next + request->size)});
    *operation = staged.size();
    return Result::Success;
}

Result Commit(const PluginContext*, TransactionHandle, CommitResult* result) noexcept {
    ++commitCalls;
    for (const auto& patch : staged) {
        if (memory[patch.rva] != patch.expected) {
            result->result = Result::ExpectedBytesMismatch;
            result->flags = CommitFlags::OriginalStateRestored;
            return result->result;
        }
    }
    const auto before = memory;
    for (size_t i = 0; i < staged.size(); ++i) {
        const auto& patch = staged[i];
        memory[patch.rva] = patch.replacement;
        if (failCommit && i == 126) {
            if (rollback) memory = before;
            result->result = rollback ? Result::CommitFailed : Result::RollbackFailed;
            result->flags = rollback ? CommitFlags::OriginalStateRestored : CommitFlags::None;
            return result->result;
        }
    }
    result->result = Result::Success;
    return Result::Success;
}

Result Cancel(const PluginContext*, TransactionHandle) noexcept {
    ++cancels;
    return Result::Success;
}

void Reset() {
    staged.clear();
    memory.clear();
    failStage = commitCalls = cancels = 0;
    failBegin = failCommit = false;
    rollback = true;
    for (const auto& patch : ExpansionProfile::Patches) {
        memory[patch.rva] = Bytes(patch.expected, patch.expected + patch.size);
    }
}
} // namespace

int main() {
    using Expansion::Publication;
    MutationService service{
        .serviceSize = MutationServiceSize,
        .serviceVersion = 1,
        .beginTransaction = Begin,
        .stageBytePatch = Stage,
        .commit = Commit,
        .cancelTransaction = Cancel
    };
    const PluginContext ctx{};
    const auto privateArena = reinterpret_cast<uintptr_t>(
        VirtualAlloc(nullptr, ExpansionProfile::ArenaBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Check(privateArena != 0 && Expansion::RegisterUnwind(privateArena), "private selector unwind registration");
    Check(Expansion::CopyBridges(privateArena, 65536), "private leaf bridges relocate external tails");
    for (const auto& blob : ExpansionProfile::Blobs) {
        const auto copied = reinterpret_cast<const unsigned char*>(privateArena + blob.offset);
        for (size_t at = 0; at < blob.size; ++at) {
            if (blob.externalDisplacement && at >= blob.externalDisplacement && at < blob.externalDisplacement + 4) continue;
            Check(copied[at] == blob.bytes[at], "bridge internal bytes retained");
        }
        if (blob.externalDisplacement) {
            int32_t prior{}, actual{};
            std::memcpy(&prior, blob.bytes + blob.externalDisplacement, 4);
            std::memcpy(&actual, copied + blob.externalDisplacement, 4);
            Check(static_cast<int64_t>(actual) == static_cast<int64_t>(prior) - 65536, "external tail stays in original image");
        }
        DWORD64 base{};
        Check(!RtlLookupFunctionEntry(privateArena + blob.offset, &base, nullptr), "stack-neutral bridge uses leaf unwinding");
    }
    if constexpr (ExpansionProfile::Side == 22) {
        Check(!Expansion::CopyBridges(privateArena, INT64_C(1) << 40), "out-of-range external tail refused");
    }
    std::memcpy(reinterpret_cast<void*>(privateArena + ExpansionProfile::CodeOffset), ExpansionProfile::Selector, sizeof(ExpansionProfile::Selector));
    for (const auto& point : ExpansionProfile::ControlPoints) {
        DWORD64 base{};
        const auto pc = privateArena + ExpansionProfile::CodeOffset + point.offset;
        const auto function = RtlLookupFunctionEntry(pc, &base, nullptr);
        if (ExpansionProfile::Selector[point.offset] == 0xc3) {
            Check(!function, "RET uses leaf unwind after POPFQ");
            continue;
        }
        Check(function && base == privateArena, "selector instruction has registered unwind metadata");
        DWORD64 stack[]{0x202, 0x12345678};
        CONTEXT context{};
        context.Rip = pc;
        context.Rsp = reinterpret_cast<DWORD64>(point.flagsPushed ? stack : stack + 1);
        context.R13 = 0x1357;
        context.R14 = 0x2468;
        PVOID handler{};
        DWORD64 frame{};
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, base, pc, function, &context, &handler, &frame, nullptr);
        Check(context.Rip == 0x12345678 && context.Rsp == reinterpret_cast<DWORD64>(stack + 2) &&
              context.R13 == 0x1357 && context.R14 == 0x2468,
              "Windows unwinds every selector instruction to caller with registers intact");
    }
    Expansion::arena = privateArena;
    Expansion::ReleaseUncommitted();

    // Verify cross-mod / Vanilla shader candidate path discovery
    PluginContext modCtx{};
    modCtx.activeMod = "Reimagined";
    modCtx.pluginConfigPath = L"F:\\game\\mods\\Reimagined\\d2rloader\\config\\ultrawide.toml";
    const auto modCandidates = Expansion::BuildShaderCandidates(&modCtx, L"F:\\game\\");
    Check(modCandidates.size() >= 4, "active mod produces mod + fallback shader candidates");
    Check(modCandidates[0].path == L"F:\\game\\mods\\Reimagined\\Reimagined.mpq\\data\\hd\\tc22sh\\" &&
          modCandidates[0].useRelativeMpqPrefix, "first candidate is activeMod MPQ directory");

    PluginContext vanillaCtx{};
    vanillaCtx.activeMod = nullptr;
    vanillaCtx.pluginConfigPath = L"F:\\game\\d2rloader\\config\\ultrawide.toml";
    const auto vanillaCandidates = Expansion::BuildShaderCandidates(&vanillaCtx, L"F:\\game\\");
    Check(!vanillaCandidates.empty() && !vanillaCandidates[0].useRelativeMpqPrefix, "vanilla candidate uses direct OS path prefix");
    std::string formattedPrefix{};
    Check(Expansion::FormatUtf8PathPrefix(L"F:\\game\\d2rloader\\data\\hd\\tc22sh\\", formattedPrefix) &&
          formattedPrefix == "F:/game/d2rloader/data/hd/tc22sh/", "UTF-8 forward-slash prefix formatting");

    Startup::Summary empty{};
    Check(Expansion::Empty(empty), "initial zero state");
    for (size_t i = 0; i < 5; ++i) {
        auto used = empty;
        used.nonzeroBytes[i] = 1;
        Check(!Expansion::Empty(used), "occupied relocated array refused");
    }
    auto used = empty;
    used.maskPointer = 1;
    Check(!Expansion::Empty(used), "retained leave-game resources refused");

    for (unsigned failure : {1U, 127U, static_cast<unsigned>(std::size(ExpansionProfile::Patches))}) {
        Reset();
        const auto original = memory;
        failStage = failure;
        Check(Expansion::Publish(&ctx, &service, 65536, []() { return true; }) == Publication::Refused, "stage failure refused");
        Check(memory == original && !commitCalls && cancels == 1, "stage failure changes no native or shader path bytes");
    }
    Reset();
    Check(Expansion::Publish(&ctx, &service, 65536, []() { return true; }) == Publication::Active, "complete paired publication");
    Check(staged.size() == std::size(ExpansionProfile::Patches) && commitCalls == 1 && !cancels && memory.at(0xd967a8)[0] == 0xe8,
          "selector and all cache edits share one commit");

    std::puts("Expansion startup, cross-mod shader resolution, relocation, and paired transaction tests passed.");
}
