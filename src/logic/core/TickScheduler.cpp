#include "TickScheduler.h"

#include <algorithm>
#include <cassert>

namespace tcp::logic {

TickScheduler::TickScheduler(const SimulationConfig& config) noexcept
    : config_(config), tickDurationMicros_(tickDurationMicros(config)) {
    assert(config_.maxCatchUpTicks > 0);
}

void TickScheduler::reset() noexcept {
    accumulatedMicros_ = 0;
    currentTick_ = 0;
}

std::int64_t TickScheduler::currentTick() const noexcept {
    return currentTick_;
}

TickStepSummary TickScheduler::step(
    std::int64_t elapsedMicros,
    const std::function<void(std::int64_t)>& onTick) noexcept {
    TickStepSummary summary{};
    summary.tickDurationMicros = tickDurationMicros_;

    if (elapsedMicros < 0) {
        elapsedMicros = 0;
    }

    accumulatedMicros_ += elapsedMicros;

    const std::int32_t maxTicks = std::max<std::int32_t>(1, config_.maxCatchUpTicks);
    while (accumulatedMicros_ >= tickDurationMicros_ && summary.ticksExecuted < maxTicks) {
        if (onTick) {
            onTick(currentTick_);
        }
        ++currentTick_;
        accumulatedMicros_ -= tickDurationMicros_;
        ++summary.ticksExecuted;
    }

    if (accumulatedMicros_ >= tickDurationMicros_) {
        summary.catchUpLimited = true;
        accumulatedMicros_ %= tickDurationMicros_;
    }

    summary.remainingMicros = accumulatedMicros_;
    return summary;
}

}  // namespace tcp::logic
