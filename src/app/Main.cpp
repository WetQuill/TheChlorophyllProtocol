#include <cstdint>
#include <cmath>
#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "GameLoop.h"
#include "../logic/debug/StateHasher.h"
#include "../logic/ecs/systems/BuiltInSystems.h"
#include "../logic/runtime/SimulationDriver.h"

#if defined(TCP_HAS_SFML) && __has_include(<SFML/Graphics/RenderWindow.hpp>)
#define TCP_VISUAL_SFML_ENABLED 1
#if __has_include(<SFML/Graphics/CircleShape.hpp>)
#include <SFML/Graphics/CircleShape.hpp>
#elif __has_include(<SFML/Graphics/CircleShape.h>)
#include <SFML/Graphics/CircleShape.h>
#endif

#if __has_include(<SFML/Graphics/RectangleShape.hpp>)
#include <SFML/Graphics/RectangleShape.hpp>
#elif __has_include(<SFML/Graphics/RectangleShape.h>)
#include <SFML/Graphics/RectangleShape.h>
#endif

#include <SFML/Graphics/RenderWindow.hpp>

#if __has_include(<SFML/System/Vector2.hpp>)
#include <SFML/System/Vector2.hpp>
#elif __has_include(<SFML/System/Vector2.h>)
#include <SFML/System/Vector2.h>
#endif

#if __has_include(<SFML/Window/Event.hpp>)
#include <SFML/Window/Event.hpp>
#elif __has_include(<SFML/Window/Event.h>)
#include <SFML/Window/Event.h>
#endif

#if __has_include(<SFML/Window/Mouse.hpp>)
#include <SFML/Window/Mouse.hpp>
#elif __has_include(<SFML/Window/Mouse.h>)
#include <SFML/Window/Mouse.h>
#endif

#if __has_include(<SFML/Window/VideoMode.hpp>)
#include <SFML/Window/VideoMode.hpp>
#elif __has_include(<SFML/Window/VideoMode.h>)
#include <SFML/Window/VideoMode.h>
#endif

#if __has_include(<SFML/Graphics/Texture.hpp>) && __has_include(<SFML/Graphics/Sprite.hpp>)
#define TCP_VISUAL_SFML_TEXTURES 1
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#else
#define TCP_VISUAL_SFML_TEXTURES 0
#endif
#else
#define TCP_VISUAL_SFML_ENABLED 0
#define TCP_VISUAL_SFML_TEXTURES 0
#endif

namespace {

tcp::logic::ecs::Transform at(std::int32_t x, std::int32_t y) {
    tcp::logic::ecs::Transform tr{};
    tr.x = tcp::logic::math::FixedPoint::fromInt(x);
    tr.y = tcp::logic::math::FixedPoint::fromInt(y);
    return tr;
}

tcp::logic::ecs::World makeDemoWorld() {
    tcp::logic::ecs::World world;
    tcp::logic::ecs::registerCoreSystems(world);

    const auto hq0 = world.createEntity();
    const auto hq1 = world.createEntity();
    world.setTeam(hq0, tcp::logic::ecs::Team{0});
    world.setTeam(hq1, tcp::logic::ecs::Team{1});
    world.setTransform(hq0, at(0, 0));
    world.setTransform(hq1, at(8, 0));
    world.setHealth(hq0, tcp::logic::ecs::Health{200, 200});
    world.setHealth(hq1, tcp::logic::ecs::Health{200, 200});
    world.setHeadquarters(hq0, tcp::logic::ecs::Headquarters{true});
    world.setHeadquarters(hq1, tcp::logic::ecs::Headquarters{true});

    const auto p0Unit = world.createEntity();
    const auto p1Unit = world.createEntity();
    world.setTeam(p0Unit, tcp::logic::ecs::Team{0});
    world.setTeam(p1Unit, tcp::logic::ecs::Team{1});
    world.setTransform(p0Unit, at(0, 1));
    world.setTransform(p1Unit, at(8, 1));
    world.setHealth(p0Unit, tcp::logic::ecs::Health{30, 30});
    world.setHealth(p1Unit, tcp::logic::ecs::Health{30, 30});
    world.setWeapon(p0Unit, tcp::logic::ecs::Weapon{tcp::logic::math::FixedPoint::fromInt(2), 5, 1, 0});
    world.setWeapon(p1Unit, tcp::logic::ecs::Weapon{tcp::logic::math::FixedPoint::fromInt(2), 5, 1, 0});
    world.setCommandBuffer(p0Unit, tcp::logic::ecs::CommandBuffer{});
    world.setCommandBuffer(p1Unit, tcp::logic::ecs::CommandBuffer{});

    return world;
}

struct Delivery {
    std::int64_t deliverTick{0};
    int target{0};
    tcp::net::CommandFramePacket packet{};
};

enum class DemoMode : std::uint8_t {
    kAll = 0,
    kSingle = 1,
    kReplay = 2,
    kLockstep = 3,
};

struct AppOptions {
    DemoMode mode{DemoMode::kAll};
    std::int64_t ticks{8};
    std::string replayPath{"simulation_driver_demo_replay.txt"};
    bool visual{false};
    bool ticksProvided{false};
};

void printUsage() {
    std::cout
        << "Usage: chlorophyll_app [--mode all|single|replay|lockstep] [--ticks N] [--replay-path PATH] [--visual]"
        << '\n';
}

std::optional<DemoMode> parseMode(const std::string_view value) {
    if (value == "all") {
        return DemoMode::kAll;
    }
    if (value == "single") {
        return DemoMode::kSingle;
    }
    if (value == "replay") {
        return DemoMode::kReplay;
    }
    if (value == "lockstep") {
        return DemoMode::kLockstep;
    }
    return std::nullopt;
}

bool parseTicks(const std::string& value, std::int64_t& outTicks) {
    try {
        std::size_t parsed = 0;
        outTicks = std::stoll(value, &parsed);
        if (parsed != value.size()) {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    if (outTicks <= 0) {
        return false;
    }

    return true;
}

bool parseOptions(int argc, char** argv, AppOptions& out) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--mode") {
            if (i + 1 >= argc) {
                return false;
            }

            const auto parsedMode = parseMode(argv[++i]);
            if (!parsedMode.has_value()) {
                return false;
            }
            out.mode = parsedMode.value();
            continue;
        }

        if (arg == "--ticks") {
            if (i + 1 >= argc || !parseTicks(argv[++i], out.ticks)) {
                return false;
            }
            out.ticksProvided = true;
            continue;
        }

        if (arg == "--replay-path") {
            if (i + 1 >= argc) {
                return false;
            }

            out.replayPath = argv[++i];
            continue;
        }

        if (arg == "--help" || arg == "-h") {
            return false;
        }

        if (arg == "--visual") {
            out.visual = true;
            continue;
        }

        return false;
    }

    return true;
}

bool runSingle(const std::int64_t ticks, const std::string& replayPath) {
    tcp::logic::runtime::SimulationDriver single(makeDemoWorld());
    single.useSingleLocalMode();

    for (std::int64_t tick = 0; tick < ticks; ++tick) {
        if (tick == 0) {
            single.queueLocalCommand(0, 3, tcp::logic::ecs::CommandType::kMove, 2, 1, 0);
        }

        if (!single.stepTick()) {
            std::cerr << "single: failed to step" << '\n';
            return false;
        }
    }

    const auto singlePos = single.world().transforms().at(3);
    std::cout << "single: tick=" << single.world().currentTick()
              << " pos=(" << singlePos.x.toIntTrunc() << ',' << singlePos.y.toIntTrunc() << ")"
              << " hash=" << tcp::logic::debug::hashWorldState(single.world()) << '\n';

    if (!single.saveReplay(replayPath)) {
        std::cerr << "single: failed to save replay file" << '\n';
        return false;
    }

    return true;
}

bool runReplay(const std::int64_t ticks, const std::string& replayPath) {
    tcp::logic::runtime::SimulationDriver replay(makeDemoWorld());
    if (!replay.useReplayModeFromFile(replayPath)) {
        std::cerr << "replay: failed to load replay file: " << replayPath << '\n';
        return false;
    }

    for (std::int64_t tick = 0; tick < ticks; ++tick) {
        if (!replay.stepTick()) {
            std::cerr << "replay: failed to step" << '\n';
            return false;
        }
    }

    const auto replayPos = replay.world().transforms().at(3);
    std::cout << "replay: tick=" << replay.world().currentTick()
              << " pos=(" << replayPos.x.toIntTrunc() << ',' << replayPos.y.toIntTrunc() << ")"
              << " hash=" << tcp::logic::debug::hashWorldState(replay.world()) << '\n';

    return true;
}

bool runLockstep(const std::int64_t ticks) {
    tcp::logic::runtime::SimulationDriver lockstepA(makeDemoWorld());
    tcp::logic::runtime::SimulationDriver lockstepB(makeDemoWorld());
    lockstepA.useLockstepMode({0, 2, 1});
    lockstepB.useLockstepMode({1, 2, 1});

    const tcp::net::CommandFramePacket emptyA{0, 0, {}};
    const tcp::net::CommandFramePacket emptyB{0, 1, {}};
    lockstepA.receivePacket(emptyA);
    lockstepA.receivePacket(emptyB);
    lockstepB.receivePacket(emptyA);
    lockstepB.receivePacket(emptyB);

    std::vector<Delivery> network;
    for (std::int64_t tick = 0; tick < ticks; ++tick) {
        lockstepA.queueLocalCommand(0, 3, tcp::logic::ecs::CommandType::kStop, 0, 0, 0);
        lockstepB.queueLocalCommand(1, 4, tcp::logic::ecs::CommandType::kStop, 0, 0, 0);
        if (tick == 0) {
            lockstepA.queueLocalCommand(0, 3, tcp::logic::ecs::CommandType::kMove, 3, 1, 0);
            lockstepB.queueLocalCommand(1, 4, tcp::logic::ecs::CommandType::kMove, 5, 1, 0);
        }

        const auto outA = lockstepA.drainOutgoingPackets();
        const auto outB = lockstepB.drainOutgoingPackets();
        for (const auto& packet : outA) {
            network.push_back({tick, 0, packet});
            network.push_back({tick + 1, 1, packet});
        }
        for (const auto& packet : outB) {
            network.push_back({tick, 1, packet});
            network.push_back({tick + 1, 0, packet});
        }

        std::vector<Delivery> pending;
        for (const auto& delivery : network) {
            if (delivery.deliverTick > tick) {
                pending.push_back(delivery);
                continue;
            }

            if (delivery.target == 0) {
                lockstepA.receivePacket(delivery.packet);
            } else {
                lockstepB.receivePacket(delivery.packet);
            }
        }
        network = std::move(pending);

        if (!lockstepA.stepTick() || !lockstepB.stepTick()) {
            std::cerr << "lockstep: frame incomplete at tick " << tick << '\n';
            return false;
        }
    }

    const auto lockHashA = tcp::logic::debug::hashWorldState(lockstepA.world());
    const auto lockHashB = tcp::logic::debug::hashWorldState(lockstepB.world());
    std::cout << "lockstep: hashA=" << lockHashA << " hashB=" << lockHashB
              << " equal=" << ((lockHashA == lockHashB) ? "yes" : "no") << '\n';

    return lockHashA == lockHashB;
}

bool runVisualSingle(const AppOptions& options) {
#if TCP_VISUAL_SFML_ENABLED
    tcp::logic::runtime::SimulationDriver driver(makeDemoWorld());
    driver.useSingleLocalMode();

    const tcp::logic::SimulationConfig config{};
    tcp::app::GameLoop loop(config);

    constexpr float kCellPixels = 48.0F;
    constexpr float kEntityRadius = 16.0F;

    sf::RenderWindow window(sf::VideoMode(1280U, 720U), "The Chlorophyll Protocol - Visual Single");
    window.setFramerateLimit(60U);

    struct MoveMarker {
        std::int32_t x{0};
        std::int32_t y{0};
        std::int64_t expireTick{-1};
    };

    std::optional<tcp::logic::ecs::EntityId> selectedEntity;
    std::optional<MoveMarker> moveMarker;
    auto previousFrameTime = std::chrono::steady_clock::now();

#if TCP_VISUAL_SFML_TEXTURES
    sf::Texture texHq0;
    sf::Texture texHq1;
    sf::Texture texPeaMilitia;
    sf::Texture texSunflower;
    sf::Texture texSelectionRing;
    sf::Texture texMoveMarker;

    const auto loadTexture = [](sf::Texture& texture, const char* relativePath) {
        const std::array<std::string, 4> prefixes = {
            "",
            "../",
            "../../",
            "../../../",
        };

        for (const auto& prefix : prefixes) {
            if (texture.loadFromFile(prefix + std::string(relativePath))) {
                return true;
            }
        }
        return false;
    };

    const bool hasHq0Texture = loadTexture(texHq0, "assets/visual/buildings/hq_team0.png");
    const bool hasHq1Texture = loadTexture(texHq1, "assets/visual/buildings/hq_team1.png");
    const bool hasPeaTexture = loadTexture(texPeaMilitia, "assets/visual/units/pea_militia.png");
    const bool hasSunflowerTexture = loadTexture(texSunflower, "assets/visual/units/sunflower_generator.png");
    const bool hasSelectionRingTexture = loadTexture(texSelectionRing, "assets/visual/fx/selection_ring.png");
    const bool hasMoveMarkerTexture = loadTexture(texMoveMarker, "assets/visual/fx/move_target_marker.png");
#endif

    const auto worldToScreen = [&](const tcp::logic::ecs::Transform& tr) {
        return sf::Vector2f{
            260.0F + (static_cast<float>(tr.x.raw()) / 1000.0F) * kCellPixels,
            180.0F + (static_cast<float>(tr.y.raw()) / 1000.0F) * kCellPixels,
        };
    };

    const auto mouseToGrid = [&](const sf::Vector2i pixelPos) {
        const float gx = (static_cast<float>(pixelPos.x) - 260.0F) / kCellPixels;
        const float gy = (static_cast<float>(pixelPos.y) - 180.0F) / kCellPixels;
        return std::pair<std::int32_t, std::int32_t>{
            static_cast<std::int32_t>(std::lround(gx)),
            static_cast<std::int32_t>(std::lround(gy)),
        };
    };

    bool running = true;
    while (running && window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                running = false;
                break;
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    const auto target = mouseToGrid(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                    const auto& world = driver.world();
                    const auto& transforms = world.transforms();
                    const auto& teams = world.teams();
                    const auto& commandBuffers = world.commandBuffers();

                    std::optional<tcp::logic::ecs::EntityId> best;
                    std::int32_t bestDistSq = 999999;
                    for (const auto& [entityId, tr] : transforms) {
                        if (commandBuffers.find(entityId) == commandBuffers.end()) {
                            continue;
                        }
                        const auto teamIt = teams.find(entityId);
                        if (teamIt == teams.end() || teamIt->second.value != 0U) {
                            continue;
                        }

                        const auto dx = tr.x.toIntTrunc() - target.first;
                        const auto dy = tr.y.toIntTrunc() - target.second;
                        const auto distSq = (dx * dx) + (dy * dy);
                        if (distSq < bestDistSq && distSq <= 1) {
                            bestDistSq = distSq;
                            best = entityId;
                        }
                    }

                    selectedEntity = best;
                }

                if (event.mouseButton.button == sf::Mouse::Right && selectedEntity.has_value()) {
                    const auto [gx, gy] = mouseToGrid(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                    const auto selected = selectedEntity.value();
                    const auto teamIt = driver.world().teams().find(selected);
                    if (teamIt != driver.world().teams().end()) {
                        driver.queueLocalCommand(
                            teamIt->second.value,
                            selected,
                            tcp::logic::ecs::CommandType::kMove,
                            gx,
                            gy,
                            0);
                        moveMarker = MoveMarker{gx, gy, driver.world().currentTick() + 20};
                    }
                }
            }
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - previousFrameTime).count();
        previousFrameTime = now;
        const auto frameReport = loop.frame(elapsed, [&](std::int64_t) {
            if (!driver.stepTick()) {
                running = false;
                return;
            }
            if (options.ticksProvided && driver.world().currentTick() >= options.ticks) {
                running = false;
            }
        });

        window.clear(sf::Color(18U, 22U, 18U));

        for (int gx = -5; gx <= 20; ++gx) {
            sf::RectangleShape vline(sf::Vector2f{1.0F, 680.0F});
            vline.setFillColor(sf::Color(36U, 45U, 36U));
            vline.setPosition(sf::Vector2f{260.0F + static_cast<float>(gx) * kCellPixels, 20.0F});
            window.draw(vline);
        }
        for (int gy = -3; gy <= 12; ++gy) {
            sf::RectangleShape hline(sf::Vector2f{1000.0F, 1.0F});
            hline.setFillColor(sf::Color(36U, 45U, 36U));
            hline.setPosition(sf::Vector2f{20.0F, 180.0F + static_cast<float>(gy) * kCellPixels});
            window.draw(hline);
        }

        const auto& world = driver.world();
        const auto& transforms = world.transforms();
        const auto& teams = world.teams();
        const auto& healths = world.healths();
        const auto& hqs = world.headquarters();
        const auto& sunProducers = world.sunProducers();

        for (const auto entityId : world.entities()) {
            const auto trIt = transforms.find(entityId);
            if (trIt == transforms.end()) {
                continue;
            }

            const auto screen = worldToScreen(trIt->second);
            const auto teamIt = teams.find(entityId);
            const bool teamOne = (teamIt != teams.end() && teamIt->second.value == 1U);
            const bool isHq = hqs.find(entityId) != hqs.end();
            const bool isSunProducer = sunProducers.find(entityId) != sunProducers.end();

            bool drewSprite = false;
#if TCP_VISUAL_SFML_TEXTURES
            if (isHq) {
                const sf::Texture* texture = nullptr;
                if (teamOne && hasHq1Texture) {
                    texture = &texHq1;
                }
                if (!teamOne && hasHq0Texture) {
                    texture = &texHq0;
                }

                if (texture != nullptr) {
                    sf::Sprite sprite(*texture);
                    const auto size = texture->getSize();
                    if (size.x > 0U && size.y > 0U) {
                        sprite.setOrigin(sf::Vector2f{static_cast<float>(size.x) * 0.5F, static_cast<float>(size.y) * 0.5F});
                        sprite.setScale(sf::Vector2f{54.0F / static_cast<float>(size.x), 54.0F / static_cast<float>(size.y)});
                    }
                    sprite.setPosition(screen);
                    window.draw(sprite);
                    drewSprite = true;
                }
            } else {
                const sf::Texture* texture = nullptr;
                if (isSunProducer && hasSunflowerTexture) {
                    texture = &texSunflower;
                } else if (hasPeaTexture) {
                    texture = &texPeaMilitia;
                }

                if (texture != nullptr) {
                    sf::Sprite sprite(*texture);
                    const auto size = texture->getSize();
                    if (size.x > 0U && size.y > 0U) {
                        sprite.setOrigin(sf::Vector2f{static_cast<float>(size.x) * 0.5F, static_cast<float>(size.y) * 0.5F});
                        sprite.setScale(sf::Vector2f{34.0F / static_cast<float>(size.x), 34.0F / static_cast<float>(size.y)});
                    }
                    sprite.setPosition(screen);
                    window.draw(sprite);
                    drewSprite = true;
                }
            }
#endif

            if (isHq && !drewSprite) {
                sf::RectangleShape hq(sf::Vector2f{54.0F, 54.0F});
                hq.setOrigin(sf::Vector2f{27.0F, 27.0F});
                hq.setPosition(screen);
                hq.setFillColor(teamOne ? sf::Color(176U, 110U, 32U) : sf::Color(58U, 162U, 74U));
                hq.setOutlineColor(sf::Color(22U, 22U, 22U));
                hq.setOutlineThickness(2.0F);
                window.draw(hq);
            } else if (!isHq && !drewSprite) {
                sf::CircleShape unit(kEntityRadius);
                unit.setOrigin(sf::Vector2f{kEntityRadius, kEntityRadius});
                unit.setPosition(screen);
                unit.setFillColor(teamOne ? sf::Color(237U, 170U, 74U) : sf::Color(118U, 206U, 122U));
                unit.setOutlineColor(sf::Color(20U, 20U, 20U));
                unit.setOutlineThickness(2.0F);
                window.draw(unit);
            }

            const auto hpIt = healths.find(entityId);
            if (hpIt != healths.end() && hpIt->second.max > 0) {
                const float ratio = static_cast<float>(hpIt->second.current) / static_cast<float>(hpIt->second.max);
                const float clamped = std::max(0.0F, std::min(1.0F, ratio));

                sf::RectangleShape barBack(sf::Vector2f{40.0F, 5.0F});
                barBack.setPosition(sf::Vector2f{screen.x - 20.0F, screen.y - 32.0F});
                barBack.setFillColor(sf::Color(35U, 35U, 35U));
                window.draw(barBack);

                sf::RectangleShape barFill(sf::Vector2f{40.0F * clamped, 5.0F});
                barFill.setPosition(sf::Vector2f{screen.x - 20.0F, screen.y - 32.0F});
                barFill.setFillColor(sf::Color(118U, 220U, 93U));
                window.draw(barFill);
            }
        }

        if (moveMarker.has_value() && moveMarker->expireTick >= world.currentTick()) {
            tcp::logic::ecs::Transform markerTr{};
            markerTr.x = tcp::logic::math::FixedPoint::fromInt(moveMarker->x);
            markerTr.y = tcp::logic::math::FixedPoint::fromInt(moveMarker->y);
            const auto screen = worldToScreen(markerTr);

#if TCP_VISUAL_SFML_TEXTURES
            if (hasMoveMarkerTexture) {
                sf::Sprite sprite(texMoveMarker);
                const auto size = texMoveMarker.getSize();
                if (size.x > 0U && size.y > 0U) {
                    sprite.setOrigin(sf::Vector2f{static_cast<float>(size.x) * 0.5F, static_cast<float>(size.y) * 0.5F});
                    sprite.setScale(sf::Vector2f{34.0F / static_cast<float>(size.x), 34.0F / static_cast<float>(size.y)});
                }
                sprite.setPosition(screen);
                window.draw(sprite);
            } else {
#endif
                sf::CircleShape marker(8.0F, 4U);
                marker.setOrigin(sf::Vector2f{8.0F, 8.0F});
                marker.setPosition(screen);
                marker.setFillColor(sf::Color(238U, 214U, 98U));
                window.draw(marker);
#if TCP_VISUAL_SFML_TEXTURES
            }
#endif
        }

        if (selectedEntity.has_value()) {
            const auto trIt = transforms.find(selectedEntity.value());
            if (trIt != transforms.end()) {
                const auto screen = worldToScreen(trIt->second);
#if TCP_VISUAL_SFML_TEXTURES
                if (hasSelectionRingTexture) {
                    sf::Sprite ringSprite(texSelectionRing);
                    const auto size = texSelectionRing.getSize();
                    if (size.x > 0U && size.y > 0U) {
                        ringSprite.setOrigin(sf::Vector2f{static_cast<float>(size.x) * 0.5F, static_cast<float>(size.y) * 0.5F});
                        ringSprite.setScale(sf::Vector2f{42.0F / static_cast<float>(size.x), 42.0F / static_cast<float>(size.y)});
                    }
                    ringSprite.setPosition(screen);
                    window.draw(ringSprite);
                } else {
#endif
                    sf::CircleShape ring(22.0F);
                    ring.setOrigin(sf::Vector2f{22.0F, 22.0F});
                    ring.setPosition(screen);
                    ring.setFillColor(sf::Color(0U, 0U, 0U, 0U));
                    ring.setOutlineColor(sf::Color(223U, 236U, 116U));
                    ring.setOutlineThickness(2.5F);
                    window.draw(ring);
#if TCP_VISUAL_SFML_TEXTURES
                }
#endif
            }
        }

        sf::RectangleShape hudBg(sf::Vector2f{1230.0F, 42.0F});
        hudBg.setPosition(sf::Vector2f{20.0F, 18.0F});
        hudBg.setFillColor(sf::Color(12U, 17U, 12U, 210U));
        hudBg.setOutlineColor(sf::Color(55U, 95U, 61U));
        hudBg.setOutlineThickness(1.0F);
        window.draw(hudBg);

        window.display();

        std::ostringstream title;
        title << "TCP Visual Single | tick=" << world.currentTick() << " hash="
              << tcp::logic::debug::hashWorldState(world)
              << " entities=" << world.entityCount() << " sun0=" << world.sunForTeam(0)
              << " selected=";
        if (selectedEntity.has_value()) {
            title << selectedEntity.value();
        } else {
            title << "none";
        }
        title << " alpha=" << frameReport.interpolationPermille;
        window.setTitle(title.str());
    }

    return true;
#else
    (void)options;
    std::cerr << "visual: SFML path is unavailable. Reconfigure with -DTCP_ENABLE_SFML=ON and ensure SFML 2.6 is installed." << '\n';
    return false;
#endif
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2) {
        const std::string maybeHelp = argv[1];
        if (maybeHelp == "--help" || maybeHelp == "-h") {
            printUsage();
            return 0;
        }
    }

    AppOptions options{};
    if (!parseOptions(argc, argv, options)) {
        printUsage();
        return 1;
    }

    std::cout << "The Chlorophyll Protocol runtime driver demo" << '\n';

    if (options.visual) {
        if (options.mode == DemoMode::kReplay || options.mode == DemoMode::kLockstep) {
            std::cerr << "visual: only single mode is available in visual v1" << '\n';
            return 1;
        }
        return runVisualSingle(options) ? 0 : 1;
    }

    if (options.mode == DemoMode::kSingle) {
        return runSingle(options.ticks, options.replayPath) ? 0 : 1;
    }

    if (options.mode == DemoMode::kReplay) {
        return runReplay(options.ticks, options.replayPath) ? 0 : 1;
    }

    if (options.mode == DemoMode::kLockstep) {
        return runLockstep(options.ticks) ? 0 : 1;
    }

    if (!runSingle(options.ticks, options.replayPath)) {
        return 1;
    }
    if (!runReplay(options.ticks, options.replayPath)) {
        return 1;
    }
    return runLockstep(options.ticks) ? 0 : 1;
}
