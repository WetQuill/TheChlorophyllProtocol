# The Chlorophyll Protocol

Deterministic C++20 RTS prototype with fixed-tick simulation, zombie wave defense, replay support, and lockstep command sync.

## Features

- **Deterministic simulation**: Fixed-point math (scale 1000), seeded RNG, lockstep-ready command sync
- **Zombie wave defense**: Zombies spawn in waves and autonomously pathfind toward the player HQ
- **Base building**: Military camps, sun power plants, and corn cannon bastion artillery
- **Unit production**: Train pea militia from military camps
- **Replay system**: Record and replay simulation runs with state hash verification
- **Fog of war**: Tile-based visibility per team
- **Isometric map rendering**: SFML-based visualization with 128×64 tile map

## Visualization v1 Plan

Single-mode visualization is implemented first, while replay/lockstep stay in CLI mode for now.

Execution steps:

1. Build SFML-backed app path behind `TCP_ENABLE_SFML=ON`.
2. Keep simulation authority in `src/logic`; renderer is read-only world presentation.
3. Drive logic with fixed ticks via `GameLoop`, render each frame independently.
4. Add single-mode interaction: left-click select controllable unit, right-click issue move command.
5. Add HUD/debug readout (tick/hash/entity count/sun/selection) without changing simulation rules.
6. Load placeholder textures by stable path when available, and fall back to procedural shapes if missing.

Out of scope for this v1:

- Replay window playback UI
- Lockstep multi-window visualization
- Final art integration (placeholders only)

## Build

### Windows (Visual Studio)

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug -j
```

To enable the visualization window path:

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DTCP_ENABLE_SFML=ON
cmake --build build --config Debug -j
```

### macOS / Linux

```bash
cmake -S . -B build -DTCP_ENABLE_SFML=ON
cmake --build build -j
```

Omit `-DTCP_ENABLE_SFML=ON` if SFML is not installed. Other optional flags: `-DTCP_ENABLE_ENTT=ON`, `-DTCP_ENABLE_GTEST=ON`.

## Run Tests

```bash
ctest --test-dir build --output-on-failure
```

## App Runtime Demo

Executable: `build/chlorophyll_app` (or `build/chlorophyll_app.exe` on Windows)

```bash
build/chlorophyll_app --mode all --ticks 8
```

Supported arguments:

- `--mode all|single|replay|lockstep`
- `--ticks N` (must be greater than 0)
- `--replay-path PATH` (used by `single` to save and `replay` to load)
- `--help`

Visualization argument:

- `--visual` (single-mode window view, requires SFML build path)

Visual placeholder assets:

- The visual path attempts to load placeholder textures from `assets/visual/` (see `assets/visual/PLACEHOLDER_ASSET_LIST.md`).
- If a texture is missing, rendering falls back to procedural shapes so the app remains runnable.

Visual controls (single mode):

- Left-click on a friendly controllable unit to select it.
- Left-click empty ground to clear current selection.
- Right-click with a selected unit to issue a move command.
- Right-click with no selected unit to open the build list.
- Left-click build list item #1 to place a pea military camp building.
- Left-click build list item #2 to place a sunflower power plant building that generates sun each tick.
- Left-click build list item #3 to place a corn cannon bastion defensive artillery.
- The top-left HUD area displays current Team 0 sun and power values.
- Building placement consumes both sun and power based on building type.
- Right-click a selected pea military camp to open a production menu and queue pea unit production (20 sun).
- Right-click with a selected combat unit on an enemy entity cell to issue an explicit attack order.
- Combat units no longer auto-acquire targets; they attack only after explicit attack commands.
- Corn cannon bastion uses scaled stats (sun 1200000, power requirement 80000, HP 2500000, armor 25000).
- Corn cannon bastion launches delayed ballistic shells with butter AOE (radius 200000) and applies frozen state for 150 ticks.
- Multiple units on the same grid cell are rendered as a stacked squad icon with count and average HP.
- Left-click a stacked squad to select the whole group.
- Right-click with a multi-unit selection issues a default group command (move or attack).
- Right-click the currently selected stack cell opens a per-unit menu to switch to single-unit control.

### Zombie Wave Defense

- Every 1500 ticks (~50 seconds), a wave of 5 zombies spawns near the right map boundary.
- Zombies (Team 2, HP 500, damage 20, melee range 1) autonomously pathfind toward the player HQ.
- If a blocking enemy (player plant or building) is within melee range, zombies switch to attack mode.
- Once the obstacle is destroyed, zombies resume marching toward the HQ.
- Zombie movement speed is reduced to 1/3 of normal units for balanced gameplay.
- Maximum 30 zombies can exist simultaneously; waves pause when the cap is reached.

### Debug Logging

Debug output is written to stderr via the `TCP_DEBUG(tag, tick, fmt, ...)` macro. Log categories:

| Tag | Source | Content |
|-----|--------|---------|
| `ENTITY` | `World.cpp` | Entity creation and destruction |
| `SPAWN` | `SpawnerSystem.cpp` | Zombie wave spawn events |
| `ZOMBIE` | `ZombieAISystem.cpp` | FSM state transitions, target selection |
| `PATH` | `BuiltInSystems.cpp` | Pathfinding results and failures |
| `COMBAT` | `BuiltInSystems.cpp` | Melee hits, projectile detonations, artillery fire |
| `CLEANUP` | `BuiltInSystems.cpp` | Dead entity cleanup summary |

To disable all debug logging, add `-DTCP_NO_DEBUG_LOG` to the compile flags.

Build placement behavior:

- Build placement snaps to the nearest grid cell from the click point.
- While the build menu is open, a highlighted preview cell shows the snapped build location.

Visual window defaults:

- The visual mode window now starts at `1600x900`.

Startup economy (demo world, fixed-point scale x1000):

- Team 0 and Team 1 each start with 6000000 sun and 160000 power.

Examples:

```bash
# CLI modes (cross-platform)
./build/chlorophyll_app --mode single --ticks 8
./build/chlorophyll_app --mode replay --ticks 8 --replay-path simulation_driver_demo_replay.txt
./build/chlorophyll_app --mode lockstep --ticks 8

# Visual mode (requires SFML)
./build/chlorophyll_app --mode single --visual
```
