#include "BuiltInSystems.h"

#include <algorithm>
#include <cstdint>
#include <set>
#include <vector>

namespace tcp::logic::ecs {

namespace {

[[nodiscard]] math::FixedPoint distanceSquared(const Transform& a, const Transform& b) noexcept {
    const auto dx = a.x - b.x;
    const auto dy = a.y - b.y;
    return (dx * dx) + (dy * dy);
}

}  // namespace

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
    (void)world;
    (void)tick;
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
