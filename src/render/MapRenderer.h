#pragma once

#include <cstdint>
#include <vector>

#if __has_include(<SFML/Graphics/Vertex.hpp>)
#include <SFML/Graphics/Vertex.hpp>
#elif __has_include(<SFML/Graphics/Vertex.h>)
#include <SFML/Graphics/Vertex.h>
#endif

#if __has_include(<SFML/Graphics/RenderTarget.hpp>)
#include <SFML/Graphics/RenderTarget.hpp>
#elif __has_include(<SFML/Graphics/RenderTarget.h>)
#include <SFML/Graphics/RenderTarget.h>
#endif

namespace tcp::logic::map {
struct Tilemap;
struct FogOfWar;
enum class TileType : std::uint8_t;
}  // namespace tcp::logic::map

namespace tcp::render {

class MapRenderer {
public:
    MapRenderer() = default;

    void rebuildTerrain(const logic::map::Tilemap& tilemap, float halfW, float halfH);

    void drawTerrain(sf::RenderTarget& target) const;

    void drawFogOfWar(sf::RenderTarget& target,
                      const logic::map::FogOfWar& fog,
                      float halfW,
                      float halfH) const;

    void drawGrid(sf::RenderTarget& target,
                  std::int32_t gridMinX,
                  std::int32_t gridMaxX,
                  std::int32_t gridMinY,
                  std::int32_t gridMaxY,
                  float halfW,
                  float halfH) const;

private:
    std::vector<sf::Vertex> terrainVertices_{};
};

}  // namespace tcp::render
