# The Chlorophyll Protocol

Deterministic C++20 RTS prototype scaffold with fixed-tick simulation, replay support, and lockstep command sync bootstrap.

## Build

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
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

Examples:

```bash
build/chlorophyll_app.exe --mode single --ticks 8
build/chlorophyll_app.exe --mode replay --ticks 8 --replay-path simulation_driver_demo_replay.txt
build/chlorophyll_app.exe --mode lockstep --ticks 8
```
