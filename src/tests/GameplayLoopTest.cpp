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

    const auto hqTeam0 = world.createEntity();
    const auto hqTeam1 = world.createEntity();

    world.setTeam(hqTeam0, tcp::logic::ecs::Team{0});
    world.setTeam(hqTeam1, tcp::logic::ecs::Team{1});

    tcp::logic::ecs::Transform hq0Transform{};
    hq0Transform.x = tcp::logic::math::FixedPoint::fromInt(0);
    hq0Transform.y = tcp::logic::math::FixedPoint::fromInt(0);
    world.setTransform(hqTeam0, hq0Transform);

    tcp::logic::ecs::Transform hq1Transform{};
    hq1Transform.x = tcp::logic::math::FixedPoint::fromInt(1);
    hq1Transform.y = tcp::logic::math::FixedPoint::fromInt(0);
    world.setTransform(hqTeam1, hq1Transform);

    world.setHealth(hqTeam0, tcp::logic::ecs::Health{30, 30});
    world.setHealth(hqTeam1, tcp::logic::ecs::Health{20, 20});
    world.setHeadquarters(hqTeam0, tcp::logic::ecs::Headquarters{true});
    world.setHeadquarters(hqTeam1, tcp::logic::ecs::Headquarters{true});

    const auto sunflower = world.createEntity();
    world.setTeam(sunflower, tcp::logic::ecs::Team{0});
    world.setTransform(sunflower, hq0Transform);
    world.setSunProducer(sunflower, tcp::logic::ecs::SunProducer{5});

    const auto barracks = world.createEntity();
    world.setTeam(barracks, tcp::logic::ecs::Team{0});
    world.setTransform(barracks, hq0Transform);
    world.setProduction(barracks, tcp::logic::ecs::Production{10, 2, 0, 700, 8});

    world.setSunForTeam(0, 0);
    world.setSunForTeam(1, 0);

    world.tick();
    ok &= verify(world.sunForTeam(0) == 5, "sun generation tick 1 mismatch");

    world.tick();
    ok &= verify(world.sunForTeam(0) == 10, "sun generation tick 2 mismatch");

    world.tick();
    ok &= verify(world.sunForTeam(0) == 5, "production should consume 10 sun on start");

    const auto entityCountBeforeSpawn = world.entityCount();
    world.tick();
    ok &= verify(world.entityCount() == entityCountBeforeSpawn + 1, "production should spawn one unit after build time");
    ok &= verify(world.sunForTeam(0) == 10, "sun balance after spawn tick mismatch");

    const auto attacker = world.createEntity();
    world.setTeam(attacker, tcp::logic::ecs::Team{0});
    world.setTransform(attacker, hq0Transform);
    world.setHealth(attacker, tcp::logic::ecs::Health{10, 10});
    world.setWeapon(attacker, tcp::logic::ecs::Weapon{
        tcp::logic::math::FixedPoint::fromInt(5),
        30,
        1,
        0,
    });

    world.tick();

    ok &= verify(world.winnerTeam() == 0, "winner team should be team 0 after HQ kill");
    ok &= verify(world.headquarters().size() == 1U, "defeated headquarters should be cleaned up");

    if (!ok) {
        return 1;
    }

    std::cout << "Gameplay loop tests passed" << '\n';
    return 0;
}
