#pragma once
#include "config.h"
#include <D2RLPlugin/api.h>
#include <D2RLPlugin/mutations.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

namespace Ultrawide::MonsterAi {

// Canonical witnesses inside 0x595750 (AiFindTarget):
// 0x595923 (6 bytes): mov r14d, 0x23 (default hostile perception distance = 35 subtiles)
inline constexpr unsigned char HostileDefaultExpected[]{
    0x41, 0xbe, 0x23, 0x00, 0x00, 0x00
};
// 0x595929 (12 bytes): mov [rsp+0x48], r14d; mov rbx, [rbx]; mov [rsp+0x3c], r14d
inline constexpr unsigned char HostileStoreGuard[]{
    0x44, 0x89, 0x74, 0x24, 0x48,
    0x48, 0x8b, 0x1b,
    0x44, 0x89, 0x74, 0x24, 0x3c
};
// 0x5959A7 (8 bytes): movzx edx, byte ptr [rsi + rcx + 0x8d] (monstats.aidist)
inline constexpr unsigned char AidistLoadGuard[]{
    0x0f, 0xb6, 0x94, 0x0e, 0x8d, 0x00, 0x00, 0x00
};
// 0x5959AF (18 bytes):
//   84 d2             test dl, dl
//   74 19             je 0x5959cc
//   49 8b cf          mov rcx, r15
//   89 54 24 48       mov [rsp+0x48], edx
//   44 8b f2          mov r14d, edx
//   89 54 24 3c       mov [rsp+0x3c], edx
inline constexpr unsigned char AidistOverrideExpected[]{
    0x84, 0xd2,
    0x74, 0x19,
    0x49, 0x8b, 0xcf,
    0x89, 0x54, 0x24, 0x48,
    0x44, 0x8b, 0xf2,
    0x89, 0x54, 0x24, 0x3c
};
// Replacement at 0x5959AF (18 bytes):
//   41 3b d6          cmp edx, r14d
//   76 18             jbe 0x5959cc  (keep monster_ai_range when aidist == 0 or aidist <= r14d)
//   89 54 24 48       mov [rsp+0x48], edx
//   44 8b f2          mov r14d, edx
//   89 54 24 3c       mov [rsp+0x3c], edx
//   eb 0b             jmp 0x5959cc  (rejoin shared mov rcx, r15; call 0x349440 tail)
inline constexpr unsigned char AidistMaxReplacement[]{
    0x41, 0x3b, 0xd6,
    0x76, 0x18,
    0x89, 0x54, 0x24, 0x48,
    0x44, 0x8b, 0xf2,
    0x89, 0x54, 0x24, 0x3c,
    0xeb, 0x0b
};
// 0x5959CC (12 bytes): mov rcx, r15; call 0x349440; mov [rsp+0x30], al
inline constexpr unsigned char AidistTailGuard[]{
    0x49, 0x8b, 0xcf,
    0xe8, 0x6c, 0x3a, 0xdb, 0xff,
    0x88, 0x44, 0x24, 0x30
};
// 0x595B19 (3 bytes): cmp esi, 0x37 (hardcoded 55-subtile player-slot ceiling)
inline constexpr unsigned char PlayerSlotCeilingExpected[]{
    0x83, 0xfe, 0x37
};
// 0x595B1C (10 bytes): jae 0x595b89; mov rcx, rdi; call 0x34c2c0
inline constexpr unsigned char PlayerSlotBranchGuard[]{
    0x73, 0x6b,
    0x48, 0x8b, 0xcf,
    0xe8, 0x9a, 0x67, 0xdb, 0xff
};
// 0x595D61 (6 bytes): mov r14d, 0x23 (friendly/minion scan distance = 35 subtiles)
inline constexpr unsigned char FriendlyDefaultExpected[]{
    0x41, 0xbe, 0x23, 0x00, 0x00, 0x00
};

inline uint8_t ComputePlayerSlotCap(uint32_t monsterAiRange) noexcept {
    const uint32_t clamped = std::clamp(monsterAiRange, Config::MinMonsterAiRange, Config::MaxMonsterAiRange);
    const uint32_t cap = std::max<uint32_t>(75U, std::min<uint32_t>(120U, clamped + 20U));
    return static_cast<uint8_t>(cap);
}

inline std::array<unsigned char, 6> BuildMovR14dImm32(uint32_t value) noexcept {
    const uint32_t clamped = std::clamp(value, Config::MinMonsterAiRange, Config::MaxMonsterAiRange);
    std::array<unsigned char, 6> out{0x41, 0xbe, 0, 0, 0, 0};
    std::memcpy(out.data() + 2, &clamped, sizeof(clamped));
    return out;
}

inline std::array<unsigned char, 3> BuildPlayerSlotCeilingPatch(uint32_t monsterAiRange) noexcept {
    return {0x83, 0xfe, ComputePlayerSlotCap(monsterAiRange)};
}

inline bool Guards(const D2RL::PluginContext* ctx, bool scaleFriendly) noexcept {
    const bool baseOk =
        ctx->CheckExpectedBytes(0x595923, HostileDefaultExpected, sizeof(HostileDefaultExpected)) &&
        ctx->CheckExpectedBytes(0x595929, HostileStoreGuard, sizeof(HostileStoreGuard)) &&
        ctx->CheckExpectedBytes(0x5959a7, AidistLoadGuard, sizeof(AidistLoadGuard)) &&
        ctx->CheckExpectedBytes(0x5959af, AidistOverrideExpected, sizeof(AidistOverrideExpected)) &&
        ctx->CheckExpectedBytes(0x5959cc, AidistTailGuard, sizeof(AidistTailGuard)) &&
        ctx->CheckExpectedBytes(0x595b19, PlayerSlotCeilingExpected, sizeof(PlayerSlotCeilingExpected)) &&
        ctx->CheckExpectedBytes(0x595b1c, PlayerSlotBranchGuard, sizeof(PlayerSlotBranchGuard));
    if (!baseOk) return false;
    if (scaleFriendly && !ctx->CheckExpectedBytes(0x595d61, FriendlyDefaultExpected, sizeof(FriendlyDefaultExpected))) {
        return false;
    }
    return true;
}

enum class ApplyResult { Applied, Disabled, GuardRefused, ServiceUnavailable, StageFailed, CommitFailed, RollbackFailed };

inline ApplyResult Apply(
    const D2RL::PluginContext* ctx,
    const D2RL::MutationService* svc,
    const Config::Settings& settings) noexcept
{
    using namespace D2RL::Mutations;
    if (!settings.enableMonsterAiRange) return ApplyResult::Disabled;
    if (!Guards(ctx, settings.scaleFriendlyAiRange)) return ApplyResult::GuardRefused;
    if (!D2RL::HasMutationServiceField(svc, D2RL::MutationServiceRequiredSize) ||
        !svc->beginTransaction || !svc->stageBytePatch || !svc->commit || !svc->cancelTransaction) {
        return ApplyResult::ServiceUnavailable;
    }

    TransactionHandle transaction{};
    if (svc->beginTransaction(ctx, &transaction) != Result::Success || !transaction) {
        return ApplyResult::StageFailed;
    }

    const auto hostileBytes = BuildMovR14dImm32(settings.monsterAiRange);
    const BytePatchRequest hostilePatch{
        .structSize = BytePatchRequestSize,
        .rva = 0x595923,
        .expected = HostileDefaultExpected,
        .expectedSize = sizeof(HostileDefaultExpected),
        .bytes = hostileBytes.data(),
        .size = static_cast<uint32_t>(hostileBytes.size())
    };
    OperationHandle opHostile{};
    if (svc->stageBytePatch(ctx, transaction, &hostilePatch, &opHostile) != Result::Success || !opHostile) {
        svc->cancelTransaction(ctx, transaction);
        return ApplyResult::StageFailed;
    }

    const BytePatchRequest aidistPatch{
        .structSize = BytePatchRequestSize,
        .rva = 0x5959af,
        .expected = AidistOverrideExpected,
        .expectedSize = sizeof(AidistOverrideExpected),
        .bytes = AidistMaxReplacement,
        .size = sizeof(AidistMaxReplacement)
    };
    OperationHandle opAidist{};
    if (svc->stageBytePatch(ctx, transaction, &aidistPatch, &opAidist) != Result::Success || !opAidist) {
        svc->cancelTransaction(ctx, transaction);
        return ApplyResult::StageFailed;
    }

    const auto ceilingBytes = BuildPlayerSlotCeilingPatch(settings.monsterAiRange);
    const BytePatchRequest ceilingPatch{
        .structSize = BytePatchRequestSize,
        .rva = 0x595b19,
        .expected = PlayerSlotCeilingExpected,
        .expectedSize = sizeof(PlayerSlotCeilingExpected),
        .bytes = ceilingBytes.data(),
        .size = static_cast<uint32_t>(ceilingBytes.size())
    };
    OperationHandle opCeiling{};
    if (svc->stageBytePatch(ctx, transaction, &ceilingPatch, &opCeiling) != Result::Success || !opCeiling) {
        svc->cancelTransaction(ctx, transaction);
        return ApplyResult::StageFailed;
    }

    if (settings.scaleFriendlyAiRange) {
        const auto friendlyBytes = BuildMovR14dImm32(settings.monsterAiRange);
        const BytePatchRequest friendlyPatch{
            .structSize = BytePatchRequestSize,
            .rva = 0x595d61,
            .expected = FriendlyDefaultExpected,
            .expectedSize = sizeof(FriendlyDefaultExpected),
            .bytes = friendlyBytes.data(),
            .size = static_cast<uint32_t>(friendlyBytes.size())
        };
        OperationHandle opFriendly{};
        if (svc->stageBytePatch(ctx, transaction, &friendlyPatch, &opFriendly) != Result::Success || !opFriendly) {
            svc->cancelTransaction(ctx, transaction);
            return ApplyResult::StageFailed;
        }
    }

    CommitResult result{.structSize = CommitResultSize};
    const auto committed = svc->commit(ctx, transaction, &result);
    if (committed == Result::Success && result.result == Result::Success) {
        return ApplyResult::Applied;
    }
    const auto canceled = svc->cancelTransaction(ctx, transaction);
    if (!HasFlag(result.flags, CommitFlags::OriginalStateRestored) && canceled != Result::Success) {
        return ApplyResult::RollbackFailed;
    }
    return ApplyResult::CommitFailed;
}

}
