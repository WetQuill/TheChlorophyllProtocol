cmake_minimum_required(VERSION 3.21)

project(TheChlorophyllProtocol VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TCP_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(TCP_ENABLE_SFML "Enable SFML integration if installed" OFF)
option(TCP_ENABLE_ENTT "Enable EnTT integration if installed" OFF)
option(TCP_ENABLE_GTEST "Enable GoogleTest integration if installed" OFF)

add_library(tcp_logic STATIC
    src/data/UnitConfigLoader.cpp
    src/logic/commands/CommandQueue.cpp
    src/logic/core/DeterministicRng.cpp
    src/logic/core/SimulationConfig.cpp
    src/logic/core/TickScheduler.cpp
    src/logic/debug/StateHasher.cpp
    src/logic/ecs/World.cpp
    src/logic/ecs/systems/BuiltInSystems.cpp
    src/logic/math/FixedPoint.cpp
    src/logic/factory/UnitFactory.cpp
    src/logic/path/AStarGrid.cpp
    src/logic/replay/ReplayPlayer.cpp
    src/logic/replay/ReplayRecorder.cpp
)

target_include_directories(tcp_logic
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

if(TCP_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(tcp_logic PRIVATE /W4 /permissive-)
    else()
        target_compile_options(tcp_logic PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

add_library(tcp_deps INTERFACE)

if(TCP_ENABLE_SFML)
    find_package(SFML 2.6 COMPONENTS graphics window system QUIET)
    if(SFML_FOUND)
        target_link_libraries(tcp_deps INTERFACE sfml-graphics sfml-window sfml-system)
        target_compile_definitions(tcp_deps INTERFACE TCP_HAS_SFML=1)
    else()
        message(WARNING "TCP_ENABLE_SFML=ON but SFML 2.6 not found. Building without SFML linkage.")
    endif()
endif()

if(TCP_ENABLE_ENTT)
    find_path(ENTT_INCLUDE_DIR entt/entt.hpp)
    if(ENTT_INCLUDE_DIR)
        target_include_directories(tcp_deps INTERFACE ${ENTT_INCLUDE_DIR})
        target_compile_definitions(tcp_deps INTERFACE TCP_HAS_ENTT=1)
    else()
        message(WARNING "TCP_ENABLE_ENTT=ON but EnTT headers not found. Building without EnTT includes.")
    endif()
endif()

if(TCP_ENABLE_GTEST)
    find_package(GTest QUIET)
    if(GTest_FOUND)
        target_compile_definitions(tcp_deps INTERFACE TCP_HAS_GTEST=1)
    else()
        message(WARNING "TCP_ENABLE_GTEST=ON but GTest not found. Falling back to plain CTest executables.")
    endif()
endif()

add_executable(chlorophyll_app
    src/app/Main.cpp
)

target_link_libraries(chlorophyll_app
    PRIVATE
        tcp_logic
        tcp_deps
)

enable_testing()

add_executable(determinism_smoke_test
    src/tests/DeterminismSmokeTest.cpp
)

target_link_libraries(determinism_smoke_test PRIVATE tcp_logic)

add_test(NAME DeterminismSmoke COMMAND determinism_smoke_test)

add_executable(fixed_point_test
    src/tests/FixedPointTest.cpp
)

target_link_libraries(fixed_point_test PRIVATE tcp_logic)

add_test(NAME FixedPointMath COMMAND fixed_point_test)

add_executable(deterministic_rng_test
    src/tests/DeterministicRngTest.cpp
)

target_link_libraries(deterministic_rng_test PRIVATE tcp_logic)

add_test(NAME DeterministicRngSequence COMMAND deterministic_rng_test)

add_executable(tick_scheduler_test
    src/tests/TickSchedulerTest.cpp
)

target_link_libraries(tick_scheduler_test PRIVATE tcp_logic)

add_test(NAME TickSchedulerDeterminism COMMAND tick_scheduler_test)

add_executable(ecs_world_test
    src/tests/EcsWorldTest.cpp
)

target_link_libraries(ecs_world_test PRIVATE tcp_logic)

add_test(NAME EcsWorldPipeline COMMAND ecs_world_test)

add_executable(ecs_core_systems_test
    src/tests/EcsCoreSystemsTest.cpp
)

target_link_libraries(ecs_core_systems_test PRIVATE tcp_logic)

add_test(NAME EcsCoreSystems COMMAND ecs_core_systems_test)

add_executable(gameplay_loop_test
    src/tests/GameplayLoopTest.cpp
)

target_link_libraries(gameplay_loop_test PRIVATE tcp_logic)

add_test(NAME GameplayLoopCore COMMAND gameplay_loop_test)

add_executable(placement_pathfinding_test
    src/tests/PlacementAndPathfindingTest.cpp
)

target_link_libraries(placement_pathfinding_test PRIVATE tcp_logic)

add_test(NAME PlacementAndPathfinding COMMAND placement_pathfinding_test)

add_executable(replay_determinism_test
    src/tests/ReplayDeterminismTest.cpp
)

target_link_libraries(replay_determinism_test PRIVATE tcp_logic)

add_test(NAME ReplayDeterminism COMMAND replay_determinism_test)

add_executable(unit_config_factory_test
    src/tests/UnitConfigFactoryTest.cpp
)

target_link_libraries(unit_config_factory_test PRIVATE tcp_logic)

add_test(NAME UnitConfigFactory COMMAND unit_config_factory_test)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME LogicNoFloatCheck
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check_logic_no_float.py ${PROJECT_SOURCE_DIR}/src/logic
    )
else()
    message(WARNING "Python3 interpreter not found, LogicNoFloatCheck test is disabled.")
endif()
