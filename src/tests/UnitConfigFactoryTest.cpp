#include "../data/UnitConfigLoader.h"
#include "../logic/factory/UnitFactory.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool verify(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

bool pathExists(const std::string& path) {
    std::ifstream in(path);
    return in.good();
}

std::string locateUnitConfigPath(const std::string& fileName) {
    const std::vector<std::string> candidates = {
        std::string("assets/data/units/") + fileName,
        std::string("../assets/data/units/") + fileName,
        std::string("../../assets/data/units/") + fileName,
    };

    for (const auto& path : candidates) {
        if (pathExists(path)) {
            return path;
        }
    }

    return {};
}

}  // namespace

int main() {
    bool ok = true;

    const auto peaPath = locateUnitConfigPath("pea_militia.json");
    const auto flowerPath = locateUnitConfigPath("sunflower_generator.json");

    ok &= verify(!peaPath.empty(), "failed to locate pea_militia.json");
    ok &= verify(!flowerPath.empty(), "failed to locate sunflower_generator.json");

    tcp::data::UnitConfig pea{};
    tcp::data::UnitConfig sunflower{};
    ok &= verify(tcp::data::loadUnitConfigFromJsonFile(peaPath, pea), "failed to load pea config");
    ok &= verify(tcp::data::loadUnitConfigFromJsonFile(flowerPath, sunflower), "failed to load sunflower config");

    ok &= verify(pea.name == "PeaMilitia", "pea config name mismatch");
    ok &= verify(pea.weaponDamage == 6, "pea config damage mismatch");
    ok &= verify(sunflower.movementMode == "static", "sunflower movement mode mismatch");

    tcp::logic::ecs::World world;
    const auto spawnedPea = tcp::logic::factory::spawnUnit(world, pea, 0, tcp::logic::ecs::Transform{});
    const auto spawnedFlower = tcp::logic::factory::spawnUnit(world, sunflower, 1, tcp::logic::ecs::Transform{});

    ok &= verify(world.entityCount() == 2U, "factory should spawn two entities");

    const auto peaIdentityIt = world.identities().find(spawnedPea);
    const auto flowerIdentityIt = world.identities().find(spawnedFlower);
    ok &= verify(peaIdentityIt != world.identities().end(), "pea identity not assigned");
    ok &= verify(flowerIdentityIt != world.identities().end(), "flower identity not assigned");

    if (peaIdentityIt != world.identities().end()) {
        ok &= verify(peaIdentityIt->second.archetypeId == 101U, "pea archetype id mismatch");
    }
    if (flowerIdentityIt != world.identities().end()) {
        ok &= verify(flowerIdentityIt->second.archetypeId == 201U, "flower archetype id mismatch");
    }

    if (!ok) {
        return 1;
    }

    std::cout << "Unit config + factory tests passed" << '\n';
    return 0;
}
