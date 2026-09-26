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
