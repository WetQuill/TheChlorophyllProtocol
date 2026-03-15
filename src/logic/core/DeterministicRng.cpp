#include "DeterministicRng.h"

#include <cassert>
#include <cstdint>

namespace tcp::logic {

DeterministicRng::DeterministicRng(std::uint32_t seed) noexcept
    : state_(normalizeSeed(seed)) {}

void DeterministicRng::reseed(std::uint32_t seed) noexcept {
    state_ = normalizeSeed(seed);
}

std::uint32_t DeterministicRng::nextU32() noexcept {
    std::uint32_t x = state_;
    x ^= x << 13U;
    x ^= x >> 17U;
    x ^= x << 5U;
    state_ = normalizeSeed(x);
    return state_;
}

std::int32_t DeterministicRng::nextI32(std::int32_t minInclusive, std::int32_t maxInclusive) noexcept {
    assert(minInclusive <= maxInclusive);
    if (minInclusive > maxInclusive) {
        return minInclusive;
    }

    const std::uint32_t span = static_cast<std::uint32_t>(
        static_cast<std::int64_t>(maxInclusive) - static_cast<std::int64_t>(minInclusive) + 1);

    const std::uint32_t value = nextU32();
    const std::uint32_t offset = (span == 0U) ? value : (value % span);

    return static_cast<std::int32_t>(static_cast<std::int64_t>(minInclusive) + static_cast<std::int64_t>(offset));
}

std::uint32_t DeterministicRng::normalizeSeed(std::uint32_t seed) noexcept {
    return (seed == 0U) ? 0xA341316CU : seed;
}

}  // namespace tcp::logic
