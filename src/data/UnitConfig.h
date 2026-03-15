#pragma once

#include <cstdint>
#include <string>

namespace tcp::data {

struct UnitConfig {
    std::uint32_t id{0};
    std::string name{};
    std::string movementMode{"static"};
    std::int32_t costSun{0};
    std::int32_t maxHealth{0};
    std::int32_t moveSpeedPerTick{0};
    std::int32_t weaponRangeCells{0};
    std::int32_t weaponDamage{0};
    std::int32_t weaponCooldownTicks{0};
};

}  // namespace tcp::data
