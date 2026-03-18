#pragma once

#include "../commands/CommandQueue.h"
#include "../debug/StateHasher.h"
#include "../ecs/World.h"
#include "../replay/ReplayPlayer.h"
#include "../replay/ReplayRecorder.h"
#include "../../net/CommandSyncController.h"

#include <cstdint>
#include <string>
#include <vector>

namespace tcp::logic::runtime {

enum class DriverMode : std::uint8_t {
    kSingleLocal = 0,
    kReplay = 1,
    kLockstep = 2,
};

struct LockstepConfig {
    std::uint8_t localPlayerId{0};
    std::int32_t totalPlayers{2};
    std::int32_t inputDelayTicks{2};
};

class SimulationDriver final {
public:
    explicit SimulationDriver(ecs::World world);

    void useSingleLocalMode();
    bool useReplayModeFromFile(const std::string& replayPath);
    void useLockstepMode(const LockstepConfig& config);

    [[nodiscard]] DriverMode mode() const noexcept;

    void queueLocalCommand(
        std::uint8_t playerId,
        std::uint32_t entityId,
        ecs::CommandType type,
        std::int32_t arg0,
        std::int32_t arg1,
        std::int32_t arg2);

    [[nodiscard]] std::vector<net::CommandFramePacket> drainOutgoingPackets();
    void receivePacket(const net::CommandFramePacket& packet);

    [[nodiscard]] bool stepTick();

    [[nodiscard]] ecs::World& world() noexcept;
    [[nodiscard]] const ecs::World& world() const noexcept;

    [[nodiscard]] bool saveReplay(const std::string& replayPath) const;

private:
    [[nodiscard]] bool gatherCommandsForCurrentTick(std::vector<commands::PlayerCommand>& out);

    ecs::World world_{};
    DriverMode mode_{DriverMode::kSingleLocal};
    commands::CommandQueue queue_{};
    replay::ReplayRecorder recorder_{};

    net::CommandSyncController lockstepSync_{0, 2, 2};
    bool lockstepConfigured_{false};
};

}  // namespace tcp::logic::runtime
