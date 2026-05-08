#pragma once

#include "UiConfig.h"
#include "FontCache.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>

#include <cstdint>

namespace tcp::logic::ecs {
using EntityId = std::uint32_t;
}  // namespace tcp::logic::ecs

namespace tcp::render {

class UnitInfoPanel {
public:
    UnitInfoPanel(const UiLayout& layout, const UiColors& colors);

    void update(const tcp::logic::ecs::EntityId* selectedId,
                std::uint32_t archetypeId,
                std::int32_t hpCurrent,
                std::int32_t hpMax,
                bool isButterMode,
                bool hasArtillery,
                std::int32_t constructionRemainingTicks,
                std::int32_t constructionTotalTicks);

    void setVisible(bool visible) { visible_ = visible; }
    [[nodiscard]] bool isVisible() const noexcept { return visible_; }

    void draw(sf::RenderTarget& target, const UiScale& scale, FontCache& fonts);

    void handleResize(const sf::FloatRect& bounds);

    [[nodiscard]] bool containsPoint(sf::Vector2f uiCoords) const;
    [[nodiscard]] bool hitTestToggleButton(sf::Vector2f uiCoords) const;
    [[nodiscard]] bool isToggleButtonVisible() const noexcept { return showToggle_; }

private:
    const UiLayout& layout_;
    const UiColors& colors_;

    sf::FloatRect bounds_{};
    bool visible_{false};

    std::uint32_t archetypeId_{0};
    std::int32_t hpCurrent_{0};
    std::int32_t hpMax_{1};
    bool isButterMode_{false};
    bool showToggle_{false};
    bool showConstruction_{false};
    std::int32_t constructionRemaining_{0};
    std::int32_t constructionTotal_{1};

    sf::RectangleShape bg_;
    sf::Text entityName_;
    sf::Text hpLabel_;

    sf::RectangleShape hpBg_;
    sf::RectangleShape hpFill_;
    sf::RectangleShape hpBorder_;

    sf::RectangleShape constructionBg_;
    sf::RectangleShape constructionFill_;
    sf::Text constructionLabel_;

    sf::FloatRect toggleButtonRect_{};
    sf::RectangleShape toggleButtonBg_;
    sf::Text toggleButtonText_;
};

}  // namespace tcp::render
