#pragma once

#include <stdint.h>

// PE32/stdcall exports from ERA_OptionsMenu.era. Resolve with GetProcAddress
// after ERA finishes loading plugins (not from another plugin's DllMain).
// GetId/RegisterJson return a non-negative ID; -1 invalid/missing, -2
// conflict, -3 exhausted. Values are 0/1 or a zero-based choice index.
// No string or container ownership crosses the plugin boundary.
// Values address preferences in menus and the current map while playing.
// SetValue on a map requires change_during_game; Save there leaves profiles alone.
typedef int32_t (__stdcall *EraOptionsGetIdProc)(const char *key);
typedef int32_t (__stdcall *EraOptionsRegisterJsonProc)(const char *jsonPath, const char *modFolder);
typedef int32_t (__stdcall *EraOptionsGetValueProc)(int32_t id, int32_t *outValue);
typedef int32_t (__stdcall *EraOptionsSetValueProc)(int32_t id, int32_t value);
typedef int32_t (__stdcall *EraOptionsSaveProc)();
// Export all options/current values. UTF-8 path; null uses the game directory.
typedef int32_t (__stdcall *EraOptionsExportJsonProc)(const char *pathUtf8);
// showBans: 0 options, 1 object bans. allowEditing: 0 viewing, 1 editing.
// On a started map only change_during_game options are editable; bans are read-only.
typedef int32_t (__stdcall *EraOptionsShowDialogProc)(int32_t showBans, int32_t allowEditing);

// ERM bridge for the active map snapshot, independent of future-map preferences.
// kind: 0 spell, 1 artifact, 2 hero, 3 secondary skill. source: 0 guilds, 1 shrines, 2 scholars,
// 3 scrolls/Pandora, 4 starting heroes. ReplacementSpell accepts level 1..5,
// returns an allowed ordinary spell of that exact level, or -1 if none.
typedef int32_t (__stdcall *EraOptionsMapObjectBannedProc)(int32_t kind, int32_t objectId);
typedef int32_t (__stdcall *EraOptionsMapBanSourceProc)(int32_t source);
typedef int32_t (__stdcall *EraOptionsReplacementSpellProc)(int32_t level);
