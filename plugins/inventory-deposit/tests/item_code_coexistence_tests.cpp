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
    bool expectedMatch{true};

    CheckContextFixture() {
        activeCheckFixture = this;
        api.apiSize = sizeof(api);
        api.checkExpectedBytes = [](const D2RL::PluginContext*, uintptr_t rva, const void*, uint32_t size) noexcept {
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
    Check(QolItemCode::Check(&f.context, 0x8b2d0, LocalContext, sizeof(LocalContext)) &&
        f.lastRva == 0x8b2d0 && f.lastSize == sizeof(LocalContext),
        "non-item-code helper RVA delegates directly to CheckExpectedBytes");
}
} // namespace

int main() {
    StructuralTests();
    CheckFallbackTests();
    std::puts("inventory-deposit item-code UnHoarder coexistence and fallback regressions passed");
    return 0;
}
