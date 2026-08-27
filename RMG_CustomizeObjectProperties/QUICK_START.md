# Object Extender API quick start

`RMG_CustomizeObjectProperties.era` exposes a C++ API that lets another ERA plugin attach custom behavior to one or more adventure-map object types/subtypes.

The public header is [`headers/EraPluginsAPI/ObjectExtenderAPI.hpp`](../headers/EraPluginsAPI/ObjectExtenderAPI.hpp). Define `OBJECTS_EXTENDER_API` before including the shared repository header:

```cpp
#define OBJECTS_EXTENDER_API
#include "../headers/header.h"
```

## Minimal extender

Derive from `extender::ObjectExtender`, declare the handled objects in the constructor, and override only the callbacks you need:

```cpp
class MyObjectExtender final : public extender::ObjectExtender
{
  public:
    explicit MyObjectExtender(PatcherInstance *pi) : ObjectExtender(pi)
    {
        // Handle exactly type 144, subtype 3, with AI scouting value 50.
        AddUniqueObjectInfo(144, 3, 50);
    }

    BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero,
                               int playerId, BOOL isRightClick) const noexcept override
    {
        libc::sprintf(h3_TextBuffer, "My custom object");
        return TRUE; // The API will use h3_TextBuffer as the object hint.
    }

    BOOL VisitMapItem(H3Hero *hero, H3MapItem *mapItem, H3Position position,
                      BOOL isHuman) const noexcept override
    {
        // Apply the custom visit behavior here.
        return TRUE;
    }
};
```

Use `subtype == -1` to register an extender as the fallback for every subtype of an object type that does not have a more specific extender:

```cpp
AddUniqueObjectInfo(objectType, -1);
```

## Registration

Create an instance whose lifetime matches the plugin lifetime and register it during your plugin initialization:

```cpp
globalPatcher = GetPatcher();
_PI = globalPatcher->CreateInstance("EraPlugin.MyPlugin.Author");
Era::ConnectEra(hModule, "EraPlugin.MyPlugin.Author");

static MyObjectExtender extender(_PI);
if (!extender.Register())
{
    // Registration failed: null and duplicate pointers are rejected, and
    // registration closes once RMG_CustomizeObjectProperties loads objects.txt.
}
```

`Register()` loads `EraPlugins/RMG_CustomizeObjectProperties.era` if necessary and calls its exported `RegisterObjectExtender(ObjectExtender*)` function. Registration must finish before the host plugin completes its `objects.txt` loading stage. Keep the extender instance alive after registration; the host stores and later calls the supplied pointer.

## Callbacks

Override any of these `ObjectExtender` methods:

| Method | Called for |
| --- | --- |
| `AfterLoadingObjectsTxtProc(maxSubtypes)` | Post-processing after object definitions are loaded; `maxSubtypes[type]` contains the highest known subtype. |
| `CreateRMGObjectGen(info, isPseudoGeneration)` | Creating an RMG generator for a custom object enabled through JSON configuration. |
| `InitNewGameMapItemSetup(mapItem, typeCounter, subtypeCounter)` | Initializing each matching object when a new map starts. |
| `InitNewWeekMapItemSetup(mapItem)` | Updating each matching object at the start of a week. |
| `VisitMapItem(hero, mapItem, position, isHuman)` | Handling a hero interaction with the object. The current dispatcher still continues the game's default visit path. |
| `SetHintInH3TextBuffer(mapItem, hero, playerId, isRightClick)` | Replacing hover/right-click text. Return `TRUE` after writing the result to `h3_TextBuffer`. |
| `SetAiMapItemWeight(mapItem, hero, player, aiWeight, moveDistance, position)` | Supplying an AI visit weight. Return `TRUE` when `aiWeight` was handled. |
| `RMGDlg_ShowCustomObjectHint(attributes, text)` | Supplying custom text for the RMG settings dialog. Return `TRUE` when `text` was handled. |

The default implementation of each callback does nothing. `CreateRMGObjectGen` is the exception: by default it calls `CreateDefaultH3RmgObjectGenerator(info)`.

## Supporting types and helpers

- `UniqueObjectType` identifies an object by `type` and `subtype`.
- `UniqueObjectInfo` adds an optional `aiScoutingWeight` to that identity.
- `RMGObjectProperties` contains `enabled`, `mapLimit`, `zoneLimit`, `value`, and `density` values received from RMG configuration.
- `GetFromMapItem<T>(mapItem)` interprets the object's setup storage as a custom structure.
- `GetObjectName(type, subtype)` and `GetObjectName(mapItem)` call the host plugin's exported name resolver. They return `nullptr` if the host module or export cannot be loaded.
- `CreateDefaultH3RmgObjectGenerator(info)` creates the standard game generator for the supplied type, subtype, value, and density.

Constants such as `WOG_OBJECT_TYPE`, `ERA_OBJECT_TYPE`, `WAREHOUSE_OBJECT_TYPE`, and the `HOTA_*_OBJECT_TYPE` family provide the reserved object-type mappings used by the repository plugins.

## Reference implementations

Working consumers of the API are available in:

- [`Objects_CommonHotaObjectsPack`](../Objects_CommonHotaObjectsPack/)
- [`Objects_WoGObjectsExtender`](../Objects_WoGObjectsExtender/)
- [`Objects_CreatureBanksExtender`](../Objects_CreatureBanksExtender/)

The API is a C++ ABI: the host and consumer plugins must use compatible class layouts, calling conventions, and shared headers. Build all participating plugins for x86/Win32.
