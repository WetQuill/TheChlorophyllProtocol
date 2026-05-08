#include "Sidebar.h"

#include "../../src/logic/ecs/World.h"
#include "../../src/logic/ecs/components/Components.h"

#include <algorithm>
#include <string>

namespace tcp::render {

namespace {
    constexpr std::uint32_t kCampArchetypeId = 901U;
    constexpr std::uint32_t kPowerPlantArchetypeId = 902U;
    constexpr std::uint32_t kBastionArchetypeId = 903U;
    constexpr std::uint32_t kPeaMilitiaArchetypeId = 101U;
}  // namespace

// CommandType values (must match Components.h enum)
namespace cmd {
    constexpr std::int32_t kBuild = 0;
    constexpr std::int32_t kBuildSunPowerPlant = 4;
    constexpr std::int32_t kProducePea = 5;
    constexpr std::int32_t kBuildCornCannonBastion = 6;
}  // namespace cmd

const std::array<ProductionSlotDef, 4> Sidebar::buildingSlots_{
    ProductionSlotDef{kCampArchetypeId, 20, 10, cmd::kBuild, "Pea Military Camp", true},
    ProductionSlotDef{kPowerPlantArchetypeId, 25, 12, cmd::kBuildSunPowerPlant, "Sun Power Plant", true},
    ProductionSlotDef{kBastionArchetypeId, 1'200'000, 80'000, cmd::kBuildCornCannonBastion, "Corn Cannon Bastion", true},
    ProductionSlotDef{0, 0, 0, -1, "Future Building", true},
};

const std::array<ProductionSlotDef, 1> Sidebar::defenseSlots_{
    ProductionSlotDef{kBastionArchetypeId, 1'200'000, 80'000, cmd::kBuildCornCannonBastion, "Corn Cannon Bastion", true},
};

const std::array<ProductionSlotDef, 1> Sidebar::infantrySlots_{
    ProductionSlotDef{kPeaMilitiaArchetypeId, 20, 0, cmd::kProducePea, "Pea Militia", false},
};

const std::array<ProductionSlotDef, 1> Sidebar::vehicleSlots_{
    ProductionSlotDef{0, 0, 0, -1, "Coming Soon", false},
};

Sidebar::Sidebar(const UiLayout& layout, const UiColors& colors)
    : layout_(layout)
    , colors_(colors) {

    tabShapes_.resize(4);
    tabLabels_.resize(4);
    for (std::size_t i = 0; i < 4; ++i) {
        tabShapes_[i].setOutlineThickness(1.0f);
    }

    // Pre-allocate for max slot count
    constexpr int kMaxSlots = 8;
    slotBgs_.reserve(kMaxSlots);
    slotNameTexts_.reserve(kMaxSlots);
    slotCostTexts_.reserve(kMaxSlots);
    slotOverlays_.reserve(kMaxSlots);
}

void Sidebar::handleResize(const sf::FloatRect& bounds) {
    bounds_ = bounds;
    layoutTabs();
}

void Sidebar::layoutTabs() {
    const float tabWidth = bounds_.width / 4.0f;
    const float tabHeight = layout_.tabButtonHeight;

    const char* labels[] = {"BUILD", "DEF", "INF", "VEH"};

    for (int i = 0; i < 4; ++i) {
        tabRects_[i] = sf::FloatRect(
            bounds_.left + tabWidth * static_cast<float>(i),
            bounds_.top,
            tabWidth,
            tabHeight);

        tabShapes_[i].setPosition(tabRects_[i].left, tabRects_[i].top);
        tabShapes_[i].setSize(sf::Vector2f(tabWidth, tabHeight));

        tabLabels_[i].setString(labels[i]);
    }
}

void Sidebar::layoutSlots() {
    const float tabHeight = layout_.tabButtonHeight;
    const float slotHeight = layout_.productionSlotHeight;
    const float contentTop = bounds_.top + tabHeight + 4.0f;
    const float contentWidth = bounds_.width - 8.0f;

    const std::size_t count = activeSlots_.size();

    // Ensure we have enough visual elements
    while (slotBgs_.size() < count) {
        sf::RectangleShape bg;
        bg.setOutlineThickness(1.0f);
        slotBgs_.push_back(bg);

        sf::Text nameText;
        slotNameTexts_.push_back(nameText);

        sf::Text costText;
        slotCostTexts_.push_back(costText);

        sf::RectangleShape overlay;
        overlay.setFillColor(colors_.grayedOut);
        slotOverlays_.push_back(overlay);
    }

    slotRects_.resize(count);

    for (std::size_t i = 0; i < count; ++i) {
        slotRects_[i] = sf::FloatRect(
            bounds_.left + 4.0f,
            contentTop + slotHeight * static_cast<float>(i) + 2.0f * static_cast<float>(i),
            contentWidth,
            slotHeight);
    }
}

void Sidebar::updateFromWorld(const tcp::logic::ecs::World& world,
                               std::uint8_t playerTeam,
                               const std::map<tcp::logic::ecs::EntityId, std::int64_t>& constructionTimers) {

    const std::int32_t sun = world.sunForTeam(playerTeam);
    const std::int32_t power = world.powerForTeam(playerTeam);

    // Find whether player has a non-construction PeaMilitaryCamp for infantry production
    bool hasActiveCamp = false;
    const auto& identities = world.identities();
    const auto& buildings = world.buildings();
    const auto& teams = world.teams();

    for (const auto& [entityId, identity] : identities) {
        if (identity.archetypeId != kCampArchetypeId) continue;
        auto teamIt = teams.find(entityId);
        if (teamIt == teams.end() || teamIt->second.value != playerTeam) continue;
        auto buildingIt = buildings.find(entityId);
        if (buildingIt == buildings.end()) continue;
        // Not under construction
        if (constructionTimers.find(entityId) != constructionTimers.end()) continue;
        hasActiveCamp = true;
        break;
    }

    buildSlotsForActiveTab(world, playerTeam, constructionTimers);
    layoutSlots();

    for (std::size_t i = 0; i < activeSlots_.size(); ++i) {
        const auto& def = activeSlots_[i];

        slotStates_[i].canAfford = (sun >= def.sunCost) && (power >= def.powerCost);

        if (def.isBuilding) {
            slotStates_[i].canProduce = true;
        } else {
            slotStates_[i].canProduce = hasActiveCamp;
        }
    }
}

void Sidebar::buildSlotsForActiveTab(const tcp::logic::ecs::World& /*world*/,
                                      std::uint8_t /*playerTeam*/,
                                      const std::map<tcp::logic::ecs::EntityId, std::int64_t>& /*constructionTimers*/) {
    const ProductionSlotDef* src = nullptr;
    std::size_t count = 0;

    switch (activeTab_) {
    case SidebarTab::kBuilding:
        src = buildingSlots_.data();
        count = buildingSlots_.size();
        break;
    case SidebarTab::kDefense:
        src = defenseSlots_.data();
        count = defenseSlots_.size();
        break;
    case SidebarTab::kInfantry:
        src = infantrySlots_.data();
        count = infantrySlots_.size();
        break;
    case SidebarTab::kVehicle:
        src = vehicleSlots_.data();
        count = vehicleSlots_.size();
        break;
    default:
        break;
    }

    activeSlots_.assign(src, src + count);
    slotStates_.resize(count);
    for (auto& state : slotStates_) {
        state = SlotState{};
    }
}

void Sidebar::draw(sf::RenderTarget& target, const UiScale& scale, FontCache& fonts) {
    // Background
    sf::RectangleShape bg;
    bg.setPosition(bounds_.left, bounds_.top);
    bg.setSize(sf::Vector2f(bounds_.width, bounds_.height));
    bg.setFillColor(colors_.panelBg);
    bg.setOutlineColor(colors_.panelBorder);
    bg.setOutlineThickness(scale.borderThickness);
    target.draw(bg);

    // Tabs
    for (int i = 0; i < 4; ++i) {
        const bool isActive = (static_cast<int>(activeTab_) == i);
        tabShapes_[i].setFillColor(isActive ? colors_.tabActive : colors_.tabInactive);
        if (isActive) {
            tabShapes_[i].setOutlineColor(colors_.panelBorder);
        } else {
            tabShapes_[i].setOutlineColor(sf::Color(30, 40, 30, 180));
        }
        target.draw(tabShapes_[i]);

        if (fonts.loaded()) {
            tabLabels_[i].setFont(fonts.monoFont());
            tabLabels_[i].setCharacterSize(static_cast<unsigned int>(scale.fontSizeSmall));
        }
        tabLabels_[i].setFillColor(isActive ? colors_.textLight : sf::Color(140, 140, 130, 255));
        const sf::FloatRect& tr = tabRects_[i];
        const auto bounds = tabLabels_[i].getLocalBounds();
        tabLabels_[i].setPosition(
            tr.left + (tr.width - bounds.width) * 0.5f,
            tr.top + (tr.height - bounds.height) * 0.5f - bounds.top);
        target.draw(tabLabels_[i]);
    }

    // Production slots
    const std::size_t count = activeSlots_.size();
    for (std::size_t i = 0; i < count; ++i) {
        const auto& rect = slotRects_[i];
        const auto& def = activeSlots_[i];
        const auto& state = slotStates_[i];

        bool affordable = state.canAfford && state.canProduce;

        auto& slotBg = slotBgs_[i];
        slotBg.setPosition(rect.left, rect.top);
        slotBg.setSize(sf::Vector2f(rect.width, rect.height));
        slotBg.setFillColor(affordable ? sf::Color(25, 35, 25, 220) : sf::Color(35, 22, 22, 220));
        slotBg.setOutlineColor(affordable ? colors_.panelBorder : sf::Color(80, 40, 40, 200));
        target.draw(slotBg);

        // Name
        auto& nameText = slotNameTexts_[i];
        if (fonts.loaded()) {
            nameText.setFont(fonts.monoFont());
            nameText.setCharacterSize(static_cast<unsigned int>(scale.fontSizeRegular));
        }
        nameText.setString(def.name);
        nameText.setFillColor(affordable ? colors_.textLight : colors_.grayedOut);
        nameText.setPosition(rect.left + 10.0f * scale.dpiScale,
                              rect.top + 6.0f * scale.dpiScale);
        target.draw(nameText);

        // Cost text
        auto& costText = slotCostTexts_[i];
        if (fonts.loaded()) {
            costText.setFont(fonts.monoFont());
            costText.setCharacterSize(static_cast<unsigned int>(scale.fontSizeSmall));
        }
        {
            std::string costStr = "S:" + std::to_string(def.sunCost);
            if (def.powerCost > 0) {
                costStr += " P:" + std::to_string(def.powerCost);
            }
            costText.setString(costStr);
        }
        costText.setFillColor(affordable ? colors_.affordYes : colors_.affordNo);
        costText.setPosition(rect.left + 10.0f * scale.dpiScale,
                              rect.top + rect.height - scale.fontSizeSmall - 8.0f * scale.dpiScale);
        target.draw(costText);

        // Gray-out overlay for unaffordable
        if (!affordable) {
            auto& overlay = slotOverlays_[i];
            overlay.setPosition(rect.left, rect.top);
            overlay.setSize(sf::Vector2f(rect.width, rect.height));
            target.draw(overlay);
        }
    }
}

bool Sidebar::containsPoint(sf::Vector2f uiCoords) const {
    return bounds_.contains(uiCoords);
}

SidebarTab Sidebar::hitTestTab(sf::Vector2f uiCoords) const {
    for (int i = 0; i < 4; ++i) {
        if (tabRects_[i].contains(uiCoords)) {
            return static_cast<SidebarTab>(i);
        }
    }
    return activeTab_;
}

int Sidebar::hitTestSlot(sf::Vector2f uiCoords) const {
    for (std::size_t i = 0; i < slotRects_.size(); ++i) {
        if (slotRects_[i].contains(uiCoords)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Sidebar::setActiveTab(SidebarTab tab) {
    activeTab_ = tab;
}

const ProductionSlotDef* Sidebar::slotDef(int index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= activeSlots_.size()) {
        return nullptr;
    }
    return &activeSlots_[static_cast<std::size_t>(index)];
}

}  // namespace tcp::render
