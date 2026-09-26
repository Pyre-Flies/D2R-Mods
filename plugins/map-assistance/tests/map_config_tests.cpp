#include "map_config.h"

#include <array>
#include <fstream>
#include <iterator>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    const auto sample = MapAssistance::ParseConfiguration(R"toml(
[map-assistance]
enabled = false

[[zones]]
map_id = 29
name = "Jail Level 1"
layout = "Semi-static"
lines = [
  "From entrance: left finds the waypoint; straight finds level 2.",
  "From the waypoint, turn left for level 2."
]

[[zones]]
map_id = 250
name = "Future Zone"
layout = "Custom"
lines = ["First line", "Second line"]

[[zones]]
map_id = 29
name = "Duplicate"
layout = "Invalid"
lines = ["Must be rejected"]
)toml");
    if (sample.enabled || sample.zones.size() != 2 || sample.rejectedEntries != 1) return 2;
    const auto* jail = MapAssistance::FindZone(sample, 29);
    const auto* custom = MapAssistance::FindZone(sample, 250);
    if (jail == nullptr || jail->lines.size() != 2 || custom == nullptr ||
        MapAssistance::JoinLines(*custom) != "First line\nSecond line") return 3;

    std::ifstream stream(argv[1], std::ios::binary);
    if (!stream.is_open()) return 4;
    const std::string text{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    const auto defaults = MapAssistance::ParseConfiguration(text);
    if (!defaults.enabled || defaults.zones.size() != 127 || defaults.rejectedEntries != 0) return 4;
    constexpr std::array<std::int32_t, 5> towns{1, 40, 75, 103, 109};
    for (std::int32_t id = 1; id <= 132; ++id) {
        bool town = false;
        for (const auto townId : towns) town = town || id == townId;
        if ((MapAssistance::FindZone(defaults, id) == nullptr) != town) return 5;
    }
    return 0;
}
