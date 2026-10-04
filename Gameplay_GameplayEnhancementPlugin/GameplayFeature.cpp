#include "GameplayFeature.h"
#include "ModuleSupport.h"
#include <memory>
namespace features
{
namespace
{
const char *demoBttn = "iDEMO.def";
Patch *demolishButtonPatch = nullptr;
// Each call through the WoG wrapper gets a scope, including attempts that return
// before the question. An inner attempt must not undo the outer attempt's patch.
std::vector<std::unique_ptr<gem::ScopedPatch>> demolishQuestionScopes;
_LHF_(WoG_StartTownbuildingDemolishQuestion)
{
    demolishQuestionScopes.emplace_back();
    return EXEC_DEFAULT;
}

_LHF_(WoG_BeforeTownbuildingDemolishQuestion)
{
    if (!demolishQuestionScopes.empty() && !demolishQuestionScopes.back())
        demolishQuestionScopes.back() = std::make_unique<gem::ScopedPatch>(demolishButtonPatch);
    return EXEC_DEFAULT;
}

_LHF_(WoG_AfterTownbuildingDemolishQuestion)
{
    if (!demolishQuestionScopes.empty())
        demolishQuestionScopes.pop_back();
    return EXEC_DEFAULT;
}

}

GameplayFeature::GameplayFeature() : IGamePatch("EraPlugin.GameplayTweaks.daemon_n")
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
        DefName && libc::strcmpi(DefName, reinterpret_cast<char *>(IntAt(0x05C5A08 + 1)))    // skip town dlg
        && libc::strcmpi(DefName, reinterpret_cast<char *>(IntAt(0x046BC7C + 1))) // skip combat dlg
    )
    {
        // add "SPACE_BAR" into hotkey list
        result->AddHotkey(eVKey::H3VK_SPACEBAR);
    }
    return result;
}

signed int __stdcall H3HeroDlg_Main(HiHook *h, const int heroId, int hideDelButton, int isKingdomOverView,
                                    const int isRightClick) noexcept
{
    const H3Hero *hero = P_Game->GetHero(heroId);

    if (!hero)
        return FASTCALL_4(int, h->GetDefaultFunc(), heroId, hideDelButton, isKingdomOverView, isRightClick);

    const int prevOwner = hero->owner;
    const INT8 curPlayer = P_CurrentPlayerID;
    if (hideDelButton && !isRightClick && P_ActivePlayer->ownerID == curPlayer && prevOwner == curPlayer)
    {

        hideDelButton = false;
        isKingdomOverView = true;
    }
    const int result = FASTCALL_4(int, h->GetDefaultFunc(), heroId, hideDelButton, isKingdomOverView, isRightClick);

    const int newOwner = hero->owner;
    auto *townMgr = P_TownMgr->Get();

    if (townMgr && prevOwner != newOwner)
    {
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
    }
    return result;
}

_LHF_(DlgEdit_CorrectInpulSymbol) noexcept
{
    // skip select scenario dialog
    const auto dlgAddr = reinterpret_cast<DWORD>(P_WindowManager->Get()->lastDlg);
    if (!dlgAddr || DwordAt(dlgAddr) == 0x0641CBC)
    {
        return EXEC_DEFAULT;
    }
    static UINT8 buttonsState[256];
    // get buttons state
    // check numlock state
    if (GetKeyboardState(buttonsState) && (buttonsState[VK_NUMLOCK] & 1))
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

                if (vkCode != VK_NPOS && (buttonsState[vkCode] & 0x80))
                {
                    c->eax = scanCode;
                    msg->subtype = eMsgSubtype(scanCode);
                }
            }
        }
    }

    return EXEC_DEFAULT;
}

H3Dlg *__stdcall ThievesGuildDlg_Ctor(HiHook *h, H3Dlg *dlg, const int tavernsNum)
{
    // open thieves guild dialog
    H3Dlg *result = THISCALL_2(H3Dlg *, h->GetDefaultFunc(), dlg, tavernsNum);
    if (result)
    {
        auto hintBarItem = result->GetText(41);
        if (!hintBarItem)
            return result;

        bool readSuccess = false;
        LPCSTR format = EraJS::read(GameplayFeature::THIEVES_GUILD_TEXT_FORMAT, readSuccess);
        if (!readSuccess || !format)
            return result;

        const int maxTavernsToShow = IntAt(0x05DDFF3 + 1); // max taverns in game

        const int tavernsNum =
            Clamp(0, THISCALL_2(int, 0x4CCAF0, P_Main->Get(), P_Main->GetPlayerID()), maxTavernsToShow);

        const H3String text = H3String::Format(format, tavernsNum, maxTavernsToShow);
        const int textX = hintBarItem->GetX();
        const int textWidth = hintBarItem->GetWidth();

        result->CreateText(textX, result->GetHeight() - 45, textWidth, 20, text.String(), NH3Dlg::Text::MEDIUM,
                           eTextColor::REGULAR, -1);
    }
    return result;
}

_LHF_(LoHook_HeroRoute_RouteUpdate)
{
    if (static_cast<INT>(c->esi) + c->Ebx<H3Hero *>()->maxMovement < 0)
    {
        WordAt(c->eax * 2 + c->edx) += 50;
        c->return_address = 0x4190DE;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
void GameplayFeature::CreatePatches() noexcept
{
    if (!m_isInited)
    {
        auto transient = globalPatcher->CreateInstance("EraPlugin.GameplayTweaks.Transient.daemon_n");
        demolishButtonPatch = transient->CreateDwordPatch(0x04F738A + 1, (int)demoBttn);
        // The register-based call at 0x70C19C returns to 0x70C1A1 on every native exit.
        _pi->WriteLoHook(0x070C19C, WoG_StartTownbuildingDemolishQuestion);
        _pi->WriteLoHook(0x070AD9A, WoG_BeforeTownbuildingDemolishQuestion);
        _pi->WriteLoHook(0x070C1A1, WoG_AfterTownbuildingDemolishQuestion);

        // Adding support for NumPad keys number input @Hawaiing
        _pi->WriteLoHook(0x05BB0B6, DlgEdit_CorrectInpulSymbol);

        // Adding "SPACE" as "OK" hotkey to close any dlg
        _pi->WriteHiHook(0x455BD0, THISCALL_, H3DlgDefButton__Ctor);

        // Call HeroDlg creation from TownDlg with del button enabled by default

        // Call HeroDlg creation from H3KigdomOverviewDlg with del button enabled by default

        // skip Hero placement in town when HeroDlg is updated
        _pi->WriteByte(0x4E1CDB, 0xEB);

        // skip next hero find after killing hero
        _pi->WriteByte(0x4DA23D, 0xEB);

        // Allow Town Hero Dismiss
        _pi->WriteHiHook(0x5D5323, FASTCALL_, H3HeroDlg_Main);
        _pi->WriteHiHook(0x5D5333, FASTCALL_, H3HeroDlg_Main);
        //   _pi->WriteHiHook(0x5D52CA, FASTCALL_, H3HeroDlg_Main);

        // add text into thieves guild dialog
        if (H3GameHeight::Get() > 607)
            _pi->WriteHiHook(0x05C8590, THISCALL_, ThievesGuildDlg_Ctor);

        _pi->WriteLoHook(0x4190D9, LoHook_HeroRoute_RouteUpdate);
        m_isInited = true;
        m_isEnabled = true;
    }
}
GameplayFeature &GameplayFeature::Get()
{
    if (!instance)
        instance = new GameplayFeature();
    return *instance;
}

} // namespace features
