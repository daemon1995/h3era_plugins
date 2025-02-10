#include "GameplayFeature.h"

namespace features
{

H3DlgDefButton *__stdcall H3DlgDefButton__Ctor(HiHook *h, H3DlgDefButton *bttn, int PosX, int PosY, int SizeX,
                                               int SizeY, int ItemInd, char *DefName, int cadre, int pressCadre,
                                               int CloseDialog, int HotKey, int Flags)
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

//void __stdcall H3DlgHero__Dismiss__BeforeRedraw(HiHook *h, int a1, int a2, int a3, int a4)
//{
//}

signed int __stdcall H3HeroDlg_Main(HiHook *h, const int heroId, int hideDelButton, int isKingdomOverView,
                                    const int isRightClick)
{
    H3Hero *hero = &P_Game->heroes[heroId];

    const int prevOwner = hero->owner;

    if (hideDelButton && !isRightClick && P_ActivePlayer->ownerID == P_CurrentPlayerID &&
        prevOwner == P_CurrentPlayerID)
    {

        hideDelButton = false;
        isKingdomOverView = true;
    }
    // auto *patch = _PI->WriteHiHook(0x4DA401, THISCALL_, H3DlgHero__Dismiss__BeforeRedraw);

    int result = FASTCALL_4(int, h->GetDefaultFunc(), heroId, hideDelButton, isKingdomOverView, isRightClick);

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

GameplayFeature::GameplayFeature() : IGamePatch(_PI)
{

    CreatePatches();
}
void GameplayFeature::CreatePatches() noexcept
{
    if (!m_isInited)
    {
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
