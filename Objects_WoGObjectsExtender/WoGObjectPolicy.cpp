#include "WoGObjectPolicy.h"
#include "WoGObjectIds.h"
#include "WoGObjectsExtender.h"
#include "WoGObjectState.h"

namespace wog { namespace policy {
namespace {
constexpr const char *VERSION = "wog_objects.native.plugin_handles.version";
bool loadingGame = false;
bool gameActive = false;
void __stdcall NewMap(Era::TEvent *)
{
    InitializeDefaults();
    // Preserve the saved seed if ERM initialization is raised inside a load operation.
    if (!loadingGame)
    {
        state::ResetMapRandomSeed();
        gameActive = true;
    }
}
void __stdcall BeforeLoad(Era::TEvent *) { loadingGame = true; gameActive = false; }
void __stdcall AfterLoad(Era::TEvent *) { InitializeDefaults(); loadingGame = false; gameActive = true; }
void __stdcall Leave(Era::TEvent *) { loadingGame = false; gameActive = false; }
}
std::string Key(int type, int subtype)
{
    return "wog_objects.native.plugin_handles." + std::to_string(type) + "." + std::to_string(subtype);
}
bool Handles(int type, int subtype) noexcept
{
    if (!((type == WOG_OBJECT_TYPE || type == ERA_OBJECT_TYPE) &&
          (objects::IsPortedSubtype(subtype) || subtype == objects::MARKET_OF_TIME || subtype == objects::ARTIFICER)) &&
        !(type == 30 && subtype == 0)) // Observer for the built-in Fountain of Fortune's potion reward.
        return false;
    // Before the map's ERM memory exists (e.g. object setup/RMG), both types have native defaults.
    return Era::GetAssocVarIntValue(VERSION) == 0 || Era::GetAssocVarIntValue(Key(type, subtype).c_str()) == 1;
}
bool Handles(const H3MapItem *item) noexcept
{
    return item && Handles(item->objectType, item->objectSubtype);
}
void InitializeDefaults() noexcept
{
    if (Era::GetAssocVarIntValue(VERSION) != 0)
        return;
    // OnBeforeErmInstructions runs after ERA clears assoc memory, before map/global instructions.
    // The version marker is essential: ERA omits zero-valued assoc entries on load. A saved 0 must
    // stay 0 rather than being mistaken for a missing key and replaced with a native default.
    for (int type : {WOG_OBJECT_TYPE, ERA_OBJECT_TYPE})
    {
        for (const auto *entry = objects::DescriptorsBegin(); entry != objects::DescriptorsEnd(); ++entry)
            Era::SetAssocVarIntValue(Key(type, entry->subtype).c_str(), 1);
        Era::SetAssocVarIntValue(Key(type, objects::MARKET_OF_TIME).c_str(), 1);
        Era::SetAssocVarIntValue(Key(type, objects::ARTIFICER).c_str(), 1);
    }
    Era::SetAssocVarIntValue(Key(30, 0).c_str(), 1);
    Era::SetAssocVarIntValue(VERSION, 1);
}
void RegisterEvents() noexcept
{
    Era::RegisterHandler(NewMap, "OnBeforeErmInstructions");
    Era::RegisterHandler(BeforeLoad, "OnBeforeLoadGame");
    Era::RegisterHandler(AfterLoad, "OnAfterLoadGame");
    Era::RegisterHandler(Leave, "OnGameLeave");
}
bool LoadingGame() noexcept { return loadingGame; }
bool GameActive() noexcept { return gameActive; }
} }
