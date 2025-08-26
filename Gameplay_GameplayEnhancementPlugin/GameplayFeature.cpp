#include "GameplayFeature.h"

namespace features
{
GameplayFeature::GameplayFeature() : IGamePatch(_PI)
{
    CreatePatches();
}
GameplayFeature *GameplayFeature::instance = nullptr;
H3DlgDefButton *__stdcall H3DlgDefButton__Ctor(HiHook *h, H3DlgDefButton *bttn, int PosX, int PosY, int SizeX,
                                               int SizeY, int ItemInd, char *DefName, int cadre, int pressCadre,
                                               int CloseDialog, int HotKey, int Flags) noexcept
{

    H3DlgDefButton *result = THISCALL_12(H3DlgDefButton *, h->GetDefaultFunc(), bttn, PosX, PosY, SizeX, SizeY, ItemInd,
                                         DefName, cadre, pressCadre, CloseDialog, HotKey, Flags);

    // check base conditions as std "OK_BTTN_ID" and not H3TownDlg's defName ("tsbtns.def")
    if (result && (ItemInd == eControlId::OK || ItemInd == 30722 || HotKey == eVKey::H3VK_ENTER) &&
        DefName != reinterpret_cast<char *>(IntAt(0x05C5A08 + 1))    // skip town dlg
        && DefName != reinterpret_cast<char *>(IntAt(0x046BC7C + 1)) // skip combat dlg
    )
    {
        // add "SPCAE_BAR" into hotkey list
        result->AddHotkey(eVKey::H3VK_SPACEBAR);
    }
    return result;
}

// void __stdcall H3DlgHero__Dismiss__BeforeRedraw(HiHook *h, int a1, int a2, int a3, int a4)
//{
// }

signed int __stdcall H3HeroDlg_Main(HiHook *h, const int heroId, int hideDelButton, int isKingdomOverView,
                                    const int isRightClick) noexcept
{
    const H3Hero *hero = &P_Game->heroes[heroId];

    const int prevOwner = hero->owner;

    if (hideDelButton && !isRightClick && P_ActivePlayer->ownerID == P_CurrentPlayerID &&
        prevOwner == P_CurrentPlayerID)
    {

        hideDelButton = false;
        isKingdomOverView = true;
    }
    // auto *patch = _PI->WriteHiHook(0x4DA401, THISCALL_, H3DlgHero__Dismiss__BeforeRedraw);
    //  ByteAt(0x04E1C30 + 1) = 0;
    const int result = FASTCALL_4(int, h->GetDefaultFunc(), heroId, hideDelButton, isKingdomOverView, isRightClick);
    // ByteAt(0x04E1C30 + 1) = 1;

    //  patch->Destroy();

    const int newOwner = hero->owner;
    if (P_TownMgr && prevOwner != newOwner)
    {
        auto *townMgr = P_TownMgr->Get();
        if (townMgr->top)
        {
            h3::H3Free(townMgr->top);
            townMgr->top = nullptr;
        }
        if (townMgr->bottom)
        {
            h3::H3Free(townMgr->bottom);
            townMgr->bottom = nullptr;
        }

        // recreate garrison bars
        THISCALL_1(void, 0x05C7210, townMgr);
        // P_TownMgr->Draw();
    }
    return result;
}

_LHF_(DlgEdit_CorrectInpulSymbol) noexcept
{
    // skip select scenario dialog
    const auto dlgAddr = reinterpret_cast<DWORD>(P_WindowManager->Get()->lastDlg);
    if (DwordAt(dlgAddr) == 0x0641CBC)
    {
        return EXEC_DEFAULT;
    }
    static UINT8 buttonsState[256];
    // get buttons state
    // check numlock state
    if (GetKeyboardState(buttonsState) && buttonsState[VK_NUMLOCK])
    {

        if (H3Msg *msg = reinterpret_cast<H3Msg *>(c->esi))
        {

            UINT scanCode = msg->GetKey();

            // if button is within range and is pressed
            if (scanCode >= eVKey::H3VK_HOME && scanCode <= eVKey::H3VK_DELETE)
            {
                constexpr UINT8 VK_NPOS = 255u;
                // assume out of range is pressed
                UINT8 vkCode = VK_NPOS;
                // check heroes scan code and convert it to VK
                switch (scanCode)
                {
                case eVKey::H3VK_HOME:
                    vkCode = VK_NUMPAD7;
                    scanCode = eVKey::H3VK_7;

                    break;
                case eVKey::H3VK_UP:
                    vkCode = VK_NUMPAD8;
                    scanCode = eVKey::H3VK_8;
                    break;
                case eVKey::H3VK_PAGE_UP:
                    vkCode = VK_NUMPAD9;
                    scanCode = eVKey::H3VK_9;

                    break;
                case eVKey::H3VK_LEFT:
                    vkCode = VK_NUMPAD4;
                    scanCode = eVKey::H3VK_4;

                    break;
                case eVKey::H3VK_NUMPAD5:
                    vkCode = VK_NUMPAD5;
                    scanCode = eVKey::H3VK_5;

                    break;
                case eVKey::H3VK_RIGHT:
                    vkCode = VK_NUMPAD6;
                    scanCode = eVKey::H3VK_6;

                    break;
                case eVKey::H3VK_END:
                    vkCode = VK_NUMPAD1;
                    scanCode = eVKey::H3VK_1;

                    break;
                case eVKey::H3VK_DOWN:
                    vkCode = VK_NUMPAD2;
                    scanCode = eVKey::H3VK_2;

                    break;
                case eVKey::H3VK_PAGE_DOWN:
                    vkCode = VK_NUMPAD3;
                    scanCode = eVKey::H3VK_3;
                    break;
                case eVKey::H3VK_INSERT:
                    vkCode = VK_NUMPAD0;
                    scanCode = eVKey::H3VK_0;
                    break;
                case eVKey::H3VK_DELETE:
                    vkCode = VK_DECIMAL;
                    scanCode = eVKey::H3VK_PERIOD;
                    break;
                default:
                    break;
                }

                if (vkCode != VK_NPOS && buttonsState[vkCode])
                {
                    c->eax = scanCode;
                    msg->subtype = eMsgSubtype(scanCode);
                }
            }
        }
    }

    return EXEC_DEFAULT;
}

H3Dlg *__stdcall H3TownDlg_OpenThievesGuild(HiHook *h, H3TownDlg *dlg, int tavernsNum)
{
    // open thieves guild dialog
    H3Dlg *result = THISCALL_2(H3Dlg *, h->GetDefaultFunc(), dlg, tavernsNum);
    if (result)
    {
        auto mes = "displayed info %d / %d "; // H3String::Format("gem_plugin.town_thieves_guild.%d",
                                              // tavernsNum).String();

        const int maxTavernsToShow = IntAt(0x05DDFF3 + 1); // max taverns in game

        const int tavernsNum =
            Clamp(0, THISCALL_2(int, 0x4CCAF0, P_Main->Get(), P_Main->GetPlayerID()), maxTavernsToShow);

        libc::sprintf(h3_TextBuffer, mes, tavernsNum, maxTavernsToShow);
        result->CreateText(12, result->GetHeight() - 45, result->GetWidth(), 20, h3_TextBuffer, NH3Dlg::Text::MEDIUM,
                           eTextColor::REGULAR, -1);
    }
    return result;
}

int GameplayFeature::HeroFullMP_Rem = 0;

// Инициализация оставшихся полных очков перемещения героя.
_LHF_(LoHook_HeroRoute_InitMaxMP)
{
    H3Hero *hero = reinterpret_cast<H3Hero *>(c->ebx);
    H3Player *player = &P_Game->players[hero->owner];

    // Получаем полные очки перемещения героя.
    GameplayFeature::HeroFullMP_Rem = hero->maxMovement;

    bool v4 = THISCALL_1(bool, 0x4BAA40, P_ActivePlayer->Get());

    int curDayOfWeek = P_Game->date.day;

    char is_human2 = player->is_human;
    char v7 = P_ActivePlayer->Get()->is_human;

    if (v4 || (is_human2 && v7 && v7 > is_human2) || v7 == is_human2 && P_CurrentPlayerID > hero->owner)
    {
        if ((hero->flags & 0x1000000) == 0) // cheats
        {
            // Лодка и конюшни
            if ((hero->flags & 0x40000) == 0 && (hero->flags & 2) != 0 && curDayOfWeek >= 7)
            {
                GameplayFeature::HeroFullMP_Rem -= IntAt(0x0698AE4); // o_MoveTXT_Obj_94
            }
        }
    }
    if (!v4)
    {
        IntAt(c->ebp - 0x4) = 0;
    }

    return EXEC_DEFAULT;
}
// Уменьшение оставшихся полных очков перемещения героя.
_LHF_(LoHook_HeroRoute_ReduceMaxMP)
{
    // Герой.
    H3Hero *hero = reinterpret_cast<H3Hero *>(c->ebx);

    // Уменьшаем полные очки перемещения героя на шаг.
    GameplayFeature::HeroFullMP_Rem -=
        FASTCALL_4(int, 0x4B1620, hero, c->esi, DwordAt(c->ebp - 0x1C), GameplayFeature::HeroFullMP_Rem);

    return EXEC_DEFAULT;
}

// Показ особых стрелок, если путь дальше, чем максимальные очки перемещения героя.
_LHF_(LoHook_HeroRoute_SpecRouteMaxMP)
{
    // Смещение кадра стрелок.
    if (GameplayFeature::HeroFullMP_Rem < 0)
    {
        WordAt(c->edx + 2 * c->eax) += 25;
    }

    return EXEC_DEFAULT;
}
void GameplayFeature::CreatePatches() noexcept
{
    if (!m_isInited)
    {
        // Adding support for NumPad keys number input @Hawaiing
        _pi->WriteLoHook(0x05BB0B6, DlgEdit_CorrectInpulSymbol);

        // Adding "SPACE" as "OK" hotkey to close any dlg
        _pi->WriteHiHook(0x455BD0, THISCALL_, H3DlgDefButton__Ctor);

        // Call HeroDlg creation from TownDlg with del button enabled by default

        // Call HeroDlg creation from H3KigdomOverviewDlg with del button enabled by default
        //  _PI->WriteWord(0x51FA26, 0xD231);
        // _PI->WriteDword(0x51F46D + 1, 0);

        // skip Hero placement in town when HeroDlg is updated
        _pi->WriteByte(0x4E1CDB, 0xEB);

        // skip next hero find after killing hero
        _pi->WriteByte(0x4DA23D, 0xEB);

        // Allow Town Hero Dismiss
        //_PI->WriteByte(0x4E1C3A, 0xEB);
        _pi->WriteHiHook(0x5D5323, FASTCALL_, H3HeroDlg_Main);
        _pi->WriteHiHook(0x5D5333, FASTCALL_, H3HeroDlg_Main);
        //   _pi->WriteHiHook(0x5D52CA, FASTCALL_, H3HeroDlg_Main);

        // add text into thieves guild dialog
        _pi->WriteHiHook(0x05D7FFF, THISCALL_, H3TownDlg_OpenThievesGuild);

        // Инициализация оставшихся полных очков перемещения героя.
        _PI->WriteLoHook(0x418F38, LoHook_HeroRoute_InitMaxMP); // 100F14C0

        // Уменьшение оставшихся полных очков перемещения героя.
        _PI->WriteLoHook(0x418FC5, LoHook_HeroRoute_ReduceMaxMP); // 1014BB40

        // Показ особых стрелок, если путь дальше, чем максимальные очки перемещения героя.
        _PI->WriteLoHook(0x4190DE, LoHook_HeroRoute_SpecRouteMaxMP);
        m_isInited = true;
    }
}
GameplayFeature &GameplayFeature::Get()
{
    if (!instance)
        instance = new GameplayFeature();
    return *instance;
}

} // namespace features
