#include "../../logic/commands/CommandQueue.h"

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

    tcp::logic::commands::CommandQueue queue;
    queue.push({3, 2, 9, tcp::logic::ecs::CommandType::kBuild, 1, 2, 3});
    queue.push({3, 1, 2, tcp::logic::ecs::CommandType::kMove, 4, 5, 6});
    queue.push({3, 1, 1, tcp::logic::ecs::CommandType::kAttack, 7, 8, 9});
    queue.push({4, 1, 1, tcp::logic::ecs::CommandType::kStop, 0, 0, 0});

    const auto tick3 = queue.popForTick(3);
    ok &= verify(tick3.size() == 3U, "expected exactly three commands for tick 3");
    if (tick3.size() == 3U) {
        ok &= verify(tick3[0].playerId == 1 && tick3[0].entityId == 1, "sorted order mismatch at index 0");
        ok &= verify(tick3[1].playerId == 1 && tick3[1].entityId == 2, "sorted order mismatch at index 1");
        ok &= verify(tick3[2].playerId == 2 && tick3[2].entityId == 9, "sorted order mismatch at index 2");
    }

    const auto tick4 = queue.popForTick(4);
    ok &= verify(tick4.size() == 1U, "expected one command for tick 4");
    ok &= verify(queue.empty(), "queue should be empty after popping all ticks");

    tcp::logic::ecs::World world;
    const auto e1 = world.createEntity();
    const auto e2 = world.createEntity();
    world.setCommandBuffer(e1, tcp::logic::ecs::CommandBuffer{});
    world.setCommandBuffer(e2, tcp::logic::ecs::CommandBuffer{});

    tcp::logic::commands::applyCommandsAtTick(world, tick3);
    ok &= verify(world.commandBuffers().at(e1).queued.size() == 1U, "entity 1 should receive one command");
    ok &= verify(world.commandBuffers().at(e2).queued.size() == 1U, "entity 2 should receive one command");

    if (!ok) {
        return 1;
    }

    std::cout << "Command queue order tests passed" << '\n';
    return 0;
}
