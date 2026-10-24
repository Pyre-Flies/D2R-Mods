#include "bars_mouse.h"
#include "monster_ai.h"
#include "monster_pop_in.h"
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <utility>
#include <vector>

extern "C" {
std::atomic<unsigned char> UltrawideInSession{};
void* UltrawideMenuContinue{};
void* UltrawideGameContinue{};
void* UltrawideMenuLabel = reinterpret_cast<void*>(0x12345678ULL);
void* UltrawideMouseClampCheck{};
void* UltrawideMouseClampContinue{};
void* UltrawideMouseUnclampedExit{};
void* UltrawidePlacardUIRootOrigin{};
void* UltrawidePlacardUIRootSize{};
void* UltrawidePlacardFullWindowDims{};
void* UltrawidePlacardContinue{};
void* UltrawideAutomapContinue{};
void* UltrawideUIRootWidthContinue{};
void* UltrawideUIRootSub1{};
void* UltrawideUIRootSub2{};
void* UltrawideCornerHudUpdateContinue{};
void* UltrawideRoomDiffContinue{};
void* UltrawideRemovedUnitsContinue{};
void* UltrawideDirtyUnitsContinue{};
extern unsigned char MouseClampCheckReturnVal;
extern uintptr_t UIRootSub1SeenRcx;
extern uintptr_t UIRootSub2SeenRcx;
uintptr_t UltrawideMenuEntry();
uint32_t InvokeMouseEntryWithR15(uint32_t r15Val);
void InvokePlacardEntry(int32_t* outLeft, int32_t* outRight);
void InvokeAutomapEntry(uint64_t* outOriginRbx, uint64_t* outStackSlot28, uint64_t* outSizeRax);
uint32_t InvokeUIRootWidthEntry(void* fakeUIRoot, uint32_t fullVirtualWidth);
void InvokeCornerHudUpdateEntry(uintptr_t fakeUIRoot);
uint64_t InvokeRoomDiffEntryWithState(uintptr_t pClient, uintptr_t pOldRoom, uintptr_t pNewRoom, uintptr_t fakeRbp);
uint64_t InvokeRemovedUnitsEntry(uintptr_t pPlayerRoom1, uintptr_t pClient);
uint64_t InvokeDirtyUnitsEntry(uintptr_t pPlayerRoom1, uintptr_t pClient);
void MenuTestContinuation();
void GameTestContinuation();
void MouseClampCheckStub();
void MouseClampContinueStub();
void MouseUnclampedExitStub();
void PlacardUIRootOriginStub();
void PlacardUIRootSizeStub();
void PlacardFullWindowDimsStub();
void PlacardContinueStub();
void AutomapContinueStub();
void UIRootWidthContinueStub();
void UIRootSub1Stub();
void UIRootSub2Stub();
void CornerHudUpdateContinueStub();
void RoomDiffContinueStub();
void RemovedUnitsContinueStub();
void DirtyUnitsContinueStub();

static uintptr_t g_testAutoMapVt = 0xAA0001ULL;
static uintptr_t g_testHireablesVt = 0xAA0002ULL;
static uintptr_t g_testHudWarningsVt = 0xAA0003ULL;
static bool g_testCornerMinimap = true;
static bool g_testCornerMercenary = true;
static bool g_testCornerGameInfo = true;

void __fastcall UltrawideUpdateCornerHudPanels(uintptr_t uiRoot) noexcept {
    Ultrawide::BarsAndMouse::UpdateCornerHudPanelsImpl(
        uiRoot,
        UltrawideInSession.load(std::memory_order_acquire) != 0,
        g_testAutoMapVt,
        g_testHireablesVt,
        g_testHudWarningsVt,
        g_testCornerMinimap,
        g_testCornerMercenary,
        g_testCornerGameInfo);
}

static uintptr_t g_lastPrepClient{};
static uintptr_t g_lastPrepOldRoom{};
static uintptr_t g_lastPrepNewRoom{};
static uintptr_t g_lastLoadRoom{};
static uintptr_t g_lastLoadClient{};
static const uintptr_t g_fakeNewArray[3]{0x1001, 0x1002, 0x1003};
static const uintptr_t g_fakeOldArray[2]{0x2001, 0x2002};

void __fastcall UltrawidePrepareClientRooms(uintptr_t pClient, uintptr_t pOldRoom, uintptr_t pNewRoom, uintptr_t rbp) noexcept {
    g_lastPrepClient = pClient;
    g_lastPrepOldRoom = pOldRoom;
    g_lastPrepNewRoom = pNewRoom;
    *reinterpret_cast<const uintptr_t**>(rbp - 0x10) = g_fakeNewArray;
    *reinterpret_cast<uint32_t*>(rbp + 0x38) = 3;
    *reinterpret_cast<const uintptr_t**>(rbp - 0x18) = g_fakeOldArray;
    *reinterpret_cast<uint32_t*>(rbp + 0x40) = 2;
}
void __fastcall UltrawideLoadClientActiveRooms(uintptr_t pPlayerRoom1, uintptr_t pClient, const uintptr_t** outRooms, uint32_t* outCount) noexcept {
    g_lastLoadRoom = pPlayerRoom1;
    g_lastLoadClient = pClient;
    *outRooms = reinterpret_cast<const uintptr_t*>(0x12340000ULL);
    *outCount = 49U;
}
}

namespace {

void Check(bool value, const char* name) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", name);
        std::exit(1);
    }
}

void TestAssemblyGates() {
    UltrawideMenuContinue = reinterpret_cast<void*>(MenuTestContinuation);
    UltrawideGameContinue = reinterpret_cast<void*>(GameTestContinuation);
    for (unsigned i = 0; i < 4; ++i) {
        UltrawideInSession = static_cast<unsigned char>(i % 2);
        Check(UltrawideMenuEntry() == (i % 2 ? 0x87654321ULL : 0x12345678ULL), "menu gate session routing");
    }

    UltrawideMouseClampCheck = reinterpret_cast<void*>(MouseClampCheckStub);
    UltrawideMouseClampContinue = reinterpret_cast<void*>(MouseClampContinueStub);
    UltrawideMouseUnclampedExit = reinterpret_cast<void*>(MouseUnclampedExitStub);

    // In session: skips horizontal clamp check and jumps straight to unclamped exit
    UltrawideInSession = 1;
    MouseClampCheckReturnVal = 1;
    Check(InvokeMouseEntryWithR15(1440) == 0xCAFEBABEU, "mouse gate in-session bypasses clamp");

    // Out of session (menus) with clamp check returning true: converts r15d to float in xmm2 and continues clamp
    UltrawideInSession = 0;
    MouseClampCheckReturnVal = 1;
    Check(InvokeMouseEntryWithR15(1440) == 1440U, "mouse gate menu-mode replays r15d -> xmm2 conversion");

    // Out of session with clamp check returning false: jumps to unclamped exit
    MouseClampCheckReturnVal = 0;
    Check(InvokeMouseEntryWithR15(1440) == 0xCAFEBABEU, "mouse gate menu-mode respects native check=false");

    // Placard horizontal bounds gate
    UltrawidePlacardUIRootOrigin = reinterpret_cast<void*>(PlacardUIRootOriginStub);
    UltrawidePlacardUIRootSize = reinterpret_cast<void*>(PlacardUIRootSizeStub);
    UltrawidePlacardFullWindowDims = reinterpret_cast<void*>(PlacardFullWindowDimsStub);
    UltrawidePlacardContinue = reinterpret_cast<void*>(PlacardContinueStub);

    int32_t leftBound = -1, rightBound = -1;
    UltrawideInSession = 1;
    InvokePlacardEntry(&leftBound, &rightBound);
    Check(leftBound == 0 && rightBound == 5120, "placard gate in-session uses [0, fullWindowWidth]");

    leftBound = -1;
    rightBound = -1;
    UltrawideInSession = 0;
    InvokePlacardEntry(&leftBound, &rightBound);
    Check(leftBound == 1040 && rightBound == 4080, "placard gate menu-mode preserves 19:9 UI root bounds");

    // Full-screen Automap bounds gate (0x14E367C)
    UltrawideAutomapContinue = reinterpret_cast<void*>(AutomapContinueStub);
    uint64_t automapOrigin = 999, automapStack28 = 999, automapSize = 0;
    UltrawideInSession = 1;
    InvokeAutomapEntry(&automapOrigin, &automapStack28, &automapSize);
    Check(automapOrigin == 0 && automapStack28 == 0 && automapSize == 0x000005A000001400ULL,
        "automap gate in-session uses origin=(0,0) and size=(5120,1440)");

    automapOrigin = 999;
    automapStack28 = 999;
    automapSize = 0;
    UltrawideInSession = 0;
    InvokeAutomapEntry(&automapOrigin, &automapStack28, &automapSize);
    Check(automapOrigin == 0x0000000000000410ULL && automapStack28 == 0x0000000000000410ULL && automapSize == 0x000005A000000BE0ULL,
        "automap gate menu-mode preserves 19:9 UI root origin and size");

    // UIRootWidget::UpdateLayout 19:9 horizontal clamp gate (0x8A6635)
    UltrawideUIRootWidthContinue = reinterpret_cast<void*>(UIRootWidthContinueStub);
    alignas(16) unsigned char fakeUIRoot[256]{};
    // In session + in-game ([rdi+0xD8] == 0): unclamps to full virtual width (7680)
    UltrawideInSession = 1;
    fakeUIRoot[0xD8] = 0;
    Check(InvokeUIRootWidthEntry(fakeUIRoot, 7680U) == 7680U, "UIRoot width gate in-session in-game unclamps 7680");
    // In session + front-end ([rdi+0xD8] == 1): clamps to 4560 (0x11D0)
    fakeUIRoot[0xD8] = 1;
    Check(InvokeUIRootWidthEntry(fakeUIRoot, 7680U) == 4560U, "UIRoot width gate front-end flag clamps to 4560");
    // Out of session: clamps to 4560 (0x11D0)
    UltrawideInSession = 0;
    fakeUIRoot[0xD8] = 0;
    Check(InvokeUIRootWidthEntry(fakeUIRoot, 7680U) == 4560U, "UIRoot width gate menu-mode clamps to 4560");

    // UIRootWidget::Update top-corner HUD panel m_anchor.x gate (0x8A6461)
    UltrawideUIRootSub1 = reinterpret_cast<void*>(UIRootSub1Stub);
    UltrawideUIRootSub2 = reinterpret_cast<void*>(UIRootSub2Stub);
    UltrawideCornerHudUpdateContinue = reinterpret_cast<void*>(CornerHudUpdateContinueStub);

    // Centered 19:9 inside 5120x1440:
    // uiRoot.x (+0x70) = 1040, uiRoot.width (+0x78) = 4560, uiRoot.scale (+0x80) = 1440/2160 = 2/3.
    // Scaled root width = 4560 * (2/3) = 3040.0f -> anchorDelta = 1040.0f / 3040.0f = 13.0f / 38.0f.
    *reinterpret_cast<int32_t*>(fakeUIRoot + 0x70) = 1040;
    *reinterpret_cast<int32_t*>(fakeUIRoot + 0x78) = 4560;
    *reinterpret_cast<float*>(fakeUIRoot + 0x80) = 1440.0f / 2160.0f;

    alignas(16) unsigned char fakeAutoMap[160]{};
    *reinterpret_cast<uintptr_t*>(fakeAutoMap) = g_testAutoMapVt;
    *reinterpret_cast<float*>(fakeAutoMap + 0x48) = 1.0f; // right-anchored minimap

    alignas(16) unsigned char fakeHireables[160]{};
    *reinterpret_cast<uintptr_t*>(fakeHireables) = g_testHireablesVt;
    *reinterpret_cast<float*>(fakeHireables + 0x48) = 0.0f; // left-anchored mercenary

    alignas(16) unsigned char fakeWarnings[160]{};
    *reinterpret_cast<uintptr_t*>(fakeWarnings) = g_testHudWarningsVt;
    *reinterpret_cast<float*>(fakeWarnings + 0x48) = 1.0f; // right-anchored game info

    const uintptr_t rootChildren[3]{
        reinterpret_cast<uintptr_t>(fakeAutoMap),
        reinterpret_cast<uintptr_t>(fakeHireables),
        reinterpret_cast<uintptr_t>(fakeWarnings)
    };
    *reinterpret_cast<const uintptr_t**>(fakeUIRoot + 0x58) = rootChildren;
    *reinterpret_cast<uint64_t*>(fakeUIRoot + 0x60) = 3;

    UltrawideInSession = 1;
    InvokeCornerHudUpdateEntry(reinterpret_cast<uintptr_t>(fakeUIRoot));
    Check(UIRootSub1SeenRcx == reinterpret_cast<uintptr_t>(fakeUIRoot) &&
          UIRootSub2SeenRcx == reinterpret_cast<uintptr_t>(fakeUIRoot),
          "corner HUD update gate replays both UIRootWidget helper calls with rcx = uiRoot");

    const float expectedDelta = 1040.0f / (4560.0f * (1440.0f / 2160.0f));
    auto approxEq = [](float a, float b) { return (a - b < 1e-5f) && (b - a < 1e-5f); };
    Check(approxEq(*reinterpret_cast<float*>(fakeAutoMap + 0x48), 1.0f + expectedDelta),
        "right-corner minimap m_anchor.x shifts to 1.0f + anchorDelta");
    Check(approxEq(*reinterpret_cast<float*>(fakeHireables + 0x48), -expectedDelta),
        "mercenary HireablesPanel m_anchor.x shifts to -anchorDelta");
    Check(approxEq(*reinterpret_cast<float*>(fakeWarnings + 0x48), 1.0f + expectedDelta),
        "HUDWarningsPanel m_anchor.x shifts to 1.0f + anchorDelta");

    // Calling a second time is idempotent (no accumulation)
    InvokeCornerHudUpdateEntry(reinterpret_cast<uintptr_t>(fakeUIRoot));
    Check(approxEq(*reinterpret_cast<float*>(fakeAutoMap + 0x48), 1.0f + expectedDelta) &&
          approxEq(*reinterpret_cast<float*>(fakeHireables + 0x48), -expectedDelta),
          "corner HUD m_anchor.x update is idempotent across frames");

    // Left-anchored minimap (m_anchor.x = 0.0f) shifts to -anchorDelta
    *reinterpret_cast<float*>(fakeAutoMap + 0x48) = 0.0f;
    InvokeCornerHudUpdateEntry(reinterpret_cast<uintptr_t>(fakeUIRoot));
    Check(approxEq(*reinterpret_cast<float*>(fakeAutoMap + 0x48), -expectedDelta),
        "left-corner minimap m_anchor.x shifts to -anchorDelta");

    // When UIRootWidget is full-length (rootOffsetX == 0), anchors reset to 0.0f / 1.0f
    *reinterpret_cast<int32_t*>(fakeUIRoot + 0x70) = 0;
    InvokeCornerHudUpdateEntry(reinterpret_cast<uintptr_t>(fakeUIRoot));
    Check(approxEq(*reinterpret_cast<float*>(fakeAutoMap + 0x48), 0.0f) &&
          approxEq(*reinterpret_cast<float*>(fakeHireables + 0x48), 0.0f) &&
          approxEq(*reinterpret_cast<float*>(fakeWarnings + 0x48), 1.0f),
          "full-length HUD (uiRoot.x == 0) restores native 0.0f / 1.0f anchors");

    // Room diff assembly bridge
    UltrawideRoomDiffContinue = reinterpret_cast<void*>(RoomDiffContinueStub);
    alignas(16) unsigned char fakeStack[256]{};
    uintptr_t fakeRbp = reinterpret_cast<uintptr_t>(fakeStack + 128);
    const uint64_t packedCounts = InvokeRoomDiffEntryWithState(0xAAA0, 0xBBB0, 0xCCC0, fakeRbp);
    Check(g_lastPrepClient == 0xAAA0 && g_lastPrepOldRoom == 0xBBB0 && g_lastPrepNewRoom == 0xCCC0, "room diff bridge forwards rsi/rbx/r14");
    Check((packedCounts & 0xFFFFFFFFULL) == 3ULL && (packedCounts >> 32) == 2ULL, "room diff bridge loads r8d=newCount and edx=oldCount");
    Check(*reinterpret_cast<const uintptr_t**>(fakeRbp + 0x48) == g_fakeOldArray, "room diff bridge loads r10=[rbp-0x18]");
    Check(*reinterpret_cast<const uint32_t*>(fakeRbp + 0x50) == 0U, "room diff bridge zeroes r15d");

    // Removed-units and dirty-units room-list bridges (0x48A149 and 0x48A2F4)
    UltrawideRemovedUnitsContinue = reinterpret_cast<void*>(RemovedUnitsContinueStub);
    UltrawideDirtyUnitsContinue = reinterpret_cast<void*>(DirtyUnitsContinueStub);
    const uint64_t removedRes = InvokeRemovedUnitsEntry(0x1110, 0x2220);
    Check(g_lastLoadRoom == 0x1110 && g_lastLoadClient == 0x2220, "removed-units bridge forwards rbx/r14");
    Check((removedRes & 0xFFFFFFFFULL) == 0x12340000ULL && (removedRes >> 32) == 49ULL, "removed-units bridge populates [rsp+0x68] and [rsp+0x60]");
    const uint64_t dirtyRes = InvokeDirtyUnitsEntry(0x3330, 0x4440);
    Check(g_lastLoadRoom == 0x3330 && g_lastLoadClient == 0x4440, "dirty-units bridge forwards rsi/rdi");
    Check((dirtyRes & 0xFFFFFFFFULL) == 0x12340000ULL && (dirtyRes >> 32) == 49ULL, "dirty-units bridge populates [rsp+0x20] and [rsp+0x88]");
}

void TestMonsterPopInReconcile() {
    using namespace Ultrawide::MonsterPopIn;
    // Construct a 5x5 grid of room pointers: roomId(x, y) = 0x10000 + y * 10 + x for x,y in [0..4]
    auto roomId = [](int x, int y) -> uintptr_t {
        return static_cast<uintptr_t>(0x10000 + y * 10 + x);
    };
    std::map<uintptr_t, std::vector<uintptr_t>> graph;
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 5; ++x) {
            std::vector<uintptr_t> neighbors;
            neighbors.push_back(roomId(x, y));
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && nx < 5 && ny >= 0 && ny < 5) {
                        neighbors.push_back(roomId(nx, ny));
                    }
                }
            }
            graph[roomId(x, y)] = std::move(neighbors);
        }
    }

    std::vector<uintptr_t> expandedRooms;
    auto queryNeighbors = [&](uintptr_t room, const uintptr_t*& outArray, uint32_t& outCount) noexcept -> bool {
        auto it = graph.find(room);
        if (it == graph.end()) return false;
        outArray = it->second.data();
        outCount = static_cast<uint32_t>(it->second.size());
        return true;
    };
    auto expandNeighbors = [&](uint8_t, uintptr_t room, RoomList& outRooms) noexcept {
        expandedRooms.push_back(room);
        const uintptr_t* nArray{};
        uint32_t nCount{};
        if (queryNeighbors(room, nArray, nCount) && nArray) {
            for (uint32_t i = 0; i < nCount; ++i) {
                outRooms.AddUnique(nArray[i]);
            }
        }
    };

    ClientRoomState state{};
    // Center room (2, 2): 1-hop has 9 rooms; 2-hop has all 25 rooms!
    ReconcileClientNeighborhood(state, 0, 0, roomId(2, 2), queryNeighbors, expandNeighbors);
    Check(state.activeRooms.count == 25, "center room expands to all 25 rooms in 5x5 grid");
    Check(state.scratchOldRooms.count == 0, "initial transition has 0 old rooms");
    Check(expandedRooms.size() == 10, "1 center + 9 1-hop rooms expanded on initial 2-hop entry");

    // Step to (4, 2) on the far right edge: previous 25 active rooms move to scratchOldRooms;
    // 1-hop around (4, 2) has 6 rooms, and 2-hop covers x in [2..4], y in [0..4] = 15 rooms
    expandedRooms.clear();
    ReconcileClientNeighborhood(state, 0, roomId(2, 2), roomId(4, 2), queryNeighbors, expandNeighbors, 2U);
    Check(state.scratchOldRooms.count == 25, "previous 2-hop rooms preserved in scratchOldRooms for diffing");
    Check(state.activeRooms.count == 15, "2-hop from (4,2) covers 3x5 = 15 rooms");
    Check(expandedRooms.size() == 7, "1 center + 6 1-hop rooms expanded on edge 2-hop entry");

    // Construct a 7x7 grid of room pointers and verify 3-hop (7x7 = 49 rooms)
    std::map<uintptr_t, std::vector<uintptr_t>> graph7;
    for (int y = 0; y < 7; ++y) {
        for (int x = 0; x < 7; ++x) {
            std::vector<uintptr_t> neighbors;
            neighbors.push_back(roomId(x, y));
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && nx < 7 && ny >= 0 && ny < 7) {
                        neighbors.push_back(roomId(nx, ny));
                    }
                }
            }
            graph7[roomId(x, y)] = std::move(neighbors);
        }
    }
    auto queryNeighbors7 = [&](uintptr_t room, const uintptr_t*& outArray, uint32_t& outCount) noexcept -> bool {
        auto it = graph7.find(room);
        if (it == graph7.end()) return false;
        outArray = it->second.data();
        outCount = static_cast<uint32_t>(it->second.size());
        return true;
    };
    auto expandNeighbors7 = [&](uint8_t, uintptr_t room, RoomList& outRooms) noexcept {
        expandedRooms.push_back(room);
        const uintptr_t* nArray{};
        uint32_t nCount{};
        if (queryNeighbors7(room, nArray, nCount) && nArray) {
            for (uint32_t i = 0; i < nCount; ++i) {
                outRooms.AddUnique(nArray[i]);
            }
        }
    };
    ClientRoomState state3Hop{};
    expandedRooms.clear();
    ReconcileClientNeighborhood(state3Hop, 0, 0, roomId(3, 3), queryNeighbors7, expandNeighbors7, 3U);
    Check(state3Hop.activeRooms.count == 49, "center room (3,3) in 3-hop mode expands to all 49 rooms in 7x7 grid");
    Check(expandedRooms.size() == 26, "1 center + 25 (9+16) rooms expanded on initial 3-hop entry");
}

void TestMonsterAiPatches() {
    using namespace Ultrawide::MonsterAi;
    Check(ComputePlayerSlotCap(35) == 75, "player slot cap floor is 75");
    Check(ComputePlayerSlotCap(50) == 75, "player slot cap for 50 is 75");
    Check(ComputePlayerSlotCap(65) == 85, "player slot cap for 65 is 85");
    Check(ComputePlayerSlotCap(100) == 120, "player slot cap for 100 is 120");

    const auto mov50 = BuildMovR14dImm32(50);
    const std::array<unsigned char, 6> expected50{0x41, 0xbe, 0x32, 0x00, 0x00, 0x00};
    Check(mov50 == expected50, "mov r14d, 50 encoding");
}

} // namespace

int main() {
    TestAssemblyGates();
    TestMonsterPopInReconcile();
    TestMonsterAiPatches();
    std::puts("Ultrawide assembly gates, 2-hop room reconciliation, and AI patch helpers passed.");
}
