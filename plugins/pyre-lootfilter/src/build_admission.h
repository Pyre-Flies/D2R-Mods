#pragma once
// Fast in-memory build admission and guarded Steam D2RCore.dll gameBuild hotfix.
// Replaces synchronous startup disk SHA-256 hashing with O(1) build-family prefix checks,
// exact PE header + code byte witnesses for D2RCore.dll 1.3.1-beta, and per-site
// CheckExpectedBytes witnesses.

#include "unit_reader.h"
#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace PyreLootFilter {

// Admits D2R 3.3 and 3.4 build families with strict '.' or '\0' boundary checks
// ("3.3", "3.3.94995", "3.4", "3.4.95032" pass; "3.30", "3.40", "3.3-beta", nullptr fail).
[[nodiscard]] inline bool IsSupportedBuildVersion(const char* v) noexcept {
    if (!v) return false;
    return (std::strncmp(v, "3.3", 3) == 0 || std::strncmp(v, "3.4", 3) == 0) &&
           (v[3] == '\0' || v[3] == '.');
}

namespace D2RCore131Beta {
// Verified against D2RCore.dll
// (1.3.1-beta, SHA-256 2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2).
inline constexpr std::uint32_t TimeDateStamp = 0x6AB37824U;
inline constexpr std::uint32_t SizeOfImage = 0x12E0000U;
inline constexpr std::uintptr_t CompilerManifestCheckRva = 0x75270U;
inline constexpr std::uintptr_t VersionBlockRva = 0x706110U;
inline constexpr std::size_t VersionBlockBytes = 0x60U;

inline constexpr std::size_t NarrowVerBufOffset = 0x00U;
inline constexpr std::size_t NarrowVerLenOffset = 0x10U;
inline constexpr std::size_t NarrowVerCapOffset = 0x18U;
inline constexpr std::size_t WideBuildBufOffset = 0x20U;
inline constexpr std::size_t WideBuildLenOffset = 0x30U;
inline constexpr std::size_t WideBuildCapOffset = 0x38U;
inline constexpr std::size_t NarrowBuildBufOffset = 0x40U;
inline constexpr std::size_t NarrowBuildLenOffset = 0x50U;
inline constexpr std::size_t NarrowBuildCapOffset = 0x58U;

inline constexpr std::uint64_t NarrowSsoCapacity = 15ULL;
inline constexpr std::uint64_t WideSsoCapacity = 7ULL;
inline constexpr std::size_t MaxExtractedBuildDigits = 7U;
inline constexpr std::size_t MaxVersionLength = 63U;

inline constexpr std::array<std::uint8_t, 32> CompilerManifestCheckBytes{{
    0x55, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55, 0x41,
    0x54, 0x56, 0x57, 0x53, 0x48, 0x81, 0xEC, 0x18,
    0x02, 0x00, 0x00, 0x48, 0x8D, 0xAC, 0x24, 0x80,
    0x00, 0x00, 0x00, 0x48, 0xC7, 0x85, 0x90, 0x01,
}};
} // namespace D2RCore131Beta

struct SteamHotfixResult {
    std::array<char, D2RCore131Beta::MaxVersionLength + 1U> gameVersion{};
    std::array<char, D2RCore131Beta::MaxExtractedBuildDigits + 1U> gameBuild{};
};

[[nodiscard]] constexpr std::string_view ExtractSteamGameBuildSuffix(
    std::string_view version) noexcept {
    if (version.size() < 5U) return {};
    const bool match33 = (version.substr(0U, 4U) == "3.3.");
    const bool match34 = (version.substr(0U, 4U) == "3.4.");
    if (!match33 && !match34) return {};

    const std::string_view rest = version.substr(4U);
    std::size_t segStart = 0U;
    for (std::size_t i = 0U; i < rest.size(); ++i) {
        const char ch = rest[i];
        if (ch == '.') {
            if (i == segStart || i + 1U == rest.size()) return {};
            segStart = i + 1U;
            continue;
        }
        if (ch < '0' || ch > '9') return {};
    }

    const std::string_view suffix = rest.substr(segStart);
    if (suffix.empty() || suffix.size() > D2RCore131Beta::MaxExtractedBuildDigits) {
        return {};
    }
    return suffix;
}

[[nodiscard]] inline bool VerifyD2RCore131BetaWitness(std::uintptr_t coreBase) noexcept {
    if (coreBase < 0x10000U ||
        coreBase > 0x7FFFFFFFFFFFULL - D2RCore131Beta::SizeOfImage) {
        return false;
    }

    IMAGE_DOS_HEADER dos{};
    if (!Reader::CopyReadable(reinterpret_cast<const void*>(coreBase), &dos, sizeof(dos)) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew < static_cast<LONG>(sizeof(IMAGE_DOS_HEADER)) ||
        dos.e_lfanew > 0x1000L) {
        return false;
    }

    IMAGE_NT_HEADERS64 nt{};
    if (!Reader::CopyReadable(
            reinterpret_cast<const void*>(coreBase + static_cast<std::uintptr_t>(dos.e_lfanew)),
            &nt, sizeof(nt)) ||
        nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt.FileHeader.TimeDateStamp != D2RCore131Beta::TimeDateStamp ||
        nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt.OptionalHeader.SizeOfImage != D2RCore131Beta::SizeOfImage) {
        return false;
    }

    if (D2RCore131Beta::CompilerManifestCheckRva + D2RCore131Beta::CompilerManifestCheckBytes.size() >
            nt.OptionalHeader.SizeOfImage ||
        D2RCore131Beta::VersionBlockRva + D2RCore131Beta::VersionBlockBytes >
            nt.OptionalHeader.SizeOfImage) {
        return false;
    }

    std::array<std::uint8_t, D2RCore131Beta::CompilerManifestCheckBytes.size()> codeBytes{};
    if (!Reader::CopyReadable(
            reinterpret_cast<const void*>(coreBase + D2RCore131Beta::CompilerManifestCheckRva),
            codeBytes.data(), codeBytes.size()) ||
        codeBytes != D2RCore131Beta::CompilerManifestCheckBytes) {
        return false;
    }

    return true;
}

// Steam installs lack Battle.net's `.build.info`, leaving D2RCore's `gameBuild` SSO strings empty
// so `D2RCore+0x75270` rejects `compiler/<ver>/manifest.json` and `D2RCore+0x1CE540` drops custom
// mod rows from `sounds.txt` on subsequent launches. Guarded by exact in-memory D2RCore 1.3.1-beta
// PE header + `D2RCore+0x75270` code byte witnesses and SEH-safe memory helpers.
[[nodiscard]] inline bool TryApplySteamGameBuildHotfix(
    std::uintptr_t coreBase, SteamHotfixResult* reportOut = nullptr) noexcept {
    if (!VerifyD2RCore131BetaWitness(coreBase)) {
        return false;
    }

    const std::uintptr_t blockAddr = coreBase + D2RCore131Beta::VersionBlockRva;
    alignas(8) std::array<std::uint8_t, D2RCore131Beta::VersionBlockBytes> snapshot{};
    if (!Reader::CopyReadable(reinterpret_cast<const void*>(blockAddr), snapshot.data(),
                              snapshot.size())) {
        return false;
    }

    std::uint64_t verLen = 0;
    std::uint64_t verCap = 0;
    std::uint64_t wbuildLen = 0;
    std::uint64_t wbuildCap = 0;
    std::uint64_t buildLen = 0;
    std::uint64_t buildCap = 0;
    std::memcpy(&verLen, snapshot.data() + D2RCore131Beta::NarrowVerLenOffset, sizeof(verLen));
    std::memcpy(&verCap, snapshot.data() + D2RCore131Beta::NarrowVerCapOffset, sizeof(verCap));
    std::memcpy(&wbuildLen, snapshot.data() + D2RCore131Beta::WideBuildLenOffset, sizeof(wbuildLen));
    std::memcpy(&wbuildCap, snapshot.data() + D2RCore131Beta::WideBuildCapOffset, sizeof(wbuildCap));
    std::memcpy(&buildLen, snapshot.data() + D2RCore131Beta::NarrowBuildLenOffset, sizeof(buildLen));
    std::memcpy(&buildCap, snapshot.data() + D2RCore131Beta::NarrowBuildCapOffset, sizeof(buildCap));

    if (buildLen != 0ULL || buildCap != D2RCore131Beta::NarrowSsoCapacity ||
        wbuildLen != 0ULL || wbuildCap != D2RCore131Beta::WideSsoCapacity) {
        return false;
    }
    if (verLen == 0ULL || verLen > verCap || verLen > D2RCore131Beta::MaxVersionLength) {
        return false;
    }

    std::array<char, D2RCore131Beta::MaxVersionLength + 1U> verBuf{};
    const auto verByteLen = static_cast<std::size_t>(verLen);
    if (verCap == D2RCore131Beta::NarrowSsoCapacity) {
        std::memcpy(verBuf.data(), snapshot.data() + D2RCore131Beta::NarrowVerBufOffset,
                    verByteLen + 1U);
    } else if (verCap > D2RCore131Beta::NarrowSsoCapacity && verCap <= 256ULL) {
        std::uintptr_t heapPtr = 0;
        std::memcpy(&heapPtr, snapshot.data() + D2RCore131Beta::NarrowVerBufOffset,
                    sizeof(heapPtr));
        if (!Reader::CopyReadable(reinterpret_cast<const void*>(heapPtr), verBuf.data(),
                                  verByteLen + 1U)) {
            return false;
        }
    } else {
        return false;
    }

    if (verBuf[verByteLen] != '\0') return false;
    for (std::size_t i = 0U; i < verByteLen; ++i) {
        if (verBuf[i] == '\0') return false;
    }

    const std::string_view suffix =
        ExtractSteamGameBuildSuffix(std::string_view{verBuf.data(), verByteLen});
    if (suffix.empty()) {
        return false;
    }

    std::array<char, D2RCore131Beta::MaxExtractedBuildDigits + 1U> narrowBuf{};
    std::array<wchar_t, D2RCore131Beta::MaxExtractedBuildDigits + 1U> wideBuf{};
    for (std::size_t i = 0U; i < suffix.size(); ++i) {
        narrowBuf[i] = suffix[i];
        wideBuf[i] = static_cast<wchar_t>(static_cast<unsigned char>(suffix[i]));
    }
    narrowBuf[suffix.size()] = '\0';
    wideBuf[suffix.size()] = L'\0';

    const std::uint64_t newBuildLen = suffix.size();
    if (!Reader::WriteWritable(
            reinterpret_cast<void*>(blockAddr + D2RCore131Beta::NarrowBuildBufOffset),
            narrowBuf.data(), suffix.size() + 1U) ||
        !Reader::WriteWritable(
            reinterpret_cast<void*>(blockAddr + D2RCore131Beta::WideBuildBufOffset),
            wideBuf.data(), (suffix.size() + 1U) * sizeof(wchar_t)) ||
        !Reader::WriteWritable(
            reinterpret_cast<void*>(blockAddr + D2RCore131Beta::NarrowBuildLenOffset),
            &newBuildLen, sizeof(newBuildLen)) ||
        !Reader::WriteWritable(
            reinterpret_cast<void*>(blockAddr + D2RCore131Beta::WideBuildLenOffset),
            &newBuildLen, sizeof(newBuildLen))) {
        return false;
    }

    if (reportOut) {
        reportOut->gameVersion = verBuf;
        reportOut->gameBuild = narrowBuf;
    }
    return true;
}

} // namespace PyreLootFilter
