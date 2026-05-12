#include "UiRenderer.h"

#include "../../src/logic/ecs/World.h"
#include "../../src/logic/ecs/components/Components.h"

#include <algorithm>

namespace tcp::render {

namespace {
    constexpr std::uint8_t kDefaultPlayerTeam = 0U;
}  // namespace

UiRenderer::UiRenderer()
    : resourceBar_(layout_, colors_)
    , sidebar_(layout_, colors_)
    , unitInfoPanel_(layout_, colors_) {
}

void UiRenderer::initialize(const sf::RenderWindow& window, FontCache& /*fonts*/) {
    scale_ = computeUiScale(window);
    windowSize_ = window.getSize();

    uiView_ = sf::View(sf::FloatRect(0.0f, 0.0f,
                                      static_cast<float>(windowSize_.x),
                                      static_cast<float>(windowSize_.y)));

    // Sidebar: right 20% of window
    const float sidebarWidth = std::max(static_cast<float>(windowSize_.x) * layout_.sidebarWidthRatio,
                                         layout_.sidebarMinWidth);
    sidebar_.handleResize(sf::FloatRect(
        static_cast<float>(windowSize_.x) - sidebarWidth,
        0.0f,
        sidebarWidth,
        static_cast<float>(windowSize_.y)));

    // Unit info panel: bottom-left
    const float panelW = layout_.unitPanelWidth * scale_.dpiScale;
    const float panelH = layout_.unitPanelHeight * scale_.dpiScale;
    unitInfoPanel_.handleResize(sf::FloatRect(
        layout_.edgeMargin,
        static_cast<float>(windowSize_.y) - panelH - layout_.edgeMargin,
        panelW,
        panelH));

    unitInfoPanel_.setVisible(false);
}

void UiRenderer::handleResize(const sf::RenderWindow& window) {
    windowSize_ = window.getSize();

    uiView_ = sf::View(sf::FloatRect(0.0f, 0.0f,
                                      static_cast<float>(windowSize_.x),
                                      static_cast<float>(windowSize_.y)));

    const float sidebarWidth = std::max(static_cast<float>(windowSize_.x) * layout_.sidebarWidthRatio,
                                         layout_.sidebarMinWidth);
    sidebar_.handleResize(sf::FloatRect(
        static_cast<float>(windowSize_.x) - sidebarWidth,
        0.0f,
        sidebarWidth,
        static_cast<float>(windowSize_.y)));

    // Unit info panel: bottom-left
    const float panelW = layout_.unitPanelWidth * scale_.dpiScale;
    const float panelH = layout_.unitPanelHeight * scale_.dpiScale;
    unitInfoPanel_.handleResize(sf::FloatRect(
        layout_.edgeMargin,
        static_cast<float>(windowSize_.y) - panelH - layout_.edgeMargin,
        panelW,
        panelH));
}

void UiRenderer::update(const tcp::logic::ecs::World& world,
                         std::uint8_t playerTeam,
                         const std::vector<tcp::logic::ecs::EntityId>& selectedGroup,
                         const std::map<tcp::logic::ecs::EntityId, std::int64_t>& constructionTimers) {

    // Compute total power consumed for the player's team
    std::int32_t totalPowerConsumed = 0;
    const auto& powerConsumers = world.powerConsumers();
    const auto& teams = world.teams();
    for (const auto& [entityId, consumer] : powerConsumers) {
        auto teamIt = teams.find(entityId);
        if (teamIt != teams.end() && teamIt->second.value == playerTeam) {
            totalPowerConsumed += consumer.requiredPower;
        }
    }

    resourceBar_.update(world.sunForTeam(playerTeam),
                         world.powerForTeam(playerTeam),
                         totalPowerConsumed);

    sidebar_.updateFromWorld(world, playerTeam, constructionTimers);

    // Unit info panel: show when exactly 1 entity is selected
    if (selectedGroup.size() == 1) {
        const auto selectedId = selectedGroup[0];
        unitInfoPanel_.setVisible(true);

        // Gather entity data
        std::uint32_t archetypeId = 0;
        std::int32_t hpCurrent = 0;
        std::int32_t hpMax = 1;
        bool isButterMode = false;
        bool hasArtillery = false;
        std::int32_t conRemaining = 0;
        std::int32_t conTotal = 1;

        const auto& identities = world.identities();
        auto idIt = identities.find(selectedId);
        if (idIt != identities.end()) {
            archetypeId = idIt->second.archetypeId;
        }

        const auto& healths = world.healths();
        auto hpIt = healths.find(selectedId);
        if (hpIt != healths.end()) {
            hpCurrent = hpIt->second.current;
            hpMax = hpIt->second.max;
        }

        const auto& artilleries = world.artilleryWeapons();
        auto artIt = artilleries.find(selectedId);
        if (artIt != artilleries.end()) {
            hasArtillery = true;
            isButterMode = artIt->second.isButterMode;
        }

        auto conIt = constructionTimers.find(selectedId);
        if (conIt != constructionTimers.end()) {
            conRemaining = static_cast<std::int32_t>(conIt->second);
            // Total ticks from archetype
            if (archetypeId == 901U) conTotal = 300;
            else if (archetypeId == 902U) conTotal = 600;
            else if (archetypeId == 903U) conTotal = 1800;
            else conTotal = conRemaining;
        }

        unitInfoPanel_.update(&selectedId, archetypeId, hpCurrent, hpMax,
                               isButterMode, hasArtillery,
                               conRemaining, conTotal);
    } else {
        unitInfoPanel_.setVisible(false);
    }
}

void UiRenderer::draw(sf::RenderTarget& target, FontCache& fonts) {
    target.setView(uiView_);
    resourceBar_.draw(target, scale_, fonts);
    sidebar_.draw(target, scale_, fonts);

    if (unitInfoPanel_.isVisible()) {
        unitInfoPanel_.draw(target, scale_, fonts);
    }
}

UiEventResult UiRenderer::handleEvent(const sf::Event& event,
                                       const sf::RenderWindow& window) {
    // Reset click state
    tabClicked_ = false;
    slotClicked_ = false;
    toggleClicked_ = false;
    clickedSlotIndex_ = -1;

    if (event.type != sf::Event::MouseButtonPressed) {
        return UiEventResult::kPassToWorld;
    }

    sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
    sf::Vector2f uiCoords = window.mapPixelToCoords(pixelPos, uiView_);

    // Priority 1: Unit info panel toggle button
    if (unitInfoPanel_.hitTestToggleButton(uiCoords)) {
        toggleClicked_ = true;
        return UiEventResult::kConsumed;
    }

    // Priority 2: Sidebar slots
    int slotIdx = sidebar_.hitTestSlot(uiCoords);
    if (slotIdx >= 0) {
        const auto* def = sidebar_.slotDef(slotIdx);
        if (def && def->commandType >= 0) {
            slotClicked_ = true;
            clickedSlotIndex_ = slotIdx;
            return UiEventResult::kConsumed;
        }
    }

    // Priority 3: Sidebar tabs
    if (sidebar_.containsPoint(uiCoords)) {
        SidebarTab hitTab = sidebar_.hitTestTab(uiCoords);
        if (hitTab != sidebar_.activeTab()) {
            sidebar_.setActiveTab(hitTab);
            tabClicked_ = true;
            clickedTab_ = hitTab;
        }
        // Always consume clicks inside the sidebar
        return UiEventResult::kConsumed;
    }

    // Priority 4: Resource bar (informational only, but consume clicks on it)
    if (resourceBar_.containsPoint(uiCoords)) {
        return UiEventResult::kConsumed;
    }

    // Priority 5: Unit info panel background
    if (unitInfoPanel_.containsPoint(uiCoords)) {
        return UiEventResult::kConsumed;
    }

    return UiEventResult::kPassToWorld;
}

const ProductionSlotDef* UiRenderer::clickedSlotDef() const {
    if (!slotClicked_ || clickedSlotIndex_ < 0) return nullptr;
    return sidebar_.slotDef(clickedSlotIndex_);
}

}  // namespace tcp::render
