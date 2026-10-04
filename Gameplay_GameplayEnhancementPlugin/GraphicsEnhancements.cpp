#include "GraphicsEnhancements.h"
#include "ModuleSupport.h"

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

LPCSTR Hero_GetMapItemDefName(const UINT heroId)
{
    bool readSuccess = false;

    // first check unique hero def name
    LPCSTR defName = EraJS::read(H3String::Format("gem_plugin.map_item_view.54.id.%d", heroId).String(), readSuccess);
    // if name is read and not empty
    if (readSuccess && defName && defName[0])
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
    if (readSuccess && defName && defName[0])
        return defName;

    return nullptr;
}
H3LoadedDef *GraphicsEnhancements::Hero_GetMapItemDef(const H3Hero *hero) noexcept
{

    if (!hero)
        return nullptr;

    H3LoadedDef *result = nullptr;
    // first set hero def by id
    if (hero->id >= 0 && hero->id < MAX_UNIQUE_HEROES &&
        (result = instance->uniqueHeroDefs[hero->id]))
        return result;
    // if empty set hero def by class
    if (hero->isFemale >= 0 && hero->isFemale < 2 && hero->hero_class >= 0 &&
        hero->hero_class < MAX_UNIQUE_CLASSES &&
        (result = instance->heroClassDefs[hero->isFemale][hero->hero_class]))
        return result;
    return result;
}
namespace
{
// Load before releasing the previous reference: the resource manager may return the same DEF.
void ReplaceHeroDef(H3LoadedDef *&stored, LPCSTR name) noexcept
{
    if (stored && name && !libc::strcmpi(stored->GetName(), name))
        return;
    auto *replacement = name ? H3LoadedDef::Load(name) : nullptr;
    if (stored)
        stored->Dereference();
    stored = replacement;
}

int PlayerTownId(const H3Player *player, int index) noexcept
{
    return player && index >= 0 && index < player->townsCount &&
                   index < static_cast<int>(std::size(player->towns)) ? player->towns[index] : -1;
}
} // namespace

H3LoadedDef *GraphicsEnhancements::InitHeroData(const UINT heroId) noexcept
{
    if (heroId >= MAX_UNIQUE_HEROES)
        return nullptr;
    ReplaceHeroDef(uniqueHeroDefs[heroId], Hero_GetMapItemDefName(heroId));
    return uniqueHeroDefs[heroId];
}

void GraphicsEnhancements::InitHeroClassData(const UINT classId) noexcept
{
    if (classId >= MAX_UNIQUE_CLASSES)
        return;
    for (int gender = 0; gender < 2; ++gender)
        ReplaceHeroDef(heroClassDefs[gender][classId], HeroClass_GetMapItemDefName(classId, gender != 0));
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
    if (auto instance = &GraphicsEnhancements::Get())
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
    GraphicsEnhancements::Get().CleanUpData();
    return EXEC_DEFAULT;
}
void GraphicsEnhancements::InitAdventureMapTownBuiltDefs() noexcept
{
    builtDefButtons.advMapDlg.fill(nullptr);
    if (!buildingHintsEnabled)
        return;
    // get max towns displayable built icons from config
    constexpr INT hdModTownsMax = 7;
    constexpr INT defaultTowns = 5;
    maxTownsDisplayableBuiltIcons =
        Clamp(defaultTowns, globalPatcher->VarGetValue<int>("HD.AdvMgr.TownList.L", defaultTowns), hdModTownsMax);
    const int firstDefButtonId = globalPatcher->VarGetValue<int>("HD.AdvMgr.ID32", 32);
    auto &dlg = P_AdventureManager->dlg;

    if (auto firstDef = dlg->GetH3DlgItem(firstDefButtonId))
    {

        const INT16 xPos = firstDef->GetX() + 32;
        const INT16 yBase = firstDef->GetY();
        auto &advMapDlg = builtDefButtons.advMapDlg;

        for (INT16 i = 0; i < hdModTownsMax; i++)
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
        }
    }
}
enum eBuildingInfoFrames : int
{
    AVAILABLE_TO_BUILD = -1,
    NOTHING_TO_BUILD = 0,
    CANT_BUILD = 1,
    CANT_AFFORD = 2,
};

static eBuildingInfoFrames Town_GetExtendedInfoFrameId(const H3Town *town)
{
    if (town->builtThisTurn)
        return eBuildingInfoFrames::CANT_BUILD;

    eBuildingInfoFrames result = eBuildingInfoFrames::NOTHING_TO_BUILD;

    const int maxTownsBuildings = GraphicsEnhancements::GetMaxTownBuildingCount();
    for (int i = 0; i < maxTownsBuildings; i++)
    {
        if (i == eBuildings::GRAIL)
            continue;
        // check if building is buildable in town
        if (THISCALL_2(bool, 0x05C1120, town, i))
        {
            // skip if building is already built
            if (town->IsBuildingBuilt(i))
                continue;
            // if player may afford that building
            if (FASTCALL_2(bool, 0x0460D10, town, i))
            {
                return eBuildingInfoFrames::AVAILABLE_TO_BUILD;
            }
            result = eBuildingInfoFrames::CANT_AFFORD;
        }
    }
    return result;
}
void AdjustTownBuiltButtonPosition(H3DlgDefButton *defButton, const int townIndex)
{
    if (!defButton)
        return;
    if (townIndex < 0 || townIndex >= static_cast<int>(P_Game->towns.Size()))
    {
        defButton->HideDeactivate();
        return;
    }

    const H3Town *currentTown = &P_Game->towns[townIndex];

    const auto result = Town_GetExtendedInfoFrameId(currentTown);

    if (result == eBuildingInfoFrames::NOTHING_TO_BUILD || result == eBuildingInfoFrames::CANT_AFFORD)
    {
        defButton->SetFrame(result); // show built
        defButton->ShowActivate();
    }
    else
    {
        defButton->HideDeactivate();
    }
}

void GraphicsEnhancements::DrawAdventureMapTownBuiltStatus(H3AdventureMgrDlg *dlg, const BOOL draw,
                                                           const BOOL updateScreen) noexcept
{
    if (!dlg)
        return;
    const auto mePlayer = P_Game->GetPlayer();
    if (!mePlayer || IntAt(0x699588) || mePlayer->ownerID != P_CurrentPlayerID)
    {
        for (auto *button : builtDefButtons.advMapDlg)
            if (button)
            {
                button->HideDeactivate();
                if (updateScreen)
                    button->Refresh();
            }
        return;
    }

    auto &advMapDlg = builtDefButtons.advMapDlg;
    for (int i = 0; i < maxTownsDisplayableBuiltIcons; ++i)
    {
        auto *button = advMapDlg[i];
        if (!button)
            continue;
        const int index = i + dlg->topTownSlotIndex;
        const int townId = PlayerTownId(mePlayer, index);
        if (townId < 0 || townId >= static_cast<int>(P_Game->towns.Size()))
        {
            button->HideDeactivate();
            if (updateScreen)
                button->Refresh();
            continue;
        }
        AdjustTownBuiltButtonPosition(button, townId);
        if (draw && button->IsVisible())
            button->Draw();
        if (updateScreen)
            button->Refresh();
    }
}

void __stdcall AdvMgr__AtSetActiveHero_BeforeScreenRedraw(HiHook *h, H3WindowManager *mgr, const int x, const int y,
                                                          const int width, const int height)
{
    GraphicsEnhancements::Get().DrawAdventureMapTownBuiltStatus(P_AdventureManager->dlg, true, false);
    THISCALL_5(void, h->GetDefaultFunc(), mgr, x, y, width, height);
}

void __stdcall H3AdventureMgrDlg__RedrawHeroSlots(HiHook *h, H3AdventureMgrDlg *dlg, signed int playrId, char updateDlg,
                                                  char redrawScreen)
{
    THISCALL_4(void, h->GetDefaultFunc(), dlg, playrId, updateDlg, redrawScreen);

    if (P_Game->GetPlayer()->isHuman)
        GraphicsEnhancements::Get().DrawAdventureMapTownBuiltStatus(dlg, updateDlg, redrawScreen);
}

void __stdcall H3AdventureMgrDlg__RedrawTownSlots(HiHook *h, H3AdventureMgrDlg *dlg, signed int playrId, char updateDlg,
                                                  char redrawScreen)
{
    THISCALL_4(void, h->GetDefaultFunc(), dlg, playrId, updateDlg, redrawScreen);
    GraphicsEnhancements::Get().DrawAdventureMapTownBuiltStatus(dlg, updateDlg, redrawScreen);
}

void __stdcall AdvMgr_AtFullUpdate(HiHook *h, H3AdventureMgrDlg *dlg, char redraw)
{
    THISCALL_2(void, h->GetDefaultFunc(), dlg, redraw);
    GraphicsEnhancements::Get().DrawAdventureMapTownBuiltStatus(dlg, true, false);
}

void GraphicsEnhancements::InitTownDlgDefButtons(H3TownDialog *dlg) noexcept
{

    builtDefButtons.townDlg.fill(nullptr);
    auto firstDefButton = dlg->GetH3DlgItem(155);
    if (firstDefButton)
    {
        const INT16 xPos = firstDefButton->GetX() + 32;
        const INT16 yBase = firstDefButton->GetY();
        for (INT16 i = 0; i < 4; i++)
        {
            H3String buttonName = H3String::Format(BUILD_BUTTON_NAME_FORMAT_TOWN, i);
            const int buttonId = Era::GetButtonID(buttonName.String());
            if (buttonId != -1)
            {
                auto defButton = dlg->GetDefButton(buttonId);
                if (!defButton)
                    continue;
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

void GraphicsEnhancements::DrawTownDlgBuiltStatus(H3TownDialog *dlg) noexcept
{

    if (!dlg)
        return;
    const auto mePlayer = P_Game->GetPlayer();

    for (INT8 i = 0; i < 3; i++)
    {
        auto defButton = builtDefButtons.townDlg[i];

        if (!defButton)
            continue;
        const int index = i + dlg->townIndex;
        const int townId = PlayerTownId(mePlayer, index);
        if (townId < 0 || townId >= static_cast<int>(P_Game->towns.Size()))
        {
            defButton->HideDeactivate();
            continue;
        }

        if (townId != -1)
        {
            const auto originalDef = dlg->GetDef(155 + i);
            if (!originalDef)
            {
                defButton->HideDeactivate();
                continue;
            }

            defButton->SetX(originalDef->GetX() + 31);
            defButton->SetY(originalDef->GetY() + 15);
            AdjustTownBuiltButtonPosition(defButton, townId);
            defButton->DeActivate();
        }
        else
        {
            defButton->HideDeactivate();
        }
    }

    auto defButton = builtDefButtons.townDlg[3];
    if (defButton)
    {
        const auto manager = P_TownManager->Get();
        const int townId = manager && manager->town ? manager->town->number : -1;
        if (townId != -1)
        {
            const auto originalDef = dlg->GetDef(150);
            if (!originalDef)
            {
                defButton->HideDeactivate();
                return;
            }

            defButton->SetX(originalDef->GetX() + 42);
            defButton->SetY(originalDef->GetY() + 48);
            AdjustTownBuiltButtonPosition(defButton, townId);
            defButton->DeActivate();
        }
        else
        {
            defButton->HideDeactivate();
        }
    }
}

void __stdcall H3TownManager__AfterDlgCtor(HiHook *h, H3TownManager *mgr, const int fadeIn)
{
    GraphicsEnhancements::Get().InitTownDlgDefButtons(mgr->dlg);
    THISCALL_2(void, h->GetDefaultFunc(), mgr, fadeIn);
}

void __stdcall H3TownDlg__SetSmallTownFrame(HiHook *h, H3TownDialog *dlg, signed int townIndex)
{
    THISCALL_2(void, h->GetDefaultFunc(), dlg, townIndex);

    if (townIndex == 2)
    {
        GraphicsEnhancements::Get().DrawTownDlgBuiltStatus(dlg);
    }
}
void __stdcall KingdomOverviewDlg_CreateAndRedrawItems(HiHook *h, H3Game *game, const BOOL redrawDlg,
                                                       const BOOL createItems)
{

    THISCALL_3(void, h->GetDefaultFunc(), game, redrawDlg, createItems);

    const BOOL isTownsView = IntAt(0x069CCA0) == 1;

    const int firstItemIdDrawn = reinterpret_cast<int *>(0x069CCA4)[1];
    const auto player = game->GetPlayer();
    auto dlg = *reinterpret_cast<H3BaseDlg **>(0x069CC88);
    if (!player || !dlg)
        return;

    for (int i = 0; i < 4; ++i)
    {
        const int itemId = 40 * (i * 5 + 5) + 4;
        const int indicatorButtonId = itemId + 21;
        auto *buildingIndicatorButton = dlg->GetDef(indicatorButtonId);
        const auto originalDlgDef = dlg->GetH3DlgItem(itemId);
        const int index = firstItemIdDrawn + i;
        const int townId = PlayerTownId(player, index);
        if (!isTownsView || !originalDlgDef || !originalDlgDef->IsVisible() ||
            townId < 0 || townId >= static_cast<int>(game->towns.Size()))
        {
            if (buildingIndicatorButton)
                buildingIndicatorButton->HideDeactivate();
            continue;
        }
        const auto town = &game->towns[townId];

        if (buildingIndicatorButton == nullptr)
        {
            H3DefLoader def("tpthchk.def");
            if (!def.Get())
                continue;
            const int x = originalDlgDef->GetX() + originalDlgDef->GetWidth() - def->widthDEF;
            const int y = originalDlgDef->GetY() + originalDlgDef->GetHeight() - def->heightDEF;
            buildingIndicatorButton = H3DlgDef::Create(x, y, indicatorButtonId, def->GetName());
            if (!buildingIndicatorButton)
                continue;
            dlg->AddItem(buildingIndicatorButton);
            buildingIndicatorButton->DeActivate();
        }

        if (buildingIndicatorButton)
        {
            const auto extendedInfoFrameId = Town_GetExtendedInfoFrameId(town);
            if (extendedInfoFrameId == eBuildingInfoFrames::NOTHING_TO_BUILD ||
                extendedInfoFrameId == eBuildingInfoFrames::CANT_AFFORD)
            {
                buildingIndicatorButton->SetFrame(extendedInfoFrameId);
                buildingIndicatorButton->Show();
                buildingIndicatorButton->Draw();
                buildingIndicatorButton->Refresh();
            }
            else
            {
                buildingIndicatorButton->Hide();
            }
        }
    }
}

DWORD __stdcall Dlg_RightClick_Town_Create(HiHook *h, H3BaseDlg *dlg, H3Town *town, const int viewAccessLevel)
{

    DWORD result = THISCALL_3(DWORD, h->GetDefaultFunc(), dlg, town, viewAccessLevel);
    if (!dlg || !town || viewAccessLevel < 3) // either town owner or ally or vision power == 3
    {
        return result;
    }

    auto extendedInfoFrameId = Town_GetExtendedInfoFrameId(town);
    if (extendedInfoFrameId == eBuildingInfoFrames::NOTHING_TO_BUILD ||
        extendedInfoFrameId == eBuildingInfoFrames::CANT_AFFORD)
    {
        if (const auto originalDlgDef = dlg->GetDef(2001))
        {
            H3DefLoader def("tpthchk.def");
            if (!def.Get())
                return result;
            const int x = originalDlgDef->GetX() + originalDlgDef->GetWidth() - def->widthDEF;
            const int y = originalDlgDef->GetY() + originalDlgDef->GetHeight() - def->heightDEF;
            H3DlgDef *buildingIndicatorButton = H3DlgDef::Create(x, y, 2002, def->GetName(), extendedInfoFrameId);
            if (buildingIndicatorButton)
                dlg->AddItem(buildingIndicatorButton);
        }
    }
    return result;
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
    builtDefButtons.advMapDlg.fill(nullptr);
    builtDefButtons.townDlg.fill(nullptr);
}
void GraphicsEnhancements::CreatePatches() noexcept
{
    if (m_isInited)
        return;
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
    buildingHintsEnabled = gem::ModuleEnabled("gem_plugin.building_hints.enable", false);
    if (buildingHintsEnabled)
    {

        WriteHiHook(0x05C697C, THISCALL_, H3TownManager__AfterDlgCtor);
        maxTownsBuildings = globalPatcher->VarGetValue<int>("ERA.Towns.max_buildings_count", limits::BUILDINGS);
        WriteHiHook(0x0417FFB, THISCALL_, AdvMgr__AtSetActiveHero_BeforeScreenRedraw);
        WriteHiHook(0x00403420, THISCALL_, H3AdventureMgrDlg__RedrawTownSlots);
        WriteHiHook(0x004032E0, THISCALL_, H3AdventureMgrDlg__RedrawHeroSlots);

        WriteHiHook(0x00417446, THISCALL_, AdvMgr_AtFullUpdate);

        WriteHiHook(0x05C5EBC, THISCALL_, H3TownDlg__SetSmallTownFrame);
        WriteHiHook(0x0530600, THISCALL_, Dlg_RightClick_Town_Create);
        WriteHiHook(0x051C210, THISCALL_, KingdomOverviewDlg_CreateAndRedrawItems);
    }
    m_isInited = true;
    m_isEnabled = true;
}

GraphicsEnhancements::GraphicsEnhancements()
    : IGamePatch(globalPatcher->CreateInstance("EraPlugin.GraphicsEnhancements.daemon_n"))
{
    CreatePatches();
}
GraphicsEnhancements &GraphicsEnhancements::Get() noexcept
{
    if (!instance)
    {
        instance = new GraphicsEnhancements();
    }
    return *instance;
}

} // namespace graphics
