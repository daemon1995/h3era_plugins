#include "MithrilDisplay.h"

namespace ERI
{
ExtendedResourcesInfo *ExtendedResourcesInfo::instance = nullptr;

inline bool IsMouseOverItem(const int x, const int y, const H3DlgItem *item) noexcept
{
    return item && item->IsActive() && x >= item->GetAbsoluteX() && x < item->GetAbsoluteX() + item->GetWidth() &&
           y >= item->GetAbsoluteY() && y < item->GetAbsoluteY() + item->GetHeight();
}

inline int GetPlayerResources(const int resourceId, const int playerId = P_Game->GetPlayerID())
{
    if (playerId < 0 || playerId >= 8 || resourceId < 0 || resourceId > eResource::MITHRIL)
        return 0;
    return resourceId == eResource::MITHRIL
               ? IntAt(0x27F9A00 + playerId * 4)
               : P_Game->players[playerId].playerResources[resourceId]; // mb later it should be replaced
}

ExtendedResourcesInfo::ExtendedResourcesInfo(PatcherInstance *_PI) : IGamePatch(_PI)
{

    CreatePatches();
}

ExtendedResourcesInfo::~ExtendedResourcesInfo()
{
    for (auto &entry : resourceBarPatchInfos)
        if (entry.second.mithrilBackPcxCache)
            entry.second.mithrilBackPcxCache->Dereference();
    instance = nullptr;
}

/// set mouse hover hints for the resources
char __stdcall H3AdventureMgrDlg_Interface_MoveHint(HiHook *h, H3AdventureMgrDlg *dlg, const int x, const int y)
{

    // check if not focused on chat log
    if (!dlg->screenlogEdit->IsFocused())
    {

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

            IntAt(0x65F228) = clickedItemId;
            return 1;
        }
    }

    return THISCALL_3(char, h->GetDefaultFunc(), dlg, x, y);
}

int __stdcall ExtendedResourcesInfo::KingdomOverviewDlgProc(HiHook *h, H3BaseDlg *dlg, H3Msg *msg)
{

    auto *hintZone = GetMitrilBarHintZone(CREATE_KINGDOM_OVERVIEW, dlg);
    if (hintZone && msg->command == eMsgCommand::MOUSE_BUTTON && msg->subtype == eMsgSubtype::RBUTTON_DOWN &&
        IsMouseOverItem(msg->GetX(), msg->GetY(), hintZone) && ShowMithrilRMCHint(msg, hintZone))
        return 1;

    // Let the native handler manage ordinary hints and mouse state before overriding our zone.
    const int result = THISCALL_2(int, h->GetDefaultFunc(), dlg, msg);
    if (hintZone && msg->command == eMsgCommand::MOUSE_OVER &&
        IsMouseOverItem(msg->GetX(), msg->GetY(), hintZone))
    {
        if (auto *bar = dlg->GetText(37))
        {
            bar->SetText(P_ResourceName[eResource::MITHRIL]);
            bar->Draw();
            bar->Refresh();
        }
    }
    return result;
}

BOOL ExtendedResourcesInfo::ShowMithrilRMCHint(const H3Msg *msg, H3DlgItem *hintZone) noexcept
{
    if (!msg || !hintZone)
        return FALSE;
    const int x = msg->GetX();
    const int y = msg->GetY();
    bool result = IsMouseOverItem(x, y, hintZone); // msg->ItemAtPosition(msg->GetDlg()) == hintZone;
    if (result)
    {
        int width = 0;
        LPCSTR hintText = EraJS::read("gem_plugin.mithril_display.popup_hint");

        int height = 0;
        FASTCALL_3(void, 0x4F6930, hintText, &width, &height);
        FASTCALL_12(void, 0x4F6C00, hintText, 4, H3GameWidth::Get() - 208 - width, (H3GameHeight::Get() - height) / 2 - 10, -1, 0, -1, 0, -1, 0, -1, 0);
    }
    else if (msg->itemId == 1008 && msg->GetX() < hintZone->GetAbsoluteX())
        result = true; // prevent extra hint for the date hint

    return result;
}

H3DlgItem *ExtendedResourcesInfo::GetMitrilBarHintZone(DWORD patchAddress, const H3BaseDlg *owner) noexcept
{
    if (!patchAddress)
        return nullptr;
    auto &map = instance->resourceBarPatchInfos;
    auto it = map.find(patchAddress);
    if (it != map.end() && owner && it->second.owner == owner)
    {
        return it->second.mithrilDlgItem;
    }

    return nullptr;
}

_LHF_(ExtendedResourcesInfo::OnAdvMgrDlgRightClick)
{

    auto hintZone = GetMitrilBarHintZone(CREATE_ADV_MAN, P_AdventureMgr->dlg);
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

BOOL ExtendedResourcesInfo::AlignOriginalResources(HookContext *c, const ResourceBarInfo &barInfo)
{
    H3ResourceBarPanel *panel = c->Eax<H3ResourceBarPanel *>();
    if (!panel)
        return FALSE;
    // Validate the whole panel before changing any item's layout.
    for (size_t i = 0; i < h3::limits::RESOURCES; ++i)
        if (!panel->resourceText[i] || !panel->resourceOverlay[i])
            return FALSE;

    const int customItemWidth = barInfo.customTextItemWidth;
    int itemX = barInfo.firstResX;
    const int step = barInfo.itemStep;
    for (size_t i = 0; i < h3::limits::RESOURCES; i++)
    {
        auto *text = panel->resourceText[i];
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
    if (!panel || !barInfo.createMithril || !panel->resourceText[eResource::GOLD] ||
        !panel->resourceText[5] || !panel->resourceText[0] || !panel->resourceOverlay[0])
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
        auto resBarPcx = panel->resbarPCX ? panel->resbarPCX->GetPcx() : nullptr;
        if (!resBarPcx)
            return nullptr;
        const int mapWidth = IntAt(0x4196EA + 1);

        if (textWidth > mapWidth)
            textWidth = mapWidth;

        const int pcxWidth = textWidth + defOverlayWidth + 2;
        const int pcxHeight = resBarPcx->height;

        auto &storedPcx = barInfo.mithrilBackPcxCache;
        if (storedPcx && (storedPcx->width != pcxWidth || storedPcx->height != pcxHeight))
        {
            storedPcx->Dereference();
            storedPcx = nullptr;
        }
        if (!storedPcx)
        {
            storedPcx = H3LoadedPcx::Create(h3_NullString, pcxWidth, pcxHeight);
            if (!storedPcx)
                return nullptr;
            // Create starts with zero references; retain one for the module's cache.
            storedPcx->IncreaseReferences();
            resBarPcx->DrawToPcx(4, 0, pcxWidth, pcxHeight, storedPcx);
            resBarPcx->DrawToPcx(28, 0, 20, pcxHeight, storedPcx, 2);
        }

        H3DlgPcx *mithrilBack = H3DlgPcx::Create(xPos, 0, pcxWidth, pcxHeight, MITHRIL_DLG_BACK_PCX_ITEM_ID, nullptr);
        if (!mithrilBack)
            return nullptr;
        // SetPcx only assigns the pointer. The widget destructor releases its reference.
        storedPcx->IncreaseReferences();
        mithrilBack->SetPcx(storedPcx);
        panel->AddItem(mithrilBack);
        mithrilBack->Show();
    }

    H3DlgText *mithrilText = H3DlgText::Create(xPos + defOverlayWidth, 3, textWidth, 18, 0, NH3Dlg::Text::SMALL,
                                               eTextColor::WHITE, MITHRIL_DLG_TEXT_ITEM_ID, eTextAlignment::TOP_LEFT);
    if (!mithrilText)
        return nullptr;
    panel->AddItem(mithrilText);
    mithrilText->ShowActivate();
    barInfo.mithrilTextItem = mithrilText;

    H3DlgItem *mithrilItem = nullptr;
    if (xOffset)
    {
        mithrilItem =
            H3DlgTransparentItem::Create(xPos + buildFrame * 2, 4, defOverlayWidth, 16, MITHRIL_DLG_DEF_ITEM_ID);
    }
    else
    {
        mithrilItem = H3DlgDef::Create(xPos + buildFrame * 2, 4, MITHRIL_DLG_DEF_ITEM_ID, MITHRIL_DEF_NAME);
    }

    if (!mithrilItem)
    {
        barInfo.mithrilDlgItem = mithrilText;
        return mithrilText;
    }
    // to set as hint zone to cover both the icon and the text
    mithrilItem->SetWidth(defOverlayWidth + textWidth);

    panel->AddItem(mithrilItem);
    mithrilItem->ShowActivate();

    barInfo.mithrilDlgItem = mithrilItem;

    return mithrilText;
}

void __stdcall ExtendedResourcesInfo::H3ResourceBarPanel__HideResources(HiHook *h, H3ResourceBarPanel *resourceBarPanel)
{

    H3BaseDlg *dlg = resourceBarPanel ? resourceBarPanel->GetParent() : nullptr;

    if (dlg)
    {
        if (auto *mithril = dlg->GetText(MITHRIL_DLG_TEXT_ITEM_ID))
            mithril->SetText(h3_NullString);
        if (auto *mithrilBack = dlg->GetPcx(MITHRIL_DLG_BACK_PCX_ITEM_ID))
            mithrilBack->ColorToPlayer(IntAt(0x6977DC));
    }
    return THISCALL_1(void, h->GetDefaultFunc(), resourceBarPanel);
}
static H3String GetPlayerDisplayedResourceText(const int mePlayerId, const int resourceId)
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
    buffer[31] = '\0';
    return H3String(buffer);
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
    resourceBarInfo.mithrilDlgItem = nullptr;
    resourceBarInfo.mithrilTextItem = nullptr;
    auto *panel = c->Eax<H3ResourceBarPanel *>();
    resourceBarInfo.owner = panel ? panel->GetParent() : nullptr;
    if (!AlignOriginalResources(c, resourceBarInfo))
        return EXEC_DEFAULT;

    if (resourceBarInfo.createMithril)
    {
        BuildMithril(c, resourceBarInfo);
    }
    return EXEC_DEFAULT;
}

void ExtendedResourcesInfo::CreatePatches()
{

    if (m_isInited)
        return;
    const BOOL hdMod = GetModuleHandleA("hd_wog.dll") != nullptr;
    if (!hdMod)
        return;

    const int gameWidth = H3GameWidth::Get();
    const int gameHeight = H3GameHeight::Get();
    const BOOL enoughSpaceForAdvResBar = gameWidth >= 860;
    const BOOL enoughSpaceForCustomResBar = gameWidth >= 808 && gameHeight >= 608;

    resourceBarPatchInfos.reserve(5);
    const auto addBar = [this](DWORD address, const ResourceBarInfo &info)
    {
        resourceBarPatchInfos.emplace(address, info);
        _pi->WriteLoHook(address, AfterDlgResBarCreate);
    };
    addBar(CREATE_ADV_MAN, ResourceBarInfo{enoughSpaceForAdvResBar, 8, 0, 0, FRAME_STATE_BUILD});
    addBar(CREATE_KINGDOM_OVERVIEW, ResourceBarInfo{TRUE, 16});
    if (enoughSpaceForCustomResBar)
    {
        const ResourceBarInfo townBar{TRUE, 6, 84, 92};
        addBar(CREATE_TOWN_MGR, townBar);
        addBar(CREATE_TOWN_HALL, townBar);
        addBar(CREATE_TOWN_FORT, townBar);
    }
    // Puzzle map, mage guild and thieves guild use HD's own layout; no hooks there.

    if (enoughSpaceForAdvResBar)
    {
        _pi->WriteLoHook(0x408945, OnAdvMgrDlgRightClick); // H3AdventureMgrDlg
    }
    _pi->WriteHiHook(0x521E20, THISCALL_, KingdomOverviewDlgProc);

    if (enoughSpaceForAdvResBar || enoughSpaceForCustomResBar)
    {
        _pi->WriteHiHook(0x559270, THISCALL_, H3ResourceBarPanel__HideResources);
    }

    // general ingame improvement of displaying resources quantities;
    _pi->WriteLoHook(0x55919D, H3ResourceBarPanel__Refresh);

    // 2 hooks cause HD Mod or WoG/ERA sets own text there
    _pi->WriteHiHook(0x40EB47, THISCALL_, H3AdventureMgrDlg_Interface_MoveHint);
    _pi->WriteHiHook(0x40E1CC, THISCALL_, H3AdventureMgrDlg_Interface_MoveHint);
    m_isInited = true;
    m_isEnabled = true;
}

} // namespace ERI
