#include "../logic/map/FogOfWar.h"

#include <cstdint>
#include <cstdio>

namespace {

bool testInitialState() {
    tcp::logic::map::FogOfWar fog;
    fog.resize(10, 10);

    for (std::int32_t y = 0; y < 10; ++y) {
        for (std::int32_t x = 0; x < 10; ++x) {
            if (fog.at(x, y) != tcp::logic::map::Visibility::Unexplored) {
                return false;
            }
        }
    }
    return true;
}

bool testReveal() {
    tcp::logic::map::FogOfWar fog;
    fog.resize(10, 10);

    fog.reveal(5, 5, 2);

    if (fog.at(5, 5) != tcp::logic::map::Visibility::Visible) return false;
    if (fog.at(5, 3) != tcp::logic::map::Visibility::Visible) return false;
    if (fog.at(3, 5) != tcp::logic::map::Visibility::Visible) return false;
    if (fog.at(7, 5) != tcp::logic::map::Visibility::Visible) return false;

    // Far away should still be unexplored
    if (fog.at(0, 0) != tcp::logic::map::Visibility::Unexplored) return false;

    return true;
}

bool testAdvanceTick() {
    tcp::logic::map::FogOfWar fog;
    fog.resize(10, 10);

    fog.reveal(5, 5, 1);
    if (!fog.isVisible(5, 5)) return false;

    fog.advanceTick();
    if (fog.isVisible(5, 5)) return false;
    if (fog.at(5, 5) != tcp::logic::map::Visibility::Fogged) return false;

    // Unexplored should stay unexplored
    if (fog.at(0, 0) != tcp::logic::map::Visibility::Unexplored) return false;

    return true;
}

bool testRevealAfterAdvance() {
    tcp::logic::map::FogOfWar fog;
    fog.resize(10, 10);

    fog.reveal(3, 3, 0);
    if (!fog.isVisible(3, 3)) return false;

    fog.advanceTick();
    if (fog.isVisible(3, 3)) return false;

    // Re-reveal
    fog.reveal(3, 3, 0);
    if (!fog.isVisible(3, 3)) return false;

    return true;
}

bool testOutOfBounds() {
    tcp::logic::map::FogOfWar fog;
    fog.resize(5, 5);

    if (fog.isInBounds(-1, 0)) return false;
    if (fog.isInBounds(0, -1)) return false;
    if (fog.isInBounds(5, 0)) return false;
    if (fog.isInBounds(0, 5)) return false;

    if (fog.at(-1, 0) != tcp::logic::map::Visibility::Unexplored) return false;
    if (fog.isVisible(-1, 0)) return false;

    // reveal should not crash on out-of-bounds
    fog.reveal(-1, -1, 3);
    return true;
}

bool testMultiReveal() {
    tcp::logic::map::FogOfWar fog;
    fog.resize(20, 20);

    fog.reveal(10, 5, 3);
    fog.reveal(10, 15, 3);

    // Overlapping reveals should both be visible
    if (!fog.isVisible(10, 5)) return false;
    if (!fog.isVisible(10, 15)) return false;

    fog.advanceTick();

    if (fog.isVisible(10, 5)) return false;
    if (fog.isVisible(10, 15)) return false;

    return true;
}

}  // namespace

int main() {
    unsigned passed = 0;
    unsigned failed = 0;

    const auto run = [&](const char* name, bool (*fn)()) {
        if (fn()) {
            ++passed;
        } else {
            ++failed;
            std::printf("  FAILED: %s\n", name);
        }
    };

    run("InitialState", testInitialState);
    run("Reveal", testReveal);
    run("AdvanceTick", testAdvanceTick);
    run("RevealAfterAdvance", testRevealAfterAdvance);
    run("OutOfBounds", testOutOfBounds);
    run("MultiReveal", testMultiReveal);

    std::printf("FogOfWar: %u passed, %u failed\n", passed, failed);
    return (failed > 0) ? 1 : 0;
}
