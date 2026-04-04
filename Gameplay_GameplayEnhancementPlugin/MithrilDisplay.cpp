#include "MithrilDisplay.h"

namespace ERI
{
ExtendedResourcesInfo *ExtendedResourcesInfo::instance = nullptr;

inline bool IsMouseOverItem(const int x, const int y, const H3DlgItem *item) noexcept
{
    return x >= item->GetAbsoluteX() && x < item->GetAbsoluteX() + item->GetWidth() && y >= item->GetAbsoluteY() &&
           y < item->GetAbsoluteY() + item->GetHeight();
}

inline int GetPlayerMithril(const int playerId = P_Game->GetPlayerID())
{
    return IntAt(0x27F9A00 + playerId * 4); // mb later it should be replaced
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
        auto &info = ExtendedResourcesInfo::Get();
        const int lastHintItemId = info.LastHintItemId(); // IntAt(0x65F228);
        // const int mithrilItemId = info.MitrilItemId();

        unsigned clickedItemId = -1;
        for (size_t i = 0; i < 8; i++)
        {
            const int itemId = i < 7 ? 1001 + i : MITHRIL_DLG_TEXT_ITEM_ID;
            auto it = dlg->GetH3DlgItem(itemId);
            if (it == nullptr)
                continue;

            if (it->IsActive() && it->GetAbsoluteX() <= x && it->GetAbsoluteX() + it->GetWidth() > x &&
                it->GetAbsoluteY() <= y && it->GetAbsoluteY() + it->GetHeight() > y)

            {
                clickedItemId = itemId;

                if (lastHintItemId != clickedItemId)
                {
                    // get player resources
                    const unsigned int resAmount =
                        i == 7 ? GetPlayerMithril() : h3::H3ActivePlayer::Get()->playerResources[i];
                    // create new hint

                    char buffer[64];
                    if (resAmount < 10000)
                        libc::sprintf(buffer, "%d", resAmount);
                    else
                        Era::DecorateInt(resAmount, buffer, true);
                    libc::sprintf(h3_TextBuffer, RESOURCE_HINT_FORMAT, P_ResourceName[i], buffer);
                    // set adv manager hint text
                    THISCALL_2(void, 0x40B040, P_AdventureManager->Get(), (int)h3_TextBuffer);
                }
                break;
            }
        }
        // clear hint if out of any of items
        if (lastHintItemId != -1 && clickedItemId == -1)
        {
            THISCALL_2(void, 0x40B040, P_AdventureManager->Get(), h3_NullString);
        }
        info.SetLastHintItemId(clickedItemId);
        return 1;
    }

    return THISCALL_3(char, h->GetDefaultFunc(), dlg, x, y);
}

int __stdcall ExtendedResourcesInfo::KingdomOverviewDlgProc(HiHook *h, H3BaseDlg *dlg, H3Msg *msg)
{

    if (auto mithrilText = instance->kingdomHintcontrol)
    {
        constexpr int HINT_BAR_ID = 37;
        if (msg->subtype == eMsgSubtype::RBUTTON_DOWN)
        {
            instance->ShowMithrilRMCHint(msg, mithrilText);
        }
        else if (msg->command == eMsgCommand::MOUSE_OVER)
        {
            const H3DlgItem *hintZone = mithrilText;
            const int x = msg->GetX();
            const int y = msg->GetY();
            const bool result = IsMouseOverItem(x, y, hintZone);
            // x >= hintZone->GetAbsoluteX() && x < hintZone->GetAbsoluteX() + hintZone->GetWidth() &&
            //                   y >= hintZone->GetAbsoluteY() && y < hintZone->GetAbsoluteY() + hintZone->GetHeight();
            if (result)
            {
                //	mithril->lastHintIsMitril = true;

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

BOOL ExtendedResourcesInfo::ShowMithrilRMCHint(const H3Msg *msg, H3DlgItem *hintZone) const noexcept
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

_LHF_(ExtendedResourcesInfo::OnAdvMgrDlgRightClick)
{

    bool ret = EXEC_DEFAULT;
    const H3Msg *msg = reinterpret_cast<H3Msg *>(c->edi);
    if (instance->ShowMithrilRMCHint(msg, instance->advMapHintControl))
    {
        c->ebx = true;
        c->return_address = 0x408974;
        ret = NO_EXEC_DEFAULT;
    }

    return ret;
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

H3DlgItem *ExtendedResourcesInfo::AlignOriginalResources(LoHook *h, HookContext *c, const ResourceBarInfo &barInfo)
{

    H3ResourceBarPanel *panel = reinterpret_cast<H3ResourceBarPanel *>(c->eax);
    if (!panel)
        return nullptr;

    const int customItemWidth = barInfo.itemWidth;
    if (customItemWidth)
    {
        int itemX = barInfo.firstResX;
        const int step = barInfo.itemStep;
        for (size_t i = 0; i < h3::limits::RESOURCES; i++)
        {
            auto *text = panel->resourceText[i];
            text->SetWidth(customItemWidth);
            text->SetX(itemX);
            itemX += step;
            text->Cast<H3DlgText>()->SetAlignment(eTextAlignment::TOP_MIDDLE);
        }
    }

    return nullptr;
}
H3DlgItem *ExtendedResourcesInfo::BuildMithril(LoHook *h, HookContext *c, const ResourceBarInfo &barInfo)
{

    H3ResourceBarPanel *panel = reinterpret_cast<H3ResourceBarPanel *>(c->eax);
    if (!panel || !barInfo.createMithril)
        return nullptr;

    const int customItemWidth = barInfo.itemWidth;

    auto *goldTextItem = panel->resourceText[eResource::GOLD];
    const int spotWidth = goldTextItem->GetWidth();
    const int span = goldTextItem->GetX() - panel->resourceText[5]->GetX() - spotWidth;
    const int xPos = goldTextItem->GetX() + spotWidth + span + 12 - panel->resourceText[0]->GetX();
    const int xOffset = customItemWidth ? -7 : 0;

    const BOOL buildFrame = barInfo.frameState == FRAME_STATE_BUILD;

    H3DlgItem *mithrilDef =
        H3DlgDef::Create(xPos + buildFrame * 2 + xOffset, 4, MITHRIL_DLG_DEF_ITEM_ID, MITHRIL_DEF_NAME);

    int textWidth = 50;
    if (buildFrame)
    {
        auto resBarPcx = panel->resbarPCX->GetPcx();
        constexpr int maxPicWidth = 80;
        const int mapWidth = IntAt(0x4196EA + 1);

        const int defWidth = mithrilDef->GetWidth();
        const int textX = xPos + defWidth;
        textWidth = textX + maxPicWidth;
        if (textWidth > mapWidth)
            textWidth = mapWidth;

        textWidth -= textX;
        if (textWidth > 50)
            textWidth = 50;

        const int pcxWidth = textWidth + defWidth + 8;
        const int pcxHeight = resBarPcx->height;

        auto &map = instance->mithrilBackPcxCache;

        const DWORD cacheKey = h->GetAddress();
        auto it = map.find(cacheKey);
        H3LoadedPcx *mithrilBackPcx = nullptr;
        if (it != map.end())
        {
            mithrilBackPcx = it->second;
        }
        else
        {
            mithrilBackPcx = H3LoadedPcx::Create(h3_NullString, pcxWidth, pcxHeight);
            resBarPcx->DrawToPcx(4, 0, pcxWidth, pcxHeight, mithrilBackPcx);
            resBarPcx->DrawToPcx(28, 0, 20, pcxHeight, mithrilBackPcx, 2);
            map[cacheKey] = mithrilBackPcx;
        }

        H3DlgPcx *mithrilBack = H3DlgPcx::Create(xPos, 0, pcxWidth, pcxHeight, MITHRIL_DLG_BACK_PCX_ITEM_ID, nullptr);
        mithrilBack->SetPcx(mithrilBackPcx);
        panel->AddItem(mithrilBack);
        mithrilBack->Show();
    }
    panel->AddItem(mithrilDef);
    mithrilDef->Show();

    H3DlgText *mithriText = H3DlgText::Create(xPos + 29, 3, textWidth, 18, 0, NH3Dlg::Text::SMALL, eTextColor::WHITE,
                                              MITHRIL_DLG_TEXT_ITEM_ID, eTextAlignment::TOP_MIDDLE);
    panel->AddItem(mithriText);
    mithriText->Show();
    mithriText->Activate();

    return mithriText;
}

_LHF_(ExtendedResourcesInfo::AfterDlgResBarCreate)
{

    auto it = instance->resourceBarPatchInfos.find(h->GetAddress());
    if (it == instance->resourceBarPatchInfos.end())
        return EXEC_DEFAULT;

    auto &resourceBarInfo = it->second;
    AlignOriginalResources(h, c, resourceBarInfo);

    if (resourceBarInfo.createMithril)
    {
        BuildMithril(h, c, resourceBarInfo);
    }
    // instance->advMapHintControl = BuildMithril(h, c, resourceBarInfo);
    return EXEC_DEFAULT;
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
static LPCSTR GetPlayerDisplayerResourceText(const int mePlayerId, const int resourceId)
{

    auto &player = P_Game->players[mePlayerId];
    const unsigned int resAmount = resourceId >= 0 && resourceId < h3::limits::RESOURCES
                                       ? player.playerResources.asArray[resourceId]
                                       : GetPlayerMithril();

    char buffer[32];
    if (resAmount < 10000)
    {
        libc::sprintf(h3_TextBuffer, "%6d", resAmount);
        return h3_TextBuffer;
    }
    else if (resAmount < 100000)
    {
        Era::DecorateInt(resAmount, buffer, false);
        libc::sprintf(h3_TextBuffer, "%10s", buffer);
    }
    else if (resAmount < 1000000)
    {
        Era::DecorateInt(resAmount, buffer, false);
        libc::sprintf(h3_TextBuffer, "%12s", buffer);
    }
    else
    {
        Era::FormatQuantity(resAmount, buffer, 32, 7, 4);
        libc::sprintf(h3_TextBuffer, "%12s", buffer);
    }

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
            mithrilTextItem->SetText(GetPlayerDisplayerResourceText(mePlayerId, 7));
        }
    }

    for (size_t i = 0; i < h3::limits::RESOURCES; i++)
    {
        auto textItem = resourceBarPanel->resourceText[i];
        textItem->SetText(GetPlayerDisplayerResourceText(mePlayerId, i));
    }

    c->return_address = 0x05591DB;
    return NO_EXEC_DEFAULT;
}

enum eCreateResBarAddress
{
    CREATE_ADV_MAN = 0x4021B2,
    CREATE_KINGDOM_OVERVIEW = 0x51F042,
    CREATE_PUZZLE_MAP = 0x52CA2E,
    CREATE_TOWN_MGR = 0x5C6976,
    CREATE_MAGE_GUILD = 0x5CEB83,
    CREATE_TOWN_HALL = 0x5D2C71,
    CREATE_TOWN_FORT = 0x5D3A16,
    CREATE_THIVE_GUILD = 0x5DF251,
};

ResourceBarInfo CreateInfo(eCreateResBarAddress createResBarAddress, const BOOL isHD, const int gameWidth, const int gameHeight)
{
    ResourceBarInfo result;


    switch (createResBarAddress)
    {
    case CREATE_ADV_MAN:
    case CREATE_TOWN_MGR:
    case CREATE_PUZZLE_MAP:
		result.itemWidth = isHD ? 60 : 78;
    case CREATE_KINGDOM_OVERVIEW:
    case CREATE_MAGE_GUILD:
    case CREATE_TOWN_HALL:
    case CREATE_TOWN_FORT:
    case CREATE_THIVE_GUILD:

    default:
        break;
    }

    return result;
}

void ExtendedResourcesInfo::CreatePatches()
{
    _PI->WriteLoHook(0x55919D, H3ResourceBarPanel__Refresh);

    return;

    DWORD createResBarAddresses[] = {CREATE_ADV_MAN,   CREATE_KINGDOM_OVERVIEW, CREATE_PUZZLE_MAP, CREATE_TOWN_MGR,
                                     CREATE_TOWN_HALL, CREATE_MAGE_GUILD,       CREATE_TOWN_FORT,  CREATE_THIVE_GUILD};

    const int gameWidth = H3GameWidth::Get();
    const int gameHeight = H3GameHeight::Get();

    const BOOL enoughSpaceForAdvResBar = gameWidth >= 860;
    const BOOL enoughSpaceForCommonResBar = gameHeight >= 608 && gameWidth >= 808;

    ResourceBarInfo resourceBarInfos[] = {
        {enoughSpaceForAdvResBar, 8, 78, 84, FRAME_STATE_BUILD},
        {FALSE, 16, 78, 84},
        {enoughSpaceForCommonResBar, 20, 70, 92},
        {enoughSpaceForCommonResBar, 20, 70, 92},
        {enoughSpaceForCommonResBar, 8, 78, 84},
        {enoughSpaceForCommonResBar, 20, 70, 92},
        {enoughSpaceForCommonResBar, 20, 70, 92},
        {enoughSpaceForCommonResBar, 20, 70, 92},
    };

    ResourceBarInfo *resourceBarInfoRefs[] = {
        &resourceBarInfos[0],
    };
    BOOL createMithril = false;
    for (size_t i = 0; i < std::size(createResBarAddresses); i++)
    {
        // auto ref = resourceBarInfoRefs[i];
        // if (!ref)
        //     continue;

        _PI->WriteLoHook(createResBarAddresses[i], AfterDlgResBarCreate);
        resourceBarPatchInfos.insert(std::make_pair(createResBarAddresses[i], resourceBarInfos[i]));
    }

    if (gameWidth >= 860)
    {
        //   _PI->WriteLoHook(0x4021B2, OnAdvMgrDlgResBarCreate); // H3AdventureMgrDlg
        _PI->WriteLoHook(0x408945, OnAdvMgrDlgRightClick); // H3AdventureMgrDlg
        createMithril = true;
    }

    if (gameHeight >= 608 && gameWidth >= 808)
    {
        // hooks of resource bar creation:
        //_PI->WriteLoHook(0x51F042, OnKingdomOverviewDlgResBarCreate); // KingdomOverviewDlg
        //_PI->WriteLoHook(0x5C6976, OnTownMgrDlgResBarCreate);         // H3TownMgrDlg
        //_PI->WriteLoHook(0x5D2C71, OnTownHallDlgResBarCreate);        // H3TownHallDlg

        _PI->WriteHiHook(0x521E20, THISCALL_, KingdomOverviewDlgProc);

        createMithril = true;
    }
    if (enoughSpaceForAdvResBar || enoughSpaceForCommonResBar)
    {
        _PI->WriteHiHook(0x559270, THISCALL_, H3ResourceBarPanel__HideResources);
    }
    // general ingame improvement of displaying resources quantities;
    // _PI->WriteHiHook(0x559170, THISCALL_, H3ResourceBarPanel__Refresh);

    // _PI->WriteLoHook(0x558E16, BuildResBar);

    // 2 hooks cause HD Mod or WoG/ERA sets own text there
    _PI->WriteHiHook(0x40EB47, THISCALL_, H3AdventureMgrDlg_Interface_MoveHint);
    _PI->WriteHiHook(0x40E1CC, THISCALL_, H3AdventureMgrDlg_Interface_MoveHint);
}

} // namespace ERI
