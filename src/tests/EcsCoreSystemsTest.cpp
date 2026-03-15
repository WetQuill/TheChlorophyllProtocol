#include "../logic/ecs/World.h"
#include "../logic/ecs/systems/BuiltInSystems.h"

#include <cstdint>
#include <iostream>

namespace {

bool verify(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

}  // namespace

int main() {
    bool ok = true;

    tcp::logic::ecs::World world;
    tcp::logic::ecs::registerCoreSystems(world);

    const auto entity = world.createEntity();

    tcp::logic::ecs::Transform transform{};
    transform.x = tcp::logic::math::FixedPoint::fromInt(0);
    transform.y = tcp::logic::math::FixedPoint::fromInt(0);
    ok &= verify(world.setTransform(entity, transform), "set transform failed");

    tcp::logic::ecs::Velocity velocity{};
    velocity.xPerTick = tcp::logic::math::FixedPoint::fromInt(2);
    velocity.yPerTick = tcp::logic::math::FixedPoint::fromInt(-1);
    ok &= verify(world.setVelocity(entity, velocity), "set velocity failed");

    tcp::logic::ecs::Health health{};
    health.current = 10;
    health.max = 10;
    ok &= verify(world.setHealth(entity, health), "set health failed");

    tcp::logic::ecs::CommandBuffer commands{};
    commands.queued.push_back({0, 0, tcp::logic::ecs::CommandType::kStop, 1, 2, 3});
    commands.queued.push_back({2, 0, tcp::logic::ecs::CommandType::kStop, 4, 5, 6});
    ok &= verify(world.setCommandBuffer(entity, commands), "set command buffer failed");

    world.tick();

    const auto transformAfterTick1 = world.transforms().at(entity);
    const auto expectedX = tcp::logic::math::FixedPoint::fromInt(2).raw();
    const auto expectedY = tcp::logic::math::FixedPoint::fromInt(-1).raw();
    if (transformAfterTick1.x.raw() != expectedX) {
        std::cerr << "movement x step failed: expected=" << expectedX << " actual=" << transformAfterTick1.x.raw() << '\n';
        ok = false;
    }
    if (transformAfterTick1.y.raw() != expectedY) {
        std::cerr << "movement y step failed: expected=" << expectedY << " actual=" << transformAfterTick1.y.raw() << '\n';
        ok = false;
    }
    ok &= verify(world.commandBuffers().at(entity).queued.size() == 1U, "input phase should consume current tick commands");

    auto& mutableHealths = world.mutableHealths();
    mutableHealths.at(entity).current = 0;
    world.tick();

    ok &= verify(world.entityCount() == 0U, "cleanup phase should destroy dead entity");
    ok &= verify(world.transforms().empty(), "transform component should be removed after cleanup");
    ok &= verify(world.commandBuffers().empty(), "command buffer should be removed after cleanup");

    if (!ok) {
        return 1;
    }

    std::cout << "ECS core systems tests passed" << '\n';
    return 0;
}
