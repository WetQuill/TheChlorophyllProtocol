#include "../../logic/commands/CommandQueue.h"
#include "../../logic/debug/StateHasher.h"
#include "../../logic/ecs/World.h"
#include "../../logic/ecs/systems/BuiltInSystems.h"

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

tcp::logic::ecs::Transform at(std::int32_t x, std::int32_t y) {
    tcp::logic::ecs::Transform tr{};
    tr.x = tcp::logic::math::FixedPoint::fromInt(x);
    tr.y = tcp::logic::math::FixedPoint::fromInt(y);
    return tr;
}

void setupWorld(tcp::logic::ecs::World& world) {
    tcp::logic::ecs::registerCoreSystems(world);

    const auto hq0 = world.createEntity();
    const auto hq1 = world.createEntity();
    world.setTeam(hq0, tcp::logic::ecs::Team{0});
    world.setTeam(hq1, tcp::logic::ecs::Team{1});
    world.setTransform(hq0, at(0, 0));
    world.setTransform(hq1, at(5, 0));
    world.setHealth(hq0, tcp::logic::ecs::Health{100, 100});
    world.setHealth(hq1, tcp::logic::ecs::Health{100, 100});
    world.setHeadquarters(hq0, tcp::logic::ecs::Headquarters{true});
    world.setHeadquarters(hq1, tcp::logic::ecs::Headquarters{true});

    const auto soldier = world.createEntity();
    world.setTeam(soldier, tcp::logic::ecs::Team{0});
    world.setTransform(soldier, at(0, 1));
    world.setHealth(soldier, tcp::logic::ecs::Health{30, 30});
    world.setWeapon(soldier, tcp::logic::ecs::Weapon{tcp::logic::math::FixedPoint::fromInt(2), 5, 1, 0});
    world.setCommandBuffer(soldier, tcp::logic::ecs::CommandBuffer{});

    world.setSunForTeam(0, 100);
}

}  // namespace

int main() {
    bool ok = true;

    tcp::logic::ecs::World worldA;
    tcp::logic::ecs::World worldB;
    setupWorld(worldA);
    setupWorld(worldB);

    tcp::logic::commands::CommandQueue queueA;
    tcp::logic::commands::CommandQueue queueB;

    const std::vector<tcp::logic::commands::PlayerCommand> script = {
        {0, 0, 3, tcp::logic::ecs::CommandType::kMove, 2, 1, 0},
        {1, 0, 3, tcp::logic::ecs::CommandType::kMove, 4, 1, 0},
        {2, 0, 3, tcp::logic::ecs::CommandType::kBuild, 1, 2, 20},
    };

    for (const auto& cmd : script) {
        queueA.push(cmd);
        queueB.push(cmd);
    }

    for (int i = 0; i < 20; ++i) {
        const auto tickA = worldA.currentTick();
        const auto tickB = worldB.currentTick();
        ok &= verify(tickA == tickB, "world ticks diverged before command apply");

        const auto commandsA = queueA.popForTick(tickA);
        const auto commandsB = queueB.popForTick(tickB);
        ok &= verify(commandsA == commandsB, "popped command streams diverged");

        tcp::logic::commands::applyCommandsAtTick(worldA, commandsA);
        tcp::logic::commands::applyCommandsAtTick(worldB, commandsB);

        worldA.tick();
        worldB.tick();

        const auto hashA = tcp::logic::debug::hashWorldState(worldA);
        const auto hashB = tcp::logic::debug::hashWorldState(worldB);
        ok &= verify(hashA == hashB, "world hashes diverged after tick");
    }

    if (!ok) {
        return 1;
    }

    std::cout << "Dual instance determinism tests passed" << '\n';
    return 0;
}
