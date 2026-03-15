#pragma once

#include "../../math/FixedPoint.h"

#include <cstdint>
#include <vector>

namespace tcp::logic::ecs {

struct Transform {
    math::FixedPoint x{};
    math::FixedPoint y{};
};

struct Velocity {
    math::FixedPoint xPerTick{};
    math::FixedPoint yPerTick{};
};

struct Health {
    std::int32_t current{0};
    std::int32_t max{0};
};

struct Team {
    std::uint8_t value{0};
};

struct Identity {
    std::uint32_t archetypeId{0};
    std::uint16_t level{1};
};

struct Production {
    std::int32_t costSun{0};
    std::int32_t buildTicks{0};
    std::int32_t progressTicks{0};
    std::uint32_t producedArchetypeId{0};
    std::int32_t producedHealth{0};
};

struct Weapon {
    math::FixedPoint range{};
    std::int32_t damage{0};
    std::int32_t cooldownTicks{0};
    std::int32_t remainingCooldownTicks{0};
};

struct Vision {
    std::int32_t radiusCells{0};
};

struct PowerConsumer {
    std::int32_t requiredPower{0};
    bool enabled{true};
};

struct SunProducer {
    std::int32_t amountPerTick{0};
};

struct Headquarters {
    bool value{true};
};

enum class CommandType : std::uint8_t {
    kBuild = 0,
    kMove = 1,
    kAttack = 2,
    kStop = 3,
};

struct QueuedCommand {
    std::int64_t tick{0};
    std::uint8_t playerId{0};
    CommandType type{CommandType::kStop};
    std::int32_t arg0{0};
    std::int32_t arg1{0};
    std::int32_t arg2{0};
};

struct CommandBuffer {
    std::vector<QueuedCommand> queued{};
};

}  // namespace tcp::logic::ecs
