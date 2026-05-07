#pragma once

#include <cstdint>
#include <vector>

namespace tcp::logic::map {

enum class Visibility : std::uint8_t {
    Unexplored = 0,
    Fogged = 1,
    Visible = 2,
};

struct FogOfWar {
    std::int32_t width{0};
    std::int32_t height{0};
    std::vector<std::uint8_t> visibility;

    void resize(std::int32_t w, std::int32_t h);

    [[nodiscard]] std::size_t indexOf(std::int32_t gx, std::int32_t gy) const noexcept;
    [[nodiscard]] bool isInBounds(std::int32_t gx, std::int32_t gy) const noexcept;

    [[nodiscard]] Visibility at(std::int32_t gx, std::int32_t gy) const noexcept;
    void set(std::int32_t gx, std::int32_t gy, Visibility v) noexcept;

    void reveal(std::int32_t gx, std::int32_t gy, std::int32_t radius) noexcept;
    void advanceTick() noexcept;

    [[nodiscard]] bool isVisible(std::int32_t gx, std::int32_t gy) const noexcept;
};

}  // namespace tcp::logic::map
