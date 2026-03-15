#include "UnitConfigLoader.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <string>

namespace tcp::data {

namespace {

bool extractInt(const std::string& json, const std::string& key, std::int32_t& out) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*(-?[0-9]+)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return false;
    }

    out = std::stoi(match[1].str());
    return true;
}

bool extractUnsigned(const std::string& json, const std::string& key, std::uint32_t& out) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*([0-9]+)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return false;
    }

    out = static_cast<std::uint32_t>(std::stoul(match[1].str()));
    return true;
}

bool extractString(const std::string& json, const std::string& key, std::string& out) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return false;
    }

    out = match[1].str();
    return true;
}

bool validateUnitConfig(const UnitConfig& config) {
    if (config.id == 0U || config.name.empty()) {
        return false;
    }

    if (config.costSun < 0 || config.maxHealth <= 0 || config.moveSpeedPerTick < 0) {
        return false;
    }

    if (config.weaponRangeCells < 0 || config.weaponDamage < 0 || config.weaponCooldownTicks < 0) {
        return false;
    }

    if (config.movementMode != "static" && config.movementMode != "ground") {
        return false;
    }

    return true;
}

}  // namespace

bool loadUnitConfigFromJsonFile(const std::string& path, UnitConfig& outConfig) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    const auto json = buffer.str();

    UnitConfig parsed{};
    if (!extractUnsigned(json, "id", parsed.id) ||
        !extractString(json, "name", parsed.name) ||
        !extractString(json, "movementMode", parsed.movementMode) ||
        !extractInt(json, "costSun", parsed.costSun) ||
        !extractInt(json, "maxHealth", parsed.maxHealth) ||
        !extractInt(json, "moveSpeedPerTick", parsed.moveSpeedPerTick) ||
        !extractInt(json, "weaponRangeCells", parsed.weaponRangeCells) ||
        !extractInt(json, "weaponDamage", parsed.weaponDamage) ||
        !extractInt(json, "weaponCooldownTicks", parsed.weaponCooldownTicks)) {
        return false;
    }

    if (!validateUnitConfig(parsed)) {
        return false;
    }

    outConfig = parsed;
    return true;
}

}  // namespace tcp::data
