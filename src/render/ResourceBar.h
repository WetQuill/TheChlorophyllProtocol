#pragma once

#include "UiConfig.h"
#include "FontCache.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>

namespace tcp::render {

class ResourceBar {
public:
    ResourceBar(const UiLayout& layout, const UiColors& colors);

    void update(std::int32_t sun, std::int32_t power, std::int32_t totalPowerConsumed);

    void draw(sf::RenderTarget& target, const UiScale& scale, FontCache& fonts);

    [[nodiscard]] bool containsPoint(sf::Vector2f uiCoords) const;

private:
    const UiLayout& layout_;
    const UiColors& colors_;

    sf::FloatRect bounds_{};

    std::int32_t sun_{0};
    std::int32_t power_{0};
    std::int32_t powerConsumed_{0};
    bool powerWarning_{false};
    float blinkTimer_{0.0f};

    sf::RectangleShape bgRect_;
    sf::CircleShape sunIcon_;
    sf::Text sunValue_;
    sf::RectangleShape powerBarBg_;
    sf::RectangleShape powerBarFill_;
    sf::Text powerLabel_;
};

}  // namespace tcp::render
