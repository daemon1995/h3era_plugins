# Configuration and object services

All calls use the main thread. Public tables have a version and structSize; they are published in process-local Patcher variables through [RuntimeServices.h](../headers/EraPluginsAPI/RuntimeServices.h). Service pointers live for the process; definition strings live until the next definition load. No STL allocations cross the DLL boundary.

| Service | Owner and contract |
| --- | --- |
| `ERA.Services.ConfigExport` | API_ErmFunctions; [ConfigExportAPI.h](../headers/EraPluginsAPI/ConfigExportAPI.h) |
| `ERA.Services.Graphics.v1` | API_ErmFunctions; [GraphicsAPI.h](../headers/EraPluginsAPI/GraphicsAPI.h) |
| `ERA.Services.ObjectExtenders.v2` | RMG registry and dispatch; [ObjectExtenderAPI.hpp](../headers/EraPluginsAPI/ObjectExtenderAPI.hpp) |
| `ERA.Services.ObjectDefinitions.v1` | RMG in the game, ObjectDefinitions in the editor; [ObjectDefinitionsAPI.h](../headers/EraPluginsAPI/ObjectDefinitionsAPI.h) |
| `ERA.Services.DwellingData.v1` | ERA_DwellingsExtender; [ObjectDataAPI.h](../headers/EraPluginsAPI/ObjectDataAPI.h) |

## Definitions and counts

Game and editor share [ObjectDefinitions.h](../headers/Objects/ObjectDefinitions.h) for validation, priorities and merging. Native loading and allocation remain in their host process. Only the declared prefix of the native text resource is active. Unsupported records are removed before the native loader.

Legacy flat/nested mod lists load first, then explicit per-subtype properties. A later matching DEF/type/subtype replaces an earlier entry; a different DEF remains a separate variant. Direct per-subtype discovery covers 0..1023; mod lists support the full signed 16-bit subtype range 0..32767.

DefinitionsApi::subtypeCount(type), also exported by RMG as GetObjectSubtypeCount(type), returns the largest loaded subtype plus one, including gaps. Repeated DEF variants do not increase the range. Before loading, for an absent type or an invalid type, it returns zero. The maximum count is 32768. It describes definitions, not a separate domain table's capacity or virtual RMG variants.

Dwelling tables use max(nativeCount, loadedSubtypeCount); there is no fixed minimum of 1024. The editor refreshes names after both native TXT and definition loading, in either order. Prototype changes require restarting; language reload refreshes names.

## Export providers

[ExportProvider.h](../headers/Config/ExportProvider.h) registers a typed collector. The service owns immutable snapshots, identity merging, selection, counts and JSON serialization. Provider registration is retried on OnConfigExportApiReady and OnAfterWog. Collection becomes available after OnAfterCreateWindow.

RMG registers `objects.rmg`: final definitions, per-object text, generation settings, inheritance, limits, sounds and university settings. The standalone dwelling plugin registers `dwellings`: names, creatures and portrait configuration. Other domain providers can register independently; clients discover categories through the API.

A provider contributes text, data and full RMG defaults. Export preserves explicit overrides and automatic dwelling valuation. Snapshots merge records with the same category/identity, reject conflicting fields, and are completed before serialization. Sink strings are borrowed for one callback and copied synchronously; callers release snapshot handles explicitly. LocalText uses ERA's current code page and output JSON is UTF-8.

Existing DLL exports remain available. New clients discover the shared service tables without loading domain DLLs as a side effect.
