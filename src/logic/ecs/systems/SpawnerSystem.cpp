#include "SpawnerSystem.h"

#include "../components/Components.h"
#include "../../debug/DebugLog.h"

#include <cstdint>

namespace tcp::logic::ecs {

namespace {
    constexpr std::int64_t kSpawnIntervalTicks = 1500;
    constexpr std::size_t kMaxZombieCount = 30;
    constexpr std::int32_t kZombieAttackCooldown = 30;
    constexpr std::int32_t kZombieVisionRadius = 3;
    constexpr std::uint32_t kRegularZombieArchetypeId = 200U;
    constexpr std::uint32_t kBucketheadZombieArchetypeId = 202U;
    constexpr std::uint8_t kZombieTeam = 2U;

    constexpr std::int32_t kRegularZombieHealth = 150000;
    constexpr std::int32_t kRegularZombieDamage = 1000;

    constexpr std::int32_t kBucketheadZombieHealth = 600000;
    constexpr std::int32_t kBucketheadZombieDamage = 2000;
    constexpr std::int32_t kBucketheadZombieArmor = 5000;

    constexpr std::int32_t kSpawnPositions[5][2] = {
        {57, 3},
        {58, 3},
        {59, 3},
        {57, 4},
        {58, 4},
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

    for (std::size_t i = 0; i < 5; ++i) {
        const std::int32_t gx = kSpawnPositions[i][0];
        const std::int32_t gy = kSpawnPositions[i][1];

        if (!world.tilemap().isInBounds(gx, gy)) {
            continue;
        }

        const bool isBuckethead = (i == 4);
        const auto archetypeId = isBuckethead ? kBucketheadZombieArchetypeId : kRegularZombieArchetypeId;
        const auto hp = isBuckethead ? kBucketheadZombieHealth : kRegularZombieHealth;
        const auto dmg = isBuckethead ? kBucketheadZombieDamage : kRegularZombieDamage;

        const auto entityId = world.createEntity();
        world.setTeam(entityId, Team{kZombieTeam});
        world.setTransform(entityId, Transform{
            math::FixedPoint::fromInt(gx),
            math::FixedPoint::fromInt(gy),
        });
        world.setHealth(entityId, Health{hp, hp});
        world.setVelocity(entityId, Velocity{});
        world.setZombieAIFSM(entityId, ZombieAIFSM{});
        world.setWeapon(entityId, Weapon{
            math::FixedPoint::fromInt(1),
            dmg,
            kZombieAttackCooldown,
            0,
        });
        world.setCommandBuffer(entityId, CommandBuffer{});
        world.setVision(entityId, Vision{kZombieVisionRadius});
        world.setIdentity(entityId, Identity{archetypeId, 1});

        if (isBuckethead) {
            world.setArmor(entityId, Armor{kBucketheadZombieArmor});
        }
    }

    lastSpawnTick = currentTick;
    TCP_DEBUG("SPAWN", currentTick, "spawned 5 zombies (4 regular + 1 buckethead), total entities=%zu", world.entityCount());
}

}  // namespace tcp::logic::ecs
