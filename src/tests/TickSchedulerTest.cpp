#include "../logic/core/SimulationConfig.h"
#include "../logic/core/TickScheduler.h"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {

bool verify(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

struct RunResult {
    std::int64_t tickCount{0};
    std::int64_t stateHash{0};
};

RunResult runSequence(const std::vector<std::int64_t>& frameTimesMicros) {
    tcp::logic::SimulationConfig config{};
    config.ticksPerSecond = 30;
    config.maxCatchUpTicks = 8;

    tcp::logic::TickScheduler scheduler(config);
    RunResult result{};

    for (const auto frameElapsed : frameTimesMicros) {
        const auto summary = scheduler.step(frameElapsed, [&](std::int64_t tick) {
            result.stateHash += (tick * 7) + 3;
            result.tickCount = tick + 1;
        });
        (void)summary;
    }

    return result;
}

}  // namespace

int main() {
    bool ok = true;

    const tcp::logic::SimulationConfig config{};
    const auto tickMicros = tcp::logic::tickDurationMicros(config);

    std::vector<std::int64_t> sequenceA;
    std::vector<std::int64_t> sequenceB;

    for (int i = 0; i < 60; ++i) {
        sequenceA.push_back(tickMicros / 2);
        sequenceA.push_back(tickMicros - (tickMicros / 2));
        sequenceB.push_back(tickMicros);
    }

    const auto resultA = runSequence(sequenceA);
    const auto resultB = runSequence(sequenceB);

    ok &= verify(resultA.tickCount == resultB.tickCount, "tick counts differ across frame-rate patterns");
    ok &= verify(resultA.stateHash == resultB.stateHash, "state hash differs across frame-rate patterns");

    tcp::logic::SimulationConfig cappedConfig{};
    cappedConfig.ticksPerSecond = 30;
    cappedConfig.maxCatchUpTicks = 2;

    tcp::logic::TickScheduler capped(cappedConfig);
    const auto cappedSummary = capped.step(tickMicros * 10, [](std::int64_t) {});

    ok &= verify(cappedSummary.ticksExecuted == 2, "max catch-up cap not enforced");
    ok &= verify(cappedSummary.catchUpLimited, "catch-up overflow not reported");

    if (!ok) {
        return 1;
    }

    std::cout << "TickScheduler tests passed" << '\n';
    return 0;
}
