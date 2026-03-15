#include "../logic/core/SimulationConfig.h"

#include <iostream>

int main() {
    const tcp::logic::SimulationConfig config{};
    const auto tickMicros = tcp::logic::tickDurationMicros(config);

    if (config.ticksPerSecond != 30) {
        std::cerr << "Expected default ticksPerSecond to be 30" << '\n';
        return 1;
    }

    if (tickMicros != 33333) {
        std::cerr << "Expected 33333us tick duration for 30 TPS, got " << tickMicros << '\n';
        return 1;
    }

    std::cout << "Determinism smoke test passed" << '\n';
    return 0;
}
