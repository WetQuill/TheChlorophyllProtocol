#include "UnitInfoPanel.h"

#include <string>

namespace tcp::render {

namespace {
    constexpr std::uint32_t kBastionArchetypeId = 903U;

    const char* archetypeName(std::uint32_t id) {
        switch (id) {
        case 101U: return "Pea Militia";
        case 201U: return "Sunflower Generator";
        case 901U: return "Pea Military Camp";
        case 902U: return "Sun Power Plant";
        case 903U: return "Corn Cannon Bastion";
        default:   return "Unknown Unit";
        }
    }
}  // namespace

UnitInfoPanel::UnitInfoPanel(const UiLayout& layout, const UiColors& colors)
    : layout_(layout)
    , colors_(colors) {

    bg_.setFillColor(colors_.panelBg);
    bg_.setOutlineColor(colors_.panelBorder);
    bg_.setOutlineThickness(2.0f);

    entityName_.setFillColor(colors_.textLight);
    hpLabel_.setFillColor(colors_.textLight);

    hpBg_.setFillColor(colors_.hpBackground);
    hpBg_.setOutlineColor(colors_.hpBorder);
    hpBg_.setOutlineThickness(1.0f);

    hpFill_.setFillColor(colors_.hpFill);
    hpBorder_.setFillColor(sf::Color::Transparent);
    hpBorder_.setOutlineColor(colors_.hpBorder);
    hpBorder_.setOutlineThickness(2.0f);

    constructionBg_.setFillColor(sf::Color(35, 30, 12, 220));
    constructionBg_.setOutlineColor(colors_.panelBorder);
    constructionBg_.setOutlineThickness(1.0f);
    constructionFill_.setFillColor(sf::Color(220, 190, 60, 255));
    constructionLabel_.setFillColor(colors_.textLight);

    toggleButtonBg_.setOutlineThickness(1.0f);
    toggleButtonText_.setFillColor(colors_.textDark);
}

void UnitInfoPanel::update(const tcp::logic::ecs::EntityId* /*selectedId*/,
                            std::uint32_t archetypeId,
                            std::int32_t hpCurrent,
                            std::int32_t hpMax,
                            bool isButterMode,
                            bool hasArtillery,
                            std::int32_t constructionRemainingTicks,
                            std::int32_t constructionTotalTicks) {
    archetypeId_ = archetypeId;
    hpCurrent_ = hpCurrent;
    hpMax_ = hpMax;
    isButterMode_ = isButterMode;
    showToggle_ = hasArtillery;
    constructionRemaining_ = constructionRemainingTicks;
    constructionTotal_ = constructionTotalTicks;
    showConstruction_ = (constructionRemainingTicks > 0 && constructionTotalTicks > 0);
}

void UnitInfoPanel::handleResize(const sf::FloatRect& bounds) {
    bounds_ = bounds;
}

void UnitInfoPanel::draw(sf::RenderTarget& target, const UiScale& scale, FontCache& fonts) {
    if (!visible_) return;

    bg_.setPosition(bounds_.left, bounds_.top);
    bg_.setSize(sf::Vector2f(bounds_.width, bounds_.height));
    bg_.setOutlineThickness(scale.borderThickness);
    target.draw(bg_);

    const float pad = 10.0f * scale.dpiScale;
    float x = bounds_.left + pad;
    float y = bounds_.top + pad;

    // Entity name
    if (fonts.loaded()) {
        entityName_.setFont(fonts.monoFont());
        entityName_.setCharacterSize(static_cast<unsigned int>(scale.fontSizeRegular));
    }
    entityName_.setString(archetypeName(archetypeId_));
    entityName_.setPosition(x, y);
    target.draw(entityName_);

    y += scale.fontSizeRegular + 6.0f * scale.dpiScale;

    // HP bar
    const float hpBarWidth = bounds_.width * 0.55f;
    const float hpBarHeight = 12.0f * scale.dpiScale;

    hpBg_.setPosition(x, y);
    hpBg_.setSize(sf::Vector2f(hpBarWidth, hpBarHeight));
    target.draw(hpBg_);

    if (hpMax_ > 0) {
        float ratio = std::min(1.0f, static_cast<float>(hpCurrent_) / static_cast<float>(hpMax_));
        hpFill_.setPosition(x + 1.0f, y + 1.0f);
        hpFill_.setSize(sf::Vector2f((hpBarWidth - 2.0f) * ratio, hpBarHeight - 2.0f));
        target.draw(hpFill_);

        hpBorder_.setPosition(x, y);
        hpBorder_.setSize(sf::Vector2f(hpBarWidth, hpBarHeight));
        target.draw(hpBorder_);
    }

    // HP text
    if (fonts.loaded()) {
        hpLabel_.setFont(fonts.monoFont());
        hpLabel_.setCharacterSize(static_cast<unsigned int>(scale.fontSizeSmall));
    }
    hpLabel_.setString(std::to_string(hpCurrent_) + " / " + std::to_string(hpMax_));
    hpLabel_.setPosition(x + hpBarWidth + 10.0f * scale.dpiScale, y - 2.0f);
    hpLabel_.setFillColor(colors_.textLight);
    target.draw(hpLabel_);

    // Construction progress bar
    if (showConstruction_) {
        y += hpBarHeight + 8.0f * scale.dpiScale;
        const float conBarWidth = hpBarWidth;

        constructionBg_.setPosition(x, y);
        constructionBg_.setSize(sf::Vector2f(conBarWidth, hpBarHeight));
        target.draw(constructionBg_);

        float conRatio = std::min(1.0f,
            static_cast<float>(constructionTotal_ - constructionRemaining_) /
            static_cast<float>(constructionTotal_));
        constructionFill_.setPosition(x + 1.0f, y + 1.0f);
        constructionFill_.setSize(sf::Vector2f((conBarWidth - 2.0f) * conRatio, hpBarHeight - 2.0f));
        target.draw(constructionFill_);

        if (fonts.loaded()) {
            constructionLabel_.setFont(fonts.monoFont());
            constructionLabel_.setCharacterSize(static_cast<unsigned int>(scale.fontSizeSmall));
        }
        constructionLabel_.setString("BUILD " + std::to_string(constructionRemaining_) + "t");
        constructionLabel_.setPosition(x + conBarWidth + 10.0f * scale.dpiScale, y - 2.0f);
        target.draw(constructionLabel_);

        y += hpBarHeight + 4.0f * scale.dpiScale;
    } else {
        y += hpBarHeight + 4.0f * scale.dpiScale;
    }

    // Toggle button (ammo mode)
    if (showToggle_) {
        const float btnWidth = 140.0f * scale.dpiScale;
        const float btnHeight = 24.0f * scale.dpiScale;
        const float btnX = bounds_.left + bounds_.width - btnWidth - pad;
        const float btnY = bounds_.top + pad;

        toggleButtonRect_ = sf::FloatRect(btnX, btnY, btnWidth, btnHeight);

        toggleButtonBg_.setPosition(btnX, btnY);
        toggleButtonBg_.setSize(sf::Vector2f(btnWidth, btnHeight));
        toggleButtonBg_.setFillColor(isButterMode_ ? colors_.ammoButter : colors_.ammoHE);
        toggleButtonBg_.setOutlineColor(colors_.hpBorder);
        target.draw(toggleButtonBg_);

        if (fonts.loaded()) {
            toggleButtonText_.setFont(fonts.monoFont());
            toggleButtonText_.setCharacterSize(static_cast<unsigned int>(scale.fontSizeSmall));
        }
        toggleButtonText_.setString(isButterMode_ ? "AMMO: BUTTER" : "AMMO: HE");
        toggleButtonText_.setFillColor(colors_.textDark);
        const auto tb = toggleButtonText_.getLocalBounds();
        toggleButtonText_.setPosition(
            btnX + (btnWidth - tb.width) * 0.5f,
            btnY + (btnHeight - tb.height) * 0.5f - tb.top);
        target.draw(toggleButtonText_);
    }
}

bool UnitInfoPanel::containsPoint(sf::Vector2f uiCoords) const {
    if (!visible_) return false;
    return bounds_.contains(uiCoords);
}

bool UnitInfoPanel::hitTestToggleButton(sf::Vector2f uiCoords) const {
    if (!visible_ || !showToggle_) return false;
    return toggleButtonRect_.contains(uiCoords);
}

}  // namespace tcp::render
