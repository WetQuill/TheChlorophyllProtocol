#pragma once

#include "../World.h"

#include <cstdint>

namespace tcp::logic::ecs {

void runZombieAISystem(World& world, std::int64_t currentTick);

}  // namespace tcp::logic::ecs
