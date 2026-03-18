#include "CommandSyncController.h"

#include <algorithm>

namespace tcp::net {

CommandSyncController::CommandSyncController(
    std::uint8_t localPlayerId,
    std::int32_t totalPlayers,
    std::int32_t inputDelayTicks)
    : localPlayerId_(localPlayerId),
      totalPlayers_(std::max<std::int32_t>(1, totalPlayers)),
      inputDelayTicks_(std::max<std::int32_t>(0, inputDelayTicks)) {}

void CommandSyncController::queueLocalCommand(
    std::int64_t currentTick,
    std::uint32_t entityId,
    logic::ecs::CommandType type,
    std::int32_t arg0,
    std::int32_t arg1,
    std::int32_t arg2) {
    logic::commands::PlayerCommand command{};
    command.tick = scheduledTick(currentTick);
    command.playerId = localPlayerId_;
    command.entityId = entityId;
    command.type = type;
    command.arg0 = arg0;
    command.arg1 = arg1;
    command.arg2 = arg2;

    localFrames_[command.tick].push_back(command);
}

std::vector<CommandFramePacket> CommandSyncController::drainOutgoingPackets() {
    std::vector<CommandFramePacket> packets;

    for (const auto& [tick, commands] : localFrames_) {
        if (sentTicks_.find(tick) != sentTicks_.end()) {
            continue;
        }

        CommandFramePacket packet{};
        packet.tick = tick;
        packet.playerId = localPlayerId_;
        packet.commands = commands;
        packets.push_back(packet);
        sentTicks_.insert(tick);
    }

    std::sort(packets.begin(), packets.end(), [](const CommandFramePacket& lhs, const CommandFramePacket& rhs) {
        return lhs.tick < rhs.tick;
    });

    return packets;
}

void CommandSyncController::receivePacket(const CommandFramePacket& packet) {
    auto& frame = receivedFrames_[packet.tick];
    frame[packet.playerId] = packet.commands;
}

bool CommandSyncController::collectCommandsForTick(
    std::int64_t tick,
    std::vector<logic::commands::PlayerCommand>& outCommands) const {
    const auto frameIt = receivedFrames_.find(tick);
    if (frameIt == receivedFrames_.end()) {
        return false;
    }

    const auto& byPlayer = frameIt->second;
    if (static_cast<std::int32_t>(byPlayer.size()) != totalPlayers_) {
        return false;
    }

    outCommands.clear();
    for (const auto& [playerId, commands] : byPlayer) {
        (void)playerId;
        outCommands.insert(outCommands.end(), commands.begin(), commands.end());
    }

    std::sort(outCommands.begin(), outCommands.end(), [](const logic::commands::PlayerCommand& lhs,
                                                         const logic::commands::PlayerCommand& rhs) {
        if (lhs.playerId != rhs.playerId) {
            return lhs.playerId < rhs.playerId;
        }
        if (lhs.entityId != rhs.entityId) {
            return lhs.entityId < rhs.entityId;
        }
        return static_cast<std::int32_t>(lhs.type) < static_cast<std::int32_t>(rhs.type);
    });

    return true;
}

std::int64_t CommandSyncController::scheduledTick(std::int64_t currentTick) const noexcept {
    return currentTick + inputDelayTicks_;
}

}  // namespace tcp::net
