#include "ReplayPlayer.h"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace tcp::logic::replay {

bool loadFromFile(const std::string& path, std::vector<commands::PlayerCommand>& outCommands) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    outCommands.clear();

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string part;
        std::vector<std::string> fields;
        while (std::getline(ss, part, ',')) {
            fields.push_back(part);
        }

        if (fields.size() != 7U) {
            return false;
        }

        commands::PlayerCommand command{};
        command.tick = static_cast<std::int64_t>(std::stoll(fields[0]));
        command.playerId = static_cast<std::uint8_t>(std::stoi(fields[1]));
        command.entityId = static_cast<std::uint32_t>(std::stoul(fields[2]));
        command.type = static_cast<ecs::CommandType>(std::stoi(fields[3]));
        command.arg0 = std::stoi(fields[4]);
        command.arg1 = std::stoi(fields[5]);
        command.arg2 = std::stoi(fields[6]);
        outCommands.push_back(command);
    }

    return true;
}

}  // namespace tcp::logic::replay
