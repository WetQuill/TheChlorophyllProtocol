#pragma once

#include "../commands/PlayerCommand.h"

#include <string>
#include <vector>

namespace tcp::logic::replay {

class ReplayRecorder final {
public:
    void record(const commands::PlayerCommand& command);
    [[nodiscard]] const std::vector<commands::PlayerCommand>& commands() const noexcept;

    [[nodiscard]] bool saveToFile(const std::string& path) const;

private:
    std::vector<commands::PlayerCommand> commands_{};
};

}  // namespace tcp::logic::replay
