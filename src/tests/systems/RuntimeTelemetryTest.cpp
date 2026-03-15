#include "../../logic/debug/StateHasher.h"
#include "../../logic/ecs/World.h"
#include "../../logic/ecs/systems/BuiltInSystems.h"

#include <iostream>

namespace {

bool verify(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

tcp::logic::ecs::Transform at(std::int32_t x, std::int32_t y) {
    tcp::logic::ecs::Transform tr{};
    tr.x = tcp::logic::math::FixedPoint::fromInt(x);
    tr.y = tcp::logic::math::FixedPoint::fromInt(y);
    return tr;
}

}  // namespace

int main() {
    bool ok = true;

    tcp::logic::ecs::World world;
    tcp::logic::ecs::registerCoreSystems(world);
    world.setDeterminismDebugEnabled(true);
    world.setTickDurationMicros(33333);

    const auto hq = world.createEntity();
    world.setTeam(hq, tcp::logic::ecs::Team{0});
    world.setTransform(hq, at(0, 0));
    world.setHealth(hq, tcp::logic::ecs::Health{100, 100});
    world.setHeadquarters(hq, tcp::logic::ecs::Headquarters{true});

    const auto mover = world.createEntity();
    world.setTeam(mover, tcp::logic::ecs::Team{0});
    world.setTransform(mover, at(0, 1));
    world.setHealth(mover, tcp::logic::ecs::Health{10, 10});
    world.setCommandBuffer(mover, tcp::logic::ecs::CommandBuffer{{{0, 0, tcp::logic::ecs::CommandType::kMove, 2, 1, 0}}});

    world.tick();
    tcp::logic::debug::updateDeterminismSnapshot(world);

    const auto& metrics = world.telemetry();
    ok &= verify(metrics.tickDurationMicros == 33333, "tick duration metric mismatch");
    ok &= verify(metrics.pathRequests >= 1, "expected at least one path request");
    ok &= verify(metrics.commandsProcessed >= 1, "expected at least one processed command");
    ok &= verify(metrics.entityCount >= 2, "entity count metric mismatch");
    ok &= verify(world.lastStateHash() != 0U, "determinism debug hash was not captured");

    if (!ok) {
        return 1;
    }

    std::cout << "Runtime telemetry tests passed" << '\n';
    return 0;
}
