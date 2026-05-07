#pragma once

#include <cstdint>
#include <vector>

namespace tcp::logic::map {

enum class TileType : std::uint8_t {
    Grass = 0,
    Water = 1,
    Obstacle = 2,
};

struct SpawnPoint {
    std::uint8_t teamId;
    std::int32_t x;
    std::int32_t y;
};

struct Tilemap {
    std::int32_t width{0};
    std::int32_t height{0};
    std::vector<TileType> tiles;
    std::vector<std::int32_t> occupancy;
    std::vector<SpawnPoint> spawnPoints;

    [[nodiscard]] std::size_t indexOf(std::int32_t gx, std::int32_t gy) const noexcept;
    [[nodiscard]] bool isInBounds(std::int32_t gx, std::int32_t gy) const noexcept;

    [[nodiscard]] TileType tileAt(std::int32_t gx, std::int32_t gy) const noexcept;
    [[nodiscard]] std::int32_t entityAt(std::int32_t gx, std::int32_t gy) const noexcept;
    void setOccupancy(std::int32_t gx, std::int32_t gy, std::int32_t entityId) noexcept;
    void clearOccupancy(std::int32_t gx, std::int32_t gy) noexcept;
    void clearAllOccupancy() noexcept;

    [[nodiscard]] bool isWalkable(std::int32_t gx, std::int32_t gy) const noexcept;
};

}  // namespace tcp::logic::map
