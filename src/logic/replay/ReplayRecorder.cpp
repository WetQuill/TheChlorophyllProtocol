#include "ReplayRecorder.h"

#include <cstdint>
#include <fstream>

namespace tcp::logic::replay {

void ReplayRecorder::record(const commands::PlayerCommand& command) {
    commands_.push_back(command);
}

const std::vector<commands::PlayerCommand>& ReplayRecorder::commands() const noexcept {
    return commands_;
}

bool ReplayRecorder::saveToFile(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    for (const auto& command : commands_) {
        out << command.tick << ','
            << static_cast<std::int32_t>(command.playerId) << ','
            << command.entityId << ','
            << static_cast<std::int32_t>(command.type) << ','
            << command.arg0 << ','
            << command.arg1 << ','
            << command.arg2 << '\n';
    }

    return out.good();
}

}  // namespace tcp::logic::replay
