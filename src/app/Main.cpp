#include "logic/core/SimulationConfig.h"

#include <iostream>

int main() {
    const tcp::logic::SimulationConfig config{};
    const auto tickMicros = tcp::logic::tickDurationMicros(config);

    std::cout << "The Chlorophyll Protocol scaffold" << '\n';
    std::cout << "Logic tick rate: " << config.ticksPerSecond << " TPS" << '\n';
    std::cout << "Tick duration: " << tickMicros << " us" << '\n';
    return 0;
}
