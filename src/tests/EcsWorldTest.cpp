#include "../logic/ecs/World.h"

#include <iostream>
#include <vector>

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

    ok &= verify(world.entityCount() == 0U, "new world should have zero entities");
    ok &= verify(world.currentTick() == 0, "new world should start at tick zero");

    for (int i = 0; i < 32; ++i) {
        world.tick();
    }

    ok &= verify(world.currentTick() == 32, "empty world tick progression failed");
    ok &= verify(world.entityCount() == 0U, "empty world should stay empty");

    std::vector<tcp::logic::ecs::SystemPhase> phaseTrace;
    world.registerSystem(tcp::logic::ecs::SystemPhase::kInput, [&](tcp::logic::ecs::World&, std::int64_t) {
        phaseTrace.push_back(tcp::logic::ecs::SystemPhase::kInput);
    });
    world.registerSystem(tcp::logic::ecs::SystemPhase::kProduction, [&](tcp::logic::ecs::World&, std::int64_t) {
        phaseTrace.push_back(tcp::logic::ecs::SystemPhase::kProduction);
    });
    world.registerSystem(tcp::logic::ecs::SystemPhase::kPathfinding, [&](tcp::logic::ecs::World&, std::int64_t) {
        phaseTrace.push_back(tcp::logic::ecs::SystemPhase::kPathfinding);
    });
    world.registerSystem(tcp::logic::ecs::SystemPhase::kMovement, [&](tcp::logic::ecs::World&, std::int64_t) {
        phaseTrace.push_back(tcp::logic::ecs::SystemPhase::kMovement);
    });
    world.registerSystem(tcp::logic::ecs::SystemPhase::kCombat, [&](tcp::logic::ecs::World&, std::int64_t) {
        phaseTrace.push_back(tcp::logic::ecs::SystemPhase::kCombat);
    });
    world.registerSystem(tcp::logic::ecs::SystemPhase::kResource, [&](tcp::logic::ecs::World&, std::int64_t) {
        phaseTrace.push_back(tcp::logic::ecs::SystemPhase::kResource);
    });
    world.registerSystem(tcp::logic::ecs::SystemPhase::kCleanup, [&](tcp::logic::ecs::World&, std::int64_t) {
        phaseTrace.push_back(tcp::logic::ecs::SystemPhase::kCleanup);
    });

    world.tick();

    const auto expected = tcp::logic::ecs::orderedSystemPhases();
    ok &= verify(phaseTrace.size() == expected.size(), "system phase callback count mismatch");
    for (std::size_t i = 0; i < phaseTrace.size() && i < expected.size(); ++i) {
        ok &= verify(phaseTrace[i] == expected[i], "system phase execution order mismatch");
    }

    const auto entity = world.createEntity();
    tcp::logic::ecs::Transform transform{};
    transform.x = tcp::logic::math::FixedPoint::fromInt(1);
    transform.y = tcp::logic::math::FixedPoint::fromInt(2);
    ok &= verify(world.setTransform(entity, transform), "failed to set transform component");
    ok &= verify(world.transforms().size() == 1U, "component storage size mismatch");
    ok &= verify(world.destroyEntity(entity), "destroy entity failed");
    ok &= verify(world.transforms().empty(), "entity component cleanup failed");

    if (!ok) {
        return 1;
    }

    std::cout << "ECS world tests passed" << '\n';
    return 0;
}
