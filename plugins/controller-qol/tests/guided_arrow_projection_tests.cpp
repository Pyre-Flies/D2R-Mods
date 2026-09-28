#include "guided_arrow_projection.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

static void Check(bool value, const char* message) {
    if (!value) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

int main() {
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    Check(GuidedArrowProjection::TryProject(1000, 1000, 998, 1001, 22, x, y),
        "near Guided Arrow controller point projects");
    const auto dx = static_cast<int>(x) - 1000;
    const auto dy = static_cast<int>(y) - 1000;
    const auto distance = std::sqrt(static_cast<double>(dx * dx + dy * dy));
    Check(distance >= 19.0 && distance <= 21.0,
        "projected point is approximately 20 tiles away");
    Check(dx < 0 && dy > 0, "projection preserves cast direction");

    Check(!GuidedArrowProjection::TryProject(1000, 1000, 998, 1001, 21, x, y),
        "other skills remain unchanged");
    Check(!GuidedArrowProjection::TryProject(1000, 1000, 1000, 1000, 22, x, y),
        "zero direction remains unchanged");
    Check(!GuidedArrowProjection::TryProject(1000, 1000, 1008, 1000, 22, x, y),
        "already-distant points remain unchanged");
    Check(!GuidedArrowProjection::TryProject(2, 2, 1, 1, 22, x, y),
        "out-of-range projected coordinates fail open");

    std::puts("Guided Arrow 20-tile projection policy passed.");
}
