# RMG_CustomizeObjectProperties

An ERA plugin that extends random-map object configuration and behavior in Heroes of Might and Magic III.

Plugins can integrate with its object lifecycle through the public `ObjectExtender` API. See the [API quick start](QUICK_START.md).

RMG remains the owner of the extender registry and dispatch. New C++ clients discover its versioned table in
Patcher variable `ERA.Services.ObjectExtenders.v2`; legacy exported entry points remain available.
Object-definition parsing/priority/merging is shared with the editor, while each host owns native loading and memory.
RMG contributes its own definitions/settings to the generic configuration-export API; domain extenders contribute
their data independently. See [configuration services](../ERA_RuntimeServices/Docs/CONFIG_SERVICES.md).

## Object subtype count API

`GetObjectSubtypeCount(int type)` is an exported `int __stdcall` function. It returns the final object-definition
range for types 0–231: the largest loaded subtype plus one, including gaps. Multiple DEFs for the same subtype
do not increase the size. The result is 0 before definitions load, for an absent type or an invalid type.
Supported subtypes are 0–32767; counts are 0–32768. The definition loaders reject unsupported rows and ignore
resource lines beyond the declared record count. This preserves the existing signed 16-bit extender ABI.
Values are refreshed after each merge of the native definitions with RMG additions; language reloads do not change them.
The result describes the definition range, not the capacity of another plugin's creature/name tables or RMG's virtual variants.

New C++ clients use [ObjectDefinitionsAPI.h](../headers/EraPluginsAPI/ObjectDefinitionsAPI.h):

```cpp
const auto *api = era_services::Find<era_objects::DefinitionsApi>(era_objects::DEFINITIONS);
const int count = api ? api->subtypeCount(17) : 0;
// The definition range is [0, count); its maximum subtype is count - 1.
```

RMG and the map editor publish the same `DefinitionsApi` through `ERA.Services.ObjectDefinitions.v1`.
DLL clients can resolve `GetObjectSubtypeCount` using the function type
`EraJS::GetObjectSubtypeCount` in `headers/Era/JsonObjectExport.hpp`.

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

Two-way monolith subtypes 8 and above use separate teleport networks, including native pathfinding and ERA savegame support. See [extended monoliths](ExtendedMonoliths.md) for configuration and in-game checks.

Creature dwellings of types 17 and 20 use the standalone [ERA_DwellingsExtender](../ERA_DwellingsExtender/README.md)
for extended gameplay. RMG keeps independent read-only dwelling hints and generator integration, including when
a different plugin owns the runtime tables. See [RMG dwelling information](DwellingsExtender.md).

## Object loop sounds

`RMG.objectGeneration.<type>.sound.loop` sets a loop sound for all subtypes of an object type. A more specific `RMG.objectGeneration.<type>.<subtype>.sound.loop` entry takes priority. An empty string explicitly disables the loop sound; without either entry, the game uses its native sound selection. WAV names refer to resources available to the game.

To check in-game, configure two different existing WAVs for a type and one of its subtypes, and an empty string for another subtype. Restart the game and move a hero near each object: the general sound, specific sound, and silence should apply respectively. Move between areas containing custom sounds and then away from them to check transitions.

## Generation limits and saved settings

Type-level JSON map/zone limits are applied to the native RMG tables for the duration of generation. Per-subtype limits still apply, and previous native table values are restored afterward. Object and dialog settings share a transactional INI save, so an unsuccessful write keeps the previous file and runtime object values. See [generation limits and saved settings](GenerationLimitsAndSettings.md) for configuration, an example JSON, and verification steps.

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
