#pragma once

#include "PlayerCommand.h"
#include "../ecs/World.h"

#include <cstdint>
#include <vector>

namespace tcp::logic::commands {

class CommandQueue final {
public:
    void push(PlayerCommand command);
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] std::vector<PlayerCommand> popForTick(std::int64_t tick);

private:
    std::vector<PlayerCommand> queued_{};
};

void applyCommandsAtTick(ecs::World& world, const std::vector<PlayerCommand>& commands);

}  // namespace tcp::logic::commands
