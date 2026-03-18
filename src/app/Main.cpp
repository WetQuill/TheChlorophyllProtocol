#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../logic/debug/StateHasher.h"
#include "../logic/ecs/systems/BuiltInSystems.h"
#include "../logic/runtime/SimulationDriver.h"

namespace {

tcp::logic::ecs::Transform at(std::int32_t x, std::int32_t y) {
    tcp::logic::ecs::Transform tr{};
    tr.x = tcp::logic::math::FixedPoint::fromInt(x);
    tr.y = tcp::logic::math::FixedPoint::fromInt(y);
    return tr;
}

tcp::logic::ecs::World makeDemoWorld() {
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
    world.setWeapon(p0Unit, tcp::logic::ecs::Weapon{tcp::logic::math::FixedPoint::fromInt(2), 5, 1, 0});
    world.setWeapon(p1Unit, tcp::logic::ecs::Weapon{tcp::logic::math::FixedPoint::fromInt(2), 5, 1, 0});
    world.setCommandBuffer(p0Unit, tcp::logic::ecs::CommandBuffer{});
    world.setCommandBuffer(p1Unit, tcp::logic::ecs::CommandBuffer{});

    return world;
}

struct Delivery {
    std::int64_t deliverTick{0};
    int target{0};
    tcp::net::CommandFramePacket packet{};
};

enum class DemoMode : std::uint8_t {
    kAll = 0,
    kSingle = 1,
    kReplay = 2,
    kLockstep = 3,
};

struct AppOptions {
    DemoMode mode{DemoMode::kAll};
    std::int64_t ticks{8};
    std::string replayPath{"simulation_driver_demo_replay.txt"};
};

void printUsage() {
    std::cout
        << "Usage: chlorophyll_app [--mode all|single|replay|lockstep] [--ticks N] [--replay-path PATH]"
        << '\n';
}

std::optional<DemoMode> parseMode(const std::string_view value) {
    if (value == "all") {
        return DemoMode::kAll;
    }
    if (value == "single") {
        return DemoMode::kSingle;
    }
    if (value == "replay") {
        return DemoMode::kReplay;
    }
    if (value == "lockstep") {
        return DemoMode::kLockstep;
    }
    return std::nullopt;
}

bool parseTicks(const std::string& value, std::int64_t& outTicks) {
    try {
        std::size_t parsed = 0;
        outTicks = std::stoll(value, &parsed);
        if (parsed != value.size()) {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    if (outTicks <= 0) {
        return false;
    }

    return true;
}

bool parseOptions(int argc, char** argv, AppOptions& out) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--mode") {
            if (i + 1 >= argc) {
                return false;
            }

            const auto parsedMode = parseMode(argv[++i]);
            if (!parsedMode.has_value()) {
                return false;
            }
            out.mode = parsedMode.value();
            continue;
        }

        if (arg == "--ticks") {
            if (i + 1 >= argc || !parseTicks(argv[++i], out.ticks)) {
                return false;
            }
            continue;
        }

        if (arg == "--replay-path") {
            if (i + 1 >= argc) {
                return false;
            }

            out.replayPath = argv[++i];
            continue;
        }

        if (arg == "--help" || arg == "-h") {
            return false;
        }

        return false;
    }

    return true;
}

bool runSingle(const std::int64_t ticks, const std::string& replayPath) {
    tcp::logic::runtime::SimulationDriver single(makeDemoWorld());
    single.useSingleLocalMode();

    for (std::int64_t tick = 0; tick < ticks; ++tick) {
        if (tick == 0) {
            single.queueLocalCommand(0, 3, tcp::logic::ecs::CommandType::kMove, 2, 1, 0);
        }

        if (!single.stepTick()) {
            std::cerr << "single: failed to step" << '\n';
            return false;
        }
    }

    const auto singlePos = single.world().transforms().at(3);
    std::cout << "single: tick=" << single.world().currentTick()
              << " pos=(" << singlePos.x.toIntTrunc() << ',' << singlePos.y.toIntTrunc() << ")"
              << " hash=" << tcp::logic::debug::hashWorldState(single.world()) << '\n';

    if (!single.saveReplay(replayPath)) {
        std::cerr << "single: failed to save replay file" << '\n';
        return false;
    }

    return true;
}

bool runReplay(const std::int64_t ticks, const std::string& replayPath) {
    tcp::logic::runtime::SimulationDriver replay(makeDemoWorld());
    if (!replay.useReplayModeFromFile(replayPath)) {
        std::cerr << "replay: failed to load replay file: " << replayPath << '\n';
        return false;
    }

    for (std::int64_t tick = 0; tick < ticks; ++tick) {
        if (!replay.stepTick()) {
            std::cerr << "replay: failed to step" << '\n';
            return false;
        }
    }

    const auto replayPos = replay.world().transforms().at(3);
    std::cout << "replay: tick=" << replay.world().currentTick()
              << " pos=(" << replayPos.x.toIntTrunc() << ',' << replayPos.y.toIntTrunc() << ")"
              << " hash=" << tcp::logic::debug::hashWorldState(replay.world()) << '\n';

    return true;
}

bool runLockstep(const std::int64_t ticks) {
    tcp::logic::runtime::SimulationDriver lockstepA(makeDemoWorld());
    tcp::logic::runtime::SimulationDriver lockstepB(makeDemoWorld());
    lockstepA.useLockstepMode({0, 2, 1});
    lockstepB.useLockstepMode({1, 2, 1});

    const tcp::net::CommandFramePacket emptyA{0, 0, {}};
    const tcp::net::CommandFramePacket emptyB{0, 1, {}};
    lockstepA.receivePacket(emptyA);
    lockstepA.receivePacket(emptyB);
    lockstepB.receivePacket(emptyA);
    lockstepB.receivePacket(emptyB);

    std::vector<Delivery> network;
    for (std::int64_t tick = 0; tick < ticks; ++tick) {
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

        std::vector<Delivery> pending;
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
        network = std::move(pending);

        if (!lockstepA.stepTick() || !lockstepB.stepTick()) {
            std::cerr << "lockstep: frame incomplete at tick " << tick << '\n';
            return false;
        }
    }

    const auto lockHashA = tcp::logic::debug::hashWorldState(lockstepA.world());
    const auto lockHashB = tcp::logic::debug::hashWorldState(lockstepB.world());
    std::cout << "lockstep: hashA=" << lockHashA << " hashB=" << lockHashB
              << " equal=" << ((lockHashA == lockHashB) ? "yes" : "no") << '\n';

    return lockHashA == lockHashB;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2) {
        const std::string maybeHelp = argv[1];
        if (maybeHelp == "--help" || maybeHelp == "-h") {
            printUsage();
            return 0;
        }
    }

    AppOptions options{};
    if (!parseOptions(argc, argv, options)) {
        printUsage();
        return 1;
    }

    std::cout << "The Chlorophyll Protocol runtime driver demo" << '\n';

    if (options.mode == DemoMode::kSingle) {
        return runSingle(options.ticks, options.replayPath) ? 0 : 1;
    }

    if (options.mode == DemoMode::kReplay) {
        return runReplay(options.ticks, options.replayPath) ? 0 : 1;
    }

    if (options.mode == DemoMode::kLockstep) {
        return runLockstep(options.ticks) ? 0 : 1;
    }

    if (!runSingle(options.ticks, options.replayPath)) {
        return 1;
    }
    if (!runReplay(options.ticks, options.replayPath)) {
        return 1;
    }
    return runLockstep(options.ticks) ? 0 : 1;
}
