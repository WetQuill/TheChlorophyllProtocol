#include "BuiltInSystems.h"

#include <algorithm>
#include <vector>

namespace tcp::logic::ecs {

void runInputPhase(World& world, std::int64_t tick) {
    auto& buffers = world.mutableCommandBuffers();
    for (auto& [entityId, commandBuffer] : buffers) {
        (void)entityId;
        auto& queue = commandBuffer.queued;
        queue.erase(
            std::remove_if(queue.begin(), queue.end(), [&](const QueuedCommand& cmd) {
                return cmd.tick <= tick;
            }),
            queue.end());
    }
}

void runMovementPhase(World& world, std::int64_t tick) {
    (void)tick;
    auto& transforms = world.mutableTransforms();
    const auto& velocities = world.velocities();
    for (auto& [entityId, transform] : transforms) {
        const auto velIt = velocities.find(entityId);
        if (velIt == velocities.end()) {
            continue;
        }
        transform.x += velIt->second.xPerTick;
        transform.y += velIt->second.yPerTick;
    }
}

void runCleanupPhase(World& world, std::int64_t tick) {
    (void)tick;
    std::vector<EntityId> toDestroy;
    const auto& healths = world.healths();
    for (const auto& [entityId, health] : healths) {
        if (health.current <= 0) {
            toDestroy.push_back(entityId);
        }
    }

    for (const auto entityId : toDestroy) {
        world.destroyEntity(entityId);
    }
}

void registerCoreSystems(World& world) {
    world.registerSystem(SystemPhase::kInput, runInputPhase);
    world.registerSystem(SystemPhase::kMovement, runMovementPhase);
    world.registerSystem(SystemPhase::kCleanup, runCleanupPhase);
}

}  // namespace tcp::logic::ecs
