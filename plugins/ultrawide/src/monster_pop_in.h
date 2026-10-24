#pragma once
#include <D2RLPlugin/api.h>
#include <D2RLPlugin/mutations.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>

namespace Ultrawide::MonsterPopIn {

// Canonical witnesses inside 0x4871A0 (ServerUpdateClientActiveRooms) and 0x48A200 (ServerRemoveClientFromActiveRooms):
// 0x487254 (15 bytes):
//   45 33 ff          xor r15d, r15d
//   4c 89 7d f0       mov [rbp - 0x10], r15
//   45 8b c7          mov r8d, r15d
//   44 89 7d 38       mov [rbp + 0x38], r15d
//   4d 85 f6          test r14, r14
inline constexpr unsigned char RoomDiffEntryExpected[]{
    0x45, 0x33, 0xff,
    0x4c, 0x89, 0x7d, 0xf0,
    0x45, 0x8b, 0xc7,
    0x44, 0x89, 0x7d, 0x38,
    0x4d, 0x85, 0xf6
};
// 0x487265 (22 bytes): native 1-hop room list fetch for pNewRoom
inline constexpr unsigned char RoomDiffFetchGuard[]{
    0x74, 0x14,
    0x4c, 0x8d, 0x45, 0x38,
    0x49, 0x8b, 0xce,
    0x48, 0x8d, 0x55, 0xf0,
    0xe8, 0x69, 0x8b, 0xe6, 0xff,
    0x44, 0x8b, 0x45, 0x38
};
// 0x4872A8 (11 bytes): native room-diff loop entry after fetching pNewRoom and pOldRoom lists
inline constexpr unsigned char RoomDiffLoopGuard[]{
    0x41, 0x8b, 0xff,
    0x45, 0x85, 0xc0,
    0x7e, 0x57,
    0x49, 0x8b, 0xdf
};
// 0x48A200 (16 bytes): ServerRemoveClientFromActiveRooms prologue
inline constexpr unsigned char DisconnectCleanupExpected[]{
    0x48, 0x89, 0x5c, 0x24, 0x10,
    0x57,
    0x48, 0x83, 0xec, 0x20,
    0x48, 0x8b, 0xfa,
    0x48, 0x8b, 0xd9,
    0x48, 0x85, 0xc9
};
// 0x3289A0 (24 bytes): RoomExEnsureActive(uint8_t act, D2RoomEx* pRoomEx)
// Materializes presets/sublevel/D2Room1 (+0x58) on a D2RoomEx without mutating DRLG stage refcounts.
inline constexpr unsigned char RoomExEnsureActiveGuard[]{
    0x48, 0x89, 0x5c, 0x24, 0x08,
    0x57,
    0x48, 0x83, 0xec, 0x20,
    0x8b, 0x42, 0x50,
    0x48, 0x8b, 0xda,
    0x0f, 0xb6, 0xf9,
    0x0f, 0xba, 0xe0, 0x18,
    0x72
};
// 0x2EFDE0 (13 bytes): Room1GetNeighbors(D2Room1* pRoom, D2Room1*** pppRooms, uint32_t* pCount)
inline constexpr unsigned char Room1GetNeighborsGuard[]{
    0x8b, 0x41, 0x40,
    0x41, 0x89, 0x00,
    0x48, 0x8b, 0x01,
    0x48, 0x89, 0x02,
    0xc3
};
// 0x2F0390 (17 bytes): Room1RemoveClient(D2Room1* pRoom, D2Client* pClient)
inline constexpr unsigned char Room1RemoveClientGuard[]{
    0x40, 0x53,
    0x48, 0x83, 0xec, 0x20,
    0x48, 0x8b, 0x41, 0x70,
    0x48, 0x8b, 0xd9,
    0x4c, 0x8b, 0x41, 0x68
};
// 0x4042A0 (16 bytes): ServerPopulateRoom1(D2Game* pGame, D2Room1* pRoom1)
inline constexpr unsigned char ServerPopulateRoom1Guard[]{
    0x48, 0x89, 0x5c, 0x24, 0x08,
    0x57,
    0x48, 0x83, 0xec, 0x20,
    0x48, 0x8b, 0xda,
    0x48, 0x8b, 0xf9,
    0xe8, 0x0d, 0x73, 0xa2, 0x03
};
// 0x48A149 (18 bytes): 1-hop room list fetch in 0x48A120 (ServerReplicateRemovedUnitsInRooms)
inline constexpr unsigned char RemovedUnitsFetchExpected[]{
    0x4c, 0x8d, 0x44, 0x24, 0x60,
    0x48, 0x8b, 0xcb,
    0x48, 0x8d, 0x54, 0x24, 0x68,
    0xe8, 0x85, 0x5c, 0xe6, 0xff
};
// 0x48A2F4 (21 bytes): 1-hop room list fetch in 0x48A270 (ServerReplicateDirtyUnitsInRooms)
inline constexpr unsigned char DirtyUnitsFetchExpected[]{
    0x4c, 0x8d, 0x84, 0x24, 0x88, 0x00, 0x00, 0x00,
    0x48, 0x8b, 0xce,
    0x48, 0x8d, 0x54, 0x24, 0x20,
    0xe8, 0xd7, 0x5a, 0xe6, 0xff
};

inline constexpr size_t MaxClients = 8;
inline constexpr size_t MaxActiveRooms = 169;

struct RoomList {
    std::array<uintptr_t, MaxActiveRooms> rooms{};
    uint32_t count{};

    bool Contains(uintptr_t room) const noexcept {
        for (uint32_t i = 0; i < count; ++i) {
            if (rooms[i] == room) return true;
        }
        return false;
    }
    bool AddUnique(uintptr_t room) noexcept {
        if (!room || Contains(room)) return false;
        if (count >= MaxActiveRooms) return false;
        rooms[count++] = room;
        return true;
    }
};

struct ClientRoomState {
    uintptr_t client{};
    uintptr_t lastRoom{};
    uint8_t act{};
    RoomList activeRooms{};
    RoomList scratchOldRooms{};
};

// Pure algorithm for computing the hopRadius-hop active room list using DRLG D2RoomEx neighbors:
// QueryRoom1NeighborsFn: bool(uintptr_t room1, const uintptr_t*& outArray, uint32_t& outCount)
// ExpandRoom1NeighborsFn: void(uint8_t act, uintptr_t room1, RoomList& outRooms)
template<class QueryRoom1NeighborsFn, class ExpandRoom1NeighborsFn>
inline void ReconcileClientNeighborhood(
    ClientRoomState& state,
    uint8_t act,
    uintptr_t pOldRoom,
    uintptr_t pNewRoom,
    QueryRoom1NeighborsFn&& queryRoom1Neighbors,
    ExpandRoom1NeighborsFn&& expandRoom1Neighbors,
    uint32_t hopRadius = 2U) noexcept
{
    const uint32_t effectiveRadius = std::clamp<uint32_t>(hopRadius, 1U, 3U);

    // Preserve previous active rooms in scratchOldRooms for the native diff loop
    if (pOldRoom && state.lastRoom == pOldRoom && state.act == act && state.activeRooms.count > 0) {
        state.scratchOldRooms = state.activeRooms;
    } else {
        state.scratchOldRooms = {};
        if (pOldRoom) {
            const uintptr_t* old1Hop{};
            uint32_t old1Count{};
            if (queryRoom1Neighbors(pOldRoom, old1Hop, old1Count) && old1Hop) {
                const uint32_t limit = std::min<uint32_t>(old1Count, static_cast<uint32_t>(MaxActiveRooms));
                for (uint32_t i = 0; i < limit; ++i) {
                    state.scratchOldRooms.AddUnique(old1Hop[i]);
                }
            }
        }
    }

    // Iteratively expand up to effectiveRadius hops around pNewRoom.
    // Native 0x2EF2F0(act, pOldRoom, pNewRoom) has ALREADY run at 0x48723D before our hook at 0x487254,
    // which promoted pNewRoom to Stage 0, 1-hop to Stage 1, 2-hop to Stage 2, and 3-hop to Stage 3.
    // Stage 2/3 D2RoomEx neighbors exist in [pRoomEx + 0x10..0x18] even if their D2Room1 (+0x58) has not
    // yet been materialized by the 5-tick background throttle (0x328A00). Calling expandRoom1Neighbors
    // walks [pRoomEx + 0x10..0x18] and invokes 0x3289A0(act, pNeighborRoomEx) to materialize D2Room1 (+0x58)
    // and its monsters immediately without touching DRLG stage refcounts or level refcounts.
    state.activeRooms = {};

    if (pNewRoom) {
        uint32_t frontierStart = 0;
        // Hop 1: neighbors of pNewRoom
        expandRoom1Neighbors(act, pNewRoom, state.activeRooms);
        uint32_t frontierEnd = state.activeRooms.count;

        for (uint32_t hop = 2U; hop <= effectiveRadius; ++hop) {
            const uint32_t nextStart = state.activeRooms.count;
            for (uint32_t i = frontierStart; i < frontierEnd; ++i) {
                const uintptr_t r = state.activeRooms.rooms[i];
                if (r) {
                    expandRoom1Neighbors(act, r, state.activeRooms);
                }
            }
            frontierStart = nextStart;
            frontierEnd = state.activeRooms.count;
        }
    }

    state.act = act;
    state.lastRoom = pNewRoom;
}

using RoomExEnsureActiveFn = uintptr_t(__fastcall*)(uint8_t act, uintptr_t pRoomEx);
using ServerPopulateRoom1Fn = void(__fastcall*)(uintptr_t pGame, uintptr_t pRoom1);
using Room1GetNeighborsFn = void(__fastcall*)(uintptr_t pRoom, const uintptr_t** pppRooms, uint32_t* pCount);
using Room1RemoveClientFn = void(__fastcall*)(uintptr_t pRoom, uintptr_t pClient);
using DisconnectCleanupFn = void(__fastcall*)(uintptr_t pRoom, uintptr_t pClient);

inline std::atomic<bool> enabled{false};
inline std::atomic<uint32_t> configuredHopRadius{3U};
inline std::atomic_flag stateLock = ATOMIC_FLAG_INIT;
inline uintptr_t exeBase{};
inline void* originalDisconnectCleanup{};
inline std::array<ClientRoomState, MaxClients> clientStates{};

inline ClientRoomState& GetOrAllocateClientState(uintptr_t pClient) noexcept {
    for (auto& s : clientStates) {
        if (s.client == pClient) return s;
    }
    for (auto& s : clientStates) {
        if (s.client == 0) {
            s = {};
            s.client = pClient;
            return s;
        }
    }
    clientStates[0] = {};
    clientStates[0].client = pClient;
    return clientStates[0];
}

inline void ClearAllStates() noexcept {
    while (stateLock.test_and_set(std::memory_order_acquire)) {}
    clientStates = {};
    stateLock.clear(std::memory_order_release);
}

inline void PrepareClientRoomsImpl(uintptr_t pClient, uintptr_t pOldRoom, uintptr_t pNewRoom, uintptr_t rbp) noexcept {
    const auto getNeighbors = reinterpret_cast<Room1GetNeighborsFn>(exeBase + 0x2efde0);
    const auto ensureRoomExActive = reinterpret_cast<RoomExEnsureActiveFn>(exeBase + 0x3289a0);
    const auto populateRoom1 = reinterpret_cast<ServerPopulateRoom1Fn>(exeBase + 0x4042a0);

    // Fallback to native 1-hop behavior if disabled or lock contended
    if (!enabled.load(std::memory_order_relaxed) || !pClient || stateLock.test_and_set(std::memory_order_acquire)) {
        const uintptr_t* newPtr{};
        uint32_t newCount{};
        if (pNewRoom) getNeighbors(pNewRoom, &newPtr, &newCount);
        const uintptr_t* oldPtr{};
        uint32_t oldCount{};
        if (pOldRoom) getNeighbors(pOldRoom, &oldPtr, &oldCount);
        *reinterpret_cast<const uintptr_t**>(rbp - 0x10) = newPtr;
        *reinterpret_cast<uint32_t*>(rbp + 0x38) = newCount;
        *reinterpret_cast<const uintptr_t**>(rbp - 0x18) = oldPtr;
        *reinterpret_cast<uint32_t*>(rbp + 0x40) = oldCount;
        return;
    }

    const uintptr_t pGame = *reinterpret_cast<const uintptr_t*>(pClient + 0x2b0);
    const uint8_t act = pGame ? *reinterpret_cast<const uint8_t*>(pGame + 0x106) : 0;
    auto& state = GetOrAllocateClientState(pClient);

    const auto ensureRoom1Populated = [&](uintptr_t room1) noexcept {
        if (!pGame || !room1 || room1 < 0x10000) return;
        const uint32_t flags = *reinterpret_cast<const uint32_t*>(room1 + 0x54);
        if ((flags & 3U) != 3U) {
            populateRoom1(pGame, room1);
        }
    };

    const auto queryRoom1Fn = [&](uintptr_t room, const uintptr_t*& outArray, uint32_t& outCount) noexcept -> bool {
        outArray = nullptr;
        outCount = 0;
        if (!room || room < 0x10000) return false;
        getNeighbors(room, &outArray, &outCount);
        return outArray != nullptr && outCount <= 64;
    };

    const auto expandRoom1Fn = [&](uint8_t roomAct, uintptr_t room1, RoomList& outRooms) noexcept {
        if (!room1 || room1 < 0x10000) return;
        ensureRoom1Populated(room1);
        outRooms.AddUnique(room1);
        // Walk DRLG D2RoomEx neighbor vector at [pRoomEx + 0x10..0x18] so Stage 2/3 rooms whose
        // D2Room1 (+0x58) was not yet materialized by 0x328A00 are materialized on-demand via 0x3289A0.
        const uintptr_t pRoomEx = *reinterpret_cast<const uintptr_t*>(room1 + 0x18);
        if (pRoomEx && pRoomEx >= 0x10000) {
            const uintptr_t* pExNeighbors = *reinterpret_cast<const uintptr_t* const*>(pRoomEx + 0x10);
            const uint64_t exCount = *reinterpret_cast<const uint64_t*>(pRoomEx + 0x18);
            if (pExNeighbors && exCount > 0 && exCount <= 64) {
                for (uint64_t i = 0; i < exCount; ++i) {
                    const uintptr_t neighborEx = pExNeighbors[i];
                    if (!neighborEx || neighborEx < 0x10000) continue;
                    uintptr_t neighborRoom1 = *reinterpret_cast<const uintptr_t*>(neighborEx + 0x58);
                    if (!neighborRoom1) {
                        neighborRoom1 = ensureRoomExActive(roomAct, neighborEx);
                    }
                    if (neighborRoom1 && neighborRoom1 >= 0x10000) {
                        ensureRoom1Populated(neighborRoom1);
                        outRooms.AddUnique(neighborRoom1);
                    }
                }
                return;
            }
        }
        // Fallback to D2Room1 neighbor list if D2RoomEx vector is empty
        const uintptr_t* nArray{};
        uint32_t nCount{};
        if (queryRoom1Fn(room1, nArray, nCount) && nArray) {
            for (uint32_t i = 0; i < nCount; ++i) {
                ensureRoom1Populated(nArray[i]);
                outRooms.AddUnique(nArray[i]);
            }
        }
    };

    ReconcileClientNeighborhood(
        state,
        act,
        pOldRoom,
        pNewRoom,
        queryRoom1Fn,
        expandRoom1Fn,
        configuredHopRadius.load(std::memory_order_relaxed));

    const uintptr_t* newPtr = state.activeRooms.count ? state.activeRooms.rooms.data() : nullptr;
    const uint32_t newCount = state.activeRooms.count;
    const uintptr_t* oldPtr = state.scratchOldRooms.count ? state.scratchOldRooms.rooms.data() : nullptr;
    const uint32_t oldCount = state.scratchOldRooms.count;

    *reinterpret_cast<const uintptr_t**>(rbp - 0x10) = newPtr;
    *reinterpret_cast<uint32_t*>(rbp + 0x38) = newCount;
    *reinterpret_cast<const uintptr_t**>(rbp - 0x18) = oldPtr;
    *reinterpret_cast<uint32_t*>(rbp + 0x40) = oldCount;

    stateLock.clear(std::memory_order_release);
}

inline void LoadClientActiveRoomsImpl(
    uintptr_t pPlayerRoom1,
    uintptr_t pClient,
    const uintptr_t** outRooms,
    uint32_t* outCount) noexcept
{
    const auto getNeighbors = reinterpret_cast<Room1GetNeighborsFn>(exeBase + 0x2efde0);
    if (enabled.load(std::memory_order_relaxed) && pClient && !stateLock.test_and_set(std::memory_order_acquire)) {
        for (const auto& s : clientStates) {
            if (s.client == pClient && s.lastRoom == pPlayerRoom1 && s.activeRooms.count > 0) {
                *outRooms = s.activeRooms.rooms.data();
                *outCount = s.activeRooms.count;
                stateLock.clear(std::memory_order_release);
                return;
            }
        }
        stateLock.clear(std::memory_order_release);
    }
    *outRooms = nullptr;
    *outCount = 0;
    if (pPlayerRoom1 && pPlayerRoom1 >= 0x10000) {
        getNeighbors(pPlayerRoom1, outRooms, outCount);
    }
}

inline void __fastcall HookDisconnectCleanup(uintptr_t pRoom, uintptr_t pClient) noexcept {
    const auto original = reinterpret_cast<DisconnectCleanupFn>(originalDisconnectCleanup);
    if (enabled.load(std::memory_order_relaxed) && pClient && !stateLock.test_and_set(std::memory_order_acquire)) {
        const auto getNeighbors = reinterpret_cast<Room1GetNeighborsFn>(exeBase + 0x2efde0);
        const auto removeClient = reinterpret_cast<Room1RemoveClientFn>(exeBase + 0x2f0390);
        // Native 0x48A200 will remove pClient from pRoom's 1-hop neighbors.
        // Because 0x2F0390 unconditionally decrements [pRoom1 + 0x70] even if pClient is not found,
        // we MUST only call 0x2F0390 on rooms in s.activeRooms that are NOT in pRoom's 1-hop neighbor list!
        const uintptr_t* native1Hop{};
        uint32_t native1HopCount{};
        if (pRoom && pRoom >= 0x10000) {
            getNeighbors(pRoom, &native1Hop, &native1HopCount);
            if (native1HopCount > 64) {
                native1Hop = nullptr;
                native1HopCount = 0;
            }
        }
        const auto isNative1Hop = [&](uintptr_t r) noexcept -> bool {
            if (!native1Hop) return false;
            for (uint32_t j = 0; j < native1HopCount; ++j) {
                if (native1Hop[j] == r) return true;
            }
            return false;
        };
        for (auto& s : clientStates) {
            if (s.client == pClient) {
                for (uint32_t i = 0; i < s.activeRooms.count; ++i) {
                    const uintptr_t r = s.activeRooms.rooms[i];
                    if (r && !isNative1Hop(r)) {
                        removeClient(r, pClient);
                    }
                }
                s = {};
                break;
            }
        }
        stateLock.clear(std::memory_order_release);
    }
    if (original) {
        original(pRoom, pClient);
    }
}

inline bool Guards(const D2RL::PluginContext* ctx) noexcept {
    return ctx->CheckExpectedBytes(0x487254, RoomDiffEntryExpected, sizeof(RoomDiffEntryExpected)) &&
        ctx->CheckExpectedBytes(0x487265, RoomDiffFetchGuard, sizeof(RoomDiffFetchGuard)) &&
        ctx->CheckExpectedBytes(0x4872a8, RoomDiffLoopGuard, sizeof(RoomDiffLoopGuard)) &&
        ctx->CheckExpectedBytes(0x48a200, DisconnectCleanupExpected, sizeof(DisconnectCleanupExpected)) &&
        ctx->CheckExpectedBytes(0x3289a0, RoomExEnsureActiveGuard, sizeof(RoomExEnsureActiveGuard)) &&
        ctx->CheckExpectedBytes(0x2efde0, Room1GetNeighborsGuard, sizeof(Room1GetNeighborsGuard)) &&
        ctx->CheckExpectedBytes(0x2f0390, Room1RemoveClientGuard, sizeof(Room1RemoveClientGuard)) &&
        ctx->CheckExpectedBytes(0x4042a0, ServerPopulateRoom1Guard, sizeof(ServerPopulateRoom1Guard)) &&
        ctx->CheckExpectedBytes(0x48a149, RemovedUnitsFetchExpected, sizeof(RemovedUnitsFetchExpected)) &&
        ctx->CheckExpectedBytes(0x48a2f4, DirtyUnitsFetchExpected, sizeof(DirtyUnitsFetchExpected));
}

enum class ApplyResult { Applied, Disabled, GuardRefused, ServiceUnavailable, StageFailed, CommitFailed, RollbackFailed };

inline ApplyResult Apply(
    const D2RL::PluginContext* ctx,
    const D2RL::MutationService* svc,
    bool enablePopInFix,
    void* roomDiffEntry,
    void* removedUnitsEntry,
    void* dirtyUnitsEntry,
    uint32_t hopRadius = 3U) noexcept
{
    using namespace D2RL::Mutations;
    if (!enablePopInFix) return ApplyResult::Disabled;
    if (!Guards(ctx) || !roomDiffEntry || !removedUnitsEntry || !dirtyUnitsEntry) return ApplyResult::GuardRefused;
    if (!D2RL::HasMutationServiceField(svc, D2RL::MutationServiceRequiredSize) ||
        !svc->beginTransaction || !svc->stageInlineHook || !svc->commit || !svc->cancelTransaction) {
        return ApplyResult::ServiceUnavailable;
    }
    if (!ctx->InstallInlineHook(
            0x48a200,
            DisconnectCleanupExpected,
            sizeof(DisconnectCleanupExpected),
            reinterpret_cast<void*>(&HookDisconnectCleanup),
            &originalDisconnectCleanup) || !originalDisconnectCleanup) {
        return ApplyResult::StageFailed;
    }

    TransactionHandle transaction{};
    if (svc->beginTransaction(ctx, &transaction) != Result::Success || !transaction) {
        return ApplyResult::StageFailed;
    }
    const InlineHookRequest diffHook{
        .structSize = InlineHookRequestSize,
        .rva = 0x487254,
        .expected = RoomDiffEntryExpected,
        .expectedSize = sizeof(RoomDiffEntryExpected),
        .target = roomDiffEntry
    };
    const InlineHookRequest removedHook{
        .structSize = InlineHookRequestSize,
        .rva = 0x48a149,
        .expected = RemovedUnitsFetchExpected,
        .expectedSize = sizeof(RemovedUnitsFetchExpected),
        .target = removedUnitsEntry
    };
    const InlineHookRequest dirtyHook{
        .structSize = InlineHookRequestSize,
        .rva = 0x48a2f4,
        .expected = DirtyUnitsFetchExpected,
        .expectedSize = sizeof(DirtyUnitsFetchExpected),
        .target = dirtyUnitsEntry
    };
    OperationHandle diffOp{}, removedOp{}, dirtyOp{};
    if (svc->stageInlineHook(ctx, transaction, &diffHook, &diffOp) != Result::Success || !diffOp ||
        svc->stageInlineHook(ctx, transaction, &removedHook, &removedOp) != Result::Success || !removedOp ||
        svc->stageInlineHook(ctx, transaction, &dirtyHook, &dirtyOp) != Result::Success || !dirtyOp) {
        svc->cancelTransaction(ctx, transaction);
        return ApplyResult::StageFailed;
    }

    CommitResult result{.structSize = CommitResultSize};
    const auto committed = svc->commit(ctx, transaction, &result);
    if (committed == Result::Success && result.result == Result::Success) {
        exeBase = ctx->exeBase;
        configuredHopRadius.store(std::clamp<uint32_t>(hopRadius, 1U, 3U), std::memory_order_release);
        enabled.store(true, std::memory_order_release);
        return ApplyResult::Applied;
    }
    const auto canceled = svc->cancelTransaction(ctx, transaction);
    if (!HasFlag(result.flags, CommitFlags::OriginalStateRestored) && canceled != Result::Success) {
        return ApplyResult::RollbackFailed;
    }
    return ApplyResult::CommitFailed;
}

}
