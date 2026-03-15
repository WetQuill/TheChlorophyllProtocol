#include "SimulationConfig.h"

#include <cassert>

namespace tcp::logic {

std::int64_t tickDurationMicros(const SimulationConfig& config) noexcept {
    assert(config.ticksPerSecond > 0);
    return 1000000LL / config.ticksPerSecond;
}

}  // namespace tcp::logic
