#pragma once

#include "../logic/core/SimulationConfig.h"
#include "../logic/core/TickScheduler.h"

#include <cstdint>
#include <functional>

namespace tcp::app {

struct FrameReport {
    std::int32_t ticksExecuted{0};
    std::int32_t interpolationPermille{0};
    bool catchUpLimited{false};
};

class GameLoop final {
public:
    explicit GameLoop(const tcp::logic::SimulationConfig& config) noexcept
        : scheduler_(config) {}

    [[nodiscard]] FrameReport frame(
        std::int64_t elapsedMicros,
        const std::function<void(std::int64_t)>& onLogicTick) noexcept {
        const auto summary = scheduler_.step(elapsedMicros, onLogicTick);
        FrameReport report{};
        report.ticksExecuted = summary.ticksExecuted;
        report.catchUpLimited = summary.catchUpLimited;

        if (summary.tickDurationMicros > 0) {
            auto alpha = (summary.remainingMicros * 1000) / summary.tickDurationMicros;
            if (alpha < 0) {
                alpha = 0;
            }
            if (alpha > 1000) {
                alpha = 1000;
            }
            report.interpolationPermille = static_cast<std::int32_t>(alpha);
        }

        return report;
    }

    [[nodiscard]] std::int64_t currentTick() const noexcept {
        return scheduler_.currentTick();
    }

    void reset() noexcept {
        scheduler_.reset();
    }

private:
    tcp::logic::TickScheduler scheduler_;
};

}  // namespace tcp::app
