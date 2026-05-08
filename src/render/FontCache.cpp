#include "FontCache.h"

#include <array>
#include <string>

namespace tcp::render {

bool FontCache::load() {
    const std::array<const char*, 12> paths = {
        "assets/fonts/Fixedsys500c.ttf",
        "../assets/fonts/Fixedsys500c.ttf",
        "../../assets/fonts/Fixedsys500c.ttf",
        "../../../assets/fonts/Fixedsys500c.ttf",
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Courier.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/lucon.ttf",
        "C:/Windows/Fonts/cour.ttf",
    };

    for (const auto* path : paths) {
        if (font_.loadFromFile(path)) {
            loaded_ = true;
            return true;
        }
    }
    return false;
}

}  // namespace tcp::render
