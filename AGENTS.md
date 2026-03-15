# AGENTS.md

## Purpose
This file provides operating rules for autonomous coding agents working in this repository.
Follow these instructions before making changes.

## Repository Snapshot (Current)
- Language target: C++20 (from project docs)
- Architecture target: ECS via EnTT
- Rendering/input target: SFML 2.6
- Networking target: lockstep command sync over UDP/ENet
- Simulation requirement: deterministic fixed-point logic in simulation code

## Source Files Reviewed
- `AGENT.md`
- `Plan.md`
- `InitialPlan.md`

## External Agent Rules
- Cursor rules: not found (`.cursor/rules/` missing, `.cursorrules` missing)
- Copilot rules: not found (`.github/copilot-instructions.md` missing)

If these files are added later, update this document and treat those rules as higher-priority constraints.

## Build / Lint / Test Commands

### Current Status
Build/test is now configured with CMake + CTest in repository root.
Use the commands below as the default workflow.

### Command Discovery Order
1. Use CMake workflow (`CMakeLists.txt` is present).
2. If CMake is unavailable in an environment, report toolchain missing; do not invent alternatives.

### Preferred CMake Workflow (when CMake exists)
- Configure:
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- Build:
  - `cmake --build build --config Debug -j`
- Run all tests:
  - `ctest --test-dir build --output-on-failure`
- Run single test by exact name:
  - `ctest --test-dir build -R "^DeterminismSmoke$" --output-on-failure`
- Run tests matching pattern:
  - `ctest --test-dir build -R "Determinism|Smoke" --output-on-failure`
- Re-run only failed tests:
  - `ctest --test-dir build --rerun-failed --output-on-failure`

### Dependency Integration Status
- SFML integration: optional via CMake option `-DTCP_ENABLE_SFML=ON` and installed SFML 2.6.
- EnTT integration: optional via CMake option `-DTCP_ENABLE_ENTT=ON` and available `entt/entt.hpp` include path.
- GoogleTest integration: optional via CMake option `-DTCP_ENABLE_GTEST=ON` and installed GTest.
- Current baseline tests use plain CTest executables and do not require GTest.

### Lint / Formatting (expected for C++)
Use these only when corresponding config files exist:
- Format check:
  - `clang-format --dry-run --Werror <files>`
- Apply formatting:
  - `clang-format -i <files>`
- Static analysis:
  - `clang-tidy <files> -- -std=c++20`
- Optional comprehensive checks:
  - `cppcheck --enable=warning,performance,portability src`

### Definition of Done for Changes
- Project config succeeds (e.g., CMake configure)
- Build succeeds
- Relevant tests pass (at minimum targeted tests)
- Formatting/lint passes or no formatter/linter is configured

## Code Style and Engineering Guidelines

### Determinism and Simulation
- Never use floating-point types in deterministic simulation code (`src/logic` target area).
- Use fixed-point arithmetic for positions, movement, cooldowns, and combat math.
- Keep simulation tick-driven and deterministic across machines.
- Avoid nondeterministic APIs in simulation (wall-clock time, unordered iteration instability, random without seeded deterministic PRNG).

### ECS Conventions (EnTT-oriented)
- Keep components as plain data (minimal behavior).
- Put game behavior in systems, not inside component structs.
- Systems should process stable, explicit component sets.
- Avoid hidden side effects between systems; prefer explicit event/command flow.
- Separate simulation state from rendering state.

### Rendering and Logic Separation
- Rendering/audio are view concerns only; no win/loss or authoritative gameplay rules there.
- Logic outputs state/events; rendering consumes them.
- Do not let frame rate affect simulation outcomes.

### Includes and Imports
- Prefer this include order in C++ files:
  1) matching header
  2) C++ standard library headers
  3) third-party headers (SFML, EnTT, ENet)
  4) project headers
- Use forward declarations where practical to reduce include coupling.
- Keep headers minimal; include only what is required.

### Formatting
- Follow any existing `.clang-format` if added; it is authoritative.
- If no formatter config exists, preserve surrounding style in touched files.
- Keep functions focused and short where reasonable.
- Avoid alignment churn and unrelated reformatting in the same change.

### Types and Memory
- Prefer fixed-width integer types for serialized/networked/simulation-critical data (`int32_t`, `uint16_t`, etc.).
- Use smart pointers for ownership; avoid raw owning pointers.
- Use object pooling for high-frequency transient entities (e.g., projectiles), per project notes.
- Pass heavy objects by reference/const reference.
- Mark non-throwing functions `noexcept` when appropriate and safe.

### Naming Conventions
- Types/classes/components/systems: `PascalCase`
- Functions/methods: `camelCase`
- Local variables/parameters: `camelCase`
- Constants/enums: `kPascalCase` or project-established style; stay consistent within a module
- File names: follow existing module convention; do not rename files without cause

### Error Handling
- Validate external inputs at boundaries (network packets, file/JSON data, config values).
- Fail fast on unrecoverable initialization errors with clear diagnostics.
- For recoverable runtime failures, return explicit error states/results instead of silent fallback.
- Never swallow errors in networking/sync code.
- Keep error messages actionable (what failed, where, and why).

### Networking / Lockstep Safety
- Exchange commands/inputs, not authoritative world states (except debug tooling).
- Serialize deterministically and version payload formats.
- Hash/checksum critical state at checkpoints for desync detection.
- Keep command application order explicit and deterministic.

### Testing Expectations
- Add or update tests for all non-trivial logic changes.
- Prioritize tests in deterministic domains:
  - fixed-point math operations
  - combat resolution
  - pathfinding determinism
  - lockstep command application
- Prefer small unit tests for systems/components and targeted integration tests for tick pipelines.
- When fixing a bug, add a regression test first if feasible.

### Performance and Safety
- Avoid per-tick heap churn in hot paths.
- Profile before large optimizations; document measured bottlenecks.
- Use data-oriented layouts where beneficial for ECS iteration.
- Be explicit about thread ownership and synchronization; avoid data races.

## Agent Workflow Rules
- Read relevant files before editing.
- Make minimal, scoped changes aligned with current architecture plans.
- Do not introduce new frameworks/build tools without clear project direction.
- Do not commit generated artifacts or local environment files.
- Document assumptions when repository scaffolding is missing.
- After completing each discrete task, record it in the `log/` folder.
- If `log/` does not exist, create it and append a brief entry (date/time, task summary, files changed, validation/tests run).
- When visual assets are required (models, textures, icons, UI images), explicitly request user-provided assets with a concrete delivery list.
- Each visual asset request must include exact file name, exact target repository path, intended usage, and expected format/spec (e.g., `.png` with transparency, resolution, sprite-sheet layout).
- Do not block implementation while waiting for final art: use temporary placeholders when feasible and clearly mark them in logs and TODO notes.
- After receiving user assets, place them in the agreed path and update references deterministically (no arbitrary renaming).

## Practical Notes for This Repository (Now)
- Repository currently appears to be planning/docs stage only.
- If you add initial scaffolding, also add and document:
  - concrete build command(s)
  - lint/format command(s)
  - test command(s)
  - single-test invocation examples
- Update this `AGENTS.md` immediately after toolchain choices become concrete.
