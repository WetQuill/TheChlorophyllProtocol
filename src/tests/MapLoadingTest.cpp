#include "../data/MapLoader.h"
#include "../logic/map/Tilemap.h"

#include <cstdint>
#include <cstdio>
#include <string>

namespace {

bool testLoadMapFromJson() {
    tcp::logic::map::Tilemap map;
    const bool ok = tcp::data::loadMapFromJsonFile(
        "assets/data/maps/frontline_grave.json", map);
    if (!ok) return false;
    if (map.width != 128) return false;
    if (map.height != 64) return false;
    if (static_cast<std::int32_t>(map.tiles.size()) != map.width * map.height) return false;
    if (static_cast<std::int32_t>(map.occupancy.size()) != map.width * map.height) return false;
    if (map.spawnPoints.size() < 2) return false;
    return true;
}

bool testTileAccess() {
    tcp::logic::map::Tilemap map;
    map.width = 10;
    map.height = 10;
    map.tiles.assign(100, tcp::logic::map::TileType::Grass);
    map.occupancy.assign(100, 0);

    if (!map.isInBounds(0, 0)) return false;
    if (!map.isInBounds(9, 9)) return false;
    if (map.isInBounds(-1, 0)) return false;
    if (map.isInBounds(0, 10)) return false;
    if (map.isInBounds(10, 0)) return false;

    if (map.tileAt(0, 0) != tcp::logic::map::TileType::Grass) return false;
    if (map.tileAt(-1, 0) != tcp::logic::map::TileType::Obstacle) return false;

    return true;
}

bool testOccupancy() {
    tcp::logic::map::Tilemap map;
    map.width = 5;
    map.height = 5;
    map.tiles.assign(25, tcp::logic::map::TileType::Grass);
    map.occupancy.assign(25, 0);

    if (map.entityAt(2, 2) != 0) return false;

    map.setOccupancy(2, 2, 42);
    if (map.entityAt(2, 2) != 42) return false;
    if (!map.isWalkable(0, 0)) return false;
    if (map.isWalkable(2, 2)) return false;

    map.clearOccupancy(2, 2);
    if (map.entityAt(2, 2) != 0) return false;
    if (!map.isWalkable(2, 2)) return false;

    map.setOccupancy(0, 0, 1);
    map.setOccupancy(1, 1, 2);
    map.clearAllOccupancy();
    if (map.entityAt(0, 0) != 0) return false;
    if (map.entityAt(1, 1) != 0) return false;

    return true;
}

bool testWalkability() {
    tcp::logic::map::Tilemap map;
    map.width = 5;
    map.height = 5;
    map.tiles.assign(25, tcp::logic::map::TileType::Grass);
    map.occupancy.assign(25, 0);

    map.tiles[map.indexOf(3, 3)] = tcp::logic::map::TileType::Obstacle;
    if (map.isWalkable(3, 3)) return false;

    map.setOccupancy(1, 0, 99);
    if (map.isWalkable(1, 0)) return false;

    if (map.isWalkable(-1, 0)) return false;
    if (map.isWalkable(0, 10)) return false;

    return true;
}

bool testMapLoadFailure() {
    tcp::logic::map::Tilemap map;
    if (tcp::data::loadMapFromJsonFile("nonexistent_file.json", map)) return false;
    return true;
}

}  // namespace

int main() {
    unsigned passed = 0;
    unsigned failed = 0;

    const auto run = [&](const char* name, bool (*fn)()) {
        if (fn()) {
            ++passed;
        } else {
            ++failed;
            std::printf("  FAILED: %s\n", name);
        }
    };

    run("LoadMapFromJson", testLoadMapFromJson);
    run("TileAccess", testTileAccess);
    run("Occupancy", testOccupancy);
    run("Walkability", testWalkability);
    run("MapLoadFailure", testMapLoadFailure);

    std::printf("MapLoading: %u passed, %u failed\n", passed, failed);
    return (failed > 0) ? 1 : 0;
}
