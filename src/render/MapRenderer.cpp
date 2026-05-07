#include "MapRenderer.h"

#include "../logic/map/FogOfWar.h"
#include "../logic/map/Tilemap.h"

#include <algorithm>

namespace tcp::render {

namespace {

sf::Color colorForTile(logic::map::TileType type) {
    switch (type) {
        case logic::map::TileType::Water:
            return sf::Color(30U, 144U, 255U);
        case logic::map::TileType::Obstacle:
            return sf::Color(139U, 90U, 43U);
        case logic::map::TileType::Grass:
        default:
            return sf::Color(34U, 139U, 34U);
    }
}

sf::Vector2f gridToScreen(std::int32_t gx, std::int32_t gy, float halfW, float halfH) {
    return sf::Vector2f{
        static_cast<float>(gx - gy) * halfW,
        static_cast<float>(gx + gy) * halfH,
    };
}

}  // namespace

void MapRenderer::rebuildTerrain(const logic::map::Tilemap& tilemap, float halfW, float halfH) {
    terrainVertices_.clear();
    const auto tileCount = static_cast<std::size_t>(tilemap.width) * static_cast<std::size_t>(tilemap.height);
    terrainVertices_.reserve(tileCount * 4U);

    for (std::int32_t gy = 0; gy < tilemap.height; ++gy) {
        for (std::int32_t gx = 0; gx < tilemap.width; ++gx) {
            const float cx = static_cast<float>(gx - gy) * halfW;
            const float cy = static_cast<float>(gx + gy) * halfH;

            const sf::Color color = colorForTile(tilemap.tileAt(gx, gy));
            const sf::Color darkColor(
                static_cast<std::uint8_t>(color.r * 3U / 4U),
                static_cast<std::uint8_t>(color.g * 3U / 4U),
                static_cast<std::uint8_t>(color.b * 3U / 4U));

            // Diamond quad: top, right, bottom, left
            terrainVertices_.emplace_back(sf::Vector2f{cx, cy - halfH}, color);       // top
            terrainVertices_.emplace_back(sf::Vector2f{cx + halfW, cy}, darkColor);   // right
            terrainVertices_.emplace_back(sf::Vector2f{cx, cy + halfH}, color);       // bottom
            terrainVertices_.emplace_back(sf::Vector2f{cx - halfW, cy}, darkColor);   // left
        }
    }
}

void MapRenderer::drawTerrain(sf::RenderTarget& target) const {
    if (terrainVertices_.empty()) {
        return;
    }
    target.draw(terrainVertices_.data(), terrainVertices_.size(), sf::Quads);
}

void MapRenderer::drawFogOfWar(sf::RenderTarget& target,
                               const logic::map::FogOfWar& fog,
                               float halfW,
                               float halfH) const {
    if (fog.width <= 0 || fog.height <= 0) {
        return;
    }

    const sf::Color unexploredColor(0U, 0U, 0U, 200U);
    const sf::Color foggedColor(20U, 20U, 20U, 120U);

    std::vector<sf::Vertex> fogVertices;
    fogVertices.reserve(static_cast<std::size_t>(fog.width * fog.height) * 4U);

    for (std::int32_t gy = 0; gy < fog.height; ++gy) {
        for (std::int32_t gx = 0; gx < fog.width; ++gx) {
            const auto vis = fog.at(gx, gy);
            if (vis == logic::map::Visibility::Visible) {
                continue;
            }

            const float cx = static_cast<float>(gx - gy) * halfW;
            const float cy = static_cast<float>(gx + gy) * halfH;
            const sf::Color color = (vis == logic::map::Visibility::Unexplored) ? unexploredColor : foggedColor;

            fogVertices.emplace_back(sf::Vector2f{cx, cy - halfH}, color);
            fogVertices.emplace_back(sf::Vector2f{cx + halfW, cy}, color);
            fogVertices.emplace_back(sf::Vector2f{cx, cy + halfH}, color);
            fogVertices.emplace_back(sf::Vector2f{cx - halfW, cy}, color);
        }
    }

    if (!fogVertices.empty()) {
        target.draw(fogVertices.data(), fogVertices.size(), sf::Quads);
    }
}

void MapRenderer::drawGrid(sf::RenderTarget& target,
                           std::int32_t gridMinX,
                           std::int32_t gridMaxX,
                           std::int32_t gridMinY,
                           std::int32_t gridMaxY,
                           float halfW,
                           float halfH) const {
    const sf::Color gridColor(255U, 255U, 255U, 30U);

    for (std::int32_t gx = gridMinX; gx <= gridMaxX; ++gx) {
        const sf::Vector2f p1 = gridToScreen(gx, gridMinY, halfW, halfH);
        const sf::Vector2f p2 = gridToScreen(gx, gridMaxY, halfW, halfH);
        const sf::Vertex line[] = {
            sf::Vertex(p1, gridColor),
            sf::Vertex(p2, gridColor),
        };
        target.draw(line, 2U, sf::Lines);
    }
    for (std::int32_t gy = gridMinY; gy <= gridMaxY; ++gy) {
        const sf::Vector2f p1 = gridToScreen(gridMinX, gy, halfW, halfH);
        const sf::Vector2f p2 = gridToScreen(gridMaxX, gy, halfW, halfH);
        const sf::Vertex line[] = {
            sf::Vertex(p1, gridColor),
            sf::Vertex(p2, gridColor),
        };
        target.draw(line, 2U, sf::Lines);
    }
}

}  // namespace tcp::render
