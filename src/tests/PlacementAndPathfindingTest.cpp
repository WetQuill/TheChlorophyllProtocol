#include "../logic/ecs/World.h"
#include "../logic/ecs/systems/BuiltInSystems.h"

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
    world.setCommandBuffer(builder, tcp::logic::ecs::CommandBuffer{
        {
            {0, 0, tcp::logic::ecs::CommandType::kBuild, 3, 0, 10},
            {1, 0, tcp::logic::ecs::CommandType::kBuild, 10, 0, 10},
            {2, 0, tcp::logic::ecs::CommandType::kBuild, 3, 0, 10},
        },
    });

    world.setSunForTeam(0, 30);

    world.tick();
    ok &= verify(world.buildings().size() == 1U, "valid nearby build should create one building");
    ok &= verify(world.sunForTeam(0) == 20, "valid build should spend sun");

    world.tick();
    ok &= verify(world.buildings().size() == 1U, "far build should be rejected");
    ok &= verify(world.sunForTeam(0) == 20, "rejected build should not spend sun");

    world.tick();
    ok &= verify(world.buildings().size() == 1U, "occupied build should be rejected");
    ok &= verify(world.sunForTeam(0) == 20, "occupied build should not spend sun");

    const auto mover = world.createEntity();
    world.setTeam(mover, tcp::logic::ecs::Team{0});
    world.setTransform(mover, at(0, 1));
    world.setHealth(mover, tcp::logic::ecs::Health{10, 10});
    world.setVelocity(mover, tcp::logic::ecs::Velocity{});
    world.setCommandBuffer(mover, tcp::logic::ecs::CommandBuffer{
        {
            {world.currentTick(), 0, tcp::logic::ecs::CommandType::kMove, 2, 1, 0},
        },
    });

    const auto wall = world.createEntity();
    world.setTeam(wall, tcp::logic::ecs::Team{0});
    world.setTransform(wall, at(1, 1));
    world.setHealth(wall, tcp::logic::ecs::Health{20, 20});
    world.setBuilding(wall, tcp::logic::ecs::Building{true});

    for (int i = 0; i < 6; ++i) {
        world.tick();
        const auto moverPos = world.transforms().at(mover);
        ok &= verify(!(moverPos.x.toIntTrunc() == 1 && moverPos.y.toIntTrunc() == 1), "mover crossed blocked wall cell");
    }

    const auto finalPos = world.transforms().at(mover);
    ok &= verify(finalPos.x.toIntTrunc() == 2 && finalPos.y.toIntTrunc() == 1, "mover should reach move target around obstacle");

    if (!ok) {
        return 1;
    }

    std::cout << "Placement and pathfinding tests passed" << '\n';
    return 0;
}
