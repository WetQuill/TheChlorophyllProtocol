#pragma once

#include "UnitConfig.h"

#include <string>

namespace tcp::data {

[[nodiscard]] bool loadUnitConfigFromJsonFile(const std::string& path, UnitConfig& outConfig);

}  // namespace tcp::data
