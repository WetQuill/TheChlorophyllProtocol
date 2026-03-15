#include "GameLoop.h"

#include <iostream>
#include <vector>

int main() {
    const tcp::logic::SimulationConfig config{};
    tcp::app::GameLoop loop(config);

    std::int64_t logicCounter = 0;
    const std::vector<std::int64_t> frameTimesMicros = {
        16666, 16667, 33333, 10000, 40000, 16666, 16667,
    };

    for (const auto frameElapsed : frameTimesMicros) {
        const auto report = loop.frame(frameElapsed, [&](std::int64_t tick) {
            logicCounter += tick + 1;
        });
        std::cout << "frame=" << frameElapsed
                  << " ticks=" << report.ticksExecuted
                  << " alpha_permille=" << report.interpolationPermille
                  << " catchup=" << (report.catchUpLimited ? "yes" : "no")
                  << '\n';
    }

    std::cout << "The Chlorophyll Protocol scaffold" << '\n';
    std::cout << "Logic tick rate: " << config.ticksPerSecond << " TPS" << '\n';
    std::cout << "Current tick: " << loop.currentTick() << '\n';
    std::cout << "Logic counter: " << logicCounter << '\n';
    return 0;
}
