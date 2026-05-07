#pragma once

#include "../logic/map/Tilemap.h"

#include <string>

namespace tcp::data {

[[nodiscard]] bool loadMapFromJsonFile(const std::string& path, logic::map::Tilemap& outMap);

}  // namespace tcp::data
