#pragma once

#include <array>
#include <cstddef>

namespace tcp::logic::ecs {

enum class SystemPhase : std::size_t {
    kInput = 0,
    kProduction,
    kPathfinding,
    kMovement,
    kCombat,
    kResource,
    kCleanup,
    kCount,
};

[[nodiscard]] constexpr std::size_t systemPhaseCount() noexcept {
    return static_cast<std::size_t>(SystemPhase::kCount);
}

[[nodiscard]] constexpr std::array<SystemPhase, systemPhaseCount()> orderedSystemPhases() noexcept {
    return {
        SystemPhase::kInput,
        SystemPhase::kProduction,
        SystemPhase::kPathfinding,
        SystemPhase::kMovement,
        SystemPhase::kCombat,
        SystemPhase::kResource,
        SystemPhase::kCleanup,
    };
}

[[nodiscard]] constexpr std::size_t systemPhaseIndex(SystemPhase phase) noexcept {
    return static_cast<std::size_t>(phase);
}

}  // namespace tcp::logic::ecs
