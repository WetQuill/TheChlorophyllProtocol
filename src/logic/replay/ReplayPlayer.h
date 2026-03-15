#pragma once

#include "../commands/PlayerCommand.h"

#include <string>
#include <vector>

namespace tcp::logic::replay {

[[nodiscard]] bool loadFromFile(const std::string& path, std::vector<commands::PlayerCommand>& outCommands);

}  // namespace tcp::logic::replay
