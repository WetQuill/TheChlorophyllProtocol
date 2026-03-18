#include "../../logic/commands/CommandQueue.h"
#include "../../logic/debug/StateHasher.h"
#include "../../logic/ecs/World.h"
#include "../../logic/ecs/systems/BuiltInSystems.h"
#include "../../net/CommandSyncController.h"

#include <cstdint>
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
    world.setTransform(hq1, at(8, 0));
    world.setHealth(hq0, tcp::logic::ecs::Health{200, 200});
    world.setHealth(hq1, tcp::logic::ecs::Health{200, 200});
    world.setHeadquarters(hq0, tcp::logic::ecs::Headquarters{true});
    world.setHeadquarters(hq1, tcp::logic::ecs::Headquarters{true});

    const auto p0Unit = world.createEntity();
    const auto p1Unit = world.createEntity();
    world.setTeam(p0Unit, tcp::logic::ecs::Team{0});
    world.setTeam(p1Unit, tcp::logic::ecs::Team{1});
    world.setTransform(p0Unit, at(0, 1));
    world.setTransform(p1Unit, at(8, 1));
    world.setHealth(p0Unit, tcp::logic::ecs::Health{30, 30});
    world.setHealth(p1Unit, tcp::logic::ecs::Health{30, 30});
    world.setWeapon(p0Unit, tcp::logic::ecs::Weapon{tcp::logic::math::FixedPoint::fromInt(2), 5, 1, 0});
    world.setWeapon(p1Unit, tcp::logic::ecs::Weapon{tcp::logic::math::FixedPoint::fromInt(2), 5, 1, 0});
    world.setCommandBuffer(p0Unit, tcp::logic::ecs::CommandBuffer{});
    world.setCommandBuffer(p1Unit, tcp::logic::ecs::CommandBuffer{});
}

struct ScheduledDelivery {
    std::int64_t deliverTick{0};
    int targetController{0};
    tcp::net::CommandFramePacket packet{};
};

}  // namespace

int main() {
    bool ok = true;

    tcp::logic::ecs::World worldA;
    tcp::logic::ecs::World worldB;
    setupWorld(worldA);
    setupWorld(worldB);

    tcp::net::CommandSyncController syncA(0, 2, 2);
    tcp::net::CommandSyncController syncB(1, 2, 2);

    for (std::int64_t warmupTick = 0; warmupTick < 2; ++warmupTick) {
        const tcp::net::CommandFramePacket emptyA{warmupTick, 0, {}};
        const tcp::net::CommandFramePacket emptyB{warmupTick, 1, {}};
        syncA.receivePacket(emptyA);
        syncA.receivePacket(emptyB);
        syncB.receivePacket(emptyA);
        syncB.receivePacket(emptyB);
    }

    std::vector<ScheduledDelivery> network;

    constexpr std::int64_t kTicks = 30;
    for (std::int64_t tick = 0; tick < kTicks; ++tick) {
        syncA.queueLocalCommand(tick, 3, tcp::logic::ecs::CommandType::kStop, 0, 0, 0);
        syncB.queueLocalCommand(tick, 4, tcp::logic::ecs::CommandType::kStop, 0, 0, 0);

        if (tick == 0) {
            syncA.queueLocalCommand(tick, 3, tcp::logic::ecs::CommandType::kMove, 3, 1, 0);
            syncB.queueLocalCommand(tick, 4, tcp::logic::ecs::CommandType::kMove, 5, 1, 0);
        }
        if (tick == 3) {
            syncA.queueLocalCommand(tick, 3, tcp::logic::ecs::CommandType::kBuild, 1, 2, 20);
            syncB.queueLocalCommand(tick, 4, tcp::logic::ecs::CommandType::kBuild, 7, 2, 20);
        }

        const auto packetsA = syncA.drainOutgoingPackets();
        const auto packetsB = syncB.drainOutgoingPackets();

        for (const auto& packet : packetsA) {
            network.push_back({tick, 0, packet});
            network.push_back({tick + 1, 1, packet});
        }
        for (const auto& packet : packetsB) {
            network.push_back({tick, 1, packet});
            network.push_back({tick + 1, 0, packet});
        }

        std::vector<ScheduledDelivery> pending;
        for (const auto& delivery : network) {
            if (delivery.deliverTick > tick) {
                pending.push_back(delivery);
                continue;
            }

            if (delivery.targetController == 0) {
                syncA.receivePacket(delivery.packet);
            } else {
                syncB.receivePacket(delivery.packet);
            }
        }
        network = pending;

        std::vector<tcp::logic::commands::PlayerCommand> commandsA;
        std::vector<tcp::logic::commands::PlayerCommand> commandsB;
        ok &= verify(syncA.collectCommandsForTick(tick, commandsA), "syncA missing full frame commands");
        ok &= verify(syncB.collectCommandsForTick(tick, commandsB), "syncB missing full frame commands");
        ok &= verify(commandsA == commandsB, "controller command sets diverged");

        tcp::logic::commands::applyCommandsAtTick(worldA, commandsA);
        tcp::logic::commands::applyCommandsAtTick(worldB, commandsB);

        worldA.tick();
        worldB.tick();

        const auto hashA = tcp::logic::debug::hashWorldState(worldA);
        const auto hashB = tcp::logic::debug::hashWorldState(worldB);
        ok &= verify(hashA == hashB, "lockstep worlds diverged after tick");
    }

    if (!ok) {
        return 1;
    }

    std::cout << "Lockstep command sync tests passed" << '\n';
    return 0;
}
