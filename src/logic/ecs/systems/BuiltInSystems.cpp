#include "BuiltInSystems.h"

#include "../../path/AStarGrid.h"

#include <algorithm>
#include <cstdint>
#include <set>
#include <vector>

namespace tcp::logic::ecs {

namespace {

constexpr std::int32_t kBuildRadiusCells = 4;
constexpr std::int32_t kDefaultBuildCostSun = 20;
constexpr std::int32_t kBuildingHealth = 20;
constexpr std::uint32_t kPeaMilitaryCampArchetypeId = 901U;

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

void tryHandleBuildCommand(World& world, EntityId issuerId, const QueuedCommand& cmd) {
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

    const auto cost = (cmd.arg2 > 0) ? cmd.arg2 : kDefaultBuildCostSun;
    if (!world.spendSunForTeam(teamIt->second.value, cost)) {
        return;
    }

    const auto buildingEntity = world.createEntity();
    world.setTeam(buildingEntity, Team{teamIt->second.value});
    world.setTransform(buildingEntity, toTransform(buildCell));
    world.setHealth(buildingEntity, Health{kBuildingHealth, kBuildingHealth});
    world.setBuilding(buildingEntity, Building{true});
    world.setIdentity(buildingEntity, Identity{kPeaMilitaryCampArchetypeId, 1});
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
            } else if (cmd.type == CommandType::kBuild) {
                tryHandleBuildCommand(world, entityId, cmd);
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

    const auto& targets = world.moveTargets();
    const auto& transforms = world.transforms();
    const auto& buildings = world.buildings();

    std::vector<EntityId> clearTargets;
    clearTargets.reserve(targets.size());

    for (const auto& [entityId, target] : targets) {
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

        const auto path = path::findPathAStar(start, goal, blocked, path::GridBounds{-32, 32, -32, 32});
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
    for (auto& [entityId, transform] : transforms) {
        const auto velIt = velocities.find(entityId);
        if (velIt == velocities.end()) {
            continue;
        }
        transform.x += velIt->second.xPerTick;
        transform.y += velIt->second.yPerTick;
    }
}

void runCombatPhase(World& world, std::int64_t tick) {
    (void)tick;
    auto& weapons = world.mutableWeapons();
    auto& healths = world.mutableHealths();
    const auto& teams = world.teams();
    const auto& transforms = world.transforms();

    for (auto& [attackerId, weapon] : weapons) {
        if (weapon.remainingCooldownTicks > 0) {
            --weapon.remainingCooldownTicks;
            continue;
        }

        const auto atkTeamIt = teams.find(attackerId);
        const auto atkPosIt = transforms.find(attackerId);
        if (atkTeamIt == teams.end() || atkPosIt == transforms.end()) {
            continue;
        }

        const auto rangeSq = weapon.range * weapon.range;
        EntityId targetId = 0;

        for (const auto& [candidateId, candidateHealth] : healths) {
            (void)candidateHealth;
            if (candidateId == attackerId) {
                continue;
            }

            const auto targetTeamIt = teams.find(candidateId);
            const auto targetPosIt = transforms.find(candidateId);
            if (targetTeamIt == teams.end() || targetPosIt == transforms.end()) {
                continue;
            }

            if (targetTeamIt->second.value == atkTeamIt->second.value) {
                continue;
            }

            if (distanceSquared(atkPosIt->second, targetPosIt->second) <= rangeSq) {
                targetId = candidateId;
                break;
            }
        }

        if (targetId == 0) {
            continue;
        }

        auto targetHealthIt = healths.find(targetId);
        if (targetHealthIt == healths.end()) {
            continue;
        }

        targetHealthIt->second.current -= weapon.damage;
        weapon.remainingCooldownTicks = std::max(0, weapon.cooldownTicks);
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

void registerCoreSystems(World& world) {
    world.registerSystem(SystemPhase::kInput, runInputPhase);
    world.registerSystem(SystemPhase::kProduction, runProductionPhase);
    world.registerSystem(SystemPhase::kPathfinding, runPathfindingPhase);
    world.registerSystem(SystemPhase::kMovement, runMovementPhase);
    world.registerSystem(SystemPhase::kCombat, runCombatPhase);
    world.registerSystem(SystemPhase::kResource, runResourcePhase);
    world.registerSystem(SystemPhase::kCleanup, runCleanupPhase);
}

}  // namespace tcp::logic::ecs
