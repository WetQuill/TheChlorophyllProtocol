#include "../../logic/debug/StateHasher.h"
#include "../../logic/ecs/World.h"
#include "../../logic/ecs/systems/BuiltInSystems.h"
#include "../../logic/runtime/SimulationDriver.h"

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

tcp::logic::ecs::Transform at(std::int32_t x, std::int32_t y) {
    tcp::logic::ecs::Transform tr{};
    tr.x = tcp::logic::math::FixedPoint::fromInt(x);
    tr.y = tcp::logic::math::FixedPoint::fromInt(y);
    return tr;
}

tcp::logic::ecs::World makeWorld() {
    tcp::logic::ecs::World world;
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
    world.setCommandBuffer(p0Unit, tcp::logic::ecs::CommandBuffer{});
    world.setCommandBuffer(p1Unit, tcp::logic::ecs::CommandBuffer{});
    world.setWeapon(p0Unit, tcp::logic::ecs::Weapon{tcp::logic::math::FixedPoint::fromInt(2), 5, 1, 0});
    world.setWeapon(p1Unit, tcp::logic::ecs::Weapon{tcp::logic::math::FixedPoint::fromInt(2), 5, 1, 0});

    return world;
}

struct ScheduledDelivery {
    std::int64_t deliverTick{0};
    int target{0};
    tcp::net::CommandFramePacket packet{};
};

}  // namespace

int main() {
    bool ok = true;

    tcp::logic::runtime::SimulationDriver single(makeWorld());
    single.useSingleLocalMode();
    single.queueLocalCommand(0, 3, tcp::logic::ecs::CommandType::kMove, 2, 1, 0);
    ok &= verify(single.stepTick(), "single mode should step immediately");
    const auto singlePos = single.world().transforms().at(3);
    ok &= verify(singlePos.x.toIntTrunc() == 1, "single mode move command not applied");

    const std::string replayPath = "simulation_driver_modes_replay.txt";
    ok &= verify(single.saveReplay(replayPath), "failed to save replay from single mode");

    tcp::logic::runtime::SimulationDriver replay(makeWorld());
    ok &= verify(replay.useReplayModeFromFile(replayPath), "failed to load replay mode");
    ok &= verify(replay.stepTick(), "replay mode should step with loaded commands");
    const auto replayPos = replay.world().transforms().at(3);
    ok &= verify(replayPos.x.toIntTrunc() == 1, "replay mode did not reproduce movement");

    tcp::logic::runtime::SimulationDriver lockstepA(makeWorld());
    tcp::logic::runtime::SimulationDriver lockstepB(makeWorld());
    lockstepA.useLockstepMode({0, 2, 1});
    lockstepB.useLockstepMode({1, 2, 1});

    for (std::int64_t warmupTick = 0; warmupTick < 1; ++warmupTick) {
        const tcp::net::CommandFramePacket emptyA{warmupTick, 0, {}};
        const tcp::net::CommandFramePacket emptyB{warmupTick, 1, {}};
        lockstepA.receivePacket(emptyA);
        lockstepA.receivePacket(emptyB);
        lockstepB.receivePacket(emptyA);
        lockstepB.receivePacket(emptyB);
    }

    std::vector<ScheduledDelivery> network;
    constexpr std::int64_t kTicks = 15;

    for (std::int64_t tick = 0; tick < kTicks; ++tick) {
        lockstepA.queueLocalCommand(0, 3, tcp::logic::ecs::CommandType::kStop, 0, 0, 0);
        lockstepB.queueLocalCommand(1, 4, tcp::logic::ecs::CommandType::kStop, 0, 0, 0);

        if (tick == 0) {
            lockstepA.queueLocalCommand(0, 3, tcp::logic::ecs::CommandType::kMove, 3, 1, 0);
            lockstepB.queueLocalCommand(1, 4, tcp::logic::ecs::CommandType::kMove, 5, 1, 0);
        }

        const auto outA = lockstepA.drainOutgoingPackets();
        const auto outB = lockstepB.drainOutgoingPackets();

        for (const auto& packet : outA) {
            network.push_back({tick, 0, packet});
            network.push_back({tick + 1, 1, packet});
        }
        for (const auto& packet : outB) {
            network.push_back({tick, 1, packet});
            network.push_back({tick + 1, 0, packet});
        }

        std::vector<ScheduledDelivery> pending;
        for (const auto& delivery : network) {
            if (delivery.deliverTick > tick) {
                pending.push_back(delivery);
                continue;
            }

            if (delivery.target == 0) {
                lockstepA.receivePacket(delivery.packet);
            } else {
                lockstepB.receivePacket(delivery.packet);
            }
        }
        network = pending;

        ok &= verify(lockstepA.stepTick(), "lockstepA should have full frame commands");
        ok &= verify(lockstepB.stepTick(), "lockstepB should have full frame commands");

        const auto hashA = tcp::logic::debug::hashWorldState(lockstepA.world());
        const auto hashB = tcp::logic::debug::hashWorldState(lockstepB.world());
        ok &= verify(hashA == hashB, "lockstep drivers diverged");
    }

    if (!ok) {
        return 1;
    }

    std::cout << "Simulation driver modes tests passed" << '\n';
    return 0;
}
