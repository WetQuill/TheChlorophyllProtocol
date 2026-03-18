#pragma once

#include "../logic/commands/PlayerCommand.h"

#include <cstdint>
#include <map>
#include <set>
#include <vector>

namespace tcp::net {

struct CommandFramePacket {
    std::int64_t tick{0};
    std::uint8_t playerId{0};
    std::vector<logic::commands::PlayerCommand> commands{};
};

class CommandSyncController final {
public:
    CommandSyncController(std::uint8_t localPlayerId, std::int32_t totalPlayers, std::int32_t inputDelayTicks);

    void queueLocalCommand(
        std::int64_t currentTick,
        std::uint32_t entityId,
        logic::ecs::CommandType type,
        std::int32_t arg0,
        std::int32_t arg1,
        std::int32_t arg2);

    [[nodiscard]] std::vector<CommandFramePacket> drainOutgoingPackets();

    void receivePacket(const CommandFramePacket& packet);

    [[nodiscard]] bool collectCommandsForTick(
        std::int64_t tick,
        std::vector<logic::commands::PlayerCommand>& outCommands) const;

private:
    [[nodiscard]] std::int64_t scheduledTick(std::int64_t currentTick) const noexcept;

    std::uint8_t localPlayerId_{0};
    std::int32_t totalPlayers_{0};
    std::int32_t inputDelayTicks_{0};

    std::map<std::int64_t, std::vector<logic::commands::PlayerCommand>> localFrames_{};
    std::set<std::int64_t> sentTicks_{};

    std::map<std::int64_t, std::map<std::uint8_t, std::vector<logic::commands::PlayerCommand>>> receivedFrames_{};
};

}  // namespace tcp::net
