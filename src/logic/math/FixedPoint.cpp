#include "FixedPoint.h"

#include <cassert>
#include <cstdint>
#include <limits>

namespace tcp::logic::math {

namespace {

[[nodiscard]] std::int32_t clampToInt32(std::int64_t value) noexcept {
    if (value > static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())) {
        return std::numeric_limits<std::int32_t>::max();
    }
    if (value < static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min())) {
        return std::numeric_limits<std::int32_t>::min();
    }
    return static_cast<std::int32_t>(value);
}

}  // namespace

FixedPoint FixedPoint::fromInt(std::int32_t intValue) noexcept {
    const std::int64_t expanded = static_cast<std::int64_t>(intValue) * kScale;
    return fromRaw(clampToInt32(expanded));
}

FixedPoint FixedPoint::operator-() const noexcept {
    const std::int64_t expanded = -static_cast<std::int64_t>(rawValue_);
    return fromRaw(clampToInt32(expanded));
}

FixedPoint& FixedPoint::operator+=(FixedPoint rhs) noexcept {
    const std::int64_t expanded = static_cast<std::int64_t>(rawValue_) + static_cast<std::int64_t>(rhs.rawValue_);
    rawValue_ = clampToInt32(expanded);
    return *this;
}

FixedPoint& FixedPoint::operator-=(FixedPoint rhs) noexcept {
    const std::int64_t expanded = static_cast<std::int64_t>(rawValue_) - static_cast<std::int64_t>(rhs.rawValue_);
    rawValue_ = clampToInt32(expanded);
    return *this;
}

FixedPoint& FixedPoint::operator*=(FixedPoint rhs) noexcept {
    const std::int64_t expanded = static_cast<std::int64_t>(rawValue_) * static_cast<std::int64_t>(rhs.rawValue_);
    rawValue_ = clampToInt32(expanded / kScale);
    return *this;
}

FixedPoint& FixedPoint::operator/=(FixedPoint rhs) noexcept {
    assert(rhs.rawValue_ != 0);
    if (rhs.rawValue_ == 0) {
        rawValue_ = (rawValue_ >= 0) ? std::numeric_limits<std::int32_t>::max() : std::numeric_limits<std::int32_t>::min();
        return *this;
    }

    const std::int64_t expanded = static_cast<std::int64_t>(rawValue_) * kScale;
    rawValue_ = clampToInt32(expanded / rhs.rawValue_);
    return *this;
}

}  // namespace tcp::logic::math
