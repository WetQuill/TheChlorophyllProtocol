#include "ResourceBar.h"

#include <cmath>
#include <string>

namespace tcp::render {

ResourceBar::ResourceBar(const UiLayout& layout, const UiColors& colors)
    : layout_(layout)
    , colors_(colors) {

    bgRect_.setFillColor(colors_.panelBg);
    bgRect_.setOutlineColor(colors_.panelBorder);
    bgRect_.setOutlineThickness(2.0f);

    sunIcon_.setPointCount(8);
    sunIcon_.setFillColor(colors_.sunGood);
    sunIcon_.setRadius(8.0f);

    sunValue_.setFillColor(colors_.textLight);
    sunValue_.setString("0");

    powerBarBg_.setFillColor(sf::Color(35, 12, 12, 220));
    powerBarBg_.setOutlineColor(colors_.panelBorder);
    powerBarBg_.setOutlineThickness(1.0f);

    powerBarFill_.setFillColor(colors_.powerGood);

    powerLabel_.setFillColor(colors_.textLight);
    powerLabel_.setString("PWR");
}

void ResourceBar::update(std::int32_t sun, std::int32_t power, std::int32_t totalPowerConsumed) {
    sun_ = sun;
    power_ = power;
    powerConsumed_ = totalPowerConsumed;
    powerWarning_ = (power < totalPowerConsumed);
}

void ResourceBar::draw(sf::RenderTarget& target, const UiScale& scale, FontCache& fonts) {
    const float margin = layout_.edgeMargin;
    const float barHeight = layout_.resourceBarHeight;
    const float barWidth = layout_.resourceBarWidth;
    const float border = scale.borderThickness;

    bounds_ = sf::FloatRect(margin, margin, barWidth, barHeight);

    bgRect_.setPosition(bounds_.left, bounds_.top);
    bgRect_.setSize(sf::Vector2f(bounds_.width, bounds_.height));
    bgRect_.setOutlineThickness(border);
    target.draw(bgRect_);

    const float padding = 10.0f * scale.dpiScale;
    const float iconSize = 16.0f * scale.dpiScale;

    // Sun icon + value
    float x = bounds_.left + padding;
    float y = bounds_.top + padding;
    sunIcon_.setPosition(x, y);
    sunIcon_.setRadius(iconSize * 0.5f);
    sunIcon_.setFillColor(colors_.sunGood);
    target.draw(sunIcon_);

    x += iconSize + 8.0f * scale.dpiScale;
    if (fonts.loaded()) {
        sunValue_.setFont(fonts.monoFont());
        sunValue_.setCharacterSize(static_cast<unsigned int>(scale.fontSizeLarge));
    }
    sunValue_.setString(std::to_string(sun_));
    sunValue_.setPosition(x, y - 2.0f * scale.dpiScale);
    target.draw(sunValue_);

    // Power gauge
    const float gaugeWidth = barWidth * 0.55f;
    const float gaugeHeight = 14.0f * scale.dpiScale;
    const float gaugeX = bounds_.left + padding;
    const float gaugeY = bounds_.top + barHeight - padding - gaugeHeight;

    powerBarBg_.setPosition(gaugeX, gaugeY);
    powerBarBg_.setSize(sf::Vector2f(gaugeWidth, gaugeHeight));
    target.draw(powerBarBg_);

    // Fill ratio
    float fillRatio = 0.0f;
    if (powerConsumed_ > 0) {
        fillRatio = std::min(1.0f, static_cast<float>(power_) / static_cast<float>(powerConsumed_));
    } else if (power_ > 0) {
        fillRatio = 1.0f;
    }

    // Blinking on shortage
    if (powerWarning_) {
        blinkTimer_ += 0.016f;
        bool visible = std::fmod(blinkTimer_, 1.0f) < 0.6f;
        powerBarFill_.setFillColor(visible ? colors_.powerBad : sf::Color(80, 30, 30, 255));
    } else {
        powerBarFill_.setFillColor(colors_.powerGood);
    }

    powerBarFill_.setPosition(gaugeX + 2.0f, gaugeY + 2.0f);
    powerBarFill_.setSize(sf::Vector2f((gaugeWidth - 4.0f) * fillRatio, gaugeHeight - 4.0f));
    target.draw(powerBarFill_);

    // Power label and values
    if (fonts.loaded()) {
        powerLabel_.setFont(fonts.monoFont());
        powerLabel_.setCharacterSize(static_cast<unsigned int>(scale.fontSizeSmall));
    }
    powerLabel_.setString("PWR " + std::to_string(power_) + "/" + std::to_string(powerConsumed_));
    powerLabel_.setPosition(gaugeX + gaugeWidth + 10.0f * scale.dpiScale, gaugeY - 1.0f);
    powerLabel_.setFillColor(powerWarning_ ? colors_.powerBad : colors_.powerGood);
    target.draw(powerLabel_);
}

bool ResourceBar::containsPoint(sf::Vector2f uiCoords) const {
    return bounds_.contains(uiCoords);
}

}  // namespace tcp::render
