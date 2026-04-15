#include "MithrilDisplay.h"

namespace ERI
{
ExtendedResourcesInfo *ExtendedResourcesInfo::instance = nullptr;

inline bool IsMouseOverItem(const int x, const int y, const H3DlgItem *item) noexcept
{
    return item->IsActive() && x >= item->GetAbsoluteX() && x < item->GetAbsoluteX() + item->GetWidth() &&
           y >= item->GetAbsoluteY() && y < item->GetAbsoluteY() + item->GetHeight();
}

inline int GetPlayerResources(const int resourceId, const int playerId = P_Game->GetPlayerID())
{
    return resourceId == eResource::MITHRIL
               ? IntAt(0x27F9A00 + playerId * 4)
               : P_Game->players[playerId].playerResources[resourceId]; // mb later it should be replaced
}

void ExtendedResourcesInfo::SetLastHintItemId(const int itemId) noexcept
{
    m_resbarLastHintItemId = itemId;
}

ExtendedResourcesInfo::ExtendedResourcesInfo(PatcherInstance *_PI) : IGamePatch(_PI)
{

    CreatePatches();
}

ExtendedResourcesInfo::~ExtendedResourcesInfo()
{
    instance = nullptr;
}

_LHF_(BuildResBar)
{
    // H3Messagebox();

    return EXEC_DEFAULT;
}
/// set mouse hover hints for the resources
char __stdcall H3AdventureMgrDlg_Interface_MoveHint(HiHook *h, H3AdventureMgrDlg *dlg, const int x, const int y)
{

    // check if not focused on chat log
    if (!dlg->screenlogEdit->IsFocused())
    {
        const int lastHintItemId = IntAt(0x65F228);
        // const int mithrilItemId = info.MitrilItemId();

        constexpr int resourceItemIds[] = {1001, 1002, 1003, 1004, 1005, 1006, 1007, MITHRIL_DLG_TEXT_ITEM_ID,
                                           1009, 1010, 1011, 1012, 1013, 1014, 1015, MITHRIL_DLG_DEF_ITEM_ID};

        int clickedItemId = -1;
        int resType = -1;

        for (size_t i = 0; i < 16; i++)
        {
            const int itemId = resourceItemIds[i];
            auto it = dlg->GetH3DlgItem(itemId);

            if (it && IsMouseOverItem(x, y, it))
            {
                clickedItemId = itemId;
                resType = i & 7;
                break;
            }
        }

        if (resType != -1)
        {
            const unsigned int resAmount = GetPlayerResources(resType);

            char buffer[64];
            if (resAmount < 10000)
                libc::sprintf(buffer, "%d", resAmount);
            else
                Era::DecorateInt(resAmount, buffer, true);
            libc::sprintf(h3_TextBuffer, RESOURCE_HINT_FORMAT, P_ResourceName[resType], buffer);
            // set adv manager hint text
            THISCALL_2(void, 0x40B040, P_AdventureManager->Get(), h3_TextBuffer);

            if (lastHintItemId != -1 && clickedItemId == -1)
            {
                THISCALL_2(void, 0x40B040, P_AdventureManager->Get(), h3_NullString);
            }
            IntAt(0x65F228) = clickedItemId;
            return 1;
        }
    }

    return THISCALL_3(char, h->GetDefaultFunc(), dlg, x, y);
}

int __stdcall ExtendedResourcesInfo::KingdomOverviewDlgProc(HiHook *h, H3BaseDlg *dlg, H3Msg *msg)
{

    if (auto mithrilText = GetMitrilBarHintZone(CREATE_KINGDOM_OVERVIEW))
    {
        constexpr int HINT_BAR_ID = 37;
        if (msg->subtype == eMsgSubtype::RBUTTON_DOWN)
        {
            ShowMithrilRMCHint(msg, mithrilText);
        }
        else if (msg->command == eMsgCommand::MOUSE_OVER)
        {
            const H3DlgItem *hintZone = mithrilText;
            const int x = msg->GetX();
            const int y = msg->GetY();
            const bool result = IsMouseOverItem(x, y, hintZone);

            if (result)
            {
                if (auto it = dlg->GetText(HINT_BAR_ID))
                {
                    it->SetText(P_ResourceName[eResource::MITHRIL]);
                    it->Draw();
                    it->Refresh();
                }
            }
            else if (x >= hintZone->GetAbsoluteX() + hintZone->GetWidth())
            {

                if (auto it = dlg->GetText(HINT_BAR_ID))
                {
                    it->SetText(h3_NullString);
                    it->Draw();
                    it->Refresh();
                }
            }
        }
    }

    // THISCALL_2(int, h->GetDefaultFunc(), dlg, msg);
    return THISCALL_2(int, h->GetDefaultFunc(), dlg, msg);
}

BOOL ExtendedResourcesInfo::ShowMithrilRMCHint(const H3Msg *msg, H3DlgItem *hintZone) noexcept
{
    const int x = msg->GetX();
    const int y = msg->GetY();
    bool result = IsMouseOverItem(x, y, hintZone); // msg->ItemAtPosition(msg->GetDlg()) == hintZone;
    if (result)
    {
        int width = 0;
        LPCSTR hintText = EraJS::read("gem_plugin.mithril_display.popup_hint");

        FASTCALL_3(void, 0x4F6930, hintText, &width, &msg);
        FASTCALL_12(void, 0x4F6C00, hintText, 4, 592 - width, (0x258 - (int)msg) / 2 - 10, -1, 0, -1, 0, -1, 0, -1, 0);
    }
    else if (msg->itemId == 1008 && msg->GetX() < hintZone->GetAbsoluteX())
        result = true; // prevent extra hint for the date hint

    return result;
}

H3DlgItem *ExtendedResourcesInfo::GetMitrilBarHintZone(DWORD patchAddress) noexcept
{
    if (!patchAddress)
        return nullptr;
    auto &map = instance->resourceBarPatchInfos;
    auto it = map.find(patchAddress);
    if (it != map.end())
    {
        return it->second.mithrilDefItem;
    }

    return nullptr;
}

_LHF_(ExtendedResourcesInfo::OnAdvMgrDlgRightClick)
{

    auto hintZone = GetMitrilBarHintZone(CREATE_ADV_MAN);
    if (!hintZone)
        return EXEC_DEFAULT;

    if (ShowMithrilRMCHint(c->Edi<H3Msg *>(), hintZone))
    {
        c->ebx = true;
        c->return_address = 0x408974;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

ExtendedResourcesInfo &ExtendedResourcesInfo::Get()
{
    if (!instance)
        instance = new ExtendedResourcesInfo(globalPatcher->CreateInstance("EraPlugin.ResourceBar.daemon_n"));
    return *instance;
}

const int ExtendedResourcesInfo::LastHintItemId() const noexcept
{
    return m_resbarLastHintItemId;
}

BOOL ExtendedResourcesInfo::AlignOriginalResources(HookContext *c, const ResourceBarInfo &barInfo)
{
    H3ResourceBarPanel *panel = c->Eax<H3ResourceBarPanel *>();
    if (!panel)
        return FALSE;

    const int customItemWidth = barInfo.customTextItemWidth;
    int itemX = barInfo.firstResX;
    const int step = barInfo.itemStep;
    for (size_t i = 0; i < h3::limits::RESOURCES; i++)
    {
        auto *text = panel->resourceText[i]->Cast<H3DlgText>();
        //  text->SetAlignment(eTextAlignment::TOP_MIDDLE);

        auto *overLay = panel->resourceOverlay[i];
        overLay->SetX(text->GetX() - overLay->GetWidth());
        if (!customItemWidth)
            continue;
        const int widthDiff = text->GetX() - overLay->GetX();

        const int newTextWidth = (i == eResource::GOLD ? customItemWidth + 8 : customItemWidth) - widthDiff;
        //  overLay->SetWidth(customItemWidth - newTextWidth);
        overLay->SetX(itemX);

        text->SetWidth(newTextWidth);
        text->SetX(itemX + widthDiff);

        itemX += step;
    }
    return TRUE;
}
H3DlgItem *ExtendedResourcesInfo::BuildMithril(HookContext *c, ResourceBarInfo &barInfo)
{

    H3ResourceBarPanel *panel = reinterpret_cast<H3ResourceBarPanel *>(c->eax);
    if (!panel || !barInfo.createMithril)
        return nullptr;

    auto *goldTextItem = panel->resourceText[eResource::GOLD];
    const int spotWidth = goldTextItem->GetWidth();
    const int span = goldTextItem->GetX() - panel->resourceText[5]->GetX() - spotWidth;

    const BOOL buildFrame = barInfo.frameState == FRAME_STATE_BUILD;
    const int xOffset = barInfo.customTextItemWidth && !buildFrame ? -7 : 0; // idk why

    const int xPos = goldTextItem->GetX() + spotWidth + span + 12 - panel->resourceText[0]->GetX() + xOffset;

    const int defOverlayWidth = panel->resourceOverlay[0]->GetWidth();

    int textWidth = panel->resourceText[eResource::WOOD]->GetWidth();
    if (buildFrame)
    {
        auto resBarPcx = panel->resbarPCX->GetPcx();
        const int mapWidth = IntAt(0x4196EA + 1);

        if (textWidth > mapWidth)
            textWidth = mapWidth;

        const int pcxWidth = textWidth + defOverlayWidth + 2;
        const int pcxHeight = resBarPcx->height;

        auto &storedPcx = barInfo.mithrilBackPcxCache;
        if (!storedPcx)
        {
            storedPcx = H3LoadedPcx::Create(h3_NullString, pcxWidth, pcxHeight);
            resBarPcx->DrawToPcx(4, 0, pcxWidth, pcxHeight, storedPcx);
            resBarPcx->DrawToPcx(28, 0, 20, pcxHeight, storedPcx, 2);
        }

        H3DlgPcx *mithrilBack = H3DlgPcx::Create(xPos, 0, pcxWidth, pcxHeight, MITHRIL_DLG_BACK_PCX_ITEM_ID, nullptr);
        mithrilBack->SetPcx(storedPcx);
        panel->AddItem(mithrilBack);
        mithrilBack->Show();
    }

    H3DlgText *mithrilText = H3DlgText::Create(xPos + defOverlayWidth, 3, textWidth, 18, 0, NH3Dlg::Text::SMALL,
                                               eTextColor::WHITE, MITHRIL_DLG_TEXT_ITEM_ID, eTextAlignment::TOP_LEFT);
    panel->AddItem(mithrilText);
    mithrilText->ShowActivate();
    barInfo.mithrilTextItem = mithrilText;

    H3DlgItem *mithrilDef = H3DlgDef::Create(xPos + buildFrame * 2, 4, MITHRIL_DLG_DEF_ITEM_ID, MITHRIL_DEF_NAME);
    // to set as hint zone to cover both the icon and the text
    mithrilDef->SetWidth(defOverlayWidth + textWidth);

    panel->AddItem(mithrilDef);
    mithrilDef->ShowActivate();

    barInfo.mithrilDefItem = mithrilDef;

    return mithrilText;
}

void __stdcall ExtendedResourcesInfo::H3ResourceBarPanel__HideResources(HiHook *h, H3ResourceBarPanel *resourceBarPanel)
{

    H3BaseDlg *dlg = P_AdventureMgr->dlg;

    if (auto *mithril = dlg->GetText(MITHRIL_DLG_TEXT_ITEM_ID))
    {
        mithril->SetText(h3_NullString); // set "noText" for MP msgBox
    }
    if (H3DlgPcx *mithrilBack = dlg->GetPcx(MITHRIL_DLG_BACK_PCX_ITEM_ID))
    {
        mithrilBack->ColorToPlayer(IntAt(0x6977DC));
    }

    return THISCALL_1(void, h->GetDefaultFunc(), resourceBarPanel);
}
static LPCSTR GetPlayerDisplayedResourceText(const int mePlayerId, const int resourceId)
{

    const unsigned int resAmount = GetPlayerResources(resourceId, mePlayerId);
    char buffer[32];
    if (resAmount < 10000)
    {
        libc::sprintf(buffer, "%d", resAmount);
    }
    else if (resAmount < 1000000)
    {
        Era::DecorateInt(resAmount, buffer, false);
    }
    else
    {
        Era::FormatQuantity(resAmount, buffer, 32, 7, 4);
    }
    libc::strncpy(h3_TextBuffer, buffer, 31);

    return h3_TextBuffer;
}

_LHF_(ExtendedResourcesInfo::H3ResourceBarPanel__Refresh)
{
    const int mePlayerId = IntAt(0x6977DC);
    auto resourceBarPanel = reinterpret_cast<H3ResourceBarPanel *>(c->edi);

    // at first we refresh created mithril item

    if (auto dlg = resourceBarPanel->GetParent())
    {
        if (H3DlgPcx *mithrilBack = dlg->GetPcx(MITHRIL_DLG_BACK_PCX_ITEM_ID))
        {
            mithrilBack->ColorToPlayer(mePlayerId);
        }
        if (H3DlgText *mithrilTextItem = dlg->GetText(MITHRIL_DLG_TEXT_ITEM_ID))
        {
            mithrilTextItem->SetText(GetPlayerDisplayedResourceText(mePlayerId, eResource::MITHRIL));
        }
    }

    for (size_t i = 0; i < h3::limits::RESOURCES; i++)
    {
        auto textItem = resourceBarPanel->resourceText[i];
        textItem->SetText(GetPlayerDisplayedResourceText(mePlayerId, i));
    }

    c->return_address = 0x05591DB;
    return NO_EXEC_DEFAULT;
}

_LHF_(ExtendedResourcesInfo::AfterDlgResBarCreate)
{
    auto it = instance->resourceBarPatchInfos.find(h->GetAddress());
    if (it == instance->resourceBarPatchInfos.end())
        return EXEC_DEFAULT;

    auto &resourceBarInfo = it->second;
    AlignOriginalResources(c, resourceBarInfo);

    if (resourceBarInfo.createMithril)
    {
        BuildMithril(c, resourceBarInfo);
    }
    return EXEC_DEFAULT;
}

void ExtendedResourcesInfo::CreatePatches()
{

    const BOOL hdMod = GetModuleHandleA("hd_wog.dll") != nullptr;
    if (!hdMod)
        return;

    DWORD createResBarAddresses[] = {CREATE_ADV_MAN,   CREATE_KINGDOM_OVERVIEW, CREATE_PUZZLE_MAP, CREATE_TOWN_MGR,
                                     CREATE_TOWN_HALL, CREATE_MAGE_GUILD,       CREATE_TOWN_FORT,  CREATE_THIEVE_GUILD};

    constexpr size_t RES_BARS_AMOUNT = std::size(createResBarAddresses);
    const int gameWidth = H3GameWidth::Get();
    const int gameHeight = H3GameHeight::Get();

    const BOOL enoughSpaceForAdvResBar = gameWidth >= 860;
    const BOOL enoughSpaceForCustomResBar = hdMod && gameWidth >= 808 && gameHeight >= 608;

    const ResourceBarInfo resourceBarInfos[] = {
        ResourceBarInfo{enoughSpaceForAdvResBar, 8, 0, 0, FRAME_STATE_BUILD}, // advMap
        ResourceBarInfo{hdMod, 16},                                           // Kingdom Overview
        ResourceBarInfo{enoughSpaceForCustomResBar, 6, 84, 92},               //
        //  ResourceBarInfo{enoughSpaceForCustomResBar, 20, 70, 92}, // will be used later;

    };

    const ResourceBarInfo *resourceBarInfoRefs[RES_BARS_AMOUNT] = {
        &resourceBarInfos[0],                                                  // advMap
        &resourceBarInfos[1],                                                  // Kingdom Overview
        hdMod || !enoughSpaceForCustomResBar ? nullptr : &resourceBarInfos[2], // Puzzle Map
        enoughSpaceForCustomResBar ? &resourceBarInfos[2] : nullptr,           // Town Mgr
        enoughSpaceForCustomResBar ? &resourceBarInfos[2] : nullptr,           // Town Hall
        hdMod || !enoughSpaceForCustomResBar ? nullptr : &resourceBarInfos[2], // Mage Guild
        enoughSpaceForCustomResBar ? &resourceBarInfos[2] : nullptr,           // Town Fort
        hdMod || !enoughSpaceForCustomResBar ? nullptr : &resourceBarInfos[2]  // Thieve Guild

    };
    BOOL createMithril = false;
    for (size_t i = 0; i < RES_BARS_AMOUNT; i++)
    {
        auto ref = resourceBarInfoRefs[i];
        if (!ref)
            continue;

        _PI->WriteLoHook(createResBarAddresses[i], AfterDlgResBarCreate);
        resourceBarPatchInfos.insert(std::make_pair(createResBarAddresses[i], *ref));
    }

    if (enoughSpaceForAdvResBar)
    {
        _PI->WriteLoHook(0x408945, OnAdvMgrDlgRightClick); // H3AdventureMgrDlg
    }
    _PI->WriteHiHook(0x521E20, THISCALL_, KingdomOverviewDlgProc);

    if (enoughSpaceForAdvResBar || enoughSpaceForCustomResBar)
    {
        _PI->WriteHiHook(0x559270, THISCALL_, H3ResourceBarPanel__HideResources);
    }

    // general ingame improvement of displaying resources quantities;
    _PI->WriteLoHook(0x55919D, H3ResourceBarPanel__Refresh);

    // _PI->WriteLoHook(0x558E16, BuildResBar);

    // 2 hooks cause HD Mod or WoG/ERA sets own text there
    _PI->WriteHiHook(0x40EB47, THISCALL_, H3AdventureMgrDlg_Interface_MoveHint);
    _PI->WriteHiHook(0x40E1CC, THISCALL_, H3AdventureMgrDlg_Interface_MoveHint);
}

} // namespace ERI
