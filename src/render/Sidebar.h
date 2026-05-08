#pragma once

#include "UiConfig.h"
#include "FontCache.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace tcp::logic::ecs {
class World;
using EntityId = std::uint32_t;
}  // namespace tcp::logic::ecs

namespace tcp::render {

enum class SidebarTab : std::uint8_t {
    kBuilding = 0,
    kDefense = 1,
    kInfantry = 2,
    kVehicle = 3,
    kCount = 4,
};

struct ProductionSlotDef {
    std::uint32_t archetypeId;
    std::int32_t sunCost;
    std::int32_t powerCost;
    std::int32_t commandType;  // ecs::CommandType value
    const char* name;
    bool isBuilding;  // true = building placement, false = unit production
};

class Sidebar {
public:
    Sidebar(const UiLayout& layout, const UiColors& colors);

    void updateFromWorld(const tcp::logic::ecs::World& world,
                         std::uint8_t playerTeam,
                         const std::map<tcp::logic::ecs::EntityId, std::int64_t>& constructionTimers);

    void draw(sf::RenderTarget& target, const UiScale& scale, FontCache& fonts);

    void handleResize(const sf::FloatRect& bounds);

    [[nodiscard]] bool containsPoint(sf::Vector2f uiCoords) const;
    [[nodiscard]] SidebarTab hitTestTab(sf::Vector2f uiCoords) const;
    [[nodiscard]] int hitTestSlot(sf::Vector2f uiCoords) const;

    void setActiveTab(SidebarTab tab);
    [[nodiscard]] SidebarTab activeTab() const noexcept { return activeTab_; }

    [[nodiscard]] const ProductionSlotDef* slotDef(int index) const;

private:
    void buildSlotsForActiveTab(const tcp::logic::ecs::World& world,
                                std::uint8_t playerTeam,
                                const std::map<tcp::logic::ecs::EntityId, std::int64_t>& constructionTimers);
    void layoutTabs();
    void layoutSlots();

    const UiLayout& layout_;
    const UiColors& colors_;

    sf::FloatRect bounds_{};
    SidebarTab activeTab_{SidebarTab::kBuilding};

    static const std::array<ProductionSlotDef, 4> buildingSlots_;
    static const std::array<ProductionSlotDef, 1> defenseSlots_;
    static const std::array<ProductionSlotDef, 1> infantrySlots_;
    static const std::array<ProductionSlotDef, 1> vehicleSlots_;

    // Tab visuals
    std::array<sf::FloatRect, 4> tabRects_{};
    std::vector<sf::RectangleShape> tabShapes_{};
    std::vector<sf::Text> tabLabels_{};

    // Current visible slots
    struct SlotState {
        bool canAfford{false};
        bool canProduce{false};
    };
    std::vector<ProductionSlotDef> activeSlots_{};
    std::vector<SlotState> slotStates_{};
    std::vector<sf::FloatRect> slotRects_{};

    // Slot visuals (pre-allocated for max slots)
    std::vector<sf::RectangleShape> slotBgs_{};
    std::vector<sf::Text> slotNameTexts_{};
    std::vector<sf::Text> slotCostTexts_{};
    std::vector<sf::RectangleShape> slotOverlays_{};
};

}  // namespace tcp::render
