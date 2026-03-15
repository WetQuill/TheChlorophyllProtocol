#pragma once

#include "SimulationConfig.h"

#include <cstdint>
#include <functional>

namespace tcp::logic {

struct TickStepSummary {
    std::int32_t ticksExecuted{0};
    std::int64_t remainingMicros{0};
    std::int64_t tickDurationMicros{0};
    bool catchUpLimited{false};
};

class TickScheduler final {
public:
    explicit TickScheduler(const SimulationConfig& config) noexcept;

    void reset() noexcept;

    [[nodiscard]] std::int64_t currentTick() const noexcept;

    [[nodiscard]] TickStepSummary step(
        std::int64_t elapsedMicros,
        const std::function<void(std::int64_t)>& onTick) noexcept;

private:
    SimulationConfig config_{};
    std::int64_t tickDurationMicros_{0};
    std::int64_t accumulatedMicros_{0};
    std::int64_t currentTick_{0};
};

}  // namespace tcp::logic
