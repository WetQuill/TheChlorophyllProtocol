#include "ZombieAISystem.h"

#include "../components/Components.h"
#include "../../debug/DebugLog.h"
#include "../../path/AStarGrid.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace tcp::logic::ecs {

namespace {

constexpr std::int32_t kPathUpdateInterval = 30;
constexpr std::int32_t kMeleeRangeCells = 1;

[[nodiscard]] path::GridCoord toGridCoord(const Transform& transform) noexcept {
    return {
        transform.x.toIntTrunc(),
        transform.y.toIntTrunc(),
    };
}

}  // namespace

void runZombieAISystem(World& world, std::int64_t currentTick) {
    auto& aiComponents = world.mutableZombieAIFSMs();
    if (aiComponents.empty()) {
        return;
    }

    const auto& transforms = world.transforms();
    const auto& teams = world.teams();
    const auto& healths = world.healths();
    const auto& headquarters = world.headquarters();
    // Find player HQ position once per tick
    bool hqFound = false;
    path::GridCoord hqGrid{};
    for (const auto& [hqEntityId, hq] : headquarters) {
        (void)hq;
        auto teamIt = teams.find(hqEntityId);
        if (teamIt == teams.end() || teamIt->second.value != 0U) {
            continue;
        }
        auto trIt = transforms.find(hqEntityId);
        if (trIt == transforms.end()) {
            continue;
        }
        auto hpIt = healths.find(hqEntityId);
        if (hpIt == healths.end() || hpIt->second.current <= 0) {
            continue;
        }
        hqGrid = toGridCoord(trIt->second);
        hqFound = true;
        break;
    }

    for (auto& [entityId, fsm] : aiComponents) {
        // Get zombie's own team and position
        auto teamIt = teams.find(entityId);
        if (teamIt == teams.end()) {
            continue;
        }
        const auto zombieTeam = teamIt->second.value;

        auto trIt = transforms.find(entityId);
        if (trIt == transforms.end()) {
            continue;
        }
        const auto zombieGrid = toGridCoord(trIt->second);

        switch (fsm.currentState) {
        case ZombieState::IDLE: {
            if (!hqFound) {
                break;
            }
            fsm.currentState = ZombieState::MOVE_TO_HQ;
            fsm.pathUpdateTimer = kPathUpdateInterval;
            world.setMoveTarget(entityId, GridTarget{hqGrid.x, hqGrid.y});
            TCP_DEBUG("ZOMBIE", currentTick, "entity %u IDLE -> MOVE_TO_HQ, HQ at (%d,%d)",
                      entityId, hqGrid.x, hqGrid.y);
            break;
        }

        case ZombieState::MOVE_TO_HQ: {
            if (!hqFound) {
                world.clearMoveTarget(entityId);
                fsm.currentState = ZombieState::IDLE;
                break;
            }

            // Decrement path update timer
            if (fsm.pathUpdateTimer > 0) {
                --fsm.pathUpdateTimer;
            }

            // Update move target periodically
            if (fsm.pathUpdateTimer == 0) {
                world.setMoveTarget(entityId, GridTarget{hqGrid.x, hqGrid.y});
                fsm.pathUpdateTimer = kPathUpdateInterval;
            }

            // Scan for enemy entities within melee range
            std::uint32_t bestTarget = 0;
            std::int32_t bestDist = kMeleeRangeCells + 1;

            for (const auto& [targetId, targetHealth] : healths) {
                if (targetId == entityId) {
                    continue;
                }
                if (targetHealth.current <= 0) {
                    continue;
                }

                auto targetTeamIt = teams.find(targetId);
                if (targetTeamIt == teams.end()) {
                    continue;
                }
                if (targetTeamIt->second.value == zombieTeam) {
                    continue;
                }

                auto targetTrIt = transforms.find(targetId);
                if (targetTrIt == transforms.end()) {
                    continue;
                }

                const auto targetGrid = toGridCoord(targetTrIt->second);
                const std::int32_t dist = std::abs(zombieGrid.x - targetGrid.x)
                                        + std::abs(zombieGrid.y - targetGrid.y);

                if (dist <= kMeleeRangeCells && dist < bestDist) {
                    bestDist = dist;
                    bestTarget = targetId;
                }
            }

            if (bestTarget != 0) {
                world.clearMoveTarget(entityId);
                world.setAttackTarget(entityId, bestTarget);
                world.setVelocity(entityId, Velocity{});
                fsm.targetPlant = bestTarget;
                fsm.currentState = ZombieState::ATTACK_OBSTACLE;
                TCP_DEBUG("ZOMBIE", currentTick, "entity %u MOVE_TO_HQ -> ATTACK_OBSTACLE, target=%u dist=%d",
                          entityId, bestTarget, bestDist);
            }
            break;
        }

        case ZombieState::ATTACK_OBSTACLE: {
            // Verify target is still alive (transforms lookup suffices — dead entities lose all components)
            bool targetValid = false;
            if (fsm.targetPlant != 0) {
                auto hpIt = healths.find(fsm.targetPlant);
                if (hpIt != healths.end() && hpIt->second.current > 0) {
                    targetValid = true;
                }
            }

            if (!targetValid) {
                world.clearAttackTarget(entityId);
                TCP_DEBUG("ZOMBIE", currentTick, "entity %u ATTACK_OBSTACLE -> MOVE_TO_HQ, target %u lost",
                          entityId, fsm.targetPlant);
                fsm.targetPlant = 0;
                fsm.currentState = ZombieState::MOVE_TO_HQ;
                fsm.pathUpdateTimer = kPathUpdateInterval;
                if (hqFound) {
                    world.setMoveTarget(entityId, GridTarget{hqGrid.x, hqGrid.y});
                }
                break;
            }

            // Ensure attack target is set and velocity is zero
            world.setAttackTarget(entityId, fsm.targetPlant);
            world.setVelocity(entityId, Velocity{});
            break;
        }
        }  // switch
    }
}

}  // namespace tcp::logic::ecs
