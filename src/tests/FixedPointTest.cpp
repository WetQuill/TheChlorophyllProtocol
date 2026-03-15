#include "../logic/math/FixedPoint.h"

#include <iostream>
#include <limits>

namespace {

using tcp::logic::math::FixedPoint;

bool verify(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

}  // namespace

int main() {
    bool ok = true;

    const auto two = FixedPoint::fromInt(2);
    const auto three = FixedPoint::fromInt(3);
    const auto half = FixedPoint::fromRaw(500);

    ok &= verify((two + three).toIntTrunc() == 5, "addition failed");
    ok &= verify((three - two).toIntTrunc() == 1, "subtraction failed");
    ok &= verify((two * three).toIntTrunc() == 6, "multiplication failed");
    ok &= verify((three / two).raw() == 1500, "division failed");
    ok &= verify((three * half).raw() == 1500, "fraction multiplication failed");

    const auto saturateUp = FixedPoint::fromRaw(2147483640) + FixedPoint::fromRaw(1000);
    ok &= verify(saturateUp.raw() == 2147483647, "saturation upper bound failed");

    const auto saturateDown = FixedPoint::fromRaw(-2147483640) - FixedPoint::fromRaw(1000);
    ok &= verify(saturateDown.raw() == std::numeric_limits<std::int32_t>::min(), "saturation lower bound failed");

    if (!ok) {
        return 1;
    }

    std::cout << "FixedPoint tests passed" << '\n';
    return 0;
}
