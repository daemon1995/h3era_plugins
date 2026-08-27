# Heroes of Might and Magic III ERA Plugins

**English** | [Русский](README_ru.md)

A monorepository of C++ plugins, shared headers, and build tooling for Heroes of Might and Magic III ERA.

The projects patch the 32-bit game executable and are therefore built for **x86/Win32 only**. The repository contains general ERA plugins, object extenders, commissioned plugins, test projects, and a map-to-dbgmap utility.

## Requirements

- Visual Studio 2022 with the **Desktop development with C++** workload
- A Windows 10 SDK
- A local Heroes III ERA installation for deployment and debugging

The repository defaults to the Visual Studio 2022 `v143` platform toolset and Windows SDK `10.0`. The solution configurations are `Debug|x86` and `Release|x86`; the C++ projects use the corresponding `Win32` platform.

### Windows XP compatibility

The plugins are intended to remain buildable for and runnable on **Windows XP**. To produce Windows XP-compatible binaries, open Visual Studio Installer, modify your Visual Studio installation, and install the **C++ Windows XP support for VS 2017 (v141) tools** component. This component provides the `v141_xp` platform toolset; it is not installed by default.

Then select it in your local `Directory.Build.props.user` file:

```xml
<Project>
  <PropertyGroup>
    <PlatformToolset>v141_xp</PlatformToolset>
  </PropertyGroup>
</Project>
```

Without that optional Visual Studio component, builds configured with `v141_xp` will fail because the toolset is unavailable.

## Repository layout

```text
├── headers/                         Shared ERA, WoG, and game API headers
├── Commissions/                     Commissioned plugins and their solution
├── Objects_*/                       Object extender plugins
├── tools/MapToDbgmap/               Release map-file conversion utility
├── Directory.Build.props            Repository-wide early MSBuild defaults
├── Directory.Build.after.props      Shared compiler and linker settings
├── Directory.Build.targets          Release .map/.dbgmap generation
├── H3EraPlugins.sln                 Main plugin solution
└── H3ObjectExtenders.sln            Object extender solution
```

Plugin projects are organized by feature-oriented directory names such as `ERA_*`, `Gameplay_*`, `Interface_*`, `Objects_*`, and `RMG_*`. `Commissions/Commissions.sln` contains the commissioned plugins. The `Test_*` projects are development/test projects rather than released features.

## Local setup

1. Clone the repository:

   ```powershell
   git clone https://github.com/daemon1995/h3era_plugins.git
   cd h3era_plugins
   ```

2. Create your untracked local settings file:

   ```powershell
   Copy-Item Directory.Build.props.template Directory.Build.props.user
   ```

3. Set the paths for your ERA installations:

   ```xml
   <Project>
     <PropertyGroup>
       <LocalGamePath>C:\Games\Heroes3ERA</LocalGamePath>
       <LocalGameTestPath>C:\Games\Heroes3ERA_Test</LocalGameTestPath>
     </PropertyGroup>
   </Project>
   ```

4. Open `H3EraPlugins.sln` in Visual Studio 2022 and build `Debug|x86` or `Release|x86`.

`Directory.Build.props.user` is ignored by Git. The repository supplies fallback game paths, but they are developer-specific defaults and should normally be overridden locally. See [VS_USER_SETTINGS.md](VS_USER_SETTINGS.md) for more detail.

## Build output

By default, plugin projects are dynamic libraries with the `.era` extension. They are written directly to:

```text
$(LocalGamePath)\Mods\$(PluginModName)\eraplugins\
```

`PluginModName` defaults to `WoG`; projects under `Commissions/` override it with `Commissions`. Individual projects may override the output extension or other shared settings.

Release builds also generate linker `.map` files and convert them to `.dbgmap` files under the sibling `DebugMaps` directory. Debug builds do not perform this conversion.

## Development

Shared MSBuild settings are inherited automatically by projects below the repository root. New plugin projects should use the shared headers in `headers/` and be added to the appropriate solution. See:

- [CONTRIBUTING.md](CONTRIBUTING.md)
- [Plugin development guide (Russian)](PLUGIN_DEVELOPMENT_GUIDE.ru.md)
- [Solution best practices](SOLUTION_BEST_PRACTICES.md)
- [RMG_CustomizeObjectProperties documentation](RMG_CustomizeObjectProperties/README.md)
- [RMG object extender API quick start](RMG_CustomizeObjectProperties/QUICK_START.md)

The `.vscode` directory contains helper tasks, but some older per-project tasks and local paths are machine-specific. Visual Studio and the solution files are the documented primary build path.

## Bundled API documentation

The README files below belong to the bundled H3API sources and document those upstream headers rather than this repository's build system:

- [H3API README](headers/H3API/Readme.md)
- [H3API single-header README](headers/H3API/single_header/Readme.md)
- [H3API structure overview](headers/H3API_for_copilot/Readme.md)

## Disclaimer

This is an unofficial fan-development repository. Heroes of Might and Magic and related marks belong to their respective owners.
