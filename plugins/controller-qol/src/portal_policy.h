#pragma once
#include <cmath>
#include <charconv>
#include <string_view>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <cstring>

namespace QolPortal {
inline constexpr int InteractSkill = 357;
inline bool AdmitSharedEntryPrefix(const unsigned char* actual,
    const unsigned char* original, bool detourTargetExecutable) noexcept {
    if (!actual || !original) return false;
    if (std::memcmp(actual, original, 5) == 0) return true;
    return actual[0] == 0xe9 && detourTargetExecutable;
}
enum class PriorityKind : uint8_t { None, Portal, Stash, Waypoint, Shrine, Well, Chest };
inline constexpr bool IsWellClass(uint32_t classId) noexcept {
    switch (classId) {
    case 111: case 113: case 115: case 118: case 130: case 132: case 137:
    case 138: case 322: case 426: case 493: case 498: case 513: case 519:
        return true;
    default:
        return false;
    }
}
inline constexpr bool IsNormalChestClass(uint32_t classId) noexcept {
    switch (classId) {
    case 5: case 6: case 87: case 88: case 139: case 140: case 141: case 144:
    case 146: case 147: case 148: case 176: case 177: case 181: case 183: case 198:
    case 240: case 241: case 242: case 243: case 246: case 329: case 330: case 331:
    case 332: case 333: case 334: case 335: case 336: case 387: case 389: case 390:
    case 391: case 397: case 413: case 420: case 424: case 425: case 430: case 431:
    case 432: case 433: case 455: case 501: case 502: case 504: case 505:
        return true;
    default:
        return false;
    }
}
inline constexpr PriorityKind ClassifyPriorityObject(uint32_t classId, uint8_t subclass) noexcept {
    if ((subclass & 0x04) != 0) return PriorityKind::Portal;
    if ((subclass & 0x40) != 0) return PriorityKind::Waypoint;
    if (classId == 267) return PriorityKind::Stash; // ObjectsTxt Bank
    if ((subclass & 0x01) != 0) return PriorityKind::Shrine;
    if (IsWellClass(classId)) return PriorityKind::Well;
    if (IsNormalChestClass(classId)) return PriorityKind::Chest;
    return PriorityKind::None;
}
inline constexpr bool KindEnabled(PriorityKind kind, bool portals, bool stash, bool waypoints,
                                  bool shrines = false, bool chests = false) noexcept {
    return (kind == PriorityKind::Portal && portals) ||
        (kind == PriorityKind::Stash && stash) ||
        (kind == PriorityKind::Waypoint && waypoints) ||
        (kind == PriorityKind::Shrine && shrines) ||
        (kind == PriorityKind::Well && shrines) ||
        (kind == PriorityKind::Chest && chests);
}
inline constexpr bool LootOwnsPortalSelection(bool enabled,bool modifierHeld,int skill,bool portal) noexcept {
    return enabled && modifierHeld && skill==InteractSkill && portal;
}
inline constexpr size_t ScoringControllerBytes = 0x1760;
inline constexpr size_t ScoringProfileBytes = 0x120;
inline size_t ScoringProfileOffset(int profile) noexcept {
    return profile == 8 ? 0xD40 : (profile >= 0 && profile < 8 ? 0xE60 + profile * ScoringProfileBytes : 0);
}
// This is a scratch scoring view, never an owned/copied native controller.
// The reviewed point scorer reads only this profile through its first argument.
inline bool PrepareScoringView(unsigned char* view, const unsigned char* native,
    int profile, float centerDistance, float& oldLimit, float& newLimit) noexcept {
    const size_t offset = ScoringProfileOffset(profile);
    if (!offset || !std::isfinite(centerDistance) || centerDistance < 0) return false;
    std::memcpy(&oldLimit, native + offset + 0x68, sizeof(float)); // category 2
    if (!std::isfinite(oldLimit) || oldLimit <= 0 || centerDistance <= oldLimit) return false;
    newLimit = centerDistance + 1.0f;
    if (!std::isfinite(newLimit) || newLimit <= centerDistance) return false;
    std::memcpy(view + offset, native + offset, ScoringProfileBytes);
    std::memcpy(view + offset + 0x68, &newLimit, sizeof(float));
    return true;
}
inline bool WithinPriorityDistance(int distance, uint32_t radius) noexcept {
    return distance >= 0 && static_cast<uint32_t>(distance) <= radius;
}
// GetInteractionTarget can discard its accepted Interact object in favor of a
// bound combat skill. Re-query through the current native Selected entry only
// after that null result. Existing results, modified input and mouse UI pass on.
template<class Query, class Eligible>
void* RecoverInteraction(bool enabled, bool controllerUi, bool modified,
    void* nativeResult, Query query, Eligible eligible) {
    if (nativeResult || !enabled || !controllerUi || modified) return nativeResult;
    void* candidate = query();
    return candidate && eligible(candidate) ? candidate : nativeResult;
}
inline bool IsCandidateContactCaller(uintptr_t rva) noexcept {
    return rva == 0x19158E || rva == 0x1922D2 || rva == 0x19237D;
}
inline bool ExtendCandidateContact(bool enabled, bool modified, uintptr_t caller,
    bool portal, int distance, uint32_t radius) noexcept {
    return enabled && !modified && IsCandidateContactCaller(caller) && portal &&
        WithinPriorityDistance(distance, radius);
}

// Populated only while evaluating one qualified native Interact candidate.
// Identity matching prevents unrelated/nested native range calls inheriting it.
struct RangeOverride {
    void* player = nullptr;
    void* candidate = nullptr;
    int distance = -1;
    uint32_t radius = 0;
    bool Allows(bool enabled, void* actualPlayer, void* actualCandidate) const noexcept {
        return enabled && player && candidate && player == actualPlayer &&
            candidate == actualCandidate && WithinPriorityDistance(distance, radius);
    }
};
class RangeScope {
    RangeOverride& slot;
    RangeOverride previous;
public:
    RangeScope(RangeOverride& storage, RangeOverride value) noexcept
        : slot(storage), previous(storage) { slot = value; }
    ~RangeScope() { slot = previous; }
    RangeScope(const RangeScope&) = delete;
    RangeScope& operator=(const RangeScope&) = delete;
};
// Read only the named setting's line; missing/malformed values retain 10.
inline uint32_t ReadPriorityDistance(std::string_view text) noexcept {
    constexpr std::string_view key = "portal_priority_distance";
    while (!text.empty()) {
        const auto end = text.find('\n');
        auto line = text.substr(0, end);
        text = end == text.npos ? std::string_view{} : text.substr(end + 1);
        const auto comment = line.find('#');
        line = line.substr(0, comment);
        const auto first = line.find_first_not_of(" \t\r");
        if (first == line.npos) continue;
        line.remove_prefix(first);
        if (!line.starts_with(key)) continue;
        line.remove_prefix(key.size());
        const auto eq = line.find_first_not_of(" \t");
        if (eq == line.npos || line[eq] != '=') continue;
        line.remove_prefix(eq + 1);
        const auto valueStart = line.find_first_not_of(" \t");
        if (valueStart == line.npos) return 10;
        line.remove_prefix(valueStart);
        int value = 0;
        auto parsed = std::from_chars(line.data(), line.data() + line.size(), value);
        if (parsed.ec != std::errc{}) return 10;
        if (std::string_view(parsed.ptr, line.data() + line.size() - parsed.ptr).find_first_not_of(" \t\r") != line.npos) return 10;
        return static_cast<uint32_t>(std::clamp(value, 1, 20));
    }
    return 10;
}

inline bool PreferPortal(bool enabled, bool modified, int skill,
    uint32_t currentType, uint32_t candidateType, uint8_t objectSubclass,
    int distance, uint32_t radius) noexcept {
    return enabled && !modified && skill == InteractSkill && currentType == 4 &&
        candidateType == 2 && (objectSubclass & 4) != 0 &&
        WithinPriorityDistance(distance, radius);
}

inline bool PreferObject(bool enabled, bool modified, int skill,
    uint32_t currentType, PriorityKind candidateKind, int distance, uint32_t radius) noexcept {
    return enabled && !modified && skill == InteractSkill && currentType == 4 &&
        candidateKind != PriorityKind::None && WithinPriorityDistance(distance, radius);
}

// Change only this comparison. The native callback still decides eligibility.
// Restore the candidate's real score after acceptance so NPC/combat/object
// comparisons retain their ordinary scores. Native Interact already prevents
// a later item from displacing a selected portal (18A713..18A75E).
template<class Evaluate>
bool EvaluatePortal(bool prefer, void* candidate, float score, float& bestScore,
    void*& selected, Evaluate evaluate) {
    float comparison = score;
    if (prefer && std::isfinite(score) && std::isfinite(bestScore) && score <= bestScore) {
        const float higher = std::nextafter(bestScore, std::numeric_limits<float>::infinity());
        if (std::isfinite(higher)) comparison = higher;
    }
    evaluate(comparison);
    if (comparison != score && selected == candidate) {
        bestScore = score;
        return true;
    }
    return false;
}
}
