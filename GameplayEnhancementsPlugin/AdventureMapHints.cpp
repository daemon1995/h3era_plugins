#include "pch.h"

namespace advMapHints
{
RECT AdventureMapHints::m_mapView;

AdventureMapHints *AdventureMapHints::instance = nullptr;
void AdventureMapHints::Init(PatcherInstance *pi)
{
    if (!instance)
    {

        if (instance = new AdventureMapHints(pi))
        {
            instance->_pi = pi;
            instance->CreatePatches();
        }
    }
}

AdventureMapHints::AdventureMapHints(PatcherInstance *pi) : IGamePatch(pi)
{
    m_mapView.left = 8;                                   // right panel
    m_mapView.top = 8;                                    // bottom panel
    m_mapView.right = H3GameWidth::Get() - (800 - 592);   // right panel
    m_mapView.bottom = H3GameHeight::Get() - (600 - 544); // bottom panel
    settings = new AdventureHintsSettings("Runtime/gem_AdventureMapHints.ini", "StaticHintsDrawByType");
}

void AdventureMapHints::CreatePatches() noexcept
{

    if (!m_isInited)
    {

        blockAdventureHintDraw = _pi->CreateHexPatch(0x040D0F4, const_cast<char *>("EB 40 90"));

        _pi->WriteLoHook(0x040F5AB, AdvMgr_BeforeObjectDraw);
        _pi->WriteHiHook(0x40F5D7, THISCALL_, AdvMgr_ObjectDraw);

        m_isInited = true;
    }
}

char changedData[32] = {};

void GameManager_HidePlayersVisitedInfo(H3Main *game, const int playerID) noexcept
{

    const int withcHutData = game->visitedWitchHut.Status(playerID);
    const int shrineData = game->visitedShrines.Status(playerID);
    const int treeOfKnoledgeData = game->visitedTreeKnowledge.Status(playerID);

    libc::memcpy(changedData, reinterpret_cast<char *>(game) + 0x4E344, 32 * sizeof(H3PlayersBitfield));
    libc::memset(reinterpret_cast<char *>(game) + 0x4E344, 0, 32 * sizeof(H3PlayersBitfield));

    game->visitedWitchHut.Set(withcHutData, playerID);
    game->visitedShrines.Set(shrineData, playerID);
    game->visitedTreeKnowledge.Set(treeOfKnoledgeData, playerID);

    // return result;
}
LPCSTR AdventureMapHints::GetHintText(const H3AdventureManager *adv, const H3MapItem *mapItem, const int mapX,
                                      const int mapY, const int mapZ) noexcept
{

    constexpr UINT NOT_VISITED_TEXT_ID = 354;
    constexpr UINT VISITED_TEXT_ID = 353;

    auto visitedText = H3GeneralText::Get()->GetText(VISITED_TEXT_ID + 1);
    auto notVisitedText = H3GeneralText::Get()->GetText(NOT_VISITED_TEXT_ID + 1);

    H3String changedVisitedText = H3String::Format("{~Green}\n%s}", visitedText).String();
    H3String changedNotVisitedText = H3String::Format("{~Orange}\n%s}", notVisitedText).String();

    H3Vector<LPCSTR> *generalTextPtr = reinterpret_cast<H3Vector<LPCSTR> *>(ADDRESS(H3GeneralText::Get()) + 0x1C);

    const BOOL isCreature = mapItem->objectType == eObject::MONSTER;
    DWORD oldCreatureHintFormat = 0;
    if (isCreature)
    {
        oldCreatureHintFormat = DwordAt(0x40C2E7 + 1);

        LPCSTR advMapCreatureHintFormat = "{%s}\n%s";
        DwordAt(0x40C2E7 + 1) = reinterpret_cast<DWORD>(advMapCreatureHintFormat);
    }

    auto p_visited = generalTextPtr->At(VISITED_TEXT_ID);
    auto p_notVisited = generalTextPtr->At(NOT_VISITED_TEXT_ID);

    *p_visited = changedVisitedText.String();
    *p_notVisited = changedNotVisitedText.String();
    instance->blockAdventureHintDraw->Apply();

    Era::SetAssocVarIntValue("GameplayEnhancementsPlugin_AdventureMapHints_AtHint", 1);
    GameManager_HidePlayersVisitedInfo(P_Game->Get(), H3CurrentPlayerID::Get());
    THISCALL_4(void, 0x40B0B0, adv, mapItem, mapX, mapY);
    Era::SetAssocVarIntValue("GameplayEnhancementsPlugin_AdventureMapHints_AtHint", 0);
    libc::memcpy(reinterpret_cast<char *>(P_Game->Get()) + 0x4E344, changedData, 32 * sizeof(H3PlayersBitfield));

    instance->blockAdventureHintDraw->Undo();

    *p_visited = visitedText;
    *p_notVisited = notVisitedText;
    if (oldCreatureHintFormat)
    {
        DwordAt(0x40C2E7 + 1) = oldCreatureHintFormat;
    }

    return h3_TextBuffer;
}

_LHF_(AdventureMapHints::AdvMgr_BeforeObjectDraw)
{

    instance->needDrawHints = false;
    const BOOL keyIsHeld = GetFocus() == H3Hwnd::Get() &&
                           STDCALL_1(SHORT, PtrAt(0x63A294), instance->settings->vKey) & 0x800 &&
                           instance->settings->isHeld;

    if (keyIsHeld)
    {
        instance->playerID = P_Game->Get()->GetPlayerID();
        static char ermVariableNameBuffer[64];

        sprintf(ermVariableNameBuffer, "gem_adventure_map_object_hints_option_%d", instance->playerID); // ;
        instance->needDrawHints = Era::GetAssocVarIntValue(ermVariableNameBuffer);
    }

    instance->m_drawnOjects.clear();

    return EXEC_DEFAULT;
}
void __stdcall AdventureMapHints::AdvMgr_ObjectDraw(HiHook *h, H3AdventureManager *adv, int mapX, int mapY, int mapZ,
                                                    int screenX, int screenY)
{

    THISCALL_6(void, h->GetDefaultFunc(), adv, mapX, mapY, mapZ, screenX, screenY);

    if (instance->needDrawHints)
    {

        H3Position pos(mapX, mapY, mapZ);
        if (mapX >= 0 && mapY >= 0 && mapX < *P_MapSize && mapY < *P_MapSize &&
            H3TileVision::CanViewTile(pos, instance->playerID))
        {
            H3MapItem *currentItem = P_Game->GetMapItem(pos.Mixed());
            if (currentItem &&
                instance->m_drawnOjects.find(currentItem->drawnObjectIndex) == instance->m_drawnOjects.cend() &&
                instance->NeedDrawMapItem(currentItem))
            {

                instance->m_drawnOjects.insert(currentItem->drawnObjectIndex);

                LPCSTR hintText = GetHintText(adv, currentItem, mapX, mapY, mapZ);

                auto &attributes = P_Game->mainSetup.objectAttributes[currentItem->drawnObjectIndex];
                auto &passability = attributes.passability;

                // passability;
                // echo(passability.m_bits[0][0]);
                // echo(P_Game->mainSetup.objectDetails.Size());

                constexpr int TILE_WIDTH = 32;

                H3FontLoader fnt(NH3Dlg::Text::TINY);

                constexpr int OBJECT_WIDTH = 1 * TILE_WIDTH;

                constexpr int minTextFieldWidth = OBJECT_WIDTH;
                const int maxHintTextWidth = fnt->GetMaxLineWidth(hintText);
                constexpr int maxTextFieldWidth = TILE_WIDTH * 3;
                const int textWidth = Clamp(minTextFieldWidth, maxHintTextWidth, maxTextFieldWidth);

                const int hintTextLines = fnt->GetLinesCountInText(hintText, textWidth);
                const int minTextFieldHeight = fnt->height + 2;
                const int textHeight = hintTextLines * (minTextFieldHeight - 1);

                // textFieldWidth =
                // if (currentItem->objectType == eObject::SPELL_SCROLL)
                //{
                //    libc::sprintf(Era::z[1], "%s", hintText);

                //    // Era::y[1] = TEMP_PCX_WIDTH;
                //    Era::y[2] = textWidth;
                //    Era::y[3] = maxHintTextWidth;
                //    Era::y[4] = HINT_MAX_WORD_WIDTH;
                //    Era::ExecErmCmd("IF:L^text = %z1, textWidth = %y2, maxHintTextWidth = %y3, HINT_MAX_WORD_WIDTH
                //    "
                //                    "width = %y4^");
                //}

                // constexpr INT START_DRAW_X = 8;

                constexpr int TEXT_MARGIN = 2;

                const int TEMP_PCX_WIDTH = textWidth + 10;
                const int TEMP_PCX_HEIGHT = textHeight + TEXT_MARGIN;

                auto tempBuffer = H3LoadedPcx16::Create(TEMP_PCX_WIDTH, TEMP_PCX_HEIGHT);

                memset(tempBuffer->buffer, 0, tempBuffer->buffSize);

                //  create golden frame
                tempBuffer->DrawFrame(0, 0, TEMP_PCX_WIDTH, TEMP_PCX_HEIGHT, 189, 149, 57);
                // draw text to temp buffer
                fnt->TextDraw(tempBuffer, hintText, TEXT_MARGIN, 0, TEMP_PCX_WIDTH - TEXT_MARGIN, TEMP_PCX_HEIGHT);

                // resize tempBuffer to align text for screen borders

                int objectWidth = 1 * TILE_WIDTH;
                const int outOfWidthBorder = (TEMP_PCX_WIDTH - objectWidth) >> 1;

                int destPcxX = screenX * TILE_WIDTH + adv->screenDrawOffset.x - outOfWidthBorder;
                int destPcxY = screenY * TILE_WIDTH + adv->screenDrawOffset.y - TEMP_PCX_HEIGHT;

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
void __stdcall AdventureMapHints::AdvMgr_DrawCornerFrames(HiHook *h, const H3AdventureManager *adv)
{
    THISCALL_1(void, h->GetDefaultFunc(), adv);
    if (instance->m_drawnOjects.size())
    {
        instance->m_drawnOjects.clear();
    }
}

bool AdventureMapHints::NeedDrawMapItem(const H3MapItem *mIt) const noexcept
{
    if (mIt)
        return settings->drawObjectHint[mIt->objectType].userValue;
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

    drawObjectHint[eObject::CORPSE].defaultValue = true;
    drawObjectHint[eObject::MARLETTO_TOWER].defaultValue = true;
    drawObjectHint[eObject::DERELICT_SHIP].defaultValue = true;
    drawObjectHint[eObject::DRAGON_UTOPIA].defaultValue = true;

    drawObjectHint[eObject::FLOTSAM].defaultValue = true;

    drawObjectHint[eObject::GARDEN_OF_REVELATION].defaultValue = true;

    // drawObjectHint[eObject::HERO].defaultValue = true;

    drawObjectHint[eObject::LIBRARY_OF_ENLIGHTENMENT].defaultValue = true;

    drawObjectHint[eObject::SCHOOL_OF_MAGIC].defaultValue = true;
    drawObjectHint[eObject::MAGIC_SPRING].defaultValue = true;

    drawObjectHint[eObject::MONSTER].defaultValue = true;
    drawObjectHint[eObject::MYSTICAL_GARDEN].defaultValue = true;

    drawObjectHint[eObject::STAR_AXIS].defaultValue = true;

    drawObjectHint[eObject::PYRAMID].defaultValue = true;

    drawObjectHint[eObject::RESOURCE].defaultValue = true;

    drawObjectHint[eObject::SEA_CHEST].defaultValue = true;
    drawObjectHint[eObject::SEER_HUT].defaultValue = true;
    drawObjectHint[eObject::CRYPT].defaultValue = true;
    drawObjectHint[eObject::SHIPWRECK].defaultValue = true;
    drawObjectHint[eObject::SHIPWRECK_SURVIVOR].defaultValue = true;

    drawObjectHint[eObject::SHRINE_OF_MAGIC_INCANTATION].defaultValue = true;
    drawObjectHint[eObject::SHRINE_OF_MAGIC_GESTURE].defaultValue = true;
    drawObjectHint[eObject::SHRINE_OF_MAGIC_THOUGHT].defaultValue = true;

    drawObjectHint[eObject::SPELL_SCROLL].defaultValue = true;
    drawObjectHint[eObject::STABLES].defaultValue = true;

    drawObjectHint[eObject::LEARNING_STONE].defaultValue = true;
    drawObjectHint[eObject::TREASURE_CHEST].defaultValue = true;
    drawObjectHint[eObject::TREE_OF_KNOWLEDGE].defaultValue = true;

    drawObjectHint[eObject::SCHOOL_OF_WAR].defaultValue = true;

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
    DeleteFileA(filePath);
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
    Era::ClearIniCache(filePath);

    return 0;
}

LPCSTR *AdventureMapHints::AccessableH3GeneralText::GetStringAdddres(const int row)
{

    return &text[row - 1];
}
} // namespace advMapHints
