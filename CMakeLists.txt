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
    src/logic/core/SimulationConfig.cpp
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
