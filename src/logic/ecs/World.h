#pragma once

#include "components/Components.h"
#include "systems/SystemPipeline.h"

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

namespace tcp::logic::ecs {

using EntityId = std::uint32_t;

using SystemCallback = std::function<void(class World&, std::int64_t)>;

class World final {
public:
    World() = default;

    [[nodiscard]] EntityId createEntity();
    bool destroyEntity(EntityId entityId);

    [[nodiscard]] std::int64_t currentTick() const noexcept;
    [[nodiscard]] std::size_t entityCount() const noexcept;
    [[nodiscard]] const std::vector<EntityId>& entities() const noexcept;

    void registerSystem(SystemPhase phase, SystemCallback callback);
    void tick();

    bool setTransform(EntityId entityId, const Transform& component);
    bool setVelocity(EntityId entityId, const Velocity& component);
    bool setHealth(EntityId entityId, const Health& component);
    bool setTeam(EntityId entityId, const Team& component);
    bool setIdentity(EntityId entityId, const Identity& component);
    bool setProduction(EntityId entityId, const Production& component);
    bool setWeapon(EntityId entityId, const Weapon& component);
    bool setVision(EntityId entityId, const Vision& component);
    bool setPowerConsumer(EntityId entityId, const PowerConsumer& component);
    bool setCommandBuffer(EntityId entityId, const CommandBuffer& component);

    [[nodiscard]] const std::map<EntityId, Transform>& transforms() const noexcept;
    [[nodiscard]] const std::map<EntityId, Velocity>& velocities() const noexcept;
    [[nodiscard]] const std::map<EntityId, Health>& healths() const noexcept;
    [[nodiscard]] const std::map<EntityId, Team>& teams() const noexcept;
    [[nodiscard]] const std::map<EntityId, Identity>& identities() const noexcept;
    [[nodiscard]] const std::map<EntityId, Production>& productions() const noexcept;
    [[nodiscard]] const std::map<EntityId, Weapon>& weapons() const noexcept;
    [[nodiscard]] const std::map<EntityId, Vision>& visions() const noexcept;
    [[nodiscard]] const std::map<EntityId, PowerConsumer>& powerConsumers() const noexcept;
    [[nodiscard]] const std::map<EntityId, CommandBuffer>& commandBuffers() const noexcept;

private:
    [[nodiscard]] bool hasEntity(EntityId entityId) const noexcept;
    void removeComponents(EntityId entityId);

    EntityId nextEntityId_{1};
    std::int64_t currentTick_{0};
    std::vector<EntityId> entities_{};
    std::array<std::vector<SystemCallback>, systemPhaseCount()> systems_{};

    std::map<EntityId, Transform> transforms_{};
    std::map<EntityId, Velocity> velocities_{};
    std::map<EntityId, Health> healths_{};
    std::map<EntityId, Team> teams_{};
    std::map<EntityId, Identity> identities_{};
    std::map<EntityId, Production> productions_{};
    std::map<EntityId, Weapon> weapons_{};
    std::map<EntityId, Vision> visions_{};
    std::map<EntityId, PowerConsumer> powerConsumers_{};
    std::map<EntityId, CommandBuffer> commandBuffers_{};
};

}  // namespace tcp::logic::ecs
