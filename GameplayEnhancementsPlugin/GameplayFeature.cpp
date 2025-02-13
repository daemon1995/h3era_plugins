#include "GameplayFeature.h"

namespace features
{
GameplayFeature::GameplayFeature() : IGamePatch(_PI)
{
    CreatePatches();
}
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

    const int result = FASTCALL_4(int, h->GetDefaultFunc(), heroId, hideDelButton, isKingdomOverView, isRightClick);

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

        m_isInited = true;
    }
}
GameplayFeature &GameplayFeature::Get()
{
    static GameplayFeature instance;
    return instance;
}

} // namespace features
