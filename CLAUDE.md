# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Additional detailed rules live in `AGENTS.md` — read that for code style, naming conventions, testing expectations, and agent workflow rules.

## Build & Test

The existing `build/` was configured on Windows (MinGW). On macOS, reconfigure first:

```bash
cmake -S . -B build -DTCP_ENABLE_SFML=ON
cmake --build build -j
```

Omit `-DTCP_ENABLE_SFML=ON` if SFML is not installed. Other optional flags: `-DTCP_ENABLE_ENTT=ON`, `-DTCP_ENABLE_GTEST=ON`.

Run all tests:

```bash
ctest --test-dir build --output-on-failure
```

Run a single test by exact name:

```bash
ctest --test-dir build -R "^ReplayDeterminism$" --output-on-failure
```

Run tests matching a pattern:

```bash
ctest --test-dir build -R "Determinism" --output-on-failure
```

## Project Architecture

This is a deterministic C++20 RTS prototype with fixed-tick simulation, replay support, and lockstep command sync. The target is eventually multiplayer PvP over lockstep networking.

### Layer Separation

| Layer | Location | Role |
|---|---|---|
| **Logic (authoritative)** | `src/logic/` | Fixed-point simulation, ECS, combat, pathfinding, replay — must be deterministic |
| **App / Entry Point** | `src/app/` | CLI argument parsing, SFML rendering (single-mode visual via `--visual`), `GameLoop` frame scheduler |
| **Data** | `src/data/` | Unit config schemas and JSON loaders |
| **Net** | `src/net/` | Lockstep command sync over UDP/ENet |
| **Render** | `src/render/` | Reserved for SFML/OpenGL; read-only view of simulation state |

### Namespace Hierarchy

All code lives under `tcp`:
- `tcp::logic::ecs` — Entity-component system (`World`, components in `Components.h`, systems in `BuiltInSystems`)
- `tcp::logic::math` — Fixed-point arithmetic (`FixedPoint` using `int32_t`)
- `tcp::logic::core` — Tick scheduler, deterministic RNG, simulation config
- `tcp::logic::path` — A* grid pathfinding on fixed-point coordinates
- `tcp::logic::commands` — Command queue per entity, `PlayerCommand` types
- `tcp::logic::runtime` — `SimulationDriver` (dispatches between single/replay/lockstep modes)
- `tcp::logic::replay` — Replay recording (commands) and playback
- `tcp::logic::debug` — `StateHasher` for deterministic hash verification
- `tcp::net` — `CommandSyncController` for lockstep packet exchange
- `tcp::app` — `GameLoop` (fixed-tick-to-wall-clock bridge) and `Main.cpp` demo

### Simulation Driver Modes

`SimulationDriver` (`src/logic/runtime/SimulationDriver.h`) supports three modes:
1. **SingleLocal** — commands injected programmatically each tick, used for both CLI demo and visual mode
2. **Replay** — replays commands from a replay file, then compares final state hash
3. **Lockstep** — uses `CommandSyncController` to exchange command frames with delayed delivery, validates dual-instance determinism

### Critical Invariants

- **No floating-point types in `src/logic/`** (enforced by `tools/check_logic_no_float.py` test). Use `FixedPoint` for all positions, movement, and combat math.
- **Determinism is the prime constraint.** Use `StableOrder::sortDeterministic`, seeded `DeterministicRng`, and deterministic command application order. Never depend on wall-clock time, unordered iteration, or unseeded RNG inside logic.
- **Simulation/render separation.** `src/logic/` is authoritative. Rendering reads state; it never mutates simulation state or decides game outcomes.
- **Fixed-point scale is 1000** (`FixedPoint::kScale`). Positions like `FixedPoint::fromInt(5)` represent 5 grid cells; fractional positions use the 1000-unit sub-cell precision internally.

### ECS Design

The ECS is a hand-rolled sparse-map layout (not EnTT by default; EnTT is optional via `-DTCP_ENABLE_ENTT=ON`). Components are plain data structs in `src/logic/ecs/components/Components.h`. Systems are registered in `World::registerSystem(SystemPhase, callback)` and executed in phase order each tick. `BuiltInSystems.cpp` registers core systems: movement, combat, pathfinding, sun production, command processing.

### Adding a Test

New test executables follow the pattern in `CMakeLists.txt`:
1. `add_executable(test_name src/tests/TestFile.cpp)`
2. `target_link_libraries(test_name PRIVATE tcp_logic)`
3. `add_test(NAME TestName COMMAND test_name)`

Place tests under `src/tests/systems/`, `src/tests/integration/`, or `src/tests/regression/` by category.
