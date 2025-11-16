#include "GraphicsEnhancements.h"

namespace graphics
{
GraphicsEnhancements *GraphicsEnhancements::instance = nullptr;

// DllExport H3LoadedDef* SetMapHeroDefName(const UINT heroId, const char* const heroDefName)
//{
//
//     return 0;
// }

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

LPCSTR Hero_GetMapItemDefName(const UINT heroId)
{
    bool readSuccess = false;

    // first check unique hero def name
    LPCSTR defName = EraJS::read(H3String::Format("gem_plugin.map_item_view.54.id.%d", heroId).String(), readSuccess);
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
H3LoadedDef *GraphicsEnhancements::InitHeroData(const UINT heroId) noexcept
{
    H3LoadedDef *result = nullptr;

    if (LPCSTR defNamePtr = Hero_GetMapItemDefName(heroId))
    {
        if (auto *defBefore = uniqueHeroDefs[heroId])
        {
            defBefore->Dereference();
        }

        result = uniqueHeroDefs[heroId] = H3LoadedDef::Load(defNamePtr);
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

_LHF_(AdventureManager_DrawHeroDef)
{
    if (const auto *hero = reinterpret_cast<H3Hero *>(c->edi))
    {
        if (auto def = GraphicsEnhancements::Hero_GetMapItemDef(hero))
        {
            c->Ecx(def);
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
            c->Ecx(def);
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
            c->Ecx(def);
        }
    }

    return EXEC_DEFAULT;
}
_LHF_(AdventureManager_Show)
{
    if (auto instance = GraphicsEnhancements::Get())
    {
        for (size_t i = 0; i < GraphicsEnhancements::MAX_UNIQUE_HEROES; i++)
        {
            instance->InitHeroData(i);
        }
        for (size_t i = 0; i < GraphicsEnhancements::MAX_UNIQUE_CLASSES; i++)
        {
            instance->InitHeroClassData(i);
        }

        instance->InitAdventureMapTownBuiltDefs();
    }
    return EXEC_DEFAULT;
}

_LHF_(AdventureManager_Hide)
{
    //  dereference hero unuique and classes defs
    GraphicsEnhancements::Get()->CleanUpData();
    return EXEC_DEFAULT;
}
void GraphicsEnhancements::InitAdventureMapTownBuiltDefs() noexcept
{
    // get max towns displayable built icons from config
    constexpr int hdModMax = 7;
    maxTownsDisplayableBuiltIcons = Clamp(5, globalPatcher->VarGetValue<int>("HD.AdvMgr.TownList.L", 5), hdModMax);
    // townBuiltDlgDefButtons.assign(nullptr);
    const int firstDefButtonId = globalPatcher->VarGetValue<int>("HD.AdvMgr.ID32", 32);
    auto &dlg = P_AdventureManager->dlg;

    if (auto firstDef = dlg->GetH3DlgItem(firstDefButtonId))
    {
        const int xPos = firstDef->GetX() + 32;
        const int yBase = firstDef->GetY();
        auto &advMapDlg = builtDefButtons.advMapDlg;

        for (size_t i = 0; i < hdModMax; i++)
        {
            H3String buttonName = H3String::Format(GraphicsEnhancements::BUILD_BUTTON_NAME_FORMAT_MAP, i);
            const int buttonId = Era::GetButtonID(buttonName.String());
            if (buttonId != -1)
            {
                auto &defButton = advMapDlg[i];
                if (defButton = dlg->GetDefButton(buttonId))
                {
                    defButton->SetX(xPos);
                    defButton->SetY(yBase + (i << 5) + 15);
                    defButton->HideDeactivate();
                }
            }
            else
            {
                break;
            }
        }
    }
}
void AdjustTownBuiltButtonPosition(H3DlgDefButton *defButton, const int townIndex)
{
    if (townIndex == -1)
    {
        defButton->HideDeactivate();
        return;
    }

    const H3Town *currentTown = &P_Game->towns[townIndex];
    if (currentTown->builtThisTurn)
    {
        defButton->HideDeactivate();
        return;
    }

    int finalFrame = 0;
    for (size_t i = 0; i < limits::BUILDINGS; i++)
    {
        if (i == eBuildings::GRAIL)
            continue;

        // check if building is buildable in town
        if (THISCALL_2(bool, 0x05C1120, currentTown, i))
        {
            // skip if building is already built
            if (currentTown->IsBuildingBuilt(i))
                continue;

            // if player may afford that building
            if (FASTCALL_2(bool, 0x0460D10, currentTown, i))
            {
                defButton->HideDeactivate();
                return;
            }
            finalFrame = 2; // show "no money"
        }
    }
    defButton->SetFrame(finalFrame); // show built
    defButton->ShowActivate();
}

void GraphicsEnhancements::DrawAdventureMapTownBuiltStatus(H3AdventureMgrDlg *dlg, const BOOL draw,
                                                           const BOOL updateScreen) noexcept
{
    if (const int forceHide = IntAt(0x699588))
        return;

    const auto mePlayer = P_Game->GetPlayer();

    auto &advMapDlg = builtDefButtons.advMapDlg;

    for (size_t i = 0; i < maxTownsDisplayableBuiltIcons; i++)
    {
        auto townIndex = mePlayer->towns[i + dlg->topTownSlotIndex];
        if (townIndex != -1)
        {
            auto &defButton = advMapDlg[i];

            AdjustTownBuiltButtonPosition(defButton, townIndex);
            if (!draw || !defButton->IsVisible())
                continue;

            defButton->Draw();
            const int buttonId = defButton->GetID();

            if (updateScreen)
                defButton->Refresh();
        }
    }
}

void __stdcall AdvMgr__AtSetActiveHero_BeforeScreenRedraw(HiHook *h, H3WindowManager *mgr, const int x, const int y,
                                                          const int width, const int height)
{
    GraphicsEnhancements::Get()->DrawAdventureMapTownBuiltStatus(P_AdventureManager->dlg, true, false);
    THISCALL_5(void, h->GetDefaultFunc(), mgr, x, y, width, height);
}
void __stdcall H3AdventureMgrDlg__RedrawTownSlots(HiHook *h, H3AdventureMgrDlg *dlg, signed int playrId, char updateDlg,
                                                  char redrawScreen)
{
    THISCALL_4(void, h->GetDefaultFunc(), dlg, playrId, updateDlg, redrawScreen);
    GraphicsEnhancements::Get()->DrawAdventureMapTownBuiltStatus(dlg, updateDlg, redrawScreen);
}

void __stdcall AdvMgr_AtFullUpdate(HiHook *h, H3AdventureMgrDlg *dlg, char redraw)
{
    THISCALL_2(void, h->GetDefaultFunc(), dlg, redraw);
    GraphicsEnhancements::Get()->DrawAdventureMapTownBuiltStatus(dlg, true, false);
}

void GraphicsEnhancements::InitTownDlgDefButtons(H3TownDialog *dlg) noexcept
{

    auto firstDefButton = dlg->GetH3DlgItem(155);
    if (firstDefButton)
    {
        const int xPos = firstDefButton->GetX() + 32;
        const int yBase = firstDefButton->GetY();
        for (size_t i = 0; i < 3; i++)
        {
            H3String buttonName = H3String::Format(BUILD_BUTTON_NAME_FORMAT_TOWN, i);
            const int buttonId = Era::GetButtonID(buttonName.String());
            if (buttonId != -1)
            {
                auto defButton = dlg->GetDefButton(buttonId);
                defButton->SetX(xPos);
                defButton->SetY(yBase + (i << 5) + 15);
                defButton->HideDeactivate();
                builtDefButtons.townDlg[i] = defButton;
            }
            else
            {
                builtDefButtons.townDlg[i] = nullptr;
            }
        }
    }
}

void GraphicsEnhancements::DrawTownDlgBuiltStatus(H3TownDialog *dlg, const INT townIndex) noexcept
{

    const auto mePlayer = P_Game->GetPlayer();
    auto defButton = builtDefButtons.townDlg[townIndex];

    if (townIndex >= mePlayer->townsCount)
    {
        defButton->HideDeactivate();
        return;
    }

    const auto townId = mePlayer->towns[townIndex + dlg->townIndex];
    if (townId != -1)
    {
        const auto originalDefId = dlg->GetDef(155 + townIndex);

        defButton->SetX(originalDefId->GetX() + 32);
        defButton->SetY(originalDefId->GetY() + 15);
        AdjustTownBuiltButtonPosition(defButton, townId);
        defButton->DeActivate();
    }
    else
    {
        defButton->HideDeactivate();
    }
}

void __stdcall H3TownManager__AfterDlgCtor(HiHook *h, H3TownManager *mgr, const int fadeIn)
{
    GraphicsEnhancements::Get()->InitTownDlgDefButtons(mgr->dlg);
    THISCALL_2(void, h->GetDefaultFunc(), mgr, fadeIn);
}

void __stdcall H3TownDlg__SetSmallTownFrame(HiHook *h, H3TownDialog *dlg, signed int townIndex)
{
    THISCALL_2(void, h->GetDefaultFunc(), dlg, townIndex);
    GraphicsEnhancements::Get()->DrawTownDlgBuiltStatus(dlg, townIndex);
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
    auto &advMapDlg = builtDefButtons.advMapDlg;
    libc::memset(advMapDlg.data(), 0, std::size(advMapDlg));
    auto &townDlg = builtDefButtons.townDlg;
    libc::memset(townDlg.data(), 0, std::size(townDlg));
}
void GraphicsEnhancements::CreatePatches() noexcept
{
    // set different town view for fort/citadel/castle
    _pi->WriteLoHook(0x4C980D, Game_AtTownSettingMapItemDef);

    // set different heroes on map views
    // hook is at "mov ecx" so chnage ecx and return NO_EXEC_DEFAULT;
    _pi->WriteLoHook(0x47F443, AdventureManager_DrawHeroDef); // @Hawaiing
    _pi->WriteLoHook(0x47F876, AdventureManager_DrawHeroDef); // @Hawaiing

    // hook is at "call" so only change ecx;
    _pi->WriteLoHook(0x04106A6, AdventureManager_DrawHeroDefTransparent); // @Hawaiing
    _pi->WriteLoHook(0x0410206, AdventureManager_DrawHeroDefTransparent); // @Hawaiing
    _pi->WriteLoHook(0x047F592, AdventureManager_DrawHeroDefShadow);      // @Hawaiing

    // is needed to load/unload defs
    _pi->WriteLoHook(0x040730F, AdventureManager_Show);
    _pi->WriteLoHook(0x04077B6, AdventureManager_Hide);
    // town dialog built status
    WriteHiHook(0x05C697C, THISCALL_, H3TownManager__AfterDlgCtor);

    if (EraJS::readInt("gem_plugin.building_hints.enable"))
    {
        WriteHiHook(0x0417FFB, THISCALL_, AdvMgr__AtSetActiveHero_BeforeScreenRedraw);
        WriteHiHook(0x00403420, THISCALL_, H3AdventureMgrDlg__RedrawTownSlots);
        // _pi->WriteLoHook(0x0417FA6, AdvMgr_Draw_MiniMap);

        // WriteHiHook(0x0417E79, THISCALL_, H3AdventureMgrDlg__RedrawTownSlotsA);

        WriteHiHook(0x00417446, THISCALL_, AdvMgr_AtFullUpdate);

        WriteHiHook(0x05C5EBC, THISCALL_, H3TownDlg__SetSmallTownFrame);
    }
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
