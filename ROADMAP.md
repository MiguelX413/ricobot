# RicoBot NG roadmap

These are planned work areas. Each stage should preserve the existing
configuration and 32-bit GoldSrc plugin interface unless a compatibility
change is explicitly documented.

| Area | Planned work | Validation |
|---|---|---|
| C++ modernization | Replace remaining internal raw buffers and implicit ownership where practical. Confine legacy string types and SDK macros to engine-facing code. | Build both 32-bit targets, run clang-format and clang-tidy, and compare plugin exports. |
| Architecture | Separate plugin callbacks, configuration, bot state, navigation, and decision-making. Add test seams around engine calls. | Exercise config parsing and both console paths, then smoke-test loading, spawning, and map transitions in Ricochet. |
| Rust migration | Prototype isolated bot logic behind a narrow C ABI. Keep the C++ Metamod entry points during the incremental port. Review dependency provenance and licensing before adopting Rust modules. | Compare behavior against a C++ baseline and verify i686 Linux and Win32 linking, calling conventions, and exports. |
| Advanced AI | Develop movement and combat goal selection, including planned jumps, power-up choices, and opponent prediction. Start with deterministic scenarios and measurable server frame cost. | Compare goal completion, valid jump rate, and frame cost against the current bot. |
| Custom-map navigation | Model Ricochet platforms and launch routes as a directed graph. Support map-specific route data and validate jumps before choosing a route. Keep the existing movement available for maps without usable data. | Test stock and custom maps for unreachable routes, falls, and recovery. |
| Adaptive difficulty | Evaluate opt-in adjustments based on recent match performance. Bound the rate and range of changes and preserve fixed skill settings. | Measure stability across matches and verify that fixed-skill play is unaffected when adaptation is disabled. |

Resolve the open provenance and distribution questions in the
[licensing audit](LICENSING.md) before a public release.
