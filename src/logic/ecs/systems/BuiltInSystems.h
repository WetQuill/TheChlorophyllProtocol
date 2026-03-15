#pragma once

#include "../World.h"

#include <cstdint>

namespace tcp::logic::ecs {

void runInputPhase(World& world, std::int64_t tick);
void runProductionPhase(World& world, std::int64_t tick);
void runPathfindingPhase(World& world, std::int64_t tick);
void runMovementPhase(World& world, std::int64_t tick);
void runCombatPhase(World& world, std::int64_t tick);
void runResourcePhase(World& world, std::int64_t tick);
void runCleanupPhase(World& world, std::int64_t tick);

void registerCoreSystems(World& world);

}  // namespace tcp::logic::ecs
