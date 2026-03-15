#include "../logic/commands/CommandQueue.h"
#include "../logic/debug/StateHasher.h"
#include "../logic/ecs/World.h"
#include "../logic/ecs/systems/BuiltInSystems.h"
#include "../logic/replay/ReplayPlayer.h"
#include "../logic/replay/ReplayRecorder.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

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

std::uint64_t runSimulation(const std::vector<tcp::logic::commands::PlayerCommand>& commands, int ticks) {
    tcp::logic::ecs::World world;
    tcp::logic::ecs::registerCoreSystems(world);

    const auto hq0 = world.createEntity();
    const auto hq1 = world.createEntity();
    world.setTeam(hq0, tcp::logic::ecs::Team{0});
    world.setTeam(hq1, tcp::logic::ecs::Team{1});
    world.setTransform(hq0, at(0, 0));
    world.setTransform(hq1, at(4, 0));
    world.setHealth(hq0, tcp::logic::ecs::Health{100, 100});
    world.setHealth(hq1, tcp::logic::ecs::Health{100, 100});
    world.setHeadquarters(hq0, tcp::logic::ecs::Headquarters{true});
    world.setHeadquarters(hq1, tcp::logic::ecs::Headquarters{true});

    const auto unit = world.createEntity();
    world.setTeam(unit, tcp::logic::ecs::Team{0});
    world.setTransform(unit, at(0, 1));
    world.setHealth(unit, tcp::logic::ecs::Health{20, 20});
    world.setWeapon(unit, tcp::logic::ecs::Weapon{tcp::logic::math::FixedPoint::fromInt(2), 10, 1, 0});
    world.setCommandBuffer(unit, tcp::logic::ecs::CommandBuffer{});

    tcp::logic::commands::CommandQueue queue;
    for (const auto& command : commands) {
        queue.push(command);
    }

    std::uint64_t lastHash = 0;
    for (int i = 0; i < ticks; ++i) {
        const auto toApply = queue.popForTick(world.currentTick());
        tcp::logic::commands::applyCommandsAtTick(world, toApply);
        world.tick();
        lastHash = tcp::logic::debug::hashWorldState(world);
    }

    return lastHash;
}

}  // namespace

int main() {
    bool ok = true;

    std::vector<tcp::logic::commands::PlayerCommand> script{
        {0, 0, 3, tcp::logic::ecs::CommandType::kMove, 2, 1, 0},
        {1, 0, 3, tcp::logic::ecs::CommandType::kMove, 3, 1, 0},
        {2, 0, 3, tcp::logic::ecs::CommandType::kMove, 4, 1, 0},
    };

    tcp::logic::replay::ReplayRecorder recorder;
    for (const auto& cmd : script) {
        recorder.record(cmd);
    }

    const std::string replayPath = "replay_determinism_test.txt";
    ok &= verify(recorder.saveToFile(replayPath), "failed to save replay file");

    std::vector<tcp::logic::commands::PlayerCommand> loaded;
    ok &= verify(tcp::logic::replay::loadFromFile(replayPath, loaded), "failed to load replay file");
    ok &= verify(loaded.size() == script.size(), "loaded replay command count mismatch");

    for (std::size_t i = 0; i < script.size() && i < loaded.size(); ++i) {
        ok &= verify(script[i] == loaded[i], "loaded replay command mismatch");
    }

    const auto baselineHash = runSimulation(loaded, 12);
    for (int i = 0; i < 100; ++i) {
        const auto hash = runSimulation(loaded, 12);
        ok &= verify(hash == baselineHash, "replay hash mismatch across repeated runs");
    }

    if (!ok) {
        return 1;
    }

    std::cout << "Replay determinism tests passed" << '\n';
    return 0;
}
