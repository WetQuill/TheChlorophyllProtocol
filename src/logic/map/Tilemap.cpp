#include "Tilemap.h"

namespace tcp::logic::map {

std::size_t Tilemap::indexOf(std::int32_t gx, std::int32_t gy) const noexcept {
    return static_cast<std::size_t>(gy) * static_cast<std::size_t>(width)
         + static_cast<std::size_t>(gx);
}

bool Tilemap::isInBounds(std::int32_t gx, std::int32_t gy) const noexcept {
    return gx >= 0 && gx < width && gy >= 0 && gy < height;
}

TileType Tilemap::tileAt(std::int32_t gx, std::int32_t gy) const noexcept {
    if (!isInBounds(gx, gy)) {
        return TileType::Obstacle;
    }
    return tiles[indexOf(gx, gy)];
}

std::int32_t Tilemap::entityAt(std::int32_t gx, std::int32_t gy) const noexcept {
    if (!isInBounds(gx, gy)) {
        return 0;
    }
    return occupancy[indexOf(gx, gy)];
}

void Tilemap::setOccupancy(std::int32_t gx, std::int32_t gy, std::int32_t entityId) noexcept {
    if (!isInBounds(gx, gy)) {
        return;
    }
    occupancy[indexOf(gx, gy)] = entityId;
}

void Tilemap::clearOccupancy(std::int32_t gx, std::int32_t gy) noexcept {
    setOccupancy(gx, gy, 0);
}

void Tilemap::clearAllOccupancy() noexcept {
    std::fill(occupancy.begin(), occupancy.end(), 0);
}

bool Tilemap::isWalkable(std::int32_t gx, std::int32_t gy) const noexcept {
    if (!isInBounds(gx, gy)) {
        return false;
    }
    const auto idx = indexOf(gx, gy);
    return tiles[idx] != TileType::Obstacle && occupancy[idx] == 0;
}

}  // namespace tcp::logic::map
