# Copilot Instructions for h3era_plugins (HoMM3 ERA Plugins Monorepo)

## Overview
This repository is a monorepo for plugins (DLLs) for Heroes of Might and Magic 3 ERA. All plugins share a common API and build environment, and are intended to be built and debugged together.

## Architecture & Structure
- **Monorepo**: All plugins live in subfolders, sharing headers and build settings.
- **Common API**: The `headers/` directory contains shared game API headers used by all plugins.
- **Build System**: Unified via `Directory.Build.props` and `.user` files for local paths.
- **Solution**: All plugins are included in `H3EraPlugins.sln` for Visual Studio.
- **Plugin Example**: Each plugin (e.g., `Assembly_MainPlugin`, `Interface_MainMenuAPI`) is a DLL with its own minimal entrypoint (`dllmain.cpp`) and may have additional logic files.

## Key Files & Folders
- `headers/` — Shared API headers for all plugins
- `Directory.Build.props` — Common MSBuild settings
- `Directory.Build.props.user` — Local developer paths (not in git)
- `H3EraPlugins.sln` — Solution file for all plugins
- `[PluginName]/` — Each plugin's source folder
- `framework.h` — Always included, minimal, only includes `../headers/header.h`
- `dllmain.cpp` — DLL entry, always declares `Patcher *globalPatcher` and `PatcherInstance *_PI`

## Build & Debug Workflow
- **Initial setup**: Copy `Directory.Build.props.template` to `Directory.Build.props.user` and set your local paths.
- **Build**: Open `H3EraPlugins.sln` in Visual Studio 2022, build desired plugin project.
- **Debug**: Launch the game via VS debugger; plugins are loaded automatically. Debug settings are preconfigured in `.vcxproj.user` files.

## Project Conventions
- **No plugin-specific era.cpp**: All plugins reference `../headers/Era/era.cpp` in their `.vcxproj`, not in their own source.
- **Minimal framework.h**: Only `#include "../headers/header.h"` and `WIN32_LEAN_AND_MEAN` define.
- **Patcher variables**: Always declare `Patcher *globalPatcher = nullptr;` and `PatcherInstance *_PI = nullptr;` in `dllmain.cpp`.
- **Plugin identity**: Use a `dllText` namespace for plugin metadata (name, version, author, date).
- **No tests**: This codebase does not use automated tests; manual in-game testing is standard.

## Patterns & Examples
- See `Assembly_MainPlugin/dllmain.cpp` for a full-featured plugin entrypoint.
- See `Interface_MainMenuAPI` for a minimal plugin template.
- All plugin build settings (warnings, optimization, etc.) are copied from `Assembly_MainPlugin`.

## External Integration
- **Game**: Plugins are loaded by the ERA game engine at runtime.
- **No external package manager**: All dependencies are local or in `headers/`.

---
If any conventions or workflows are unclear, please ask for clarification or check the main `README.md`.
