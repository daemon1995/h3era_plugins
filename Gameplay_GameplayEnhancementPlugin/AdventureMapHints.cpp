#include "AdventureMapHints.h"
#include "ModuleSupport.h"

namespace advMapHints
{
namespace
{
constexpr int OBJECT_MASK_WIDTH = 8;
constexpr int OBJECT_MASK_HEIGHT = 6;
constexpr int HINT_SHADOW_SIZE = 3;
constexpr int HINT_STAGGER_Y = 8;

bool RectanglesIntersect(const RECT &left, const RECT &right) noexcept
{
    return left.left < right.right && left.right > right.left && left.top < right.bottom && left.bottom > right.top;
}

int GetObjectWidthInTiles(const H3MapItem *mapItem) noexcept
{
    auto &mainSetup = P_Game->mainSetup;
    const UINT objectDetailsIndex = mapItem->drawnObjectIndex;
    if (objectDetailsIndex >= mainSetup.objectDetails.Size())
    {
        return 1;
    }

    const UINT attributesIndex = mainSetup.objectDetails[objectDetailsIndex].num;
    if (attributesIndex >= mainSetup.objectAttributes.Size())
    {
        return 1;
    }

    auto &attributes = mainSetup.objectAttributes[attributesIndex];
    const int maskWidth = Clamp(1, static_cast<int>(attributes.width), OBJECT_MASK_WIDTH);
    const int maskHeight = Clamp(1, static_cast<int>(attributes.height), OBJECT_MASK_HEIGHT);
    int leftmostUsedColumn = -1;

    // Mask column 0 is the object's bottom-right tile; columns grow to the left.
    // Include both visible and transparent-but-blocked parts of the footprint.
    for (int column = 0; column < maskWidth; ++column)
    {
        for (int row = 0; row < maskHeight; ++row)
        {
            if (attributes.colors(column, row) || !attributes.passability(column, row) ||
                attributes.entrances(column, row))
            {
                leftmostUsedColumn = column;
                break;
            }
        }
    }

    return leftmostUsedColumn >= 0 ? leftmostUsedColumn + 1 : maskWidth;
}
} // namespace

RECT AdventureMapHints::m_mapView;

AdventureMapHints *AdventureMapHints::instance = nullptr;
AdventureMapHints &AdventureMapHints::Get()
{
    if (!instance)
    {
        auto pi = globalPatcher->CreateInstance(vipPluginInstanceName);
        instance = new AdventureMapHints(pi);
    }
    return *instance;
}
AdventureMapHints::AdventureMapHints(PatcherInstance *pi)
    : IGamePatch(pi), settings("Runtime/gem_AdventureMapHints.ini", "StaticHintsDrawByType")

{
    drawnHintRects.reserve(64);
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
        auto transient = globalPatcher->CreateInstance("EraPlugin.AdventureMapHints.Transient.daemon_n");
        // Skip both the hint-bar text assignment and its redraw; keep h3_TextBuffer for map labels.
        blockAdventureHintDraw = transient->CreateHexPatch(0x040D0DB, const_cast<char *>("EB 59 90 90 90"));
        blockIgnoreHintBarFocus = transient->CreateHexPatch(0x040B0DC, const_cast<char *>("90 90 90 90 90 90"));

        _pi->WriteLoHook(0x040F5AB, AdvMgr_BeforeObjectsDraw);
        if (settings.drawOverFogOfWar)
        {
            _pi->WriteHiHook(0x040F6A5, THISCALL_, AdvMgr_TileObjectDraw);
        }
        else
        {
            _pi->WriteHiHook(0x040F5D7, THISCALL_, AdvMgr_TileObjectDraw);
        }

        // hero movement hint
        // H3AdventureManager::ProcMapScreen - нажатие/отжатие кнопки
        _pi->WriteHiHook(0x408710, SPLICE_, EXTENDED_, THISCALL_, H3AdventureManager_ProcMapScreen);
        // H3AdventureManager::UpdateHintMessage - Перевод мыши на другой тайл
        _pi->WriteHiHook(0x40b0b0, SPLICE_, EXTENDED_, THISCALL_, H3AdventureManager_SetHint);

        // hero mp status at hero_obj mouse over
        _pi->WriteLoHook(0x40BBEE, H3AdventureManager_SetHeroObjectHint);

        Era::RegisterHandler(ReloadLanguage, "OnAfterReloadLanguageData");
        m_isInited = true;
        m_isEnabled = true;
    }
}

void __stdcall AdventureMapHints::ReloadLanguage(Era::TEvent *)
{
    if (instance)
        instance->settings.load();
}

void GameManager_HidePlayersVisitedInfo(H3Main *game, const int playerID, H3PlayersBitfield (&changedData)[32]) noexcept
{

    const int withcHutData = game->visitedWitchHut.Status(playerID);
    const int shrineData = game->visitedShrines.Status(playerID);
    const int treeOfKnoledgeData = game->visitedTreeKnowledge.Status(playerID);

    void *address = (void *)AddressOf(game->visitedBuoy);
    libc::memcpy(changedData, address, sizeof(changedData));
    libc::memset(address, 0, sizeof(changedData));

    game->visitedWitchHut.Set(playerID, withcHutData != 0);
    game->visitedShrines.Set(playerID, shrineData != 0);
    game->visitedTreeKnowledge.Set(playerID, treeOfKnoledgeData != 0);

}
namespace
{
class ScopedVisitedInfo
{
    H3Main *game;
    H3PlayersBitfield backup[32];
  public:
    ScopedVisitedInfo(H3Main *main, int player) : game(main)
    {
        GameManager_HidePlayersVisitedInfo(game, player, backup);
    }
    ~ScopedVisitedInfo()
    {
        libc::memcpy(&game->visitedBuoy, backup, sizeof(backup));
    }
};
}

LPCSTR AdventureMapHints::GetHintText(const H3AdventureManager *adv, const H3MapItem *mapItem, const int mapX,
                                      const int mapY, const int mapZ) noexcept
{
    auto *game = P_Game->Get();
    const int player = H3CurrentPlayerID::Get();
    if (!mapItem || !game || player < 0 || player >= 8 || isCustomHintCreation)
        return h3_NullString;

    gem::ScopedValue<BYTE> mineOwnership(ByteAt(0x040D635));
    gem::ScopedValue<UINT32> mineArmy(DwordAt(0x040D77F));
    gem::ScopedValue<UINT32> creatureFormat(DwordAt(0x40C2E7 + 1));
    if (mapItem->objectType == eObject::MINE)
    {
        ByteAt(0x040D635) = 0xEB;
        DwordAt(0x040D77F) = 0x000096E9;
    }
    else if (mapItem->objectType == eObject::MONSTER)
        DwordAt(0x40C2E7 + 1) = reinterpret_cast<DWORD>(settings.creatureHintFormat);

    auto texts = reinterpret_cast<H3Vector<LPCSTR> *>(ADDRESS(H3GeneralText::Get()) + 0x1C);
    constexpr UINT visitedId = 353;
    constexpr UINT notVisitedId = 354;
    if (texts->Size() <= notVisitedId)
        return h3_NullString;
    H3String visited = H3String::Format(settings.visitedHintFormat, (*texts)[visitedId]);
    H3String notVisited = H3String::Format(settings.nonVisitedHintFormat, (*texts)[notVisitedId]);
    gem::ScopedValue<LPCSTR> visitedText((*texts)[visitedId], visited.String());
    gem::ScopedValue<LPCSTR> notVisitedText((*texts)[notVisitedId], notVisited.String());
    gem::ScopedPatch blockDraw(blockAdventureHintDraw);
    gem::ScopedPatch ignoreFocus(blockIgnoreHintBarFocus);
    ScopedVisitedInfo visitedInfo(game, player);
    gem::ScopedValue<BOOL> creating(isCustomHintCreation, TRUE);
    const char *flag = "GameplayEnhancementsPlugin_AdventureMapHints_AtHint";
    const int oldFlag = Era::GetAssocVarIntValue(flag);
    struct RestoreFlag
    {
        const char *name;
        int value;
        ~RestoreFlag() { Era::SetAssocVarIntValue(name, value); }
    } restoreFlag{flag, oldFlag};
    Era::SetAssocVarIntValue(flag, 1);
    THISCALL_4(void, 0x40B0B0, adv, mapItem, mapX, mapY);
    return h3_TextBuffer;
}

_LHF_(AdventureMapHints::AdvMgr_BeforeObjectsDraw)
{

    instance->needDrawHints = false;
    m_mapView.right = H3GameWidth::Get() - 208;
    m_mapView.bottom = H3GameHeight::Get() - 56;
    // if map isn't forcelly hidden
    if (IntAt(0x699588) == 0)
    {
        const BOOL keyIsHeld = GetFocus() == H3Hwnd::Get() &&
                               (STDCALL_1(SHORT, PtrAt(0x63A294), instance->settings.vKey) & 0x8000) &&
                               instance->settings.isHeld;

        if (keyIsHeld)
        {
            instance->playerID = P_Game->Get()->GetPlayerID();
            libc::sprintf(Era::z[0], ERM_VARIABLE_FORMAT, instance->playerID); // ;
            instance->needDrawHints = instance->playerID >= 0 && instance->playerID < 8 &&
                                      Era::GetAssocVarIntValue(Era::z[0]);
        }
    }

    instance->drawnOjectIndexes.clear();
    instance->drawnHintRects.clear();

    return EXEC_DEFAULT;
}
void __stdcall AdventureMapHints::AdvMgr_TileObjectDraw(HiHook *h, H3AdventureManager *adv, int mapX, int mapY,
                                                        int mapZ, int screenX, int screenY)
{

    THISCALL_6(void, h->GetDefaultFunc(), adv, mapX, mapY, mapZ, screenX, screenY);

    if (instance->needDrawHints)
    {
        // check if tile may have object and visible by user
        const int mapSize = *P_MapSize;
        if (mapX >= 0 && mapY >= 0 && mapX < mapSize && mapY < mapSize &&
            H3TileVision::CanViewTile(mapX, mapY, mapZ, instance->playerID))
        {

            H3MapItem *currentItem = adv->GetMapItem(mapX, mapY, mapZ);
            if (!currentItem)
                return;

            // if not entrance point find if it visible by player and don't draw current item
            if (!currentItem->IsEntrance())
            {
                auto *entrance = currentItem->GetEntrance();
                if (!entrance || H3TileVision::CanViewTile(entrance->GetCoordinates(), instance->playerID))
                    return;
            }

            if (instance->NeedDrawMapItem(currentItem) &&
                instance->drawnOjectIndexes.insert(currentItem->drawnObjectIndex).second)
            {

                constexpr int TILE_WIDTH = 32;
                constexpr int TEXT_MARGIN = 2;
                const int objectWidthInTiles = GetObjectWidthInTiles(currentItem);
                H3LoadedPcx16 *tempBuffer = nullptr;
                {
                    H3String hintText;
                    hintText = instance->GetHintText(adv, currentItem, mapX, mapY, mapZ);

                    if (hintText.Empty())
                        return;
                    LPCSTR hintTextPtr = hintText.String();

                    constexpr int minTextFieldWidth = TILE_WIDTH;

                    auto fnt = P_TinyFont->Get();
                    if (!fnt)
                        return;
                    const int maxHintTextLineWidth = fnt->GetMaxLineWidth(hintTextPtr); // get max text width

                    const int maxAllowedTextWidth = TILE_WIDTH * (objectWidthInTiles + 1); // allow max width
                    int textWidth = Clamp(minTextFieldWidth, maxHintTextLineWidth, maxAllowedTextWidth);

                    if (maxHintTextLineWidth > textWidth) // if over the limit
                    {
                        H3Vector<H3String> stingList;
                        fnt->SplitTextIntoLines(hintTextPtr, textWidth, stingList); // rearrange text
                        hintText = h3_NullString;
                        for (auto &str : stingList)
                        {
                            hintText += str;
                            if (!str.Empty())
                                hintText.Append('\n');
                        }
                        if (!hintText.Empty())
                            hintText.SetLength(hintText.Length() - 1); // remove last symbol
                        hintTextPtr = hintText.String();           // reset ptr
                        textWidth = fnt->GetMaxLineWidth(hintTextPtr);
                    }

                    const int pcxWidth = textWidth + TEXT_MARGIN * 2;

                    const int hintTextLines = fnt->GetLinesCountInText(hintTextPtr, textWidth);
                    const int minTextFieldHeight = fnt->height + 2;
                    const int textHeight = hintTextLines * (minTextFieldHeight - 1);
                    const int pcxHeight = textHeight + TEXT_MARGIN;

                    tempBuffer = H3LoadedPcx16::Create(pcxWidth, pcxHeight);
                    if (!tempBuffer)
                        return;

                    libc::memset(tempBuffer->buffer, 0, tempBuffer->buffSize);

                    fnt->TextDraw(tempBuffer, hintTextPtr, TEXT_MARGIN, 0, textWidth, pcxHeight);
                }

                if (tempBuffer)
                {
                    const int pcxWidth = tempBuffer->width;
                    const int pcxHeight = tempBuffer->height;

                    tempBuffer->DrawFrame(0, 0, pcxWidth, pcxHeight, 189, 149, 57);
                    // resize tempBuffer to align text for screen borders

                    int objectWidth = 1 * TILE_WIDTH;
                    const int outOfWidthBorder = (pcxWidth - objectWidth) >> 1;

                    int destPcxX = screenX * TILE_WIDTH + adv->screenDrawOffset.x - outOfWidthBorder;

                    const int additionalYOffset = instance->settings.drawObjectHint[currentItem->objectType].yOffset;

                    int destPcxY = screenY * TILE_WIDTH + adv->screenDrawOffset.y - pcxHeight + additionalYOffset;
                    const RECT originalRect{destPcxX, destPcxY, destPcxX + pcxWidth + HINT_SHADOW_SIZE,
                                            destPcxY + pcxHeight + HINT_SHADOW_SIZE};
                    bool overlapsHintOnTheLeft = false;
                    for (const auto &previousHint : instance->drawnHintRects)
                    {
                        if (previousHint.mapX < mapX && RectanglesIntersect(originalRect, previousHint.rect))
                        {
                            overlapsHintOnTheLeft = true;
                            break;
                        }
                    }
                    if (overlapsHintOnTheLeft)
                        destPcxY += ((mapX + mapY) & 1) ? HINT_STAGGER_Y : -HINT_STAGGER_Y;

                    const int srcX = destPcxX < m_mapView.left ? m_mapView.left - destPcxX : 0;
                    const int srcY = destPcxY < m_mapView.top ? m_mapView.top - destPcxY : 0;
                    destPcxX += srcX;
                    destPcxY += srcY;
                    tempBuffer->width = (std::min<int>)(pcxWidth - srcX, m_mapView.right - destPcxX);
                    tempBuffer->height = (std::min<int>)(pcxHeight - srcY, m_mapView.bottom - destPcxY);

                    // if need to draw any hint
                    if (tempBuffer->height > 0 && tempBuffer->width > 0)
                    {
                        // get general Window draw buffer to draw temp pcx with x/y offsets
                        auto drawBuffer = P_WindowManager->GetDrawBuffer();
                        tempBuffer->DrawToPcx16(destPcxX, destPcxY, 1, drawBuffer, srcX, srcY);

                        int heightReserve = m_mapView.bottom - tempBuffer->height - destPcxY;
                        UINT shadowWidth = 0;

                        UINT shadowHeight = 0;

                        if (heightReserve > 0)
                            shadowHeight = heightReserve >= HINT_SHADOW_SIZE ? HINT_SHADOW_SIZE : heightReserve;

                        int widthReserve = m_mapView.right - tempBuffer->width - destPcxX;
                        if (widthReserve > 0)
                            shadowWidth = widthReserve >= HINT_SHADOW_SIZE ? HINT_SHADOW_SIZE : widthReserve;

                        if (shadowWidth)
                            drawBuffer->DrawShadow(destPcxX + tempBuffer->width, destPcxY, shadowWidth,
                                                   tempBuffer->height + shadowHeight);

                        if (shadowHeight)
                            drawBuffer->DrawShadow(destPcxX, destPcxY + tempBuffer->height,
                                                   tempBuffer->width + (shadowHeight ? 0 : shadowWidth), shadowHeight);

                        const RECT drawnRect{destPcxX, destPcxY,
                                             destPcxX + tempBuffer->width + static_cast<int>(shadowWidth),
                                             destPcxY + tempBuffer->height + static_cast<int>(shadowHeight)};
                        instance->drawnHintRects.push_back({drawnRect, mapX});
                    }

                    tempBuffer->Destroy();
                }
            }
        }
    }
}
// Movement hint uses the native pathfinder and leaves normal hints to SetHint.
bool CreateKeyAltHint(H3AdventureManager *advMgr, H3MapItem *cell)
{
    ////////////////////////////////////////// КЕЙСЫ, КОГДА НЕ ВЫДАЕМ ХИНТ
    //
    // Если мышь за пределами карты - ничего не делаем
    if (!advMgr || !cell)
        return false;
    H3Position mousePosition = advMgr->mousePosition;
    const int mapSize = H3MapSize::Get();
    const int mouseX = mousePosition.GetX();
    const int mouseY = mousePosition.GetY();
    if (mouseX >= mapSize || mouseX < 0 || mouseY >= mapSize || mouseY < 0)
    {
        return false;
    }

    // Если навели на клетку, до которой не добраться, или герой не выбран - ничего не делаем
    const int currentHero = P_Game->GetPlayer()->currentHero;
    if (currentHero < 0)
    {
        return false;
    }
    H3Hero *hero = P_Game->GetHero(currentHero);
    if (!hero)
        return false;

    // Если навели на героя - ничего не делаем
    if (H3Position(hero->x, hero->y, static_cast<INT8>(hero->z)) == mousePosition)
    {
        return false;
    }

    ////////////////////////////////////////// КЕЙСЫ, КОГДА ВЫДАЕМ ХИНТ
    //
    // Если уровни карты героя и наведения разные
    if (hero->z != mousePosition.GetZ())
    {
        libc::sprintf(h3_TextBuffer, EraJS::read(KEY_ALT_HINT_CANT_REACH), hero->name);
        return true;
    }

    // Let the game account for boats, flying, water walking and visitable objects.
    advMgr->MovementCalculationsMouse();
    H3PathNode *pathNode = P_Pathfinder->GetPathNode(mousePosition);
    if (!pathNode)
        return false;
    UINT32 flags = pathNode->access;
    if ((flags & 1) == 0)
    {
        libc::sprintf(h3_TextBuffer, EraJS::read(KEY_ALT_HINT_CANT_REACH), hero->name);
        return true;
    }
    const int movementCost = pathNode->movementCost;
    const int movement = hero->movement;
    if (movement >= movementCost && THISCALL_1(bool, 0x4BAA40, P_ActivePlayer->Get()))
    {
        const int movementLeft = movement - movementCost;
        libc::sprintf(h3_TextBuffer, EraJS::read(KEY_ALT_HINT_ENOUGH_POINTS), hero->name, movementCost, movementLeft);
    }
    else
    {
        libc::sprintf(h3_TextBuffer, EraJS::read(KEY_ALT_HINT_NOT_ENOUGH_POINTS), hero->name, movementCost);
    }

    return true;
}

int __stdcall AdventureMapHints::H3AdventureManager_ProcMapScreen(HiHook *h, H3AdventureManager *advMgr, H3Msg *msg)
{
    const bool editFocused = advMgr->dlg->screenlogEdit->IsFocused();
    const bool altKeyEvent = (msg->command == eMsgCommand::KEY_DOWN || msg->command == eMsgCommand::KEY_UP) &&
                             msg->GetKey() == eVKey::H3VK_ALT;
    const BOOL previousAltState = instance->altIsPressed;
    // Key transitions are authoritative; other messages may omit modifier flags.
    const BOOL keyIsHeld = GetFocus() == H3Hwnd::Get() &&
                           (STDCALL_1(SHORT, PtrAt(0x63A294), VK_MENU) & 0x8000);
    instance->altIsPressed = !editFocused &&
                             (altKeyEvent ? msg->command == eMsgCommand::KEY_DOWN : keyIsHeld != 0);
    if (!editFocused && (altKeyEvent || previousAltState != instance->altIsPressed))
        advMgr->UpdateHintMessage();

    return THISCALL_2(int, h->GetDefaultFunc(), advMgr, msg);
}

_LHF_(AdventureMapHints::H3AdventureManager_SetHeroObjectHint)
{
    H3Hero *hero = reinterpret_cast<H3Hero *>(c->esi);

    if (hero->owner == P_Game->GetPlayerID())
    {
        LPCSTR baseHint = reinterpret_cast<LPCSTR>(c->edi);
        bool readResult = false;
        LPCSTR addHint = EraJS::read(MOUSEOVER_HINT_MOVEPTS, readResult);
        if (!readResult)
        {
            addHint = " (%d / %d)";
        }
        H3String resultHintStr = baseHint;
        resultHintStr += addHint;

        LPCSTR heroClassName = reinterpret_cast<LPCSTR>(c->eax);
        libc::sprintf(h3_TextBuffer, resultHintStr.String(), hero->name, heroClassName, hero->movement,
                      hero->maxMovement);

        c->return_address = 0x40D09B;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

void __stdcall AdventureMapHints::H3AdventureManager_SetHint(HiHook *h, H3AdventureManager *advMgr, H3MapItem *cell,
                                                             int x, int y)
{

    if (advMgr->dlg->screenlogEdit->IsFocused() || instance->isCustomHintCreation || !instance->altIsPressed ||
        !CreateKeyAltHint(advMgr, cell))
    {
        THISCALL_4(void, h->GetDefaultFunc(), advMgr, cell, x, y);
    }
    else
    {

        H3AdventureMgrDlg *dlg = advMgr->dlg;
        H3DlgTextPcx *hintbar = dlg->hintbar;
        const int itemId = hintbar->GetID();
        dlg->SendCommandToItem(3, itemId, int(h3_TextBuffer));
        THISCALL_4(BOOL8, 0x5FF5E0, dlg, 0, itemId, itemId); // redraw items
        dlg->RedrawItem(itemId);
    }
}

bool AdventureMapHints::NeedDrawMapItem(const H3MapItem *mIt) const noexcept
{
    return mIt && mIt->objectType >= 0 &&
           mIt->objectType < sizeof(settings.drawObjectHint) / sizeof(settings.drawObjectHint[0]) &&
           settings.drawObjectHint[mIt->objectType].userValue;
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

    libc::memset(drawObjectHint, false, sizeof(drawObjectHint));

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
    for (UINT i = 0; i < sizeof(drawObjectHint) / sizeof(drawObjectHint[0]); ++i)
    {

        drawObjectHint[i].userValue = drawObjectHint[i].defaultValue;
        if (Era::ReadStrFromIni(Era::IntToStr(i).c_str(), sectionName, filePath, h3_TextBuffer))
        {
            const bool userValue = atoi(h3_TextBuffer);
            drawObjectHint[i].userValue = userValue;
        }
    }
    vKey = VK_MENU;
    if (Era::ReadStrFromIni("KeyCode", "ControlSettings", filePath, h3_TextBuffer))
    {
        const int key = atoi(h3_TextBuffer);
        if (key > 0 && key < 256)
            vKey = key;
    }
    return TRUE;
}

BOOL AdventureHintsSettings::save()
{
    Era::ClearIniCache(filePath);
    for (UINT i = 0; i < sizeof(drawObjectHint) / sizeof(drawObjectHint[0]); ++i)
    {
        Era::WriteStrToIni(Era::IntToStr(i).c_str(), Era::IntToStr(drawObjectHint[i].userValue).c_str(),
                           sectionName, filePath);
    }
    Era::WriteStrToIni("KeyCode", Era::IntToStr(vKey).c_str(), "ControlSettings", filePath);

    Era::SaveIni(filePath);

    return TRUE;
}
} // namespace advMapHints
