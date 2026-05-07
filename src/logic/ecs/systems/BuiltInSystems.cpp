#include "BuiltInSystems.h"

#include "../../map/FogOfWar.h"
#include "../../map/Tilemap.h"
#include "../../path/AStarGrid.h"

#include <algorithm>
#include <cstdint>
#include <set>
#include <vector>

namespace tcp::logic::ecs {

namespace {

constexpr std::int32_t kBuildRadiusCells = 4;
constexpr std::int32_t kDefaultBuildCostSun = 20;
constexpr std::int32_t kDefaultSunPowerPlantCostSun = 25;
constexpr std::int32_t kBuildingHealth = 20;
constexpr std::uint32_t kPeaMilitaryCampArchetypeId = 901U;
constexpr std::uint32_t kSunPowerPlantArchetypeId = 902U;
constexpr std::uint32_t kCornCannonBastionArchetypeId = 903U;
constexpr std::int32_t kSunPowerPlantPerTick = 3;
constexpr std::uint32_t kPeaMilitiaArchetypeId = 101U;
constexpr std::int32_t kPeaMilitiaHealth = 30;
constexpr std::int32_t kPeaMilitiaDamage = 5;
constexpr std::int32_t kPeaMilitiaAttackCooldown = 1;
constexpr std::int32_t kPeaMilitiaVisionRadius = 4;
constexpr std::int32_t kProducePeaCostSun = 20;
constexpr std::int32_t kCornCannonCostSun = 1200000;
constexpr std::int32_t kCornCannonPowerRequirement = 80000;
constexpr std::int32_t kCornCannonHealth = 2500000;
constexpr std::int32_t kCornCannonArmor = 25000;
constexpr std::int32_t kCornCannonButterDamage = 300000;
constexpr std::int32_t kCornCannonReloadTicks = 150;
constexpr std::int32_t kCornCannonFrozenTicks = 150;
constexpr std::int32_t kCornCannonProjectileTravelTicks = 24;

struct BuildBlueprint {
    std::uint32_t archetypeId{0};
    std::int32_t defaultCostSun{0};
    std::int32_t defaultCostPower{0};
    std::int32_t health{kBuildingHealth};
    std::int32_t armor{0};
    std::int32_t requiredPower{0};
    bool hasCommandBuffer{false};
    bool hasArtilleryWeapon{false};
    ArtilleryWeapon artilleryWeapon{};
    bool grantsSunProduction{false};
    std::int32_t sunPerTick{0};
};

[[nodiscard]] path::GridCoord toGridCoord(const Transform& transform) noexcept {
    return {
        transform.x.toIntTrunc(),
        transform.y.toIntTrunc(),
    };
}

[[nodiscard]] Transform toTransform(path::GridCoord grid) noexcept {
    Transform transform{};
    transform.x = math::FixedPoint::fromInt(grid.x);
    transform.y = math::FixedPoint::fromInt(grid.y);
    return transform;
}

[[nodiscard]] bool isCellOccupied(const World& world, path::GridCoord cell) {
    if (world.tilemap().width > 0 && world.tilemap().height > 0) {
        return world.tilemap().entityAt(cell.x, cell.y) != 0;
    }
    const auto& buildings = world.buildings();
    const auto& transforms = world.transforms();
    for (const auto& [entityId, building] : buildings) {
        (void)building;
        const auto trIt = transforms.find(entityId);
        if (trIt == transforms.end()) {
            continue;
        }
        if (toGridCoord(trIt->second) == cell) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool hasFriendlyHqInRange(const World& world, std::uint8_t teamId, path::GridCoord cell) {
    const auto& hqs = world.headquarters();
    const auto& teams = world.teams();
    const auto& transforms = world.transforms();
    for (const auto& [entityId, marker] : hqs) {
        if (!marker.value) {
            continue;
        }

        const auto teamIt = teams.find(entityId);
        const auto trIt = transforms.find(entityId);
        if (teamIt == teams.end() || trIt == transforms.end()) {
            continue;
        }
        if (teamIt->second.value != teamId) {
            continue;
        }

        const auto base = toGridCoord(trIt->second);
        const auto dx = (base.x > cell.x) ? (base.x - cell.x) : (cell.x - base.x);
        const auto dy = (base.y > cell.y) ? (base.y - cell.y) : (cell.y - base.y);
        if (dx + dy <= kBuildRadiusCells) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::int32_t mitigatedDamage(const World& world, EntityId targetId, std::int32_t baseDamage) {
    if (baseDamage <= 0) {
        return 0;
    }

    const auto armorIt = world.armors().find(targetId);
    const auto armor = (armorIt == world.armors().end()) ? 0 : armorIt->second.value;
    if (baseDamage <= armor) {
        return 0;
    }
    return baseDamage - armor;
}

void updatePowerConsumerStates(World& world) {
    auto& consumers = world.mutablePowerConsumers();
    const auto& teams = world.teams();
    for (auto& [entityId, consumer] : consumers) {
        const auto teamIt = teams.find(entityId);
        if (teamIt == teams.end()) {
            consumer.enabled = false;
            continue;
        }
        consumer.enabled = world.powerForTeam(teamIt->second.value) >= consumer.requiredPower;
    }
}

void applyButterAoeAt(World& world,
                      std::uint8_t sourceTeam,
                      const math::FixedPoint centerX,
                      const math::FixedPoint centerY,
                      const math::FixedPoint aoeRadius,
                      std::int32_t damage,
                      std::int32_t frozenTicks) {
    auto& healths = world.mutableHealths();
    const auto& transforms = world.transforms();
    const auto& teams = world.teams();
    const auto radiusSq = aoeRadius * aoeRadius;

    for (auto& [entityId, health] : healths) {
        const auto trIt = transforms.find(entityId);
        const auto teamIt = teams.find(entityId);
        if (trIt == transforms.end() || teamIt == teams.end()) {
            continue;
        }
        if (teamIt->second.value == sourceTeam) {
            continue;
        }

        const auto dx = trIt->second.x - centerX;
        const auto dy = trIt->second.y - centerY;
        const auto distSq = (dx * dx) + (dy * dy);
        if (distSq > radiusSq) {
            continue;
        }

        health.current -= mitigatedDamage(world, entityId, damage);
        if (frozenTicks > 0) {
            world.setFrozenState(entityId, FrozenState{frozenTicks, 1000});
        }
    }
}

void tryHandleBuildCommand(World& world,
                           EntityId issuerId,
                           const QueuedCommand& cmd,
                           const BuildBlueprint& blueprint) {
    const auto teamIt = world.teams().find(issuerId);
    if (teamIt == world.teams().end()) {
        return;
    }

    const path::GridCoord buildCell{cmd.arg0, cmd.arg1};
    if (isCellOccupied(world, buildCell)) {
        return;
    }

    if (!hasFriendlyHqInRange(world, teamIt->second.value, buildCell)) {
        return;
    }

    const auto costSun = (cmd.arg2 > 0) ? cmd.arg2 : blueprint.defaultCostSun;
    const auto costPower = blueprint.defaultCostPower;

    if (world.sunForTeam(teamIt->second.value) < costSun) {
        return;
    }
    if (world.powerForTeam(teamIt->second.value) < costPower) {
        return;
    }

    if (!world.spendSunForTeam(teamIt->second.value, costSun)) {
        return;
    }
    if (!world.spendPowerForTeam(teamIt->second.value, costPower)) {
        return;
    }

    const auto buildingEntity = world.createEntity();
    world.setTeam(buildingEntity, Team{teamIt->second.value});
    world.setTransform(buildingEntity, toTransform(buildCell));
    world.setHealth(buildingEntity, Health{blueprint.health, blueprint.health});
    world.setBuilding(buildingEntity, Building{true});
    world.setIdentity(buildingEntity, Identity{blueprint.archetypeId, 1});
    if (blueprint.armor > 0) {
        world.setArmor(buildingEntity, Armor{blueprint.armor});
    }
    if (blueprint.requiredPower > 0) {
        world.setPowerConsumer(buildingEntity, PowerConsumer{blueprint.requiredPower, true});
    }
    if (blueprint.hasCommandBuffer) {
        world.setCommandBuffer(buildingEntity, CommandBuffer{});
    }
    if (blueprint.hasArtilleryWeapon) {
        world.setArtilleryWeapon(buildingEntity, blueprint.artilleryWeapon);
    }
    if (blueprint.grantsSunProduction && blueprint.sunPerTick > 0) {
        world.setSunProducer(buildingEntity, SunProducer{blueprint.sunPerTick});
    }

    if (world.tilemap().width > 0 && world.tilemap().height > 0) {
        world.mutableTilemap().setOccupancy(buildCell.x, buildCell.y,
                                            static_cast<std::int32_t>(buildingEntity));
    }
}

void tryHandleProducePeaCommand(World& world, EntityId issuerId, const QueuedCommand& cmd) {
    (void)cmd;

    const auto issuerTeamIt = world.teams().find(issuerId);
    const auto issuerTrIt = world.transforms().find(issuerId);
    const auto issuerIdentityIt = world.identities().find(issuerId);
    const auto issuerBuildingIt = world.buildings().find(issuerId);
    if (issuerTeamIt == world.teams().end() ||
        issuerTrIt == world.transforms().end() ||
        issuerIdentityIt == world.identities().end() ||
        issuerBuildingIt == world.buildings().end()) {
        return;
    }

    if (issuerIdentityIt->second.archetypeId != kPeaMilitaryCampArchetypeId) {
        return;
    }

    if (!world.spendSunForTeam(issuerTeamIt->second.value, kProducePeaCostSun)) {
        return;
    }

    const auto spawnBase = toGridCoord(issuerTrIt->second);
    const path::GridCoord spawnCell{spawnBase.x + 1, spawnBase.y};

    const auto unitEntity = world.createEntity();
    world.setTeam(unitEntity, Team{issuerTeamIt->second.value});
    world.setTransform(unitEntity, toTransform(spawnCell));
    world.setHealth(unitEntity, Health{kPeaMilitiaHealth, kPeaMilitiaHealth});
    world.setIdentity(unitEntity, Identity{kPeaMilitiaArchetypeId, 1});
    world.setCommandBuffer(unitEntity, CommandBuffer{});
    world.setVision(unitEntity, Vision{kPeaMilitiaVisionRadius});
    world.setWeapon(unitEntity,
                    Weapon{math::FixedPoint::fromInt(2),
                           kPeaMilitiaDamage,
                           kPeaMilitiaAttackCooldown,
                           0});
}

[[nodiscard]] math::FixedPoint distanceSquared(const Transform& a, const Transform& b) noexcept {
    const auto dx = a.x - b.x;
    const auto dy = a.y - b.y;
    return (dx * dx) + (dy * dy);
}

}  // namespace

void runInputPhase(World& world, std::int64_t tick) {
    auto& buffers = world.mutableCommandBuffers();
    for (auto& [entityId, commandBuffer] : buffers) {
        auto& queue = commandBuffer.queued;
        for (const auto& cmd : queue) {
            if (cmd.tick > tick) {
                continue;
            }

            world.addCommandProcessed();

            if (cmd.type == CommandType::kMove) {
                world.setMoveTarget(entityId, GridTarget{cmd.arg0, cmd.arg1});
                world.clearAttackTarget(entityId);
            } else if (cmd.type == CommandType::kAttack) {
                const auto targetId = static_cast<EntityId>(cmd.arg0);
                world.setAttackTarget(entityId, targetId);
                world.clearMoveTarget(entityId);
            } else if (cmd.type == CommandType::kBuild) {
                const BuildBlueprint campBlueprint{
                    kPeaMilitaryCampArchetypeId,
                    kDefaultBuildCostSun,
                    10,
                    kBuildingHealth,
                    0,
                    0,
                    false,
                    false,
                    ArtilleryWeapon{},
                    false,
                    0,
                };
                tryHandleBuildCommand(world, entityId, cmd, campBlueprint);
            } else if (cmd.type == CommandType::kBuildSunPowerPlant) {
                const BuildBlueprint sunPlantBlueprint{
                    kSunPowerPlantArchetypeId,
                    kDefaultSunPowerPlantCostSun,
                    12,
                    kBuildingHealth,
                    0,
                    0,
                    false,
                    false,
                    ArtilleryWeapon{},
                    true,
                    kSunPowerPlantPerTick,
                };
                tryHandleBuildCommand(world, entityId, cmd, sunPlantBlueprint);
            } else if (cmd.type == CommandType::kBuildCornCannonBastion) {
                const BuildBlueprint bastionBlueprint{
                    kCornCannonBastionArchetypeId,
                    kCornCannonCostSun,
                    0,
                    kCornCannonHealth,
                    kCornCannonArmor,
                    kCornCannonPowerRequirement,
                    true,
                    true,
                    ArtilleryWeapon{
                        math::FixedPoint::fromRaw(400000),
                        math::FixedPoint::fromRaw(1800000),
                        math::FixedPoint::fromRaw(200000),
                        kCornCannonButterDamage,
                        kCornCannonReloadTicks,
                        0,
                        kCornCannonProjectileTravelTicks,
                        kCornCannonFrozenTicks,
                    },
                    false,
                    0,
                };
                tryHandleBuildCommand(world, entityId, cmd, bastionBlueprint);
            } else if (cmd.type == CommandType::kProducePea) {
                tryHandleProducePeaCommand(world, entityId, cmd);
            }
        }

        queue.erase(
            std::remove_if(queue.begin(), queue.end(), [&](const QueuedCommand& cmd) {
                return cmd.tick <= tick;
            }),
            queue.end());
    }
}

void runProductionPhase(World& world, std::int64_t tick) {
    (void)tick;
    const auto& productions = world.productions();
    const auto& teams = world.teams();
    const auto& transforms = world.transforms();

    struct SpawnPlan {
        std::uint8_t teamId{0};
        Transform transform{};
        std::uint32_t archetypeId{0};
        std::int32_t health{0};
    };

    std::vector<SpawnPlan> spawns;
    spawns.reserve(productions.size());

    for (const auto& [entityId, productionConst] : productions) {
        auto prod = productionConst;

        if (prod.buildTicks <= 0) {
            continue;
        }

        const auto teamIt = teams.find(entityId);
        const auto trIt = transforms.find(entityId);
        if (teamIt == teams.end() || trIt == transforms.end()) {
            continue;
        }

        if (prod.progressTicks <= 0) {
            if (!world.spendSunForTeam(teamIt->second.value, prod.costSun)) {
                continue;
            }
            prod.progressTicks = 1;
        } else {
            ++prod.progressTicks;
        }

        if (prod.progressTicks >= prod.buildTicks) {
            spawns.push_back(SpawnPlan{
                teamIt->second.value,
                trIt->second,
                prod.producedArchetypeId,
                prod.producedHealth,
            });
            prod.progressTicks = 0;
        }

        world.setProduction(entityId, prod);
    }

    for (const auto& spawn : spawns) {
        const auto newEntity = world.createEntity();

        world.setTeam(newEntity, Team{spawn.teamId});
        world.setTransform(newEntity, spawn.transform);
        world.setIdentity(newEntity, Identity{spawn.archetypeId, 1});

        Health hp{};
        hp.current = spawn.health;
        hp.max = spawn.health;
        world.setHealth(newEntity, hp);
    }
}

void runPathfindingPhase(World& world, std::int64_t tick) {
    (void)tick;

    // Refresh tilemap occupancy from current building state
    auto& tilemap = world.mutableTilemap();
    if (tilemap.width > 0 && tilemap.height > 0) {
        tilemap.clearAllOccupancy();
        const auto& buildings = world.buildings();
        const auto& transforms = world.transforms();
        for (const auto& [entityId, building] : buildings) {
            (void)building;
            const auto trIt = transforms.find(entityId);
            if (trIt == transforms.end()) {
                continue;
            }
            const auto cell = toGridCoord(trIt->second);
            tilemap.setOccupancy(cell.x, cell.y, static_cast<std::int32_t>(entityId));
        }
    }

    const auto& targets = world.moveTargets();
    const auto& transforms = world.transforms();
    const auto& buildings = world.buildings();

    const bool useTilemap = tilemap.width > 0 && tilemap.height > 0;
    const path::GridBounds pathBounds{
        useTilemap ? 0 : -32,
        useTilemap ? tilemap.width - 1 : 32,
        useTilemap ? 0 : -32,
        useTilemap ? tilemap.height - 1 : 32,
    };

    std::vector<EntityId> clearTargets;
    clearTargets.reserve(targets.size());

    for (const auto& [entityId, target] : targets) {
        if (buildings.find(entityId) != buildings.end()) {
            world.setVelocity(entityId, Velocity{});
            clearTargets.push_back(entityId);
            continue;
        }

        const auto trIt = transforms.find(entityId);
        if (trIt == transforms.end()) {
            clearTargets.push_back(entityId);
            continue;
        }

        const auto start = toGridCoord(trIt->second);
        const path::GridCoord goal{target.x, target.y};

        if (start == goal) {
            world.setVelocity(entityId, Velocity{});
            clearTargets.push_back(entityId);
            continue;
        }

        std::set<path::GridCoord> blocked;
        if (useTilemap) {
            for (std::int32_t gy = pathBounds.minY; gy <= pathBounds.maxY; ++gy) {
                for (std::int32_t gx = pathBounds.minX; gx <= pathBounds.maxX; ++gx) {
                    if (!tilemap.isWalkable(gx, gy)) {
                        blocked.insert({gx, gy});
                    }
                }
            }
        } else {
            for (const auto& [blockEntity, marker] : buildings) {
                (void)marker;
                if (blockEntity == entityId) {
                    continue;
                }
                const auto blockTrIt = transforms.find(blockEntity);
                if (blockTrIt == transforms.end()) {
                    continue;
                }
                blocked.insert(toGridCoord(blockTrIt->second));
            }
        }

        const auto path = path::findPathAStar(start, goal, blocked, pathBounds);
        world.addPathRequest();
        if (path.size() < 2U) {
            world.setVelocity(entityId, Velocity{});
            continue;
        }

        const auto next = path[1];
        Velocity velocity{};
        velocity.xPerTick = math::FixedPoint::fromInt(next.x - start.x);
        velocity.yPerTick = math::FixedPoint::fromInt(next.y - start.y);
        world.setVelocity(entityId, velocity);
    }

    for (const auto entityId : clearTargets) {
        world.clearMoveTarget(entityId);
    }
}

void runMovementPhase(World& world, std::int64_t tick) {
    (void)tick;
    auto& transforms = world.mutableTransforms();
    const auto& velocities = world.velocities();
    auto& frozenStates = world.mutableFrozenStates();

    for (auto it = frozenStates.begin(); it != frozenStates.end();) {
        if (it->second.remainingTicks > 0) {
            --it->second.remainingTicks;
        }
        if (it->second.remainingTicks <= 0) {
            it = frozenStates.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& [entityId, transform] : transforms) {
        const auto velIt = velocities.find(entityId);
        if (velIt == velocities.end()) {
            continue;
        }
        const auto frozenIt = frozenStates.find(entityId);
        if (frozenIt != frozenStates.end() && frozenIt->second.remainingTicks > 0 && frozenIt->second.slowPermille >= 1000) {
            continue;
        }
        transform.x += velIt->second.xPerTick;
        transform.y += velIt->second.yPerTick;
    }
}

void runCombatPhase(World& world, std::int64_t tick) {
    (void)tick;
    updatePowerConsumerStates(world);

    {
        auto& projectiles = world.mutableBallisticProjectiles();
        std::vector<EntityId> detonated;
        detonated.reserve(projectiles.size());

        for (auto& [projectileId, projectile] : projectiles) {
            if (projectile.remainingTicks > 0) {
                --projectile.remainingTicks;
            }

            if (projectile.remainingTicks > 0) {
                continue;
            }

            applyButterAoeAt(
                world,
                projectile.sourceTeam,
                projectile.targetX,
                projectile.targetY,
                projectile.aoeRadius,
                projectile.damage,
                projectile.frozenTicks);
            detonated.push_back(projectileId);
        }

        for (const auto projectileId : detonated) {
            world.destroyEntity(projectileId);
        }
    }

    auto& weapons = world.mutableWeapons();
    auto& artilleryWeapons = world.mutableArtilleryWeapons();
    auto& healths = world.mutableHealths();
    const auto& teams = world.teams();
    const auto& transforms = world.transforms();
    const auto& attackTargets = world.attackTargets();
    const auto& consumers = world.powerConsumers();

    for (auto& [attackerId, weapon] : weapons) {
        const auto powerIt = consumers.find(attackerId);
        const bool attackerEnabled = (powerIt == consumers.end()) || powerIt->second.enabled;
        if (attackerEnabled && weapon.remainingCooldownTicks > 0) {
            --weapon.remainingCooldownTicks;
        }
        if (!attackerEnabled) {
            continue;
        }

        const auto atkTeamIt = teams.find(attackerId);
        const auto atkPosIt = transforms.find(attackerId);
        if (atkTeamIt == teams.end() || atkPosIt == transforms.end()) {
            continue;
        }

        const auto targetIt = attackTargets.find(attackerId);
        if (targetIt == attackTargets.end()) {
            continue;
        }

        const EntityId targetId = targetIt->second;
        const auto targetTeamIt = teams.find(targetId);
        const auto targetPosIt = transforms.find(targetId);
        auto targetHealthIt = healths.find(targetId);
        if (targetTeamIt == teams.end() || targetPosIt == transforms.end() || targetHealthIt == healths.end()) {
            world.clearAttackTarget(attackerId);
            continue;
        }

        if (targetTeamIt->second.value == atkTeamIt->second.value) {
            world.clearAttackTarget(attackerId);
            continue;
        }

        if (targetHealthIt->second.current <= 0) {
            world.clearAttackTarget(attackerId);
            continue;
        }

        const auto rangeSq = weapon.range * weapon.range;
        if (distanceSquared(atkPosIt->second, targetPosIt->second) > rangeSq) {
            continue;
        }

        if (weapon.remainingCooldownTicks > 0) {
            continue;
        }

        targetHealthIt->second.current -= mitigatedDamage(world, targetId, weapon.damage);
        weapon.remainingCooldownTicks = std::max(0, weapon.cooldownTicks);
    }

    for (auto& [attackerId, weapon] : artilleryWeapons) {
        const auto powerIt = consumers.find(attackerId);
        const bool attackerEnabled = (powerIt == consumers.end()) || powerIt->second.enabled;
        if (attackerEnabled && weapon.remainingCooldownTicks > 0) {
            --weapon.remainingCooldownTicks;
        }
        if (!attackerEnabled) {
            continue;
        }

        const auto atkTeamIt = teams.find(attackerId);
        const auto atkPosIt = transforms.find(attackerId);
        if (atkTeamIt == teams.end() || atkPosIt == transforms.end()) {
            continue;
        }

        const auto targetIt = attackTargets.find(attackerId);
        if (targetIt == attackTargets.end()) {
            continue;
        }

        const EntityId targetId = targetIt->second;
        const auto targetTeamIt = teams.find(targetId);
        const auto targetPosIt = transforms.find(targetId);
        auto targetHealthIt = healths.find(targetId);
        if (targetTeamIt == teams.end() || targetPosIt == transforms.end() || targetHealthIt == healths.end()) {
            world.clearAttackTarget(attackerId);
            continue;
        }

        if (targetTeamIt->second.value == atkTeamIt->second.value) {
            world.clearAttackTarget(attackerId);
            continue;
        }

        if (targetHealthIt->second.current <= 0) {
            world.clearAttackTarget(attackerId);
            continue;
        }

        const auto distSq = distanceSquared(atkPosIt->second, targetPosIt->second);
        const auto minRangeSq = weapon.minRange * weapon.minRange;
        const auto maxRangeSq = weapon.maxRange * weapon.maxRange;
        if (distSq < minRangeSq || distSq > maxRangeSq) {
            continue;
        }

        if (weapon.remainingCooldownTicks > 0) {
            continue;
        }

        const auto projectileEntity = world.createEntity();
        world.setTeam(projectileEntity, Team{atkTeamIt->second.value});
        world.setTransform(projectileEntity, atkPosIt->second);
        world.setBallisticProjectile(projectileEntity,
                                     BallisticProjectile{
                                         atkTeamIt->second.value,
                                         weapon.damage,
                                         weapon.aoeRadius,
                                         weapon.frozenTicks,
                                         std::max(1, weapon.projectileTravelTicks),
                                         targetPosIt->second.x,
                                         targetPosIt->second.y,
                                     });

        weapon.remainingCooldownTicks = std::max(0, weapon.reloadTicks);
    }
}

void runResourcePhase(World& world, std::int64_t tick) {
    (void)tick;
    const auto& producers = world.sunProducers();
    const auto& teams = world.teams();
    for (const auto& [entityId, producer] : producers) {
        const auto teamIt = teams.find(entityId);
        if (teamIt == teams.end()) {
            continue;
        }
        world.addSunForTeam(teamIt->second.value, producer.amountPerTick);
    }
}

void runCleanupPhase(World& world, std::int64_t tick) {
    (void)tick;
    std::vector<EntityId> toDestroy;
    const auto& healths = world.healths();
    toDestroy.reserve(healths.size());
    for (const auto& [entityId, health] : healths) {
        if (health.current <= 0) {
            toDestroy.push_back(entityId);
        }
    }

    for (const auto entityId : toDestroy) {
        world.destroyEntity(entityId);
    }

    std::set<std::uint8_t> aliveHeadquarterTeams;
    const auto& hqs = world.headquarters();
    const auto& teams = world.teams();
    const auto& healthsAfter = world.healths();
    for (const auto& [entityId, marker] : hqs) {
        if (!marker.value) {
            continue;
        }

        const auto teamIt = teams.find(entityId);
        const auto hpIt = healthsAfter.find(entityId);
        if (teamIt == teams.end() || hpIt == healthsAfter.end()) {
            continue;
        }

        if (hpIt->second.current > 0) {
            aliveHeadquarterTeams.insert(teamIt->second.value);
        }
    }

    if (aliveHeadquarterTeams.size() == 1U) {
        world.setWinnerTeam(static_cast<std::int32_t>(*aliveHeadquarterTeams.begin()));
    } else {
        world.clearWinnerTeam();
    }
}

void runFogOfWarSystem(World& world, std::int64_t tick) {
    (void)tick;
    auto& tilemap = world.mutableTilemap();
    if (tilemap.width <= 0 || tilemap.height <= 0) {
        return;
    }

    const auto& visions = world.visions();
    const auto& teams = world.teams();
    const auto& transforms = world.transforms();

    // Collect unique team IDs from all existing teams
    std::set<std::uint8_t> teamIds;
    for (const auto& [entityId, team] : teams) {
        (void)entityId;
        teamIds.insert(team.value);
    }

    // Advance fog for each known team
    for (const auto teamId : teamIds) {
        world.fogOfWarForTeam(teamId).advanceTick();
    }

    // Reveal from vision entities
    for (const auto& [entityId, vision] : visions) {
        const auto teamIt = teams.find(entityId);
        const auto trIt = transforms.find(entityId);
        if (teamIt == teams.end() || trIt == transforms.end()) {
            continue;
        }
        auto& fog = world.fogOfWarForTeam(teamIt->second.value);
        const auto cell = toGridCoord(trIt->second);
        fog.reveal(cell.x, cell.y, vision.radiusCells);
    }
}

void registerCoreSystems(World& world) {
    world.registerSystem(SystemPhase::kInput, runInputPhase);
    world.registerSystem(SystemPhase::kProduction, runProductionPhase);
    world.registerSystem(SystemPhase::kPathfinding, runPathfindingPhase);
    world.registerSystem(SystemPhase::kMovement, runMovementPhase);
    world.registerSystem(SystemPhase::kCombat, runCombatPhase);
    world.registerSystem(SystemPhase::kResource, runResourcePhase);
    world.registerSystem(SystemPhase::kCleanup, runCleanupPhase);
    world.registerSystem(SystemPhase::kResource, runFogOfWarSystem);
}

}  // namespace tcp::logic::ecs
