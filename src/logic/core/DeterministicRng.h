#pragma once

#include <cstdint>

namespace tcp::logic {

class DeterministicRng final {
public:
    explicit DeterministicRng(std::uint32_t seed = 0xA341316CU) noexcept;

    void reseed(std::uint32_t seed) noexcept;

    [[nodiscard]] std::uint32_t nextU32() noexcept;

    [[nodiscard]] std::int32_t nextI32(std::int32_t minInclusive, std::int32_t maxInclusive) noexcept;

private:
    [[nodiscard]] static std::uint32_t normalizeSeed(std::uint32_t seed) noexcept;

    std::uint32_t state_{0xA341316CU};
};

}  // namespace tcp::logic
