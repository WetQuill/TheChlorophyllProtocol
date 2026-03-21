#include "StateHasher.h"

#include <cstdint>

namespace tcp::logic::debug {

namespace {

constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

void mix(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value;
    hash *= kFnvPrime;
}

std::uint64_t asU64(std::int64_t value) noexcept {
    return static_cast<std::uint64_t>(value);
}

}  // namespace

std::uint64_t hashWorldState(const ecs::World& world) noexcept {
    std::uint64_t hash = kFnvOffset;

    mix(hash, asU64(world.currentTick()));
    mix(hash, static_cast<std::uint64_t>(world.winnerTeam()));

    for (const auto& [entityId, team] : world.teams()) {
        mix(hash, entityId);
        mix(hash, team.value);
        mix(hash, static_cast<std::uint64_t>(world.sunForTeam(team.value)));
        mix(hash, static_cast<std::uint64_t>(world.powerForTeam(team.value)));
    }

    for (const auto& [entityId, transform] : world.transforms()) {
        mix(hash, entityId);
        mix(hash, static_cast<std::uint64_t>(transform.x.raw()));
        mix(hash, static_cast<std::uint64_t>(transform.y.raw()));
    }

    for (const auto& [entityId, velocity] : world.velocities()) {
        mix(hash, entityId);
        mix(hash, static_cast<std::uint64_t>(velocity.xPerTick.raw()));
        mix(hash, static_cast<std::uint64_t>(velocity.yPerTick.raw()));
    }

    for (const auto& [entityId, health] : world.healths()) {
        mix(hash, entityId);
        mix(hash, static_cast<std::uint64_t>(health.current));
        mix(hash, static_cast<std::uint64_t>(health.max));
    }

    for (const auto& [entityId, production] : world.productions()) {
        mix(hash, entityId);
        mix(hash, static_cast<std::uint64_t>(production.costSun));
        mix(hash, static_cast<std::uint64_t>(production.buildTicks));
        mix(hash, static_cast<std::uint64_t>(production.progressTicks));
        mix(hash, production.producedArchetypeId);
        mix(hash, static_cast<std::uint64_t>(production.producedHealth));
    }

    for (const auto& [entityId, weapon] : world.weapons()) {
        mix(hash, entityId);
        mix(hash, static_cast<std::uint64_t>(weapon.range.raw()));
        mix(hash, static_cast<std::uint64_t>(weapon.damage));
        mix(hash, static_cast<std::uint64_t>(weapon.cooldownTicks));
        mix(hash, static_cast<std::uint64_t>(weapon.remainingCooldownTicks));
    }

    for (const auto& [entityId, queue] : world.commandBuffers()) {
        mix(hash, entityId);
        mix(hash, static_cast<std::uint64_t>(queue.queued.size()));
    }

    for (const auto& [entityId, targetId] : world.attackTargets()) {
        mix(hash, entityId);
        mix(hash, targetId);
    }

    return hash;
}

void updateDeterminismSnapshot(ecs::World& world) noexcept {
    if (!world.determinismDebugEnabled()) {
        return;
    }

    world.setLastStateHash(hashWorldState(world));
}

}  // namespace tcp::logic::debug
