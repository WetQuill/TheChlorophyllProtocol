#pragma once

#include "../ecs/World.h"

#include <cstdint>

namespace tcp::logic::debug {

[[nodiscard]] std::uint64_t hashWorldState(const ecs::World& world) noexcept;
void updateDeterminismSnapshot(ecs::World& world) noexcept;

}  // namespace tcp::logic::debug
