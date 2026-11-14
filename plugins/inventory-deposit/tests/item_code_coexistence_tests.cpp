#include "item_code_compatibility.h"
#include "belt_signatures.h"
#include "native_identify_profile.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
using QolChatForwarding::Kind;

void Check(bool ok, const char* message) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

struct Block {
    uintptr_t address;
    Kind kind;
    std::vector<unsigned char> bytes;
};

struct StructuralFixture {
    uintptr_t game = 0x140000000;
    uintptr_t companion = 0x7ff98b8d0000;
    uintptr_t relay = 0x13fff0113;
    uintptr_t hook = 0x7ff98b8ec550;
    uintptr_t slot = 0x7ff98b980428;
    uintptr_t trampoline = 0x13fff0100;
    std::vector<Block> blocks;

    explicit StructuralFixture(bool indirectCallWrapper = false) {
        using namespace QolItemCode;
        std::vector<unsigned char> wrapper(32, 0x90);
        if (indirectCallWrapper) {
            // FF 15 disp32 (RIP-indirect call to original trampoline slot)
            wrapper[4] = 0xff;
            wrapper[5] = 0x15;
            const int32_t slotDisp = static_cast<int32_t>(static_cast<int64_t>(slot) - static_cast<int64_t>(hook + 10));
            std::memcpy(wrapper.data() + 6, &slotDisp, 4);
        } else {
            // 48 8B 05 disp32 (RIP-relative mov rax, [slot])
            wrapper[0] = 0x48;
            wrapper[1] = 0x8b;
            wrapper[2] = 0x05;
            const int32_t slotDisp = static_cast<int32_t>(static_cast<int64_t>(slot) - static_cast<int64_t>(hook + 7));
            std::memcpy(wrapper.data() + 3, &slotDisp, 4);
        }

        blocks = {
            {game + Rva, Kind::Trampoline, {Original, Original + sizeof(Original)}},
            {relay, Kind::Trampoline, std::vector<unsigned char>(14)},
            {hook, Kind::Code, std::move(wrapper)},
            {slot, Kind::Data, std::vector<unsigned char>(8)},
            {trampoline, Kind::Trampoline, std::vector<unsigned char>(19)},
        };
        blocks[0].bytes[0] = 0xe9;
        const int32_t rel = static_cast<int32_t>(static_cast<int64_t>(relay) - static_cast<int64_t>(game + Rva + 5));
        std::memcpy(blocks[0].bytes.data() + 1, &rel, 4);
        std::memcpy(blocks[1].bytes.data(), Jump, 6);
        std::memcpy(blocks[1].bytes.data() + 6, &hook, 8);
        std::memcpy(blocks[3].bytes.data(), &trampoline, 8);
        std::memcpy(blocks[4].bytes.data(), Original, 5);
        std::memcpy(blocks[4].bytes.data() + 5, Jump, 6);
        const auto resume = game + Rva + 5;
        std::memcpy(blocks[4].bytes.data() + 11, &resume, 8);
    }

    QolChatForwarding::Memory Reader() {
        return {this, [](void* user, uintptr_t address, void* out, size_t count, Kind kind) noexcept {
            for (const auto& b : static_cast<StructuralFixture*>(user)->blocks) {
                if (b.kind == kind && address == b.address && count == b.bytes.size()) {
                    std::memcpy(out, b.bytes.data(), count);
                    return true;
                }
            }
            return false;
        }};
    }

    bool Valid() {
        return QolItemCode::ValidateStructural(Reader(), game, companion);
    }
};

void StructuralTests() {
    for (const bool indirectCall : {false, true}) {
        StructuralFixture live(indirectCall);
        Check(live.Valid(), "structural UnHoarder detour graph accepted");

        // Corrupt entry tail bytes (+5..sizeof(Original)-1)
        for (size_t byte = 5; byte < live.blocks[0].bytes.size(); ++byte) {
            StructuralFixture changed(indirectCall);
            changed.blocks[0].bytes[byte] ^= 1;
            Check(!changed.Valid(), "corrupted post-detour entry bytes refused");
        }
        // Corrupt relay bytes
        for (size_t byte = 0; byte < live.blocks[1].bytes.size(); ++byte) {
            StructuralFixture changed(indirectCall);
            changed.blocks[1].bytes[byte] ^= 1;
            Check(!changed.Valid(), "corrupted relay bytes refused");
        }
        // Corrupt trampoline bytes
        for (size_t byte = 0; byte < live.blocks[4].bytes.size(); ++byte) {
            StructuralFixture changed(indirectCall);
            changed.blocks[4].bytes[byte] ^= 1;
            Check(!changed.Valid(), "corrupted original-prologue trampoline or resume target refused");
        }
        // Wrong memory kind / ownership
        for (size_t block = 0; block < live.blocks.size(); ++block) {
            StructuralFixture wrongKind(indirectCall);
            wrongKind.blocks[block].kind = static_cast<Kind>(99);
            Check(!wrongKind.Valid(), "wrong memory page ownership or protection refused");
        }
    }
}

struct CheckContextFixture;
CheckContextFixture* activeCheckFixture{};

struct CheckContextFixture {
    D2RL::PluginApi api{};
    D2RL::PluginContext context{};
    uintptr_t lastRva{};
    uint32_t lastSize{};
    unsigned callCount{};
    bool expectedMatch{true};

    CheckContextFixture() {
        activeCheckFixture = this;
        api.apiSize = sizeof(api);
        api.checkExpectedBytes = [](const D2RL::PluginContext*, uintptr_t rva, const void*, uint32_t size) noexcept {
            ++activeCheckFixture->callCount;
            activeCheckFixture->lastRva = rva;
            activeCheckFixture->lastSize = size;
            return activeCheckFixture->expectedMatch;
        };
        context.contextSize = sizeof(context);
        context.api = &api;
        context.exeBase = 0x140000000;
    }
    ~CheckContextFixture() { activeCheckFixture = nullptr; }
};

struct FinishDetourFixture {
    uintptr_t game = 0x140000000;
    uintptr_t relay1 = 0x13fff0200;
    uintptr_t relay2 = 0x13fff0220;
    uintptr_t relay3 = 0x13fff0240;
    uintptr_t relay4 = 0x13fff0260;
    uintptr_t pluginTarget = 0x7ff98b901000;
    bool pluginOwned = true;
    std::vector<Block> blocks;

    static std::vector<unsigned char> MakeAbsJump(uintptr_t destination) {
        std::vector<unsigned char> stub(14);
        std::memcpy(stub.data(), QolItemCode::Jump, 6);
        std::memcpy(stub.data() + 6, &destination, 8);
        return stub;
    }

    static std::vector<unsigned char> MakeRelJump(uintptr_t from, uintptr_t to) {
        std::vector<unsigned char> stub(5);
        stub[0] = 0xe9;
        const int32_t rel = static_cast<int32_t>(static_cast<int64_t>(to) - static_cast<int64_t>(from + 5));
        std::memcpy(stub.data() + 1, &rel, 4);
        return stub;
    }

    explicit FinishDetourFixture(unsigned relayHops = 1, bool directAbsEntry = false) {
        using namespace QolItemCode;
        if (relayHops == 0 && !directAbsEntry) {
            pluginTarget = 0x180010000;
        }
        std::vector<unsigned char> entry(FinishInteractionOriginal,
            FinishInteractionOriginal + sizeof(FinishInteractionOriginal));
        const uintptr_t entryAddr = game + FinishInteractionRva;

        const uintptr_t firstHop = relayHops == 0 ? pluginTarget : relay1;
        if (directAbsEntry) {
            const auto absJump = MakeAbsJump(firstHop);
            std::memcpy(entry.data(), absJump.data(), absJump.size());
        } else {
            const auto relJump = MakeRelJump(entryAddr, firstHop);
            std::memcpy(entry.data(), relJump.data(), relJump.size());
        }
        blocks.push_back({entryAddr, Kind::Trampoline, std::move(entry)});

        if (relayHops >= 1) {
            if (relayHops == 1) {
                blocks.push_back({relay1, Kind::Trampoline, MakeAbsJump(pluginTarget)});
            } else {
                blocks.push_back({relay1, Kind::Trampoline, MakeRelJump(relay1, relay2)});
            }
        }
        if (relayHops >= 2) {
            const uintptr_t next2 = relayHops == 2 ? pluginTarget : relay3;
            blocks.push_back({relay2, Kind::Trampoline, MakeAbsJump(next2)});
        }
        if (relayHops >= 3) {
            const uintptr_t next3 = relayHops == 3 ? pluginTarget : relay4;
            blocks.push_back({relay3, Kind::Trampoline, MakeAbsJump(next3)});
        }
        if (relayHops >= 4) {
            blocks.push_back({relay4, Kind::Trampoline, MakeAbsJump(pluginTarget)});
        }
    }

    QolChatForwarding::Memory Reader() {
        return {this, [](void* user, uintptr_t address, void* out, size_t count, Kind kind) noexcept {
            for (const auto& b : static_cast<FinishDetourFixture*>(user)->blocks) {
                if (b.kind == kind && address == b.address && count <= b.bytes.size()) {
                    std::memcpy(out, b.bytes.data(), count);
                    return true;
                }
            }
            return false;
        }};
    }

    bool Valid() {
        using namespace QolBelt::Signatures;
        return QolItemCode::ValidateFinishInteractionDetour(
            Reader(), game, FinishInteraction, sizeof(FinishInteraction),
            [this](uintptr_t candidate) noexcept { return pluginOwned && candidate == pluginTarget; });
    }
};

void FinishInteractionDetourTests() {
    using namespace QolBelt::Signatures;
    // 0..3 relay hops (direct E9, MinHook E9->FF25, ChainHook E9->FF25->FF25, 4-hop chain)
    for (unsigned hops = 0; hops <= 3; ++hops) {
        FinishDetourFixture f(hops, false);
        Check(f.Valid(), "admitted PlaySoundById E9 detour chain accepted");
    }
    {
        FinishDetourFixture directAbs(0, true);
        Check(directAbs.Valid(), "admitted PlaySoundById 14-byte FF 25 entry detour accepted");
        for (size_t byte = 14; byte < directAbs.blocks[0].bytes.size(); ++byte) {
            FinishDetourFixture mutated(0, true);
            mutated.blocks[0].bytes[byte] ^= 1;
            Check(!mutated.Valid(), "mutated post-FF25 tail byte at 0x1A0780 refused");
        }
    }
    {
        FinishDetourFixture tooDeep(4, false);
        Check(!tooDeep.Valid(), "over-4-hop detour chain at 0x1A0780 refused");
    }
    for (size_t byte = 5; byte < sizeof(FinishInteraction); ++byte) {
        FinishDetourFixture mutated(1, false);
        mutated.blocks[0].bytes[byte] ^= 1;
        Check(!mutated.Valid(), "mutated post-E9 tail byte at 0x1A0780 refused");
    }
    {
        FinishDetourFixture nonPlugin(1, false);
        nonPlugin.pluginOwned = false;
        Check(!nonPlugin.Valid(), "non-plugin detour target at 0x1A0780 refused");
    }
    for (size_t i = 0; i < 2; ++i) {
        FinishDetourFixture wrongKind(1, false);
        wrongKind.blocks[i].kind = Kind::Data;
        Check(!wrongKind.Valid(), "non-executable page at 0x1A0780 or relay refused");
    }
    {
        FinishDetourFixture pristine(0, false);
        std::memcpy(pristine.blocks[0].bytes.data(),
            QolItemCode::FinishInteractionOriginal, sizeof(QolItemCode::FinishInteractionOriginal));
        Check(!pristine.Valid(), "pristine 0x1A0780 handled by CheckExpectedBytes rather than detour path");
    }
    {
        FinishDetourFixture f(1, false);
        Check(!QolItemCode::ValidateFinishInteractionDetour(
            f.Reader(), f.game, FinishInteraction, 16,
            [](uintptr_t) noexcept { return true; }),
            "short expected window for 0x1A0780 refused");
    }
}

void CheckFallbackTests() {
    using namespace QolBelt::Signatures;
    using namespace QolNativeIdentifyProfile;
    Check(!QolItemCode::Check(nullptr, 0x36ef50, Code, sizeof(Code)), "null context refused");

    CheckContextFixture f;
    Check(QolItemCode::Check(&f.context, 0x36ef50, Code, sizeof(Code)) &&
        f.lastRva == 0x36ef50 && f.lastSize == sizeof(Code),
        "pristine 0x36ef50 falls back to CheckExpectedBytes when companion is absent");
    f.expectedMatch = false;
    Check(!QolItemCode::Check(&f.context, 0x36ef50, Code, sizeof(Code)),
        "mismatched 0x36ef50 without admitted companion is refused");

    f.expectedMatch = true;
    Check(QolItemCode::Check(&f.context, 0x1a0780, FinishInteraction, sizeof(FinishInteraction)) &&
        f.lastRva == 0x1a0780 && f.lastSize == sizeof(FinishInteraction),
        "unhooked 0x1a0780 delegates to CheckExpectedBytes");
    f.expectedMatch = false;
    Check(!QolItemCode::Check(&f.context, 0x1a0780, FinishInteraction, sizeof(FinishInteraction)),
        "mismatched 0x1a0780 without admitted plugin detour is refused");

    f.expectedMatch = true;
    Check(QolItemCode::Check(&f.context, 0x8b2d0, LocalContext, sizeof(LocalContext)) &&
        f.lastRva == 0x8b2d0 && f.lastSize == sizeof(LocalContext),
        "non-item-code helper RVA delegates directly to CheckExpectedBytes");
}

void LiveFinishInteractionPluginHookTests(const char* pluginDllPath) {
    using namespace QolBelt::Signatures;
    HMODULE plugin = LoadLibraryA(pluginDllPath);
    Check(plugin != nullptr, "load real D2RLoader plugin DLL for live 0x1A0780 coexistence check");
    const auto pluginExport = reinterpret_cast<uintptr_t>(GetProcAddress(plugin, "D2RLoaderGetPluginInfo"));
    Check(pluginExport != 0, "resolve D2RLoaderGetPluginInfo code address");

    const size_t pageMask = 4095;
    const size_t pageOffset = QolItemCode::FinishInteractionRva & ~pageMask;
    const size_t reserveSize = pageOffset + 8192;
    auto* region = static_cast<unsigned char*>(VirtualAlloc(nullptr, reserveSize, MEM_RESERVE, PAGE_NOACCESS));
    Check(region != nullptr, "reserve synthetic game region");
    Check(VirtualAlloc(region + pageOffset, 8192, MEM_COMMIT, PAGE_READWRITE) != nullptr,
        "commit 0x1A0780 and relay pages");

    auto* entry = region + QolItemCode::FinishInteractionRva;
    auto* relay = entry + 128;
    std::memcpy(entry, QolItemCode::FinishInteractionOriginal, sizeof(QolItemCode::FinishInteractionOriginal));
    entry[0] = 0xe9;
    const int32_t rel = 128 - 5;
    std::memcpy(entry + 1, &rel, 4);
    std::memcpy(relay, QolItemCode::Jump, 6);
    std::memcpy(relay + 6, &pluginExport, 8);

    DWORD oldProtect{};
    Check(VirtualProtect(region + pageOffset, 8192, PAGE_EXECUTE_READ, &oldProtect) != 0,
        "mark synthetic 0x1A0780 and relay pages executable");

    CheckContextFixture f;
    f.context.exeBase = reinterpret_cast<uintptr_t>(region);
    f.expectedMatch = false;
    f.callCount = 0;

    Check(QolItemCode::Check(&f.context, 0x1a0780, FinishInteraction, sizeof(FinishInteraction)),
        "QolItemCode::Check admits live D2RLoader plugin detour at 0x1A0780");
    Check(f.callCount == 0,
        "admitted 0x1A0780 plugin detour short-circuits before CheckExpectedBytes so D2RCore logs no error");
    Check(QolItemCode::Match(f.context.exeBase, 0x1a0780, FinishInteraction, sizeof(FinishInteraction)),
        "QolItemCode::Match admits live D2RLoader plugin detour at 0x1A0780");

    // Corrupt a post-detour prologue byte: must fall through and fail closed.
    Check(VirtualProtect(region + pageOffset, 8192, PAGE_READWRITE, &oldProtect) != 0, "unprotect for mutation");
    entry[20] ^= 1;
    Check(VirtualProtect(region + pageOffset, 8192, PAGE_EXECUTE_READ, &oldProtect) != 0, "reprotect after mutation");
    f.callCount = 0;
    Check(!QolItemCode::Check(&f.context, 0x1a0780, FinishInteraction, sizeof(FinishInteraction)) && f.callCount == 1,
        "corrupted 0x1A0780 prologue tail rejected and falls through to CheckExpectedBytes");
    Check(!QolItemCode::Match(f.context.exeBase, 0x1a0780, FinishInteraction, sizeof(FinishInteraction)),
        "QolItemCode::Match rejects corrupted 0x1A0780 prologue tail");

    // Restore prologue tail, point relay at non-plugin code in this test executable: must fail closed.
    Check(VirtualProtect(region + pageOffset, 8192, PAGE_READWRITE, &oldProtect) != 0, "unprotect for non-plugin target");
    entry[20] ^= 1;
    const auto nonPluginCode = reinterpret_cast<uintptr_t>(&StructuralTests);
    std::memcpy(relay + 6, &nonPluginCode, 8);
    Check(VirtualProtect(region + pageOffset, 8192, PAGE_EXECUTE_READ, &oldProtect) != 0, "reprotect for non-plugin target");
    f.callCount = 0;
    Check(!QolItemCode::Check(&f.context, 0x1a0780, FinishInteraction, sizeof(FinishInteraction)) && f.callCount == 1,
        "detour targeting non-D2RLoader module rejected");

    VirtualFree(region, 0, MEM_RELEASE);
    FreeLibrary(plugin);
}
} // namespace

int main(int argc, char** argv) {
    StructuralTests();
    FinishInteractionDetourTests();
    CheckFallbackTests();
    if (argc >= 2 && argv[1] && *argv[1]) {
        LiveFinishInteractionPluginHookTests(argv[1]);
    }
    std::puts("item-code UnHoarder, PlaySoundById (0x1A0780) plugin hook coexistence, and fallback regressions passed");
    return 0;
}
