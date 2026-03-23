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

    const auto bastion = world.createEntity();
    world.setTeam(bastion, tcp::logic::ecs::Team{0});
    world.setTransform(bastion, at(0, 0));
    world.setHealth(bastion, tcp::logic::ecs::Health{2500000, 2500000});
    world.setBuilding(bastion, tcp::logic::ecs::Building{true});
    world.setCommandBuffer(bastion, tcp::logic::ecs::CommandBuffer{});
    world.setPowerConsumer(bastion, tcp::logic::ecs::PowerConsumer{80000, true});
    world.setArtilleryWeapon(
        bastion,
        tcp::logic::ecs::ArtilleryWeapon{
            tcp::logic::math::FixedPoint::fromInt(1),
            tcp::logic::math::FixedPoint::fromInt(10),
            tcp::logic::math::FixedPoint::fromInt(1),
            300,
            150,
            0,
            1,
            3,
        });

    const auto enemyA = world.createEntity();
    world.setTeam(enemyA, tcp::logic::ecs::Team{1});
    world.setTransform(enemyA, at(3, 0));
    world.setHealth(enemyA, tcp::logic::ecs::Health{1000, 1000});

    const auto enemyB = world.createEntity();
    world.setTeam(enemyB, tcp::logic::ecs::Team{1});
    world.setTransform(enemyB, at(3, 0));
    world.setHealth(enemyB, tcp::logic::ecs::Health{1000, 1000});

    world.setAttackTarget(bastion, enemyA);

    world.setPowerForTeam(0, 0);
    world.tick();
    ok &= verify(world.ballisticProjectiles().empty(), "bastion should not fire while power disabled");
    ok &= verify(world.artilleryWeapons().at(bastion).remainingCooldownTicks == 0,
                 "bastion should not charge cooldown while power disabled");

    world.setPowerForTeam(0, 80000);
    world.tick();
    ok &= verify(world.ballisticProjectiles().size() == 1U, "bastion should spawn projectile when powered");

    world.tick();

    ok &= verify(world.ballisticProjectiles().empty(), "projectile should detonate after travel ticks");
    ok &= verify(world.healths().at(enemyA).current == 700, "butter aoe should damage primary enemy");
    ok &= verify(world.healths().at(enemyB).current == 700, "butter aoe should damage all enemies in radius");
    ok &= verify(world.frozenStates().find(enemyA) != world.frozenStates().end(), "primary enemy should be frozen");
    ok &= verify(world.frozenStates().find(enemyB) != world.frozenStates().end(), "secondary enemy should be frozen");

    world.setVelocity(enemyA,
                      tcp::logic::ecs::Velocity{
                          tcp::logic::math::FixedPoint::fromInt(1),
                          tcp::logic::math::FixedPoint::fromInt(0),
                      });

    const auto enemyAXFrozenStart = world.transforms().at(enemyA).x.toIntTrunc();

    world.tick();
    const auto enemyAXAfter = world.transforms().at(enemyA).x.toIntTrunc();
    ok &= verify(enemyAXAfter == enemyAXFrozenStart, "100% frozen should prevent movement while active");

    for (int i = 0; i < 4; ++i) {
        world.tick();
    }
    ok &= verify(world.frozenStates().find(enemyA) == world.frozenStates().end(), "frozen state should expire deterministically");

    if (!ok) {
        return 1;
    }

    std::cout << "Corn cannon bastion systems test passed" << '\n';
    return 0;
}
