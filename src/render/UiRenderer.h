#pragma once

#include "UiConfig.h"
#include "FontCache.h"
#include "ResourceBar.h"
#include "Sidebar.h"
#include "UnitInfoPanel.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Window/Event.hpp>

#include <cstdint>
#include <map>
#include <vector>

namespace tcp::logic::ecs {
class World;
using EntityId = std::uint32_t;
}  // namespace tcp::logic::ecs

namespace tcp::render {

enum class UiEventResult {
    kConsumed,
    kPassToWorld,
};

class UiRenderer {
public:
    UiRenderer();

    void initialize(const sf::RenderWindow& window, FontCache& fonts);
    void handleResize(const sf::RenderWindow& window);

    void update(const tcp::logic::ecs::World& world,
                std::uint8_t playerTeam,
                const std::vector<tcp::logic::ecs::EntityId>& selectedGroup,
                const std::map<tcp::logic::ecs::EntityId, std::int64_t>& constructionTimers);

    void draw(sf::RenderTarget& target, FontCache& fonts);

    [[nodiscard]] UiEventResult handleEvent(const sf::Event& event,
                                             const sf::RenderWindow& window);

    // Post-event queries
    [[nodiscard]] bool tabClicked() const noexcept { return tabClicked_; }
    [[nodiscard]] SidebarTab clickedTab() const noexcept { return clickedTab_; }
    [[nodiscard]] bool slotClicked() const noexcept { return slotClicked_; }
    [[nodiscard]] const ProductionSlotDef* clickedSlotDef() const;
    [[nodiscard]] bool toggleButterModeClicked() const noexcept { return toggleClicked_; }

    [[nodiscard]] const sf::View& uiView() const noexcept { return uiView_; }

private:
    sf::View uiView_{};
    UiLayout layout_{};
    UiColors colors_{};
    UiScale scale_{};

    ResourceBar resourceBar_{layout_, colors_};
    Sidebar sidebar_{layout_, colors_};
    UnitInfoPanel unitInfoPanel_{layout_, colors_};

    sf::Vector2u windowSize_{0, 0};

    // Event state
    bool tabClicked_{false};
    SidebarTab clickedTab_{SidebarTab::kBuilding};
    bool slotClicked_{false};
    int clickedSlotIndex_{-1};
    bool toggleClicked_{false};
};

}  // namespace tcp::render
