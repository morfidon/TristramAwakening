# AGENTS.md

## Top priority: Windows build after code changes
- This repository is actively built on Windows.
- After modifying code, agents should prefer the existing configured build directory at `build/vs2022-release`.
- Build command to use from that directory: `cmake --build . --config Release`
- If `devilutionx.exe` fails to link with `LNK1104`, first make sure the game is not currently running.
- Do not default to Linux/macOS examples like `-j $(getconf _NPROCESSORS_ONLN)` when working in this workspace.

This document gives AI/code agents a fast operational guide for working in this repository.

## Project identity
- **Repository**: `TristramAwakening` (fork/derivative of DevilutionX)
- **Primary language**: C++ (CMake-based)
- **Goal**: Cross-platform Diablo/Hellfire engine port with gameplay, UI, networking, and platform packaging support.

## Primary working areas
- `Source/`: Main engine/game code (core gameplay, rendering, systems, UI, networking wrappers).
- `test/`: Unit/integration/benchmark tests.
- `assets/`: Built-in runtime assets and data files.
- `docs/`: Developer and user-facing documentation.
- `CMake/` + `CMakeLists.txt`: Build system logic.
- `Packaging/`: Platform/distribution scripts and packaging rules.
- `3rdParty/`: Vendored third-party dependencies.
- `Translations/`: Localization files (`.po`, `.pot`, glossary).

## Recommended workflow for agents
1. **Understand scope first**
   - Read `README.md` and relevant files under `docs/` for context.
   - Touch only files required by the request.
2. **Preserve architecture boundaries**
   - Put engine/gameplay logic under `Source/`.
   - Put tests under `test/`.
   - Avoid editing vendored sources under `3rdParty/` unless explicitly requested.
3. **Use CMake-native commands**
   - Configure: `cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release`
   - Build: `cmake --build build -j $(getconf _NPROCESSORS_ONLN)`
4. **Validate changes**
   - Run relevant tests when feasible:
     - `ctest --test-dir build --output-on-failure`
   - If full test/build is not feasible, run targeted checks and clearly report limitations.
5. **Keep changes reviewable**
   - Make minimal, focused edits.
   - Avoid broad refactors unless asked.

## Conventions and guardrails
- Prefer existing patterns in neighboring files over introducing new style variants.
- Do not add new heavy dependencies without clear justification.
- Keep platform-specific logic isolated to existing platform modules.
- Keep documentation updated when behavior or structure changes.

## High-value files for orientation
- `README.md`: Project purpose and contributor entry points.
- `docs/building.md`: Build/dependency instructions across platforms.
- `CMakeLists.txt`: Top-level targets and options.
- `structure.md`: Repository structure map and component responsibilities.

## When uncertain
- Favor conservative edits.
- Document assumptions in commit message/PR body.
- Leave clear notes on any unverified paths (missing deps, environment constraints, etc.).
