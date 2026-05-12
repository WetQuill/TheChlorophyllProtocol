#pragma once

#include <cstdint>
#include <vector>

#include "../../math/FixedPoint.h"

namespace tcp::logic::ecs {

struct Transform {
    math::FixedPoint x{};
    math::FixedPoint y{};
};

struct Velocity {
    math::FixedPoint xPerTick{};
    math::FixedPoint yPerTick{};
};

// Health(current, max)
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

struct ArtilleryWeapon {
    math::FixedPoint minRange{};
    math::FixedPoint maxRange{};
    math::FixedPoint aoeRadius{};
    std::int32_t damage{0};
    std::int32_t reloadTicks{0};
    std::int32_t remainingCooldownTicks{0};
    std::int32_t projectileTravelTicks{0};
    std::int32_t frozenTicks{0};
    bool isButterMode{false};
};

struct BallisticProjectile {
    std::uint8_t sourceTeam{0};
    std::int32_t damage{0};
    math::FixedPoint aoeRadius{};
    std::int32_t frozenTicks{0};
    std::int32_t remainingTicks{0};
    math::FixedPoint targetX{};
    math::FixedPoint targetY{};
};

struct Armor {
    std::int32_t value{0};
};

struct FrozenState {
    std::int32_t remainingTicks{0};
    std::int32_t slowPermille{1000};
};

struct FlashComponent {
    std::int32_t ticksLeft{0};
};

struct ExplosionEffect {
    int currentRadius{0};
    int maxRadius{150};
    int alpha{255};
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
    std::int32_t cycleTicks{0};
    std::int32_t maxCycleTicks{0};
};

struct Headquarters {
    bool value{true};
};

struct Building {
    bool blocksMovement{true};
};

enum class ZombieState : std::uint8_t {
    IDLE = 0,
    MOVE_TO_HQ = 1,
    ATTACK_OBSTACLE = 2,
};

struct ZombieAIFSM {
    ZombieState currentState{ZombieState::IDLE};
    std::uint32_t targetPlant{0};
    std::int32_t pathUpdateTimer{0};
};

struct GridTarget {
    std::int32_t x{0};
    std::int32_t y{0};
};

enum class CommandType : std::uint8_t {
    kBuild = 0,
    kMove = 1,
    kAttack = 2,
    kStop = 3,
    kBuildSunPowerPlant = 4,
    kProducePea = 5,
    kBuildCornCannonBastion = 6,
    kToggleButterMode = 7,
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
