#pragma once

#include "../math/FixedPoint.h"

#include <cstdint>

namespace tcp::logic {

struct DeterminismConfig {
    std::int32_t ticksPerSecond{30};
    std::int32_t maxCatchUpTicks{5};
    std::uint32_t matchSeed{0xA341316CU};
    std::int32_t fixedPointScale{math::FixedPoint::kScale};
};

}  // namespace tcp::logic
