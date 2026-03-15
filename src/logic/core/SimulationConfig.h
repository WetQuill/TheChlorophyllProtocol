#pragma once

#include <cstdint>

namespace tcp::logic {

struct SimulationConfig {
    std::int32_t ticksPerSecond{30};
    std::int32_t maxCatchUpTicks{5};
};

[[nodiscard]] std::int64_t tickDurationMicros(const SimulationConfig& config) noexcept;

}  // namespace tcp::logic
