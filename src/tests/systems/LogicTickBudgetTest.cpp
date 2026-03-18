#include "../../logic/ecs/World.h"
#include "../../logic/ecs/systems/BuiltInSystems.h"

#include <chrono>
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

void setupStressScene(tcp::logic::ecs::World& world) {
    tcp::logic::ecs::registerCoreSystems(world);

    const auto hq0 = world.createEntity();
    const auto hq1 = world.createEntity();
    world.setTeam(hq0, tcp::logic::ecs::Team{0});
    world.setTeam(hq1, tcp::logic::ecs::Team{1});
    world.setTransform(hq0, at(0, 0));
    world.setTransform(hq1, at(10, 10));
    world.setHealth(hq0, tcp::logic::ecs::Health{1000, 1000});
    world.setHealth(hq1, tcp::logic::ecs::Health{1000, 1000});
    world.setHeadquarters(hq0, tcp::logic::ecs::Headquarters{true});
    world.setHeadquarters(hq1, tcp::logic::ecs::Headquarters{true});

    world.setSunForTeam(0, 10000);
    world.setSunForTeam(1, 10000);

    for (int y = -8; y <= 8; y += 4) {
        for (int x = -8; x <= 8; x += 4) {
            const auto obstacle = world.createEntity();
            world.setTeam(obstacle, tcp::logic::ecs::Team{0});
            world.setTransform(obstacle, at(x, y));
            world.setHealth(obstacle, tcp::logic::ecs::Health{200, 200});
            world.setBuilding(obstacle, tcp::logic::ecs::Building{true});
        }
    }

    for (int i = 0; i < 180; ++i) {
        const auto unit = world.createEntity();
        const std::uint8_t teamId = static_cast<std::uint8_t>(i % 2);
        const int lane = i % 30;
        const int column = i / 30;
        const int x = (teamId == 0) ? -14 + lane : 14 - lane;
        const int y = -12 + column;

        world.setTeam(unit, tcp::logic::ecs::Team{teamId});
        world.setTransform(unit, at(x, y));
        world.setHealth(unit, tcp::logic::ecs::Health{40, 40});
        world.setWeapon(unit, tcp::logic::ecs::Weapon{tcp::logic::math::FixedPoint::fromInt(2), 3, 1, 0});

        tcp::logic::ecs::CommandBuffer queue{};
        queue.queued.push_back({0, teamId, tcp::logic::ecs::CommandType::kMove, 0, 0, 0});
        queue.queued.push_back({10, teamId, tcp::logic::ecs::CommandType::kMove, (teamId == 0) ? 8 : -8, y, 0});
        queue.queued.push_back({40, teamId, tcp::logic::ecs::CommandType::kMove, (teamId == 0) ? 12 : -12, y, 0});
        world.setCommandBuffer(unit, queue);
    }
}

}  // namespace

int main() {
    bool ok = true;

    tcp::logic::ecs::World world;
    setupStressScene(world);

    constexpr std::int64_t kTickBudgetMicros = 33333;
    constexpr int kTicks = 240;
    constexpr int kMaxConsecutiveOverruns = 2;

    std::int64_t totalMicros = 0;
    std::int64_t maxMicros = 0;
    int overrunTicks = 0;
    int consecutiveOverruns = 0;
    int maxConsecutiveOverruns = 0;

    for (int i = 0; i < kTicks; ++i) {
        const auto start = std::chrono::steady_clock::now();
        world.tick();
        const auto end = std::chrono::steady_clock::now();

        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        totalMicros += elapsed;
        if (elapsed > maxMicros) {
            maxMicros = elapsed;
        }

        if (elapsed > kTickBudgetMicros) {
            ++overrunTicks;
            ++consecutiveOverruns;
            if (consecutiveOverruns > maxConsecutiveOverruns) {
                maxConsecutiveOverruns = consecutiveOverruns;
            }
        } else {
            consecutiveOverruns = 0;
        }
    }

    const auto averageMicros = totalMicros / kTicks;
    const auto& metrics = world.telemetry();

    ok &= verify(maxConsecutiveOverruns <= kMaxConsecutiveOverruns,
                 "too many consecutive tick overruns detected");
    ok &= verify(averageMicros <= kTickBudgetMicros,
                 "average tick time exceeded 30 TPS budget");
    ok &= verify(metrics.entityCount > 0, "telemetry entity count should be populated");

    std::cout << "Tick budget metrics: avg=" << averageMicros
              << "us max=" << maxMicros
              << "us overruns=" << overrunTicks
              << " max_consecutive_overruns=" << maxConsecutiveOverruns
              << " entity_count=" << metrics.entityCount
              << " path_requests=" << metrics.pathRequests
              << '\n';

    if (!ok) {
        return 1;
    }

    std::cout << "Logic tick budget test passed" << '\n';
    return 0;
}
