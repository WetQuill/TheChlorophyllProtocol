#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <cmath>

namespace tcp::render {

struct UiLayout {
    float sidebarWidthRatio{0.20f};
    float sidebarMinWidth{220.0f};
    float edgeMargin{12.0f};
    float resourceBarWidth{320.0f};
    float resourceBarHeight{58.0f};
    float unitPanelWidth{480.0f};
    float unitPanelHeight{64.0f};
    float tabButtonHeight{36.0f};
    float productionSlotHeight{56.0f};
};

struct UiColors {
    sf::Color panelBg{18, 25, 18, 210};
    sf::Color panelBorder{55, 95, 61, 255};
    sf::Color tabActive{28, 42, 28, 240};
    sf::Color tabInactive{14, 20, 14, 200};

    sf::Color sunGood{246, 218, 116, 255};
    sf::Color powerGood{162, 226, 246, 255};
    sf::Color powerBad{228, 74, 74, 255};

    sf::Color hpBackground{45, 8, 8, 255};
    sf::Color hpFill{118, 220, 93, 255};
    sf::Color hpBorder{22, 22, 22, 255};

    sf::Color ammoHE{228, 74, 74, 255};
    sf::Color ammoButter{238, 214, 98, 255};

    sf::Color affordYes{118, 220, 93, 255};
    sf::Color affordNo{100, 50, 50, 255};
    sf::Color grayedOut{80, 80, 80, 180};

    sf::Color resourceWarning{228, 74, 74, 255};
    sf::Color textLight{220, 220, 210, 255};
    sf::Color textDark{30, 30, 25, 255};
};

struct UiScale {
    float dpiScale{1.0f};
    float fontSizeSmall{11.0f};
    float fontSizeRegular{14.0f};
    float fontSizeLarge{18.0f};
    float borderThickness{2.0f};
};

inline UiScale computeUiScale(const sf::RenderWindow& /*window*/) {
    UiScale scale;
    auto desktop = sf::VideoMode::getDesktopMode();
    float diagonalPixels = std::sqrt(
        static_cast<float>(desktop.width) * static_cast<float>(desktop.width) +
        static_cast<float>(desktop.height) * static_cast<float>(desktop.height));
    float assumedDiagInches = 24.0f;
    float rawDpi = diagonalPixels / assumedDiagInches;
    scale.dpiScale = std::max(0.75f, std::min(3.0f, rawDpi / 96.0f));
    scale.fontSizeSmall = std::round(11.0f * scale.dpiScale);
    scale.fontSizeRegular = std::round(14.0f * scale.dpiScale);
    scale.fontSizeLarge = std::round(18.0f * scale.dpiScale);
    scale.borderThickness = std::round(2.0f * scale.dpiScale);
    return scale;
}

}  // namespace tcp::render
