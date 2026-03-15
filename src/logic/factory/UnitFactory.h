#pragma once

#include "../../data/UnitConfig.h"
#include "../ecs/World.h"

#include <cstdint>

namespace tcp::logic::factory {

[[nodiscard]] std::uint32_t spawnUnit(
    ecs::World& world,
    const data::UnitConfig& config,
    std::uint8_t teamId,
    const ecs::Transform& transform);

}  // namespace tcp::logic::factory
