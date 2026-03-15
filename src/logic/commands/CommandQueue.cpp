#include "CommandQueue.h"

#include <algorithm>

namespace tcp::logic::commands {

void CommandQueue::push(PlayerCommand command) {
    queued_.push_back(command);
}

std::size_t CommandQueue::size() const noexcept {
    return queued_.size();
}

bool CommandQueue::empty() const noexcept {
    return queued_.empty();
}

std::vector<PlayerCommand> CommandQueue::popForTick(std::int64_t tick) {
    std::vector<PlayerCommand> out;
    auto it = std::remove_if(queued_.begin(), queued_.end(), [&](const PlayerCommand& command) {
        if (command.tick == tick) {
            out.push_back(command);
            return true;
        }
        return false;
    });
    queued_.erase(it, queued_.end());

    std::sort(out.begin(), out.end(), [](const PlayerCommand& lhs, const PlayerCommand& rhs) {
        if (lhs.playerId != rhs.playerId) {
            return lhs.playerId < rhs.playerId;
        }
        if (lhs.entityId != rhs.entityId) {
            return lhs.entityId < rhs.entityId;
        }
        return static_cast<std::int32_t>(lhs.type) < static_cast<std::int32_t>(rhs.type);
    });
    return out;
}

void applyCommandsAtTick(ecs::World& world, const std::vector<PlayerCommand>& commands) {
    auto& buffers = world.mutableCommandBuffers();
    const auto& entities = world.entities();

    for (const auto& command : commands) {
        const auto exists = std::find(entities.begin(), entities.end(), command.entityId) != entities.end();
        if (!exists) {
            continue;
        }

        ecs::QueuedCommand queued{};
        queued.tick = command.tick;
        queued.playerId = command.playerId;
        queued.type = command.type;
        queued.arg0 = command.arg0;
        queued.arg1 = command.arg1;
        queued.arg2 = command.arg2;

        buffers[command.entityId].queued.push_back(queued);
    }
}

}  // namespace tcp::logic::commands
