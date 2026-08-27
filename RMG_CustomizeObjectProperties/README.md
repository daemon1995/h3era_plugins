# RMG_CustomizeObjectProperties

An ERA plugin that extends random-map object configuration and behavior in Heroes of Might and Magic III.

Plugins can integrate with its object lifecycle through the public `ObjectExtender` API. See the [API quick start](QUICK_START.md).

## Development setup

From the repository root, create the local MSBuild settings file and edit the game paths:

```powershell
Copy-Item Directory.Build.props.template Directory.Build.props.user
notepad Directory.Build.props.user
```

```xml
<Project>
  <PropertyGroup>
    <LocalGamePath>C:\Games\Heroes3ERA</LocalGamePath>
    <LocalGameTestPath>C:\Games\Heroes3ERA_Test</LocalGameTestPath>
  </PropertyGroup>
</Project>
```

Open `H3EraPlugins.sln` in Visual Studio 2022, select an x86 configuration, set `RMG_CustomizeObjectProperties` as the startup project, and press F5. The shared settings start `$(LocalGamePath)\h3era hd.exe` with the game directory as its working directory.

For Windows XP-compatible builds, install the optional **C++ Windows XP support for VS 2017 (v141) tools** component in Visual Studio Installer and set `<PlatformToolset>v141_xp</PlatformToolset>` in `Directory.Build.props.user`.

## Implemented object extensions

The project currently compiles extenders for spell markets, universities, creature banks, chests/gazebos, shrines, and other random-map object handling. The Visual Studio project file is the authoritative list of compiled source files.

## Build output

Shared repository settings build the plugin with the `.era` extension and write it to:

```text
$(LocalGamePath)\Mods\WoG\eraplugins\
```

Release builds generate `.map` and `.dbgmap` files in:

```text
$(LocalGamePath)\Mods\WoG\DebugMaps\
```

The `.dbgmap` conversion is a Release-only build step. See the root [README](../README.md) and [Visual Studio settings guide](../VS_USER_SETTINGS.md) for repository-wide details.
