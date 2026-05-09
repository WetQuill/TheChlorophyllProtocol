#include "SpawnerSystem.h"

#include "../components/Components.h"
#include "../../debug/DebugLog.h"

#include <cstdint>

namespace tcp::logic::ecs {

namespace {
    constexpr std::int64_t kSpawnIntervalTicks = 1500;
    constexpr std::size_t kMaxZombieCount = 30;
    constexpr std::int32_t kZombieHealth = 500;
    constexpr std::int32_t kZombieDamage = 20;
    constexpr std::int32_t kZombieAttackCooldown = 30;
    constexpr std::int32_t kZombieVisionRadius = 3;
    constexpr std::uint32_t kZombieArchetypeId = 200U;
    constexpr std::uint8_t kZombieTeam = 2U;

    constexpr std::int32_t kSpawnPositions[5][2] = {
        {59, 16},
        {60, 16},
        {61, 16},
        {59, 17},
        {60, 17},
    };
}  // namespace

void runSpawnerSystem(World& world, std::int64_t currentTick) {
    static std::int64_t lastSpawnTick = 0;

    if (world.tilemap().width <= 0 || world.tilemap().height <= 0) {
        return;
    }

    if (currentTick - lastSpawnTick < kSpawnIntervalTicks) {
        return;
    }

    if (world.zombieAIFSMs().size() >= kMaxZombieCount) {
        return;
    }

    for (const auto& pos : kSpawnPositions) {
        const std::int32_t gx = pos[0];
        const std::int32_t gy = pos[1];

        if (!world.tilemap().isInBounds(gx, gy)) {
            continue;
        }

        const auto entityId = world.createEntity();
        world.setTeam(entityId, Team{kZombieTeam});
        world.setTransform(entityId, Transform{
            math::FixedPoint::fromInt(gx),
            math::FixedPoint::fromInt(gy),
        });
        world.setHealth(entityId, Health{kZombieHealth, kZombieHealth});
        world.setVelocity(entityId, Velocity{});
        world.setZombieAIFSM(entityId, ZombieAIFSM{});
        world.setWeapon(entityId, Weapon{
            math::FixedPoint::fromInt(1),
            kZombieDamage,
            kZombieAttackCooldown,
            0,
        });
        world.setCommandBuffer(entityId, CommandBuffer{});
        world.setVision(entityId, Vision{kZombieVisionRadius});
        world.setIdentity(entityId, Identity{kZombieArchetypeId, 1});
    }

    lastSpawnTick = currentTick;
    TCP_DEBUG("SPAWN", currentTick, "spawned 5 zombies, total entities=%zu", world.entityCount());
}

}  // namespace tcp::logic::ecs
