#include "World.h"

#include <algorithm>
#include <utility>

namespace tcp::logic::ecs {

EntityId World::createEntity() {
    const auto entityId = nextEntityId_;
    ++nextEntityId_;
    entities_.push_back(entityId);
    return entityId;
}

bool World::destroyEntity(EntityId entityId) {
    const auto it = std::find(entities_.begin(), entities_.end(), entityId);
    if (it == entities_.end()) {
        return false;
    }

    entities_.erase(it);
    removeComponents(entityId);
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
    for (const auto phase : orderedSystemPhases()) {
        auto& phaseSystems = systems_[systemPhaseIndex(phase)];
        for (const auto& callback : phaseSystems) {
            if (callback) {
                callback(*this, currentTick_);
            }
        }
    }
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
}

}  // namespace tcp::logic::ecs
