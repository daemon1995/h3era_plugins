#include "pch.h"

namespace advMapHints
{
RECT AdventureMapHints::m_mapView;

AdventureMapHints *AdventureMapHints::instance = nullptr;
void AdventureMapHints::Init(PatcherInstance *pi)
{
    if (!instance)
    {
        instance = new AdventureMapHints(pi);
    }
}

AdventureMapHints::AdventureMapHints(PatcherInstance *pi)
    : IGamePatch(pi), settings("Runtime/gem_AdventureMapHints.ini", "StaticHintsDrawByType")

{
    m_mapView.left = 8;                                   // right panel
    m_mapView.top = 8;                                    // bottom panel
    m_mapView.right = H3GameWidth::Get() - (800 - 592);   // right panel
    m_mapView.bottom = H3GameHeight::Get() - (600 - 544); // bottom panel
    CreatePatches();
}

void AdventureMapHints::CreatePatches() noexcept
{

    if (!m_isInited)
    {
        blockAdventureHintDraw = _pi->CreateHexPatch(0x040D0F4, const_cast<char *>("EB 40 90"));
        blockIgnoreHintBarFocus = _pi->CreateHexPatch(0x040B0DC, const_cast<char *>("90 90 90 90 90 90"));

        _pi->WriteLoHook(0x040F5AB, AdvMgr_BeforeObjectsDraw);
        if (settings.drawOverFogOfWar)
        {
            _pi->WriteHiHook(0x040F6A5, THISCALL_, AdvMgr_TileObjectDraw);
        }
        else
        {
            _pi->WriteHiHook(0x040F5D7, THISCALL_, AdvMgr_TileObjectDraw);
        }

        m_isInited = true;
    }
}

void GameManager_HidePlayersVisitedInfo(H3Main *game, const int playerID, H3PlayersBitfield (&changedData)[32]) noexcept
{

    const int withcHutData = game->visitedWitchHut.Status(playerID);
    const int shrineData = game->visitedShrines.Status(playerID);
    const int treeOfKnoledgeData = game->visitedTreeKnowledge.Status(playerID);

    void *address = (void *)AddressOf(P_Game->Get()->visitedBuoy);
    libc::memcpy(changedData, address, sizeof(changedData));
    libc::memset(address, 0, sizeof(changedData));

    game->visitedWitchHut.Set(withcHutData, playerID);
    game->visitedShrines.Set(shrineData, playerID);
    game->visitedTreeKnowledge.Set(treeOfKnoledgeData, playerID);

    // return result;
}
LPCSTR AdventureMapHints::GetHintText(const H3AdventureManager *adv, const H3MapItem *mapItem, const int mapX,
                                      const int mapY, const int mapZ) noexcept
{

    // create custom hints for some object types

    DWORD oldCreatureHintFormat = 0;
    BYTE skipMineOwnershipHint = 0;

    DWORD skipMineArmyHint = 0;

    BOOL memoryPatchSet = true;
    switch (mapItem->objectType)
    {
    case ::eObject::CREATURE_GENERATOR1:
    case ::eObject::CREATURE_GENERATOR4:

        break;
    case ::eObject::MINE:
        skipMineOwnershipHint = ByteAt(0x040D635);
        ByteAt(0x040D635) = 0xEB; // oldCreatureHintFormat;
        skipMineArmyHint = DwordAt(0x040D77F);
        DwordAt(0x040D77F) = 0x000096E9; // oldCreatureHintFormat;
        break;
    case ::eObject::MONSTER:
        oldCreatureHintFormat = DwordAt(0x40C2E7 + 1);

        // LPCSTR advMapCreatureHintFormat = settings.creatureHintFormat;
        DwordAt(0x40C2E7 + 1) = reinterpret_cast<DWORD>(settings.creatureHintFormat);
        break;
    default:
        memoryPatchSet = false;
        break;
    }

    constexpr UINT NOT_VISITED_TEXT_ID = 354;
    constexpr UINT VISITED_TEXT_ID = 353;

    // store old text pointers
    auto visitedText = H3GeneralText::Get()->GetText(VISITED_TEXT_ID + 1);
    auto notVisitedText = H3GeneralText::Get()->GetText(NOT_VISITED_TEXT_ID + 1);

    H3String changedVisitedText = H3String::Format(settings.visitedHintFormat, visitedText).String();
    H3String changedNotVisitedText = H3String::Format(settings.nonVisitedHintFormat, notVisitedText).String();

    H3Vector<LPCSTR> *generalTextPtr = reinterpret_cast<H3Vector<LPCSTR> *>(ADDRESS(H3GeneralText::Get()) + 0x1C);
    auto p_visited = generalTextPtr->At(VISITED_TEXT_ID);
    auto p_notVisited = generalTextPtr->At(NOT_VISITED_TEXT_ID);

    // replace "visited" and "not visited" text with own text pointers
    *p_visited = changedVisitedText.String();
    *p_notVisited = changedNotVisitedText.String();

    instance->blockAdventureHintDraw->Apply();  // don't draw hint whne custom function call
    instance->blockIgnoreHintBarFocus->Apply(); // don't skip hint creation when focus is on hint bar text edit

    // hide "visited" info for objects by players
    auto &visitedData = instance->playersVisitedObjectData;
    GameManager_HidePlayersVisitedInfo(P_Game->Get(), H3CurrentPlayerID::Get(), visitedData);

    // set "hint_from_plugin" flag
    Era::SetAssocVarIntValue("GameplayEnhancementsPlugin_AdventureMapHints_AtHint", 1);

    THISCALL_4(void, 0x40B0B0, adv, mapItem, mapX, mapY);

    // remove "hint_from_plugin" flag
    Era::SetAssocVarIntValue("GameplayEnhancementsPlugin_AdventureMapHints_AtHint", 0);

    // restore "visited" info for objects by players
    libc::memcpy((void *)AddressOf(P_Game->Get()->visitedBuoy), visitedData, sizeof(visitedData));

    // undo patches
    instance->blockIgnoreHintBarFocus->Undo();
    instance->blockAdventureHintDraw->Undo();

    // restore "visited" and "not visited" text pointers
    *p_visited = visitedText;
    *p_notVisited = notVisitedText;

    // restore memory patches for some object types
    if (memoryPatchSet)
    {
        switch (mapItem->objectType)
        {
        case ::eObject::CREATURE_GENERATOR1:
        case ::eObject::CREATURE_GENERATOR4:

            break;
        case ::eObject::MINE:
            ByteAt(0x040D635) = skipMineOwnershipHint;
            DwordAt(0x040D77F) = skipMineArmyHint;
            break;
        case ::eObject::MONSTER:
            DwordAt(0x40C2E7 + 1) = oldCreatureHintFormat;
            break;
        default:
            break;
        }
    }

    return h3_TextBuffer;
}

_LHF_(AdventureMapHints::AdvMgr_BeforeObjectsDraw)
{

    instance->needDrawHints = false;
    // if map isn't forcelly hidden
    if (IntAt(0x699588) == 0)
    {
        const BOOL keyIsHeld = GetFocus() == H3Hwnd::Get() &&
                               STDCALL_1(SHORT, PtrAt(0x63A294), instance->settings.vKey) & 0x800 &&
                               instance->settings.isHeld;

        if (keyIsHeld)
        {
            instance->playerID = P_Game->Get()->GetPlayerID();
            sprintf(Era::z[0], "gem_adventure_map_object_hints_option_%d", instance->playerID); // ;
            instance->needDrawHints = Era::GetAssocVarIntValue(Era::z[0]);
        }
    }

    instance->drawnOjectIndexes.clear();

    return EXEC_DEFAULT;
}
void __stdcall AdventureMapHints::AdvMgr_TileObjectDraw(HiHook *h, H3AdventureManager *adv, int mapX, int mapY,
                                                        int mapZ, int screenX, int screenY)
{

    THISCALL_6(void, h->GetDefaultFunc(), adv, mapX, mapY, mapZ, screenX, screenY);

    if (instance->needDrawHints)
    {
        // check if tile may have object and visible by user
        if (mapX >= 0 && mapY >= 0 && mapX < *P_MapSize && mapY < *P_MapSize &&
            H3TileVision::CanViewTile(mapX, mapY, mapZ, instance->playerID))
        {

            H3MapItem *currentItem = adv->GetMapItem(mapX, mapY, mapZ);
            if (!currentItem)
            {
                return;
            }

            // if not entrance point find if it visible by player and don't draw current item
            if (!currentItem->IsEntrance())
            {
                if (auto *entrance = currentItem->GetEntrance())
                {
                    if (H3TileVision::CanViewTile(entrance->GetCoordinates(), instance->playerID))
                    {
                        return;
                    }
                }
            }

            if (instance->NeedDrawMapItem(currentItem) &&
                instance->drawnOjectIndexes.insert(currentItem->drawnObjectIndex).second)
            {

                const bool isHero = currentItem->objectType == eObject::HERO;

                constexpr int TILE_WIDTH = 32;
                constexpr int TEXT_MARGIN = 2;

                constexpr int OBJECT_WIDTH = 1 * TILE_WIDTH;

                H3LoadedPcx16 *tempBuffer = nullptr;
                if (isHero)
                {

                    H3DefLoader flagDef(NH3Dlg::Assets::CREST58);
                    currentItem->hero.index;
                    // return;

                    //   Era::y[1] = currentItem->hero.index;
                    // Era::ExecErmCmd("HEy1:Z?y1^");
                    //   if (Era::y[1] <= 0)

                    // Era::ExecErmCmd("IF:L^%y1^");
                    //  return;

                    const H3Hero *hero = &P_Game->heroes[currentItem->hero.index];
                    // Era::y[1];//P_Game->heroes[currentItem->hero.index];

                    const int owner = hero->owner;

                    const auto heroInfos = P_HeroInfo->Get();

                    if (hero->id < 0 || hero->id > 154)
                    {
                        return;
                    }
                    H3PcxLoader portrait(heroInfos[hero->id].smallPortrait);

                    if (true)
                    {
                    }

                    // libc::sprintf(Era::z[1], "%d", currentItem->hero.index);
                    // Era::ExecErmCmd("IF:L^%z1^");
                    //                    H3Messagebox(heroInfos[hero.id].smallPortrait);
                    //  return;

                    //  if (!heroInfos[hero->id].smallPortrait)
                    {
                        //       return;
                    }
                    // return;

                    const int TEMP_PCX_WIDTH = portrait->width + TEXT_MARGIN;
                    const int TEMP_PCX_HEIGHT = portrait->height + (flagDef->heightDEF >> 1) + TEXT_MARGIN;
                    //   return;

                    tempBuffer = H3LoadedPcx16::Create(TEMP_PCX_WIDTH, TEMP_PCX_HEIGHT);
                    //  return;
                    const int frameId = owner >= 0 && owner < 8 ? owner : 8;

                    memset(tempBuffer->buffer, 0, tempBuffer->buffSize);
                    portrait->DrawToPcx16(tempBuffer, 1, 1, 1);

                    flagDef->DrawToPcx16(0, frameId, ((flagDef->widthDEF - portrait->width) >> 1) - 1, 4,
                                         portrait->width, (flagDef->heightDEF >> 1) - 1, tempBuffer, 1,
                                         portrait->height + 1);
                    //    return;
                }
                else
                {

                    H3FontLoader fnt(NH3Dlg::Text::TINY);

                    LPCSTR hintText = instance->GetHintText(adv, currentItem, mapX, mapY, mapZ);
                    constexpr int minTextFieldWidth = OBJECT_WIDTH;
                    const int maxHintTextWidth = fnt->GetMaxLineWidth(hintText);
                    constexpr int maxTextFieldWidth = TILE_WIDTH * 3;
                    const int textWidth = Clamp(minTextFieldWidth, maxHintTextWidth, maxTextFieldWidth);

                    const int hintTextLines = fnt->GetLinesCountInText(hintText, textWidth);
                    const int minTextFieldHeight = fnt->height + 2;
                    const int textHeight = hintTextLines * (minTextFieldHeight - 1);
                    const int TEMP_PCX_WIDTH = textWidth + 10;
                    const int TEMP_PCX_HEIGHT = textHeight + TEXT_MARGIN;

                    tempBuffer = H3LoadedPcx16::Create(TEMP_PCX_WIDTH, TEMP_PCX_HEIGHT);

                    memset(tempBuffer->buffer, 0, tempBuffer->buffSize);

                    fnt->TextDraw(tempBuffer, hintText, TEXT_MARGIN, 0, TEMP_PCX_WIDTH - TEXT_MARGIN, TEMP_PCX_HEIGHT);
                }

                if (tempBuffer)
                {
                    //                    auto &attributes =
                    //                    P_Game->mainSetup.objectAttributes[currentItem->drawnObjectIndex];
                    // auto &passability = attributes.passability;

                    // passability;

                    //  create golden frame

                    // draw text to temp buffer
                    const int TEMP_PCX_WIDTH = tempBuffer->width;
                    const int TEMP_PCX_HEIGHT = tempBuffer->height;

                    tempBuffer->DrawFrame(0, 0, TEMP_PCX_WIDTH, TEMP_PCX_HEIGHT, 189, 149, 57);
                    // resize tempBuffer to align text for screen borders

                    int objectWidth = 1 * TILE_WIDTH;
                    const int outOfWidthBorder = (TEMP_PCX_WIDTH - objectWidth) >> 1;

                    int destPcxX = screenX * TILE_WIDTH + adv->screenDrawOffset.x - outOfWidthBorder;

                    const int additionalYOffset = instance->settings.drawObjectHint[currentItem->objectType].yOffset;

                    int destPcxY = screenY * TILE_WIDTH + adv->screenDrawOffset.y - TEMP_PCX_HEIGHT + additionalYOffset;

                    // adjust left border draw
                    UINT srcX = 0;
                    if (destPcxX < m_mapView.left)
                    {
                        srcX = m_mapView.left - destPcxX;
                        destPcxX = m_mapView.left;
                        tempBuffer->width -= srcX;
                    }
                    if (destPcxX + TEMP_PCX_WIDTH - m_mapView.left > m_mapView.right)
                        tempBuffer->width = m_mapView.right + m_mapView.left - destPcxX;

                    UINT srcY = 0;
                    if (destPcxY < m_mapView.top)
                    {
                        srcY = m_mapView.top - destPcxY;
                        destPcxY = m_mapView.top;
                        tempBuffer->height -= srcY;
                    }
                    if (destPcxY + TEMP_PCX_HEIGHT - m_mapView.top > m_mapView.bottom)
                        tempBuffer->height = m_mapView.top + m_mapView.bottom - destPcxY;

                    // if need to draw any hint
                    if (tempBuffer->height > 0 && tempBuffer->width > 0)
                    {
                        // get general Window draw buffer to draw temp pcx with x/y offsets
                        auto drawBuffer = P_WindowManager->GetDrawBuffer();
                        tempBuffer->DrawToPcx16(destPcxX, destPcxY, 1, drawBuffer, srcX, srcY);

                        constexpr UINT SHADOW_SIZE = 3;
                        int heightReserve = m_mapView.bottom - tempBuffer->height - destPcxY + m_mapView.top;
                        UINT shadowWidth = 0;

                        UINT shadowHeight = 0;

                        if (heightReserve > 0)
                            shadowHeight = heightReserve >= SHADOW_SIZE ? SHADOW_SIZE : heightReserve;

                        int widthReserve = m_mapView.right - tempBuffer->width - destPcxX + m_mapView.left;
                        if (widthReserve > 0)
                            shadowWidth = widthReserve >= SHADOW_SIZE ? SHADOW_SIZE : widthReserve;

                        if (shadowWidth)
                            drawBuffer->DrawShadow(destPcxX + tempBuffer->width, destPcxY, shadowWidth,
                                                   tempBuffer->height + shadowHeight);

                        if (shadowHeight)
                            drawBuffer->DrawShadow(destPcxX, destPcxY + tempBuffer->height,
                                                   tempBuffer->width + (shadowHeight ? 0 : shadowWidth), shadowHeight);
                    }

                    //	backPcx->Dereference();
                    tempBuffer->Destroy();
                }
            }
        }
    }
}
void __stdcall AdventureMapHints::AdvMgr_DrawCornerFrames(HiHook *h, const H3AdventureManager *adv)
{
    THISCALL_1(void, h->GetDefaultFunc(), adv);
    if (instance->drawnOjectIndexes.size())
    {
        instance->drawnOjectIndexes.clear();
    }
}

bool AdventureMapHints::NeedDrawMapItem(const H3MapItem *mIt) const noexcept
{
    if (mIt)
        return settings.drawObjectHint[mIt->objectType].userValue;
    return false;
}

AdventureMapHints::~AdventureMapHints()
{
}

AdventureHintsSettings::AdventureHintsSettings(const char *filePath, const char *sectionName)
    : ISettings{filePath, sectionName}
{
    reset();
    load();
    save();
    isHeld = true;
}

void AdventureHintsSettings::reset()
{
    vKey = VK_MENU;

    memset(drawObjectHint, false, sizeof(drawObjectHint));

    drawObjectHint[eObject::HERO].yOffset = 38;
    drawObjectHint[eObject::ARTIFACT].yOffset = 16;
    drawObjectHint[eObject::RESOURCE].yOffset = 16;
    drawObjectHint[eObject::PANDORAS_BOX].yOffset = 16;
    drawObjectHint[eObject::SPELL_SCROLL].yOffset = 16;
    drawObjectHint[eObject::TREASURE_CHEST].yOffset = 16;

    drawObjectHint[eObject::ARENA].defaultValue = true;
    drawObjectHint[eObject::ARTIFACT].defaultValue = true;
    drawObjectHint[eObject::PANDORAS_BOX].defaultValue = true;
    drawObjectHint[eObject::BLACK_MARKET].defaultValue = true;
    drawObjectHint[eObject::BOAT].defaultValue = true;
    drawObjectHint[eObject::BORDERGUARD].defaultValue = true;
    drawObjectHint[eObject::KEYMASTER].defaultValue = true;
    drawObjectHint[eObject::BUOY].defaultValue = true;
    drawObjectHint[eObject::CAMPFIRE].defaultValue = true;
    drawObjectHint[eObject::CARTOGRAPHER].defaultValue = true;
    drawObjectHint[eObject::SWAN_POND].defaultValue = true;

    drawObjectHint[eObject::CREATURE_BANK].defaultValue = true;
    drawObjectHint[eObject::CREATURE_GENERATOR1].defaultValue = true;

    drawObjectHint[eObject::CREATURE_GENERATOR4].defaultValue = true;

    drawObjectHint[eObject::CORPSE].defaultValue = true;
    drawObjectHint[eObject::MARLETTO_TOWER].defaultValue = true;
    drawObjectHint[eObject::DERELICT_SHIP].defaultValue = true;
    drawObjectHint[eObject::DRAGON_UTOPIA].defaultValue = true;

    drawObjectHint[eObject::FLOTSAM].defaultValue = true;
    drawObjectHint[eObject::FOUNTAIN_OF_FORTUNE].defaultValue = true;

    drawObjectHint[eObject::GARDEN_OF_REVELATION].defaultValue = true;

    drawObjectHint[eObject::HERO].defaultValue = true;
    drawObjectHint[eObject::HILL_FORT].defaultValue = true;

    drawObjectHint[eObject::LIBRARY_OF_ENLIGHTENMENT].defaultValue = true;
    drawObjectHint[eObject::LIGHTHOUSE].defaultValue = true;
    drawObjectHint[eObject::MONOLITH_ONE_WAY_ENTRANCE].defaultValue = true;
    drawObjectHint[eObject::MONOLITH_ONE_WAY_EXIT].defaultValue = true;

    drawObjectHint[eObject::MONOLITH_TWO_WAY].defaultValue = true;

    drawObjectHint[eObject::SCHOOL_OF_MAGIC].defaultValue = true;
    drawObjectHint[eObject::MAGIC_SPRING].defaultValue = true;
    drawObjectHint[eObject::MAGIC_WELL].defaultValue = true;

    drawObjectHint[eObject::MERCENARY_CAMP].defaultValue = true;
    drawObjectHint[eObject::MERMAID].defaultValue = true;
    drawObjectHint[eObject::MINE].defaultValue = true;
    drawObjectHint[eObject::MONSTER].defaultValue = true;
    drawObjectHint[eObject::MYSTICAL_GARDEN].defaultValue = true;

    drawObjectHint[eObject::OBELISK].defaultValue = true;
    drawObjectHint[eObject::REDWOOD_OBSERVATORY].defaultValue = true;

    drawObjectHint[eObject::PILLAR_OF_FIRE].defaultValue = true;
    drawObjectHint[eObject::STAR_AXIS].defaultValue = true;

    drawObjectHint[eObject::PYRAMID].defaultValue = true;
    drawObjectHint[eObject::RALLY_FLAG].defaultValue = true;

    drawObjectHint[eObject::RESOURCE].defaultValue = true;

    drawObjectHint[eObject::SCHOLAR].defaultValue = true;
    drawObjectHint[eObject::SEA_CHEST].defaultValue = true;
    drawObjectHint[eObject::SEER_HUT].defaultValue = true;
    drawObjectHint[eObject::CRYPT].defaultValue = true;
    drawObjectHint[eObject::SHIPWRECK].defaultValue = true;
    drawObjectHint[eObject::SHIPWRECK_SURVIVOR].defaultValue = true;
    drawObjectHint[eObject::SHIPYARD].defaultValue = true;
    drawObjectHint[eObject::SHRINE_OF_MAGIC_INCANTATION].defaultValue = true;
    drawObjectHint[eObject::SHRINE_OF_MAGIC_GESTURE].defaultValue = true;
    drawObjectHint[eObject::SHRINE_OF_MAGIC_THOUGHT].defaultValue = true;

    drawObjectHint[eObject::SIRENS].defaultValue = true;
    drawObjectHint[eObject::SPELL_SCROLL].defaultValue = true;
    drawObjectHint[eObject::STABLES].defaultValue = true;
    drawObjectHint[eObject::TAVERN].defaultValue = true;

    drawObjectHint[eObject::LEARNING_STONE].defaultValue = true;
    drawObjectHint[eObject::TREASURE_CHEST].defaultValue = true;
    drawObjectHint[eObject::TREE_OF_KNOWLEDGE].defaultValue = true;

    drawObjectHint[eObject::UNIVERSITY].defaultValue = true;
    drawObjectHint[eObject::WAGON].defaultValue = true;
    drawObjectHint[eObject::WAR_MACHINE_FACTORY].defaultValue = true;
    drawObjectHint[eObject::SCHOOL_OF_WAR].defaultValue = true;
    drawObjectHint[eObject::WARRIORS_TOMB].defaultValue = true;

    drawObjectHint[eObject::WATER_WHEEL].defaultValue = true;

    drawObjectHint[eObject::WINDMILL].defaultValue = true;
    drawObjectHint[eObject::WITCH_HUT].defaultValue = true;
    drawObjectHint[142].defaultValue = true;

    drawObjectHint[144].defaultValue = true;

    drawObjectHint[eObject::BORDER_GATE].defaultValue = true;
    drawObjectHint[eObject::QUEST_GUARD].defaultValue = true;

    // set all to default
    for (auto &i : drawObjectHint)
    {
        i.userValue = i.defaultValue;
    }
}

BOOL AdventureHintsSettings::load()
{
    bool readSuccess = false;
    creatureHintFormat = EraJS::read("gem_plugin.adventure_hints.creature_format", readSuccess);
    if (!readSuccess)
    {
        creatureHintFormat = "{%s}\n{~r}%s}";
    }
    visitedHintFormat = EraJS::read("gem_plugin.adventure_hints.visited_format", readSuccess);
    if (!readSuccess)
    {
        visitedHintFormat = "{~LightGreen}\n%s}";
    }
    nonVisitedHintFormat = EraJS::read("gem_plugin.adventure_hints.not_visited_format", readSuccess);
    if (!readSuccess)
    {
        nonVisitedHintFormat = "{~Orange}\n%s}";
    }
    for (UINT8 i = 0; i < limits::OBJECTS; ++i)
    {

        if (Era::ReadStrFromIni(Era::IntToStr(i).c_str(), sectionName, filePath, h3_TextBuffer))
        {
            const bool userValue = atoi(h3_TextBuffer);
            if (userValue != drawObjectHint[i].defaultValue)
            {
                drawObjectHint[i].userValue = userValue;
            }
        }
    }
    if (Era::ReadStrFromIni("KeyCode", "ControlSettings", filePath, h3_TextBuffer))
        vKey = atoi(h3_TextBuffer);
    return 0;
}

BOOL AdventureHintsSettings::save()
{
    Era::ClearIniCache(filePath);
    // DeleteFileA(filePath);
    for (UINT8 i = 0; i < limits::OBJECTS; ++i)
    {
        if (drawObjectHint[i].userValue != drawObjectHint[i].defaultValue)
        {
            Era::WriteStrToIni(Era::IntToStr(i).c_str(), Era::IntToStr(drawObjectHint[i].userValue).c_str(),
                               sectionName, filePath);
        }
    }
    Era::WriteStrToIni("KeyCode", Era::IntToStr(vKey).c_str(), "ControlSettings", filePath);

    Era::SaveIni(filePath);

    return 0;
}
} // namespace advMapHints
