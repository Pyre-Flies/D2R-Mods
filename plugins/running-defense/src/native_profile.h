#pragma once

#include <windows.h>
#include <bcrypt.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace RunningDefense::Native {

// Exact-build admission hashes for D2R 3.3.93787 + D2RCore.dll.
inline constexpr char ExeHash[] = "93021DAD48533BCFCA8A95C69A0CC00CDB9008A3CE6CEBA1D04C7ACA71985A10";
inline constexpr char CoreHash[] = "2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2";

// 1. SUnitDmg_ResolveHitFlags (0x44B720): melee auto-hit check on running player (PLRMODE_RUN == 3).
//   0044B7D7: 48 8B CF          mov rcx, rdi
//   0044B7DA: E8 81 F3 EF FF    call 0x34AB60 (UNITS_GetAnimMode)
//   0044B7DF: 83 F8 03          cmp eax, 3
//   0044B7E2: 75 03             jne 0x44B7E7
//   0044B7E4: 41 8B DD          mov ebx, r13d (sets auto-hit flag ebx = 1)
inline constexpr std::uintptr_t ResolveHitRunCheckRva = 0x44B7D7;
inline constexpr unsigned char ResolveHitRunCheckBytes[16] = {
    0x48, 0x8B, 0xCF, 0xE8, 0x81, 0xF3, 0xEF, 0xFF,
    0x83, 0xF8, 0x03, 0x75, 0x03, 0x41, 0x8B, 0xDD,
};
inline constexpr std::uintptr_t ResolveHitAutoHitAssignRva = 0x44B7E4;
inline constexpr unsigned char ResolveHitAutoHitAssignBytes[3] = {0x41, 0x8B, 0xDD};
inline constexpr unsigned char Nop3Bytes[3] = {0x90, 0x90, 0x90};

// 2. UNITS_GetDefense (0x347B10): computes total Defense Rating (Armor Class) for a unit.
// Called by SUnitDmg_CalculateToHit (0x44BA60), CharacterStatsPanel::UpdateDefenseStat (0x14E5EA0),
// and BuildDefenseTooltip (0x15149F0).
inline constexpr std::uintptr_t UnitsGetDefenseRva = 0x347B10;
inline constexpr unsigned char UnitsGetDefenseBytes[16] = {
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x50, 0x48, 0x89,
    0x74, 0x24, 0x70, 0x48, 0x89, 0x7C, 0x24, 0x48,
};

// 3. UNITS_GetAnimMode (0x34AB60): returns current animation mode for a unit (PLRMODE_RUN == 3, PLRMODE_WALK == 2).
inline constexpr std::uintptr_t UnitsGetAnimModeRva = 0x34AB60;
inline constexpr unsigned char UnitsGetAnimModeBytes[18] = {
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x85, 0xC9, 0x75,
    0x1A, 0x88, 0x4C, 0x24, 0x30, 0x48, 0x8D, 0x4C,
    0x24, 0x30,
};

// 4. UNITS_IsMoving (0x34C390): returns non-zero when a unit is moving.
inline constexpr std::uintptr_t UnitsIsMovingRva = 0x34C390;
inline constexpr unsigned char UnitsIsMovingBytes[13] = {
    0x40, 0x53, 0x48, 0x83, 0xEC, 0x20, 0x48, 0x85,
    0xC9, 0x74, 0x4D, 0x8B, 0x11,
};

// 5. UNITS_GetBlockRate (0x347DA0) & SUnitDmg_CheckBlock (0x44B930) running block / 3 site (0x44B9BB).
inline constexpr std::uintptr_t UnitsGetBlockRateRva = 0x347DA0;
inline constexpr unsigned char UnitsGetBlockRateBytes[17] = {
    0x40, 0x55, 0x57, 0x41, 0x57, 0x48, 0x83, 0xEC,
    0x30, 0x33, 0xED, 0x44, 0x8B, 0xFA, 0x48, 0x8B,
    0xF9,
};
inline constexpr std::uintptr_t CheckBlockRunDiv3Rva = 0x44B9BB;
inline constexpr unsigned char CheckBlockRunDiv3Bytes[14] = {
    0xB8, 0x56, 0x55, 0x55, 0x55, 0xF7, 0xEE, 0x8B,
    0xF2, 0xC1, 0xEE, 0x1F, 0x03, 0xF2,
};
inline constexpr unsigned char Nop14Bytes[14] = {
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
};

// 6. CharacterStatsPanel::UpdateDefenseStat (0x14E5EA0) & running "---" branch (0x14E5F34..0x14E5F40).
inline constexpr std::uintptr_t UpdateDefenseStatRva = 0x14E5EA0;
inline constexpr unsigned char UpdateDefenseStatBytes[15] = {
    0x40, 0x53, 0x56, 0x57, 0x48, 0x83, 0xEC, 0x70,
    0x48, 0x8B, 0x05, 0x19, 0x54, 0x4E, 0x01,
};
inline constexpr std::uintptr_t UpdateDefenseRunBranchRva = 0x14E5F34;
inline constexpr unsigned char UpdateDefenseRunBranchBytes[13] = {
    0x48, 0x8B, 0xCE, 0xE8, 0x24, 0x4C, 0xE6, 0xFE,
    0x83, 0xF8, 0x03, 0x75, 0x78,
};
inline constexpr std::uintptr_t UpdateDefenseRunJneOpcodeRva = 0x14E5F3F;
inline constexpr unsigned char JneOpcodeByte = 0x75;
inline constexpr unsigned char JmpOpcodeByte = 0xEB;

// 7. UI helpers: Widget::FindChild (0x856220), GetStatTextColor (0x908690), and overlay clip helpers.
inline constexpr std::uintptr_t WidgetFindChildRva = 0x856220;
inline constexpr unsigned char WidgetFindChildBytes[15] = {
    0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x74,
    0x24, 0x18, 0x57, 0x48, 0x83, 0xEC, 0x20,
};

inline constexpr std::uintptr_t GetStatTextColorRva = 0x908690;
inline constexpr unsigned char GetStatTextColorBytes[15] = {
    0x40, 0x53, 0x57, 0x48, 0x83, 0xEC, 0x38, 0xC6,
    0x44, 0x24, 0x58, 0x00, 0x41, 0x8B, 0xD8,
};

inline constexpr std::uintptr_t DrawListRva = 0xCEAED0;
inline constexpr unsigned char DrawListBytes[] = {
    0x48, 0x8B, 0x0D, 0x79, 0x5F, 0x80, 0x02, 0x48, 0x8B, 0x81, 0x80, 0x1A, 0x00, 0x00,
    0xC6, 0x80, 0x80, 0x00, 0x00, 0x00, 0x01, 0x48, 0x8B, 0x81, 0x80, 0x1A, 0x00, 0x00,
    0x48, 0x8B, 0x80, 0xB0, 0x02, 0x00, 0x00, 0xC3,
};

inline constexpr std::uintptr_t PushClipRva = 0xD03A30;
inline constexpr unsigned char PushClipBytes[] = {
    0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x20, 0x57, 0x48, 0x83, 0xEC, 0x40, 0x48, 0x89,
    0x54, 0x24, 0x50, 0x48, 0x8B, 0xF9, 0xF3, 0x0F, 0x10, 0x4C, 0x24, 0x50, 0xF3, 0x0F,
    0x10, 0x44, 0x24, 0x54, 0x4C, 0x89, 0x44, 0x24, 0x20, 0xF3, 0x0F, 0x10, 0x5C, 0x24,
    0x20, 0xF3, 0x0F, 0x10, 0x54, 0x24, 0x24, 0xF3, 0x0F, 0x11, 0x48, 0xE8, 0xF3, 0x0F,
    0x11, 0x40, 0xEC, 0xF3, 0x0F, 0x11, 0x58, 0xF0, 0xF3, 0x0F, 0x11, 0x50, 0xF4,
};

inline constexpr std::uintptr_t PopClipRva = 0xD03760;
inline constexpr unsigned char PopClipBytes[] = {
    0x8B, 0x41, 0x60, 0x83, 0xE8, 0x01, 0x89, 0x41, 0x60, 0x75, 0x17, 0x48, 0x8B, 0x41,
    0x38, 0x48, 0x83, 0xC0, 0x1C, 0x0F, 0x10, 0x00, 0x0F, 0x11, 0x81, 0x90, 0x00, 0x00,
    0x00, 0xE9, 0xDE, 0x3B,
};

inline bool Bytes(std::uintptr_t address, void* buffer, std::size_t size) noexcept {
    if (address < 65536 || size > 65536 || address > 0x7fffffffffffULL - size) return false;
    SIZE_T got{};
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), buffer, size, &got) && got == size;
}

template <class T>
inline bool Field(std::uintptr_t address, T& value) noexcept {
    return Bytes(address, &value, sizeof(value));
}

inline bool Hash(HMODULE module, const char* expected) noexcept {
    if (!module || !expected) return false;
    wchar_t path[32768]{};
    const auto length = GetModuleFileNameW(module, path, 32768);
    if (!length || length >= 32768) return false;
    HANDLE file = CreateFileW(
        path,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;

    BCRYPT_ALG_HANDLE alg{};
    BCRYPT_HASH_HANDLE hash{};
    bool match = false;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0) {
        DWORD objectSize{}, got{};
        if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &got, 0) >= 0) {
            try {
                std::vector<unsigned char> object(objectSize), block(65536);
                if (BCryptCreateHash(alg, &hash, object.data(), objectSize, nullptr, 0, 0) >= 0) {
                    DWORD count{};
                    bool valid = true;
                    for (;;) {
                        if (!ReadFile(file, block.data(), static_cast<DWORD>(block.size()), &count, nullptr)) {
                            valid = false;
                            break;
                        }
                        if (!count) break;
                        if (BCryptHashData(hash, block.data(), count, 0) < 0) {
                            valid = false;
                            break;
                        }
                    }
                    unsigned char digest[32]{};
                    if (valid && BCryptFinishHash(hash, digest, 32, 0) >= 0) {
                        char text[65]{};
                        for (unsigned i = 0; i < 32; ++i) {
                            std::snprintf(text + i * 2, 3, "%02X", digest[i]);
                        }
                        match = std::strcmp(text, expected) == 0;
                    }
                    BCryptDestroyHash(hash);
                    hash = nullptr;
                }
            } catch (...) {
                if (hash) BCryptDestroyHash(hash);
            }
        }
        BCryptCloseAlgorithmProvider(alg, 0);
    }
    CloseHandle(file);
    return match;
}

} // namespace RunningDefense::Native
