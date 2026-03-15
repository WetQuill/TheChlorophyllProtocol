#pragma once

#include <cstdint>
#include <set>
#include <vector>

namespace tcp::logic::path {

struct GridCoord {
    std::int32_t x{0};
    std::int32_t y{0};

    [[nodiscard]] bool operator==(const GridCoord& rhs) const noexcept {
        return x == rhs.x && y == rhs.y;
    }

    [[nodiscard]] bool operator<(const GridCoord& rhs) const noexcept {
        if (x != rhs.x) {
            return x < rhs.x;
        }
        return y < rhs.y;
    }
};

struct GridBounds {
    std::int32_t minX{-32};
    std::int32_t maxX{32};
    std::int32_t minY{-32};
    std::int32_t maxY{32};
};

[[nodiscard]] std::vector<GridCoord> findPathAStar(
    GridCoord start,
    GridCoord goal,
    const std::set<GridCoord>& blocked,
    const GridBounds& bounds);

}  // namespace tcp::logic::path
