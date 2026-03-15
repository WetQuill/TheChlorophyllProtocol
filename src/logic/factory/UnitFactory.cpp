#include "UnitFactory.h"

namespace tcp::logic::factory {

std::uint32_t spawnUnit(
    ecs::World& world,
    const data::UnitConfig& config,
    std::uint8_t teamId,
    const ecs::Transform& transform) {
    const auto entity = world.createEntity();

    world.setTeam(entity, ecs::Team{teamId});
    world.setTransform(entity, transform);
    world.setIdentity(entity, ecs::Identity{config.id, 1});
    world.setHealth(entity, ecs::Health{config.maxHealth, config.maxHealth});

    ecs::Velocity velocity{};
    if (config.movementMode == "ground") {
        velocity.xPerTick = math::FixedPoint::fromInt(config.moveSpeedPerTick);
        velocity.yPerTick = math::FixedPoint::fromInt(0);
    }
    world.setVelocity(entity, velocity);

    ecs::Weapon weapon{};
    weapon.range = math::FixedPoint::fromInt(config.weaponRangeCells);
    weapon.damage = config.weaponDamage;
    weapon.cooldownTicks = config.weaponCooldownTicks;
    weapon.remainingCooldownTicks = 0;
    world.setWeapon(entity, weapon);

    return entity;
}

}  // namespace tcp::logic::factory
