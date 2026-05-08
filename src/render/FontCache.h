#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

namespace tcp::render {

class FontCache {
public:
    bool load();

    [[nodiscard]] const sf::Font& monoFont() const noexcept { return font_; }
    [[nodiscard]] bool loaded() const noexcept { return loaded_; }

private:
    sf::Font font_{};
    bool loaded_{false};
};

}  // namespace tcp::render
