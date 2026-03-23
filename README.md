# The Chlorophyll Protocol

Deterministic C++20 RTS prototype scaffold with fixed-tick simulation, replay support, and lockstep command sync bootstrap.

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

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug -j
```

To enable the visualization window path:

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DTCP_ENABLE_SFML=ON
cmake --build build --config Debug -j
```

## Run Tests

```bash
ctest --test-dir build -C Debug --output-on-failure
```

## App Runtime Demo

Executable: `build/chlorophyll_app.exe`

```bash
build/chlorophyll_app.exe --mode all --ticks 8
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

Build placement behavior:

- Build placement snaps to the nearest grid cell from the click point.
- While the build menu is open, a highlighted preview cell shows the snapped build location.

Visual window defaults:

- The visual mode window now starts at `1600x900`.

Startup economy (demo world, fixed-point scale x1000):

- Team 0 and Team 1 each start with 6000000 sun and 160000 power.

Examples:

```bash
build/chlorophyll_app.exe --mode single --ticks 8
build/chlorophyll_app.exe --mode replay --ticks 8 --replay-path simulation_driver_demo_replay.txt
build/chlorophyll_app.exe --mode lockstep --ticks 8
build/chlorophyll_app.exe --mode single --visual
```
