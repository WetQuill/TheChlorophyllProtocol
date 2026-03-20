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

tcp::logic::ecs::Transform at(int x, int y) {
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

    const auto hq = world.createEntity();
    world.setTeam(hq, tcp::logic::ecs::Team{0});
    world.setTransform(hq, at(0, 0));
    world.setHealth(hq, tcp::logic::ecs::Health{100, 100});
    world.setHeadquarters(hq, tcp::logic::ecs::Headquarters{true});

    const auto builder = world.createEntity();
    world.setTeam(builder, tcp::logic::ecs::Team{0});
    world.setTransform(builder, at(0, 0));
    world.setHealth(builder, tcp::logic::ecs::Health{10, 10});

    tcp::logic::ecs::CommandBuffer buffer{};
    buffer.queued.push_back({0, 0, tcp::logic::ecs::CommandType::kBuild, 2, 0, 10});
    buffer.queued.push_back({1, 0, tcp::logic::ecs::CommandType::kBuild, 2, 0, 10});
    world.setCommandBuffer(builder, buffer);
    world.setSunForTeam(0, 30);
    world.setPowerForTeam(0, 30);

    world.tick();
    ok &= verify(world.buildings().size() == 1U, "first build should succeed");
    ok &= verify(world.sunForTeam(0) == 20, "first build should spend exactly one cost");
    ok &= verify(world.powerForTeam(0) == 20, "first build should spend exactly one power cost");

    world.tick();
    ok &= verify(world.buildings().size() == 1U, "duplicate build on occupied cell must be rejected");
    ok &= verify(world.sunForTeam(0) == 20, "duplicate build must not spend additional sun");
    ok &= verify(world.powerForTeam(0) == 20, "duplicate build must not spend additional power");

    if (!ok) {
        return 1;
    }

    std::cout << "Build placement regression passed" << '\n';
    return 0;
}
