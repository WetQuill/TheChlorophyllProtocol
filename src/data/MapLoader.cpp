#include "MapLoader.h"

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

bool extractString(const std::string& json, const std::string& key, std::string& out) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return false;
    }
    out = match[1].str();
    return true;
}

bool extractArrayContent(const std::string& json, const std::string& key, std::string& out) {
    const std::regex pattern("\\\"" + key + "\\\"\\s*:\\s*\\[([^\\]]*)\\]");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) {
        return false;
    }
    out = match[1].str();
    return true;
}

bool parseTileArray(const std::string& content,
                    std::int32_t expectedCount,
                    std::vector<logic::map::TileType>& outTiles) {
    outTiles.clear();
    outTiles.reserve(static_cast<std::size_t>(expectedCount));

    const std::regex numberPattern("[0-9]+");
    auto it = std::sregex_iterator(content.begin(), content.end(), numberPattern);
    const auto end = std::sregex_iterator();

    for (; it != end; ++it) {
        const auto val = static_cast<std::uint8_t>(std::stoi(it->str()));
        outTiles.push_back(static_cast<logic::map::TileType>(val));
    }

    return static_cast<std::int32_t>(outTiles.size()) == expectedCount;
}

bool parseSpawnPoints(const std::string& content,
                      std::vector<logic::map::SpawnPoint>& outPoints) {
    outPoints.clear();

    const std::regex objectPattern(
        "\\{[^}]*\"team\"\\s*:\\s*([0-9]+)[^}]*\"x\"\\s*:\\s*(-?[0-9]+)[^}]*\"y\"\\s*:\\s*(-?[0-9]+)[^}]*\\}");

    auto it = std::sregex_iterator(content.begin(), content.end(), objectPattern);
    const auto end = std::sregex_iterator();

    for (; it != end; ++it) {
        logic::map::SpawnPoint sp{};
        sp.teamId = static_cast<std::uint8_t>(std::stoi((*it)[1].str()));
        sp.x = std::stoi((*it)[2].str());
        sp.y = std::stoi((*it)[3].str());
        outPoints.push_back(sp);
    }

    return !outPoints.empty();
}

}  // namespace

bool loadMapFromJsonFile(const std::string& path, logic::map::Tilemap& outMap) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    const auto json = buffer.str();

    logic::map::Tilemap parsed{};

    std::string mapName;
    extractString(json, "map_name", mapName);

    if (!extractInt(json, "width", parsed.width) ||
        !extractInt(json, "height", parsed.height)) {
        return false;
    }

    if (parsed.width <= 0 || parsed.height <= 0) {
        return false;
    }

    const auto expectedTiles = static_cast<std::int32_t>(
        static_cast<std::int64_t>(parsed.width) * static_cast<std::int64_t>(parsed.height));

    std::string tilesContent;
    if (!extractArrayContent(json, "tiles", tilesContent)) {
        return false;
    }

    if (!parseTileArray(tilesContent, expectedTiles, parsed.tiles)) {
        return false;
    }

    parsed.occupancy.assign(static_cast<std::size_t>(expectedTiles), 0);

    std::string spawnContent;
    if (extractArrayContent(json, "spawn_points", spawnContent)) {
        parseSpawnPoints(spawnContent, parsed.spawnPoints);
    }

    outMap = std::move(parsed);
    return true;
}

}  // namespace tcp::data
