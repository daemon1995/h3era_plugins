#include "GraphicsEnhancements.h"

namespace graphics
{
GraphicsEnhancements *GraphicsEnhancements::instance = nullptr;
_LHF_(Game_AtTownSettingMapItemDef)
{
    if (const auto town = *reinterpret_cast<H3Town **>(c->ebp - 0x8))
    {
        // if castle is not built
        if (!town->IsBuildingBuilt(eBuildings::CASTLE))
        {
            LPCSTR defName = nullptr;

            bool readSucces = false;
            // citadel is built
            if (town->IsBuildingBuilt(eBuildings::CITADEL))
            {
                defName = EraJS::read(H3String::Format("gem_plugin.map_item_view.98.%d.citadel", town->type).String(),
                                      readSucces);
            }
            else // only fort is built
            {
                defName = EraJS::read(H3String::Format("gem_plugin.map_item_view.98.%d.fort", town->type).String(),
                                      readSucces);
            }
            if (readSucces)
            {
                c->edi = reinterpret_cast<int>(defName);
                c->return_address = 0x04C9827;

                return NO_EXEC_DEFAULT;
            }
        }
    }

    return EXEC_DEFAULT;
}

// DllExport H3LoadedDef *SetMapHeroDefName(const UINT heroId, const char *const heroDefName)
//{
//
//     return 0;
// }
LPCSTR Hero_GetMapItemDefName(const H3Hero *hero)
{
    bool readSuccess = false;

    // first check unique hero def name
    LPCSTR defName = EraJS::read(H3String::Format("gem_plugin.map_item_view.54.id.%d", hero->id).String(), readSuccess);
    // if name is read and not empty
    if (readSuccess && libc::strcmpi(defName, h3_NullString))
    {
        return defName;
    }

    return nullptr;
}
LPCSTR HeroClass_GetMapItemDefName(const UINT classId, const bool isFemale)
{
    bool readSuccess = false;

    LPCSTR defName = EraJS::read(
        H3String::Format("gem_plugin.map_item_view.54.class.%d.%d", classId, isFemale).String(), readSuccess);
    // if name is read and not empty
    if (readSuccess && libc::strcmpi(defName, h3_NullString))
    {
        return defName;
    }

    return nullptr;
}
H3LoadedDef *GraphicsEnhancements::Hero_GetMapItemDef(const H3Hero *hero) noexcept
{

    H3LoadedDef *result = nullptr;
    if (hero)
    {
        // first set hero def by id
        if (result = instance->uniqueHeroDefs[hero->id])
        {
            return result;
        }
        // if empty set hero def by class
        if (result = instance->heroClassDefs[hero->isFemale][hero->hero_class])
        {
            return result;
        }
    }
    return result;
}
H3LoadedDef *GraphicsEnhancements::InitHeroData(const H3Hero *hero) noexcept
{
    H3LoadedDef *result = nullptr;

    if (LPCSTR defNamePtr = Hero_GetMapItemDefName(hero))
    {
        if (auto *defBefore = uniqueHeroDefs[hero->id])
        {
            defBefore->Dereference();
        }

        result = uniqueHeroDefs[hero->id] = H3LoadedDef::Load(defNamePtr);
    }

    return result;
}
void GraphicsEnhancements::InitHeroClassData(const UINT classId) noexcept
{
    for (size_t i = 0; i < 2; i++)
    {
        if (heroClassDefs[i][classId] == nullptr)
        {
            if (LPCSTR defNamePtr = HeroClass_GetMapItemDefName(classId, i))
            {
                heroClassDefs[i][classId] = H3LoadedDef::Load(defNamePtr);
            }
        }
    }
}
void GraphicsEnhancements::CleanUpData() noexcept
{
    // clean unique hero defs
    for (auto &i : uniqueHeroDefs)
    {
        if (i)
        {
            i->Dereference();
            i = nullptr;
        }
    }

    // clean hero class arrays (male and female)
    for (auto &vec : heroClassDefs)
    {
        for (auto &i : vec)
        {
            if (i)
            {
                i->Dereference();
                i = nullptr;
            }
        }
    }
}

_LHF_(AdventureManager_DrawHeroDef)
{
    if (const auto *hero = reinterpret_cast<H3Hero *>(c->edi))
    {
        if (auto def = GraphicsEnhancements::Hero_GetMapItemDef(hero))
        {
            c->ecx = reinterpret_cast<int>(def);
            return NO_EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(AdventureManager_DrawHeroDefTransparent)
{
    if (const auto *hero = reinterpret_cast<H3Hero *>(c->esi))
    {
        if (auto def = GraphicsEnhancements::Hero_GetMapItemDef(hero))
        {
            c->ecx = reinterpret_cast<int>(def);
            // return NO_EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;
}
_LHF_(AdventureManager_DrawHeroDefShadow)
{
    if (const auto *hero = reinterpret_cast<H3Hero *>(c->ebx))
    {
        if (auto def = GraphicsEnhancements::Hero_GetMapItemDef(hero))
        {
            c->ecx = reinterpret_cast<int>(def);
        }
    }

    return EXEC_DEFAULT;
}
_LHF_(AdventureManager_Show)
{
    if (auto instance = GraphicsEnhancements::Get())
    {
        for (auto &i : P_Main->heroes)
        {
            instance->InitHeroData(&i);
        }
        for (size_t i = 0; i < GraphicsEnhancements::MAX_UNIQUE_CLASSES; i++)
        {
            instance->InitHeroClassData(i);
        }
    }
    return EXEC_DEFAULT;
}
_LHF_(AdventureManager_Hide)
{
    // dereference hero unuique and classes defs
    GraphicsEnhancements::Get()->CleanUpData();
    return EXEC_DEFAULT;
}
void GraphicsEnhancements::CreatePatches() noexcept
{
    // set different town view for fort/citadel/castle
    _pi->WriteLoHook(0x4C980D, Game_AtTownSettingMapItemDef);

    // set different heroes on map views
    // hok is at "mov ecx" so chnage ecx and return NO_EXEC_DEFAULT;
    _pi->WriteLoHook(0x47F443, AdventureManager_DrawHeroDef); // @Hawaiing
    _pi->WriteLoHook(0x47F876, AdventureManager_DrawHeroDef); // @Hawaiing

    // hook is at "call" so only change ecx;
    _pi->WriteLoHook(0x04106A6, AdventureManager_DrawHeroDefTransparent); // @Hawaiing
    _pi->WriteLoHook(0x0410206, AdventureManager_DrawHeroDefTransparent); // @Hawaiing
    _pi->WriteLoHook(0x047F592, AdventureManager_DrawHeroDefShadow);      // @Hawaiing

    // is needed to load/unload defs
    _pi->WriteLoHook(0x04077B6, AdventureManager_Hide);
    _pi->WriteLoHook(0x0407302, AdventureManager_Show);
}

GraphicsEnhancements::GraphicsEnhancements()
    : IGamePatch(globalPatcher->CreateInstance("EraPlugin.GraphicsEnhancements.daemon_n"))
{
    CreatePatches();
}
GraphicsEnhancements *GraphicsEnhancements::Get() noexcept
{
    if (!instance)
    {
        instance = new GraphicsEnhancements();
    }
    return instance;
}

} // namespace graphics
