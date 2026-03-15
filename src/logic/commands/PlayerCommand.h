#pragma once

#include "../ecs/components/Components.h"

#include <cstdint>

namespace tcp::logic::commands {

struct PlayerCommand {
    std::int64_t tick{0};
    std::uint8_t playerId{0};
    std::uint32_t entityId{0};
    ecs::CommandType type{ecs::CommandType::kStop};
    std::int32_t arg0{0};
    std::int32_t arg1{0};
    std::int32_t arg2{0};

    [[nodiscard]] bool operator==(const PlayerCommand& rhs) const noexcept {
        return tick == rhs.tick &&
               playerId == rhs.playerId &&
               entityId == rhs.entityId &&
               type == rhs.type &&
               arg0 == rhs.arg0 &&
               arg1 == rhs.arg1 &&
               arg2 == rhs.arg2;
    }
};

}  // namespace tcp::logic::commands
