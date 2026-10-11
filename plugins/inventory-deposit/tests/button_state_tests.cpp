#include "button_state_profile.h"
#include <Windows.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
namespace P = InventoryDepositButtonStateProfile;
using Buffer = std::array<unsigned char, 0xB88>;
Buffer* active{};
bool pressed{};
unsigned pressedCalls{}, adjustCalls{}, positionCalls{};
void Check(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::abort(); }
}
template<class T> void Put(Buffer& b, std::size_t at, T value) {
    std::memcpy(b.data() + at, &value, sizeof(value));
}
template<class T> T Get(const Buffer& b, std::size_t at) {
    T value{}; std::memcpy(&value, b.data() + at, sizeof(value)); return value;
}
bool __fastcall Pressed(void* button) noexcept {
    Check(active && button == active->data(), "pressed lookup receives the button in RCX");
    ++pressedCalls; return pressed;
}
void __fastcall Adjust(void* button) noexcept {
    Check(active && button == active->data(), "disable tail-call preserves the button in RCX");
    ++adjustCalls;
}
void __fastcall Position(void* text, const void* point) noexcept {
    Check(active && text == active->data() + 0x818 && point, "enable text-position ABI");
    std::memcpy(static_cast<unsigned char*>(text) + 0x70, point, 8);
    ++positionCalls;
}
void Thunk(unsigned char* page, std::size_t offset, std::uintptr_t function) {
    page[offset] = 0x48; page[offset + 1] = 0xB8;
    std::memcpy(page + offset + 2, &function, sizeof(function));
    page[offset + 10] = 0xFF; page[offset + 11] = 0xE0;
}
void Relative(unsigned char* page, std::size_t instruction, std::size_t target, unsigned char opcode) {
    Check(page[instruction] == opcode, "expected relative branch at the recorded instruction boundary");
    const auto displacement = static_cast<std::int32_t>(target) - static_cast<std::int32_t>(instruction + 5);
    std::memcpy(page + instruction + 1, &displacement, sizeof(displacement));
}
Buffer Widget() {
    Buffer b{}; b.fill(0xA5);
    Put<unsigned char>(b, 0x50, 0); Put<unsigned char>(b, 0x51, 1);
    Put<unsigned char>(b, 0x67A, 0); Put<unsigned char>(b, 0x679, 0);
    Put<int>(b, 0x7E8, 0); Put<int>(b, 0x7EC, 2);
    Put<int>(b, 0x7F0, 1); Put<int>(b, 0x7F4, 3);
    Put<int>(b, 0x710, 1);
    const float normal[] = {0.9f, 0.8f, 0.7f, 0.6f};
    const float disabled[] = {0.4f, 0.3f, 0.2f, 0.1f};
    std::memcpy(b.data() + 0x808, normal, sizeof(normal));
    std::memcpy(b.data() + 0x7F8, disabled, sizeof(disabled));
    Put<int>(b, 0xAC0, 3); Put<int>(b, 0xAC4, 4);
    Put<int>(b, 0xA48, 5); Put<int>(b, 0xA4C, 6);
    Put<int>(b, 0xA50, 7); Put<int>(b, 0xA54, 8);
    return b;
}
void Colors(Buffer& expected, std::size_t source) {
    std::memcpy(expected.data() + 0x71C, expected.data() + source, 16);
    Put<float>(expected, 0x714, Get<float>(expected, source + 12));
}
}

int main() {
    // Execute only the fully guarded, short state bodies in this process.
    // Every external call is redirected to a bounded test callback or the
    // exact seven-byte frame setter; no game process/module is loaded.
    auto* page = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    Check(page != nullptr, "allocate isolated code page");
    std::memcpy(page, P::Disable, sizeof(P::Disable));
    std::memcpy(page + 0x100, P::Enable, sizeof(P::Enable));
    std::memcpy(page + 0x200, P::SetFrame, sizeof(P::SetFrame));
    Thunk(page, 0x300, reinterpret_cast<std::uintptr_t>(&Pressed));
    Thunk(page, 0x320, reinterpret_cast<std::uintptr_t>(&Adjust));
    Thunk(page, 0x340, reinterpret_cast<std::uintptr_t>(&Position));
    Relative(page, 0x27, 0x200, 0xE8);
    Relative(page, 0x73, 0x320, 0xE9);
    Relative(page, 0x100 + 0x30, 0x300, 0xE8);
    Relative(page, 0x100 + 0x4E, 0x200, 0xE8);
    Relative(page, 0x100 + 0xCA, 0x340, 0xE8);
    DWORD previous{};
    Check(VirtualProtect(page, 4096, PAGE_EXECUTE_READ, &previous) != 0, "seal isolated code read/execute");
    Check(FlushInstructionCache(GetCurrentProcess(), page, 4096) != 0, "flush isolated code cache");
    const auto disable = reinterpret_cast<void(__fastcall*)(void*)>(page);
    const auto enable = reinterpret_cast<void(__fastcall*)(void*)>(page + 0x100);

    for (bool custom : {false, true}) {
        auto actual = Widget(); active = &actual;
        Put<int>(actual, 0x7F4, custom ? 3 : -1);
        auto expected = actual;
        Put<unsigned char>(expected, 0x67A, 1);
        Put<int>(expected, 0x710, custom ? 3 : 0);
        Colors(expected, custom ? 0x808 : 0x7F8);
        const auto before = adjustCalls;
        disable(actual.data());
        Check(adjustCalls == before + 1, "disable tail-calls text adjustment exactly once");
        Check(actual == expected, "disable state/frame/tint footprint and unrelated bytes preserved");
    }
    for (int mode = 0; mode != 4; ++mode) {
        auto actual = Widget(); active = &actual;
        Put<unsigned char>(actual, 0x50, 1);
        Put<unsigned char>(actual, 0x67A, 1);
        Put<unsigned char>(actual, 0x679, mode == 1 || mode == 2 ? 1 : 0);
        Put<int>(actual, 0x710, mode == 3 ? 1 : 3);
        pressed = mode == 2;
        auto expected = actual;
        Put<unsigned char>(expected, 0x67A, 0);
        if (mode != 3) {
            Put<int>(expected, 0x710, pressed ? 2 : 0);
            Put<int>(expected, 0x888, pressed ? 15 : 10);
            Put<int>(expected, 0x88C, pressed ? 18 : 12);
        }
        Colors(expected, 0x808);
        const auto positions = positionCalls, lookups = pressedCalls;
        enable(actual.data());
        Check(positionCalls == positions + (mode == 3 ? 0 : 1), "enable only adjusts text when replacing disabled frame");
        Check(pressedCalls == lookups + (mode == 1 || mode == 2 ? 1 : 0), "pressed lookup branch is bounded");
        Check(actual == expected, "enable normal/pressed/preserved-hover state and exact write footprint");
    }
    active = nullptr;
    Check(VirtualFree(page, 0, MEM_RELEASE) != 0, "free isolated code page");
    std::puts("PASS: isolated native ButtonWidget ABI, disabled frame/tint, normal/pressed/hover restoration, exact mutation footprint; no game process accessed.");
}
