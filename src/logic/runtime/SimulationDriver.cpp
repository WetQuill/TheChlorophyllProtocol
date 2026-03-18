#include "SimulationDriver.h"

#include <utility>

namespace tcp::logic::runtime {

SimulationDriver::SimulationDriver(ecs::World world)
    : world_(std::move(world)) {}

void SimulationDriver::useSingleLocalMode() {
    mode_ = DriverMode::kSingleLocal;
    lockstepConfigured_ = false;
}

bool SimulationDriver::useReplayModeFromFile(const std::string& replayPath) {
    std::vector<commands::PlayerCommand> loaded;
    if (!replay::loadFromFile(replayPath, loaded)) {
        return false;
    }

    queue_ = commands::CommandQueue{};
    for (const auto& command : loaded) {
        queue_.push(command);
    }

    mode_ = DriverMode::kReplay;
    lockstepConfigured_ = false;
    return true;
}

void SimulationDriver::useLockstepMode(const LockstepConfig& config) {
    lockstepSync_ = net::CommandSyncController(
        config.localPlayerId,
        config.totalPlayers,
        config.inputDelayTicks);
    lockstepConfigured_ = true;
    mode_ = DriverMode::kLockstep;
}

DriverMode SimulationDriver::mode() const noexcept {
    return mode_;
}

void SimulationDriver::queueLocalCommand(
    std::uint8_t playerId,
    std::uint32_t entityId,
    ecs::CommandType type,
    std::int32_t arg0,
    std::int32_t arg1,
    std::int32_t arg2) {
    if (mode_ == DriverMode::kLockstep && lockstepConfigured_) {
        lockstepSync_.queueLocalCommand(world_.currentTick(), entityId, type, arg0, arg1, arg2);
        return;
    }

    if (mode_ == DriverMode::kReplay) {
        return;
    }

    commands::PlayerCommand command{};
    command.tick = world_.currentTick();
    command.playerId = playerId;
    command.entityId = entityId;
    command.type = type;
    command.arg0 = arg0;
    command.arg1 = arg1;
    command.arg2 = arg2;
    queue_.push(command);
}

std::vector<net::CommandFramePacket> SimulationDriver::drainOutgoingPackets() {
    if (mode_ == DriverMode::kLockstep && lockstepConfigured_) {
        return lockstepSync_.drainOutgoingPackets();
    }
    return {};
}

void SimulationDriver::receivePacket(const net::CommandFramePacket& packet) {
    if (mode_ == DriverMode::kLockstep && lockstepConfigured_) {
        lockstepSync_.receivePacket(packet);
    }
}

bool SimulationDriver::stepTick() {
    std::vector<commands::PlayerCommand> commandsToApply;
    if (!gatherCommandsForCurrentTick(commandsToApply)) {
        return false;
    }

    commands::applyCommandsAtTick(world_, commandsToApply);
    for (const auto& command : commandsToApply) {
        recorder_.record(command);
    }

    world_.tick();
    debug::updateDeterminismSnapshot(world_);
    return true;
}

ecs::World& SimulationDriver::world() noexcept {
    return world_;
}

const ecs::World& SimulationDriver::world() const noexcept {
    return world_;
}

bool SimulationDriver::saveReplay(const std::string& replayPath) const {
    return recorder_.saveToFile(replayPath);
}

bool SimulationDriver::gatherCommandsForCurrentTick(std::vector<commands::PlayerCommand>& out) {
    if (mode_ == DriverMode::kLockstep) {
        if (!lockstepConfigured_) {
            return false;
        }
        return lockstepSync_.collectCommandsForTick(world_.currentTick(), out);
    }

    out = queue_.popForTick(world_.currentTick());
    return true;
}

}  // namespace tcp::logic::runtime
