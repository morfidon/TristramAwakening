# Repository Structure (`TristramAwakening`)

This file maps the project layout and explains what each top-level area is responsible for.

## Top-level hierarchy

```text
.
├── Source/              # Main C++ game/engine code
├── test/                # Unit tests, integration tests, benchmarks
├── assets/              # Runtime assets bundled with the project
├── docs/                # Documentation (build/install/changelog/manual links)
├── CMake/               # CMake modules, helper functions, toolchains/finders
├── Packaging/           # Platform packaging scripts/configuration
├── 3rdParty/            # Vendored third-party libraries
├── Translations/        # Localization files (.po/.pot + glossary)
├── android-project/     # Android-specific Gradle project
├── uwp-project/         # UWP-specific project files
├── mods/                # Mod-related content/examples
├── tools/               # Developer helper tools/scripts
├── .github/             # CI workflows and GitHub metadata
├── CMakeLists.txt       # Root build definition
├── README.md            # Project overview and entry point
└── vcpkg.json           # Package manifest/dependency metadata
```

## Detailed breakdown

### `Source/` (core implementation)
Primary game code, grouped by concern. Notable subareas:
- `Source/DiabloUI/`: UI framework/screens.
- `Source/engine/`: Rendering, low-level engine systems, core runtime utilities.
- `Source/levels/`, `Source/monsters/`, `Source/items/`, `Source/quests/`: Gameplay domains.
- `Source/control/` + `Source/controls/`: Input and control handling.
- `Source/data/`, `Source/tables/`: Data structures and static tables.
- `Source/lua/`: Lua scripting integration.
- `Source/dvlnet/`: Multiplayer/networking abstractions.
- `Source/platform/`: Platform-specific integration.
- `Source/utils/`: Shared utility helpers.

### `test/` (quality gates)
Contains C++ tests and benchmarks covering gameplay systems, parsers, rendering helpers, math/utilities, and data handling.
- Uses CMake/CTest integration.
- Includes fixture data in `test/fixtures/`.

### `assets/` (game data shipped with engine)
Contains CLX art, fonts, UI art, level data, Lua scripts, and text datasets.
- Subfolders include `fonts/`, `ui_art/`, `levels/`, `lua/`, `txtdata/`, etc.

### `docs/` (developer and user docs)
Documentation for building, installation, changelog, and project references.
- `docs/building.md` is the key build reference.

### `CMake/` and `CMakeLists.txt` (build orchestration)
- Root `CMakeLists.txt`: Targets and global options.
- `CMake/finders/`: Custom dependency find modules.
- `CMake/functions/`: Reusable CMake helper functions.
- `CMake/platforms/`: Cross-compilation/toolchain definitions.

### `Packaging/` (distribution)
Platform-specific packaging and release scripts for multiple ecosystems (desktop, consoles, handhelds, web targets).

### `3rdParty/` (vendored dependencies)
Bundled libraries (SDL variants, gtest, benchmark, png/zlib/bzip2, fmt, asio, etc.) used for reproducible builds and cross-platform support.

### `Translations/` (localization)
- Locale translation files (`*.po`)
- Template catalog (`devilutionx.pot`)
- Translation glossary/reference.

### Platform projects
- `android-project/`: Native Android project (Gradle-based).
- `uwp-project/`: Windows UWP integration.

### Supporting areas
- `.github/`: CI pipelines and templates.
- `mods/`: Mod content/overrides.
- `tools/`: Debugger configs and helper tooling.

## Typical contribution paths
- **Gameplay/engine change**: edit `Source/...` + add/update `test/...`.
- **Build system change**: edit `CMakeLists.txt` and/or `CMake/...`.
- **Packaging change**: edit `Packaging/<platform>/...`.
- **Localization change**: edit `Translations/...`.
- **Docs/process change**: edit `docs/...`, `README.md`, or root operational docs like `AGENTS.md`.

## Quick orientation commands
- List top-level directories:
  - `find . -maxdepth 2 -type d | sort`
- Configure build:
  - `cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release`
- Build:
  - `cmake --build build -j $(getconf _NPROCESSORS_ONLN)`
- Run tests (if built):
  - `ctest --test-dir build --output-on-failure`
