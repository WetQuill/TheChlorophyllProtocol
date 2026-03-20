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

    world.setSunForTeam(0, 120);
    world.setSunForTeam(1, 120);
    world.setPowerForTeam(0, 80);
    world.setPowerForTeam(1, 80);

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
    constexpr std::uint32_t kPeaMilitaryCampArchetypeId = 901U;
    constexpr std::uint32_t kSunPowerPlantArchetypeId = 902U;
    constexpr std::int32_t kPeaMilitaryCampCostSun = 20;
    constexpr std::int32_t kSunPowerPlantCostSun = 25;

    tcp::logic::runtime::SimulationDriver driver(makeDemoWorld());
    driver.useSingleLocalMode();

    const tcp::logic::SimulationConfig config{};
    tcp::app::GameLoop loop(config);

    constexpr unsigned int kWindowWidth = 1600U;
    constexpr unsigned int kWindowHeight = 900U;
    constexpr float kCellPixels = 48.0F;
    constexpr float kEntityRadius = 16.0F;
    constexpr float kWorldOriginX = 320.0F;
    constexpr float kWorldOriginY = 210.0F;

    sf::RenderWindow window(sf::VideoMode(kWindowWidth, kWindowHeight), "The Chlorophyll Protocol - Visual Single");
    window.setFramerateLimit(60U);

    struct MoveMarker {
        std::int32_t x{0};
        std::int32_t y{0};
        std::int64_t expireTick{-1};
    };

    struct BuildMenuState {
        sf::Vector2f anchor{};
        std::int32_t gridX{0};
        std::int32_t gridY{0};
    };

    std::optional<tcp::logic::ecs::EntityId> selectedEntity;
    std::optional<MoveMarker> moveMarker;
    std::optional<BuildMenuState> buildMenu;
    auto previousFrameTime = std::chrono::steady_clock::now();

#if TCP_VISUAL_SFML_TEXTURES
    sf::Texture texHq0;
    sf::Texture texHq1;
    sf::Texture texPeaMilitia;
    sf::Texture texSunflower;
    sf::Texture texSunPowerPlant;
    sf::Texture texPeaMilitaryCamp;
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
    const bool hasSunPowerPlantTexture =
        loadTexture(texSunPowerPlant, "assets/visual/buildings/sunflower_power_plant.png") ||
        loadTexture(texSunPowerPlant, "assets/visual/units/sunflower_generator.png");
    const bool hasPeaMilitaryCampTexture = loadTexture(texPeaMilitaryCamp, "assets/visual/buildings/pea_military_camp.png");
    const bool hasSelectionRingTexture = loadTexture(texSelectionRing, "assets/visual/fx/selection_ring.png");
    const bool hasMoveMarkerTexture = loadTexture(texMoveMarker, "assets/visual/fx/move_target_marker.png");
#endif

    const auto worldToScreen = [&](const tcp::logic::ecs::Transform& tr) {
        return sf::Vector2f{
            kWorldOriginX + (static_cast<float>(tr.x.raw()) / 1000.0F) * kCellPixels,
            kWorldOriginY + (static_cast<float>(tr.y.raw()) / 1000.0F) * kCellPixels,
        };
    };

    const auto mouseToGrid = [&](const sf::Vector2i pixelPos) {
        const float gx = (static_cast<float>(pixelPos.x) - kWorldOriginX) / kCellPixels;
        const float gy = (static_cast<float>(pixelPos.y) - kWorldOriginY) / kCellPixels;
        return std::pair<std::int32_t, std::int32_t>{
            static_cast<std::int32_t>(std::lround(gx)),
            static_cast<std::int32_t>(std::lround(gy)),
        };
    };

    const auto snapBuildTargetFromMenu = [&](const BuildMenuState& state) {
        return std::pair<std::int32_t, std::int32_t>{state.gridX, state.gridY};
    };

    const auto buildMenuCampRect = [&](const BuildMenuState& state) {
        return sf::FloatRect{state.anchor.x, state.anchor.y, 196.0F, 52.0F};
    };

    const auto buildMenuSunPlantRect = [&](const BuildMenuState& state) {
        return sf::FloatRect{state.anchor.x, state.anchor.y + 56.0F, 196.0F, 52.0F};
    };

    const auto pointInRect = [](const sf::FloatRect& rect, const sf::Vector2f point) {
        return point.x >= rect.left && point.x <= (rect.left + rect.width) &&
               point.y >= rect.top && point.y <= (rect.top + rect.height);
    };

    const auto findBuildIssuer = [&]() -> std::optional<tcp::logic::ecs::EntityId> {
        const auto& world = driver.world();
        const auto& teams = world.teams();
        const auto& commandBuffers = world.commandBuffers();
        std::optional<tcp::logic::ecs::EntityId> best;
        for (const auto& [entityId, buffer] : commandBuffers) {
            (void)buffer;
            const auto teamIt = teams.find(entityId);
            if (teamIt == teams.end() || teamIt->second.value != 0U) {
                continue;
            }
            if (!best.has_value() || entityId < best.value()) {
                best = entityId;
            }
        }
        return best;
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
                    const sf::Vector2f clickPos{static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y)};

                    bool consumedByBuildMenu = false;
                    if (buildMenu.has_value()) {
                        const auto campRect = buildMenuCampRect(buildMenu.value());
                        const auto sunPlantRect = buildMenuSunPlantRect(buildMenu.value());
                        const auto issuer = findBuildIssuer();
                        if (issuer.has_value()) {
                            const auto [snappedX, snappedY] = snapBuildTargetFromMenu(buildMenu.value());
                            if (pointInRect(campRect, clickPos)) {
                                driver.queueLocalCommand(
                                    0,
                                    issuer.value(),
                                    tcp::logic::ecs::CommandType::kBuild,
                                    snappedX,
                                    snappedY,
                                    kPeaMilitaryCampCostSun);
                                consumedByBuildMenu = true;
                            } else if (pointInRect(sunPlantRect, clickPos)) {
                                driver.queueLocalCommand(
                                    0,
                                    issuer.value(),
                                    tcp::logic::ecs::CommandType::kBuildSunPowerPlant,
                                    snappedX,
                                    snappedY,
                                    kSunPowerPlantCostSun);
                                consumedByBuildMenu = true;
                            }
                        }
                        buildMenu.reset();
                    }

                    if (consumedByBuildMenu) {
                        continue;
                    }

                    const auto& world = driver.world();
                    const auto& transforms = world.transforms();
                    const auto& teams = world.teams();
                    const auto& commandBuffers = world.commandBuffers();

                    std::optional<tcp::logic::ecs::EntityId> best;
                    float bestDistSq = 999999.0F;
                    constexpr float kSelectRadiusPixels = 26.0F;
                    constexpr float kSelectRadiusSq = kSelectRadiusPixels * kSelectRadiusPixels;
                    for (const auto& [entityId, tr] : transforms) {
                        if (commandBuffers.find(entityId) == commandBuffers.end()) {
                            continue;
                        }
                        const auto teamIt = teams.find(entityId);
                        if (teamIt == teams.end() || teamIt->second.value != 0U) {
                            continue;
                        }

                        const auto screen = worldToScreen(tr);
                        const float dx = screen.x - clickPos.x;
                        const float dy = screen.y - clickPos.y;
                        const float distSq = (dx * dx) + (dy * dy);
                        if (distSq < bestDistSq && distSq <= kSelectRadiusSq) {
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
                        buildMenu.reset();
                    }
                } else if (event.mouseButton.button == sf::Mouse::Right) {
                    const auto [gx, gy] = mouseToGrid(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                    buildMenu = BuildMenuState{
                        sf::Vector2f{static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y)},
                        gx,
                        gy,
                    };
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

        const float worldLeft = 20.0F;
        const float hudTop = 18.0F;
        const float hudHeight = 58.0F;
        const float worldTop = hudTop + hudHeight + 10.0F;
        const float worldRight = static_cast<float>(window.getSize().x) - 20.0F;
        const float worldBottom = static_cast<float>(window.getSize().y) - 20.0F;

        const int gridMinX = static_cast<int>(std::floor((worldLeft - kWorldOriginX) / kCellPixels)) - 1;
        const int gridMaxX = static_cast<int>(std::ceil((worldRight - kWorldOriginX) / kCellPixels)) + 1;
        const int gridMinY = static_cast<int>(std::floor((worldTop - kWorldOriginY) / kCellPixels)) - 1;
        const int gridMaxY = static_cast<int>(std::ceil((worldBottom - kWorldOriginY) / kCellPixels)) + 1;

        const float gridActualLeft = kWorldOriginX + static_cast<float>(gridMinX) * kCellPixels;
        const float gridActualRight = kWorldOriginX + static_cast<float>(gridMaxX) * kCellPixels;
        const float gridActualTop = kWorldOriginY + static_cast<float>(gridMinY) * kCellPixels;
        const float gridActualBottom = kWorldOriginY + static_cast<float>(gridMaxY) * kCellPixels;
        const float gridActualWidth = gridActualRight - gridActualLeft;
        const float gridActualHeight = gridActualBottom - gridActualTop;

        for (int gx = gridMinX; gx <= gridMaxX; ++gx) {
            sf::RectangleShape vline(sf::Vector2f{1.0F, gridActualHeight});
            vline.setFillColor(sf::Color(36U, 45U, 36U));
            vline.setPosition(sf::Vector2f{kWorldOriginX + static_cast<float>(gx) * kCellPixels, gridActualTop});
            window.draw(vline);
        }
        for (int gy = gridMinY; gy <= gridMaxY; ++gy) {
            sf::RectangleShape hline(sf::Vector2f{gridActualWidth, 1.0F});
            hline.setFillColor(sf::Color(36U, 45U, 36U));
            hline.setPosition(sf::Vector2f{gridActualLeft, kWorldOriginY + static_cast<float>(gy) * kCellPixels});
            window.draw(hline);
        }

        const auto& world = driver.world();
        const auto& transforms = world.transforms();
        const auto& teams = world.teams();
        const auto& healths = world.healths();
        const auto& hqs = world.headquarters();
        const auto& buildings = world.buildings();
        const auto& identities = world.identities();
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
            const bool isBuilding = buildings.find(entityId) != buildings.end();
            const bool isSunProducer = sunProducers.find(entityId) != sunProducers.end();
            const auto idIt = identities.find(entityId);
            const bool isPeaMilitaryCamp =
                (idIt != identities.end() && idIt->second.archetypeId == kPeaMilitaryCampArchetypeId);
            const bool isSunPowerPlant =
                (idIt != identities.end() && idIt->second.archetypeId == kSunPowerPlantArchetypeId);

            bool drewSprite = false;
#if TCP_VISUAL_SFML_TEXTURES
            if (isHq || isBuilding) {
                const sf::Texture* texture = nullptr;
                if (isPeaMilitaryCamp && hasPeaMilitaryCampTexture) {
                    texture = &texPeaMilitaryCamp;
                } else if (isSunPowerPlant && hasSunPowerPlantTexture) {
                    texture = &texSunPowerPlant;
                } else if (teamOne && hasHq1Texture) {
                    texture = &texHq1;
                } else if (!teamOne && hasHq0Texture) {
                    texture = &texHq0;
                }

                if (texture != nullptr) {
                    sf::Sprite sprite(*texture);
                    const auto size = texture->getSize();
                    if (size.x > 0U && size.y > 0U) {
                        sprite.setOrigin(sf::Vector2f{static_cast<float>(size.x) * 0.5F, static_cast<float>(size.y) * 0.5F});
                        const float targetSize = (isPeaMilitaryCamp || isSunPowerPlant) ? 62.0F : 54.0F;
                        sprite.setScale(sf::Vector2f{targetSize / static_cast<float>(size.x), targetSize / static_cast<float>(size.y)});
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

            if ((isHq || isBuilding) && !drewSprite) {
                sf::RectangleShape hq(sf::Vector2f{54.0F, 54.0F});
                hq.setOrigin(sf::Vector2f{27.0F, 27.0F});
                hq.setPosition(screen);
                if (isPeaMilitaryCamp) {
                    hq.setFillColor(teamOne ? sf::Color(170U, 137U, 63U) : sf::Color(65U, 133U, 81U));
                } else if (isSunPowerPlant) {
                    hq.setFillColor(teamOne ? sf::Color(221U, 183U, 84U) : sf::Color(171U, 194U, 88U));
                } else {
                    hq.setFillColor(teamOne ? sf::Color(176U, 110U, 32U) : sf::Color(58U, 162U, 74U));
                }
                hq.setOutlineColor(sf::Color(22U, 22U, 22U));
                hq.setOutlineThickness(2.0F);
                window.draw(hq);
            } else if (!isHq && !isBuilding && !drewSprite) {
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

        if (buildMenu.has_value()) {
            const auto [snapX, snapY] = snapBuildTargetFromMenu(buildMenu.value());
            tcp::logic::ecs::Transform buildPreview{};
            buildPreview.x = tcp::logic::math::FixedPoint::fromInt(snapX);
            buildPreview.y = tcp::logic::math::FixedPoint::fromInt(snapY);
            const auto screen = worldToScreen(buildPreview);

            sf::RectangleShape buildCell(sf::Vector2f{kCellPixels - 4.0F, kCellPixels - 4.0F});
            buildCell.setOrigin(sf::Vector2f{(kCellPixels - 4.0F) * 0.5F, (kCellPixels - 4.0F) * 0.5F});
            buildCell.setPosition(screen);
            buildCell.setFillColor(sf::Color(186U, 214U, 126U, 70U));
            buildCell.setOutlineColor(sf::Color(202U, 236U, 138U, 210U));
            buildCell.setOutlineThickness(2.0F);
            window.draw(buildCell);
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

        if (buildMenu.has_value()) {
            auto campRect = buildMenuCampRect(buildMenu.value());
            auto sunPlantRect = buildMenuSunPlantRect(buildMenu.value());
            const float maxMenuRight = static_cast<float>(window.getSize().x) - 20.0F;
            const float maxMenuBottom = static_cast<float>(window.getSize().y) - 20.0F;
            if ((sunPlantRect.left + sunPlantRect.width) > maxMenuRight) {
                const float offset = maxMenuRight - (sunPlantRect.left + sunPlantRect.width);
                campRect.left += offset;
                sunPlantRect.left += offset;
            }
            if ((sunPlantRect.top + sunPlantRect.height) > maxMenuBottom) {
                const float offset = maxMenuBottom - (sunPlantRect.top + sunPlantRect.height);
                campRect.top += offset;
                sunPlantRect.top += offset;
            }

            sf::RectangleShape campBg(sf::Vector2f{campRect.width, campRect.height});
            campBg.setPosition(sf::Vector2f{campRect.left, campRect.top});
            campBg.setFillColor(sf::Color(23U, 30U, 23U, 230U));
            campBg.setOutlineColor(sf::Color(88U, 128U, 84U));
            campBg.setOutlineThickness(2.0F);
            window.draw(campBg);

            sf::RectangleShape sunBg(sf::Vector2f{sunPlantRect.width, sunPlantRect.height});
            sunBg.setPosition(sf::Vector2f{sunPlantRect.left, sunPlantRect.top});
            sunBg.setFillColor(sf::Color(28U, 36U, 24U, 230U));
            sunBg.setOutlineColor(sf::Color(102U, 146U, 90U));
            sunBg.setOutlineThickness(2.0F);
            window.draw(sunBg);

#if TCP_VISUAL_SFML_TEXTURES
            if (hasPeaMilitaryCampTexture) {
                sf::Sprite campPreview(texPeaMilitaryCamp);
                const auto size = texPeaMilitaryCamp.getSize();
                if (size.x > 0U && size.y > 0U) {
                    campPreview.setOrigin(sf::Vector2f{static_cast<float>(size.x) * 0.5F, static_cast<float>(size.y) * 0.5F});
                    campPreview.setScale(sf::Vector2f{34.0F / static_cast<float>(size.x), 34.0F / static_cast<float>(size.y)});
                }
                campPreview.setPosition(sf::Vector2f{campRect.left + 28.0F, campRect.top + 26.0F});
                window.draw(campPreview);
            } else {
#endif
                sf::RectangleShape campPreview(sf::Vector2f{30.0F, 30.0F});
                campPreview.setOrigin(sf::Vector2f{15.0F, 15.0F});
                campPreview.setPosition(sf::Vector2f{campRect.left + 28.0F, campRect.top + 26.0F});
                campPreview.setFillColor(sf::Color(91U, 142U, 95U));
                campPreview.setOutlineColor(sf::Color(26U, 26U, 26U));
                campPreview.setOutlineThickness(2.0F);
                window.draw(campPreview);
#if TCP_VISUAL_SFML_TEXTURES
            }
#endif

#if TCP_VISUAL_SFML_TEXTURES
            if (hasSunPowerPlantTexture) {
                sf::Sprite sunPreview(texSunPowerPlant);
                const auto size = texSunPowerPlant.getSize();
                if (size.x > 0U && size.y > 0U) {
                    sunPreview.setOrigin(sf::Vector2f{static_cast<float>(size.x) * 0.5F, static_cast<float>(size.y) * 0.5F});
                    sunPreview.setScale(sf::Vector2f{34.0F / static_cast<float>(size.x), 34.0F / static_cast<float>(size.y)});
                }
                sunPreview.setPosition(sf::Vector2f{sunPlantRect.left + 28.0F, sunPlantRect.top + 26.0F});
                window.draw(sunPreview);
            } else {
#endif
                sf::CircleShape sunPreview(14.0F, 8U);
                sunPreview.setOrigin(sf::Vector2f{14.0F, 14.0F});
                sunPreview.setPosition(sf::Vector2f{sunPlantRect.left + 28.0F, sunPlantRect.top + 26.0F});
                sunPreview.setFillColor(sf::Color(235U, 193U, 81U));
                sunPreview.setOutlineColor(sf::Color(49U, 41U, 19U));
                sunPreview.setOutlineThickness(2.0F);
                window.draw(sunPreview);
#if TCP_VISUAL_SFML_TEXTURES
            }
#endif

            sf::RectangleShape labelBar(sf::Vector2f{126.0F, 12.0F});
            labelBar.setPosition(sf::Vector2f{campRect.left + 50.0F, campRect.top + 20.0F});
            labelBar.setFillColor(sf::Color(166U, 214U, 164U));
            window.draw(labelBar);

            sf::RectangleShape costBar(sf::Vector2f{58.0F, 8.0F});
            costBar.setPosition(sf::Vector2f{campRect.left + 50.0F, campRect.top + 36.0F});
            costBar.setFillColor(sf::Color(236U, 212U, 103U));
            window.draw(costBar);

            sf::RectangleShape sunLabelBar(sf::Vector2f{126.0F, 12.0F});
            sunLabelBar.setPosition(sf::Vector2f{sunPlantRect.left + 50.0F, sunPlantRect.top + 20.0F});
            sunLabelBar.setFillColor(sf::Color(208U, 229U, 132U));
            window.draw(sunLabelBar);

            sf::RectangleShape sunCostBar(sf::Vector2f{72.0F, 8.0F});
            sunCostBar.setPosition(sf::Vector2f{sunPlantRect.left + 50.0F, sunPlantRect.top + 36.0F});
            sunCostBar.setFillColor(sf::Color(244U, 207U, 96U));
            window.draw(sunCostBar);
        }

        const float hudWidth = std::max(360.0F, static_cast<float>(window.getSize().x) - 40.0F);
        sf::RectangleShape hudBg(sf::Vector2f{hudWidth, 58.0F});
        hudBg.setPosition(sf::Vector2f{20.0F, 18.0F});
        hudBg.setFillColor(sf::Color(12U, 17U, 12U, 210U));
        hudBg.setOutlineColor(sf::Color(55U, 95U, 61U));
        hudBg.setOutlineThickness(1.0F);
        window.draw(hudBg);

        const auto drawSevenSegmentDigit = [&](int digit, const sf::Vector2f origin, const sf::Color color) {
            static constexpr std::array<std::uint8_t, 10> kDigitMasks = {
                0b0111111,  // 0
                0b0000110,  // 1
                0b1011011,  // 2
                0b1001111,  // 3
                0b1100110,  // 4
                0b1101101,  // 5
                0b1111101,  // 6
                0b0000111,  // 7
                0b1111111,  // 8
                0b1101111,  // 9
            };

            if (digit < 0 || digit > 9) {
                return;
            }

            const float w = 8.0F;
            const float h = 10.0F;
            const float t = 3.0F;
            const auto mask = kDigitMasks[static_cast<std::size_t>(digit)];

            const auto drawSegment = [&](bool enabled, const sf::Vector2f pos, const sf::Vector2f size) {
                if (!enabled) {
                    return;
                }
                sf::RectangleShape seg(size);
                seg.setPosition(pos);
                seg.setFillColor(color);
                window.draw(seg);
            };

            drawSegment((mask & 0b0000001U) != 0U, sf::Vector2f{origin.x, origin.y}, sf::Vector2f{w, t});
            drawSegment((mask & 0b0000010U) != 0U, sf::Vector2f{origin.x + w - t, origin.y}, sf::Vector2f{t, h});
            drawSegment((mask & 0b0000100U) != 0U, sf::Vector2f{origin.x + w - t, origin.y + h}, sf::Vector2f{t, h});
            drawSegment((mask & 0b0001000U) != 0U, sf::Vector2f{origin.x, origin.y + (2.0F * h)}, sf::Vector2f{w, t});
            drawSegment((mask & 0b0010000U) != 0U, sf::Vector2f{origin.x, origin.y + h}, sf::Vector2f{t, h});
            drawSegment((mask & 0b0100000U) != 0U, sf::Vector2f{origin.x, origin.y}, sf::Vector2f{t, h});
            drawSegment((mask & 0b1000000U) != 0U, sf::Vector2f{origin.x, origin.y + h}, sf::Vector2f{w, t});
        };

        const auto drawNumber = [&](int value, const sf::Vector2f origin, const sf::Color color) {
            const std::string text = std::to_string(std::max(0, value));
            for (std::size_t i = 0; i < text.size(); ++i) {
                const int digit = text[i] - '0';
                drawSevenSegmentDigit(digit, sf::Vector2f{origin.x + static_cast<float>(i) * 18.0F, origin.y}, color);
            }
        };

        const int sunValue = world.sunForTeam(0);
        const int powerValue = world.powerForTeam(0);

        sf::CircleShape sunIcon(6.0F, 8U);
        sunIcon.setPosition(sf::Vector2f{36.0F, 24.0F});
        sunIcon.setFillColor(sf::Color(243U, 204U, 82U));
        window.draw(sunIcon);

        sf::RectangleShape powerIcon(sf::Vector2f{8.0F, 12.0F});
        powerIcon.setPosition(sf::Vector2f{36.0F, 48.0F});
        powerIcon.setFillColor(sf::Color(118U, 208U, 240U));
        window.draw(powerIcon);

        drawNumber(sunValue, sf::Vector2f{54.0F, 22.0F}, sf::Color(246U, 218U, 116U));
        drawNumber(powerValue, sf::Vector2f{54.0F, 46.0F}, sf::Color(162U, 226U, 246U));

        window.display();

        std::ostringstream title;
        title << "TCP Visual Single | tick=" << world.currentTick() << " hash="
              << tcp::logic::debug::hashWorldState(world)
              << " entities=" << world.entityCount() << " sun0=" << world.sunForTeam(0)
              << " power0=" << world.powerForTeam(0)
              << " selected=";
        if (selectedEntity.has_value()) {
            title << selectedEntity.value();
        } else {
            title << "none";
        }
        if (buildMenu.has_value()) {
            const auto [snapX, snapY] = snapBuildTargetFromMenu(buildMenu.value());
            title << " | build-menu camp/solar @(" << snapX << ',' << snapY << ')';
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
