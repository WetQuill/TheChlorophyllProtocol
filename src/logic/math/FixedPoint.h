#pragma once

#include <cstdint>

namespace tcp::logic::math {

class FixedPoint final {
public:
    static constexpr std::int32_t kScale = 1000;

    constexpr FixedPoint() noexcept = default;

    static constexpr FixedPoint fromRaw(std::int32_t rawValue) noexcept {
        FixedPoint value;
        value.rawValue_ = rawValue;
        return value;
    }

    static FixedPoint fromInt(std::int32_t intValue) noexcept;

    [[nodiscard]] constexpr std::int32_t raw() const noexcept {
        return rawValue_;
    }

    [[nodiscard]] constexpr std::int32_t toIntTrunc() const noexcept {
        return rawValue_ / kScale;
    }

    [[nodiscard]] constexpr FixedPoint operator+() const noexcept {
        return *this;
    }

    [[nodiscard]] FixedPoint operator-() const noexcept;

    FixedPoint& operator+=(FixedPoint rhs) noexcept;
    FixedPoint& operator-=(FixedPoint rhs) noexcept;
    FixedPoint& operator*=(FixedPoint rhs) noexcept;
    FixedPoint& operator/=(FixedPoint rhs) noexcept;

    [[nodiscard]] friend FixedPoint operator+(FixedPoint lhs, FixedPoint rhs) noexcept {
        lhs += rhs;
        return lhs;
    }

    [[nodiscard]] friend FixedPoint operator-(FixedPoint lhs, FixedPoint rhs) noexcept {
        lhs -= rhs;
        return lhs;
    }

    [[nodiscard]] friend FixedPoint operator*(FixedPoint lhs, FixedPoint rhs) noexcept {
        lhs *= rhs;
        return lhs;
    }

    [[nodiscard]] friend FixedPoint operator/(FixedPoint lhs, FixedPoint rhs) noexcept {
        lhs /= rhs;
        return lhs;
    }

    [[nodiscard]] friend constexpr bool operator==(FixedPoint lhs, FixedPoint rhs) noexcept {
        return lhs.rawValue_ == rhs.rawValue_;
    }

    [[nodiscard]] friend constexpr bool operator!=(FixedPoint lhs, FixedPoint rhs) noexcept {
        return !(lhs == rhs);
    }

    [[nodiscard]] friend constexpr bool operator<(FixedPoint lhs, FixedPoint rhs) noexcept {
        return lhs.rawValue_ < rhs.rawValue_;
    }

    [[nodiscard]] friend constexpr bool operator<=(FixedPoint lhs, FixedPoint rhs) noexcept {
        return lhs.rawValue_ <= rhs.rawValue_;
    }

    [[nodiscard]] friend constexpr bool operator>(FixedPoint lhs, FixedPoint rhs) noexcept {
        return lhs.rawValue_ > rhs.rawValue_;
    }

    [[nodiscard]] friend constexpr bool operator>=(FixedPoint lhs, FixedPoint rhs) noexcept {
        return lhs.rawValue_ >= rhs.rawValue_;
    }

private:
    std::int32_t rawValue_{0};
};

}  // namespace tcp::logic::math
