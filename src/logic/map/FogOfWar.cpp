#include "FogOfWar.h"

#include <cstdlib>

namespace tcp::logic::map {

void FogOfWar::resize(std::int32_t w, std::int32_t h) {
    width = w;
    height = h;
    visibility.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
                      static_cast<std::uint8_t>(Visibility::Unexplored));
}

std::size_t FogOfWar::indexOf(std::int32_t gx, std::int32_t gy) const noexcept {
    return static_cast<std::size_t>(gy) * static_cast<std::size_t>(width)
         + static_cast<std::size_t>(gx);
}

bool FogOfWar::isInBounds(std::int32_t gx, std::int32_t gy) const noexcept {
    return gx >= 0 && gx < width && gy >= 0 && gy < height;
}

Visibility FogOfWar::at(std::int32_t gx, std::int32_t gy) const noexcept {
    if (!isInBounds(gx, gy)) {
        return Visibility::Unexplored;
    }
    return static_cast<Visibility>(visibility[indexOf(gx, gy)]);
}

void FogOfWar::set(std::int32_t gx, std::int32_t gy, Visibility v) noexcept {
    if (!isInBounds(gx, gy)) {
        return;
    }
    visibility[indexOf(gx, gy)] = static_cast<std::uint8_t>(v);
}

void FogOfWar::reveal(std::int32_t gx, std::int32_t gy, std::int32_t radius) noexcept {
    if (radius <= 0) {
        set(gx, gy, Visibility::Visible);
        return;
    }
    const auto r = static_cast<std::size_t>(std::abs(radius));
    for (std::int32_t dy = -static_cast<std::int32_t>(r); dy <= static_cast<std::int32_t>(r); ++dy) {
        for (std::int32_t dx = -static_cast<std::int32_t>(r); dx <= static_cast<std::int32_t>(r); ++dx) {
            const std::int32_t nx = gx + dx;
            const std::int32_t ny = gy + dy;
            if (!isInBounds(nx, ny)) {
                continue;
            }
            const auto idx = indexOf(nx, ny);
            if (visibility[idx] < static_cast<std::uint8_t>(Visibility::Visible)) {
                visibility[idx] = static_cast<std::uint8_t>(Visibility::Visible);
            }
        }
    }
}

void FogOfWar::advanceTick() noexcept {
    for (auto& v : visibility) {
        if (v == static_cast<std::uint8_t>(Visibility::Visible)) {
            v = static_cast<std::uint8_t>(Visibility::Fogged);
        }
    }
}

bool FogOfWar::isVisible(std::int32_t gx, std::int32_t gy) const noexcept {
    return at(gx, gy) == Visibility::Visible;
}

}  // namespace tcp::logic::map
