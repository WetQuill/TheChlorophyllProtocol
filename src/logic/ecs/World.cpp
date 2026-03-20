#include "World.h"

#include <algorithm>
#include <utility>

namespace tcp::logic::ecs {

EntityId World::createEntity() {
    const auto entityId = nextEntityId_;
    ++nextEntityId_;
    entities_.push_back(entityId);
    ++telemetry_.spawnedEntities;
    return entityId;
}

bool World::destroyEntity(EntityId entityId) {
    const auto it = std::find(entities_.begin(), entities_.end(), entityId);
    if (it == entities_.end()) {
        return false;
    }

    entities_.erase(it);
    removeComponents(entityId);
    ++telemetry_.destroyedEntities;
    return true;
}

std::int64_t World::currentTick() const noexcept {
    return currentTick_;
}

std::size_t World::entityCount() const noexcept {
    return entities_.size();
}

const std::vector<EntityId>& World::entities() const noexcept {
    return entities_;
}

void World::registerSystem(SystemPhase phase, SystemCallback callback) {
    systems_[systemPhaseIndex(phase)].push_back(std::move(callback));
}

void World::tick() {
    telemetry_.tickIndex = currentTick_;
    telemetry_.pathRequests = 0;
    telemetry_.commandsProcessed = 0;
    telemetry_.spawnedEntities = 0;
    telemetry_.destroyedEntities = 0;

    for (const auto phase : orderedSystemPhases()) {
        auto& phaseSystems = systems_[systemPhaseIndex(phase)];
        for (const auto& callback : phaseSystems) {
            if (callback) {
                callback(*this, currentTick_);
            }
        }
    }

    telemetry_.entityCount = static_cast<std::int32_t>(entities_.size());
    ++currentTick_;
}

bool World::setTransform(EntityId entityId, const Transform& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    transforms_[entityId] = component;
    return true;
}

bool World::setVelocity(EntityId entityId, const Velocity& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    velocities_[entityId] = component;
    return true;
}

bool World::setHealth(EntityId entityId, const Health& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    healths_[entityId] = component;
    return true;
}

bool World::setTeam(EntityId entityId, const Team& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    teams_[entityId] = component;
    return true;
}

bool World::setIdentity(EntityId entityId, const Identity& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    identities_[entityId] = component;
    return true;
}

bool World::setProduction(EntityId entityId, const Production& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    productions_[entityId] = component;
    return true;
}

bool World::setWeapon(EntityId entityId, const Weapon& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    weapons_[entityId] = component;
    return true;
}

bool World::setVision(EntityId entityId, const Vision& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    visions_[entityId] = component;
    return true;
}

bool World::setPowerConsumer(EntityId entityId, const PowerConsumer& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    powerConsumers_[entityId] = component;
    return true;
}

bool World::setCommandBuffer(EntityId entityId, const CommandBuffer& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    commandBuffers_[entityId] = component;
    return true;
}

bool World::setSunProducer(EntityId entityId, const SunProducer& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    sunProducers_[entityId] = component;
    return true;
}

bool World::setHeadquarters(EntityId entityId, const Headquarters& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    headquarters_[entityId] = component;
    return true;
}

bool World::setBuilding(EntityId entityId, const Building& component) {
    if (!hasEntity(entityId)) {
        return false;
    }
    buildings_[entityId] = component;
    return true;
}

const std::map<EntityId, Transform>& World::transforms() const noexcept {
    return transforms_;
}

const std::map<EntityId, Velocity>& World::velocities() const noexcept {
    return velocities_;
}

const std::map<EntityId, Health>& World::healths() const noexcept {
    return healths_;
}

const std::map<EntityId, Team>& World::teams() const noexcept {
    return teams_;
}

const std::map<EntityId, Identity>& World::identities() const noexcept {
    return identities_;
}

const std::map<EntityId, Production>& World::productions() const noexcept {
    return productions_;
}

const std::map<EntityId, Weapon>& World::weapons() const noexcept {
    return weapons_;
}

const std::map<EntityId, Vision>& World::visions() const noexcept {
    return visions_;
}

const std::map<EntityId, PowerConsumer>& World::powerConsumers() const noexcept {
    return powerConsumers_;
}

const std::map<EntityId, CommandBuffer>& World::commandBuffers() const noexcept {
    return commandBuffers_;
}

const std::map<EntityId, SunProducer>& World::sunProducers() const noexcept {
    return sunProducers_;
}

const std::map<EntityId, Headquarters>& World::headquarters() const noexcept {
    return headquarters_;
}

const std::map<EntityId, Building>& World::buildings() const noexcept {
    return buildings_;
}

std::map<EntityId, Transform>& World::mutableTransforms() noexcept {
    return transforms_;
}

std::map<EntityId, Velocity>& World::mutableVelocities() noexcept {
    return velocities_;
}

std::map<EntityId, Health>& World::mutableHealths() noexcept {
    return healths_;
}

std::map<EntityId, CommandBuffer>& World::mutableCommandBuffers() noexcept {
    return commandBuffers_;
}

std::map<EntityId, Weapon>& World::mutableWeapons() noexcept {
    return weapons_;
}

std::int32_t World::sunForTeam(std::uint8_t teamId) const noexcept {
    const auto it = teamSun_.find(teamId);
    if (it == teamSun_.end()) {
        return 0;
    }
    return it->second;
}

void World::setSunForTeam(std::uint8_t teamId, std::int32_t value) noexcept {
    teamSun_[teamId] = value;
}

void World::addSunForTeam(std::uint8_t teamId, std::int32_t delta) noexcept {
    teamSun_[teamId] = sunForTeam(teamId) + delta;
}

bool World::spendSunForTeam(std::uint8_t teamId, std::int32_t amount) noexcept {
    if (amount <= 0) {
        return true;
    }

    const auto available = sunForTeam(teamId);
    if (available < amount) {
        return false;
    }

    teamSun_[teamId] = available - amount;
    return true;
}

std::int32_t World::powerForTeam(std::uint8_t teamId) const noexcept {
    const auto it = teamPower_.find(teamId);
    if (it == teamPower_.end()) {
        return 0;
    }
    return it->second;
}

void World::setPowerForTeam(std::uint8_t teamId, std::int32_t value) noexcept {
    teamPower_[teamId] = value;
}

void World::addPowerForTeam(std::uint8_t teamId, std::int32_t delta) noexcept {
    teamPower_[teamId] = powerForTeam(teamId) + delta;
}

bool World::spendPowerForTeam(std::uint8_t teamId, std::int32_t amount) noexcept {
    if (amount <= 0) {
        return true;
    }

    const auto available = powerForTeam(teamId);
    if (available < amount) {
        return false;
    }

    teamPower_[teamId] = available - amount;
    return true;
}

std::int32_t World::winnerTeam() const noexcept {
    return winnerTeam_;
}

void World::setWinnerTeam(std::int32_t teamId) noexcept {
    winnerTeam_ = teamId;
}

void World::clearWinnerTeam() noexcept {
    winnerTeam_ = -1;
}

void World::setMoveTarget(EntityId entityId, GridTarget target) noexcept {
    moveTargets_[entityId] = target;
}

void World::clearMoveTarget(EntityId entityId) noexcept {
    moveTargets_.erase(entityId);
}

const std::map<EntityId, GridTarget>& World::moveTargets() const noexcept {
    return moveTargets_;
}

void World::setTickDurationMicros(std::int64_t micros) noexcept {
    telemetry_.tickDurationMicros = (micros < 0) ? 0 : micros;
}

void World::addPathRequest() noexcept {
    ++telemetry_.pathRequests;
}

void World::addCommandProcessed() noexcept {
    ++telemetry_.commandsProcessed;
}

const TickTelemetry& World::telemetry() const noexcept {
    return telemetry_;
}

void World::setDeterminismDebugEnabled(bool enabled) noexcept {
    determinismDebugEnabled_ = enabled;
}

bool World::determinismDebugEnabled() const noexcept {
    return determinismDebugEnabled_;
}

void World::setLastStateHash(std::uint64_t hash) noexcept {
    lastStateHash_ = hash;
}

std::uint64_t World::lastStateHash() const noexcept {
    return lastStateHash_;
}

bool World::hasEntity(EntityId entityId) const noexcept {
    return std::find(entities_.begin(), entities_.end(), entityId) != entities_.end();
}

void World::removeComponents(EntityId entityId) {
    transforms_.erase(entityId);
    velocities_.erase(entityId);
    healths_.erase(entityId);
    teams_.erase(entityId);
    identities_.erase(entityId);
    productions_.erase(entityId);
    weapons_.erase(entityId);
    visions_.erase(entityId);
    powerConsumers_.erase(entityId);
    commandBuffers_.erase(entityId);
    sunProducers_.erase(entityId);
    headquarters_.erase(entityId);
    buildings_.erase(entityId);
    moveTargets_.erase(entityId);
}

}  // namespace tcp::logic::ecs
