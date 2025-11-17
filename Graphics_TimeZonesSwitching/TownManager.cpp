#include "pch.h"

namespace timezone
{
constexpr UINT DOT_TIMEZONE_COUNT = 6;

// TownManager::BackgroundDrawInfo TownManager::backgroundDrawInfo{0,0};

_LHF_(TownMgrDlg_SwitchTown)
{

    // allow same town type to redraw background graphics
    c->flags.ZF = false;

    return EXEC_DEFAULT;
}

void TownMgr_RedirectBuildingGraphics(const UINT townType, const UINT time)
{

    if (townType < h3::limits::TOWNS)
    {

        LPCSTR townPrefix = (*reinterpret_cast<LPCSTR **>(0x05C6E40 + 3))[townType];
        bool readSucces = false;
        LPCSTR townFolderName =
            EraJS::read(H3String::Format("todan.townTypeNames.%s", townPrefix).String(), readSucces);
        // check if prefix exists and folder isn't empty
        if (readSucces && libc::strcmp(townFolderName, ""))
        {
            for (size_t buildingId = 0; buildingId < h3::limits::BUILDINGS; buildingId++)
            {
                LPCSTR buildingDefName =
                    reinterpret_cast<LPCSTR *>(0x643074)[townType * h3::limits::BUILDINGS + buildingId];
                for (size_t frameId = 0; frameId < 100; frameId++)
                {
                    H3String oldDefFrameName = H3String::Format("%s.def:0_%d.png", buildingDefName, frameId);

                    H3String newDefFrameName = H3String::Format("Data\\Defs\\Buildings\\%s\\%d\\%s.def\\0_%d.png",
                                                                townFolderName, time, buildingDefName, frameId);

                    Era::RedirectFile(oldDefFrameName.String(), newDefFrameName.String());
                }
            }
        }
    }
}
const UINT GetTownTime(const H3Town *town)
{

    int result = 0;

    const UINT zoneWidth = (H3MapSize::Get() + 1) / DOT_TIMEZONE_COUNT;
    const UINT timeZone = town->x / zoneWidth;
    if (timeZone < DOT_TIMEZONE_COUNT)
    {
        const UINT timeOffset = P_Game->date.day % DOT_TIMEZONE_COUNT;
        result = timeZone + timeOffset;
        if (result >= DOT_TIMEZONE_COUNT)
        {
            result %= DOT_TIMEZONE_COUNT;
        }
        if (result == 5)
        {
            // make same time for the sunrise/sunset
            result = 2;
        }
    }
    return result;
}

int __cdecl TownManager::TownMgrDlg_CreateBackgroundImageName(HiHook *h, char *buffer, char *formatName,
                                                              char *townPrefixName)
{
    const H3Town *town = P_TownManager->town;

    if (!town || town->type >= h3::limits::TOWNS)
        return CDECL_3(int, h->GetDefaultFunc(), buffer, formatName, townPrefixName);

    const UINT thisTownTime = GetTownTime(town);
    TownMgr_RedirectBuildingGraphics(town->type, thisTownTime);
    Get().backgroundDrawInfo.Update(townPrefixName, thisTownTime);
    Get().animationDrawInfo.Update(townPrefixName, thisTownTime);

    // set new background name
    return h3::libc::sprintf(buffer, "%s_%dBG.pcx", townPrefixName, thisTownTime);
}
void __stdcall TownManager::TownMgrDlg_DrawBackgroundImageNameInProc(HiHook *h, H3DlgPcx16 *background)
{
    auto &backgroundDrawInfo = Get().backgroundDrawInfo;
    if (backgroundDrawInfo.lastFrameId > 0)
    {
        if (auto pcx = background->GetPcx())
        {
            pcx->Dereference();
            background->SetPcx(backgroundDrawInfo.GetNextFrame());
        }
        // P_TownManager->background->Dereference();
        // P_TownManager->background = backgroundDrawInfo.GetNextFrame();
        // backgroundImage = P_TownManager->background;
    }
    THISCALL_1(void, h->GetDefaultFunc(), background);

    return;
}
void __stdcall TownManager::TownMgrDlg_BuildingsWindowsRedraw(HiHook *h, H3WindowManager *wndMgr, const int x,
                                                              const int y, const int w, const int height)
{

    // draw new weather defs above all the buildings
    auto &drawInfo = Get().animationDrawInfo;
    for (size_t i = 0; i < drawInfo.defsToDraw; i++)
    {
        if (auto dlgDefButton = &drawInfo.defDrawData[i])
        {
            if (dlgDefButton->defButton->IsVisible())
            {
                const int nextFrameId = dlgDefButton->defButton->GetFrame() + 1;
                // set next frame to draw
                dlgDefButton->defButton->SetFrame(nextFrameId < dlgDefButton->framesNumber ? nextFrameId : 0);
                dlgDefButton->defButton->Draw();
            }
        }
    }

    // now redraw the screen
    THISCALL_5(void, h->GetDefaultFunc(), wndMgr, x, y, w, height);

    return;
}

H3LoadedPcx16 *TownManager::BackgroundDrawInfo::GetNextFrame()
{

    auto pcx = H3LoadedPcx16::Load(H3String::Format("%s%d.pcx", formatName.String(), currentFrame++).String());

    if (currentFrame == lastFrameId)
    {
        currentFrame = 0;
    }
    return pcx;
}

void TownManager::BackgroundDrawInfo::Update(LPCSTR townPrefixName, const UINT thisTownTime)
{
    currentFrame = 0;
    lastFrameId = 0;
    h3::libc::sprintf(h3_TextBuffer, "%s_%d", townPrefixName, thisTownTime);
    formatName = H3String(h3_TextBuffer);

    for (size_t i = 0; i < 100; i++)
    {
        h3::libc::sprintf(h3_TextBuffer, "%s_%d%d.pcx", townPrefixName, thisTownTime, i);

        if (Era::PcxPngExists(h3_TextBuffer))
        {
            lastFrameId++;
        }
    }
}

const BOOL TownManager::AnimationDrawInfo::LoadFromJsonByTime(LPCSTR townPrefixName, const INT thisTownTime)
{
    // now fill buttons with needed def names
    bool readSucces = false;
    //	for (size_t i = 0; i < dlgDefs.size(); i++)

    int counter = 0;
    for (auto &data : defDrawData)
    {
        if (!data.defButton->IsVisible())
        {
            h3::libc::sprintf(h3_TextBuffer, "todan.animations.%s.%d.%d", townPrefixName, thisTownTime, counter++);
            H3String defName = EraJS::read(h3_TextBuffer, readSucces);
            if (readSucces && !defName.Empty())
            {
                // show item cause it is used
                // get loaded Def
                auto def = data.defButton->GetDef();
                // deref current
                def->Dereference();
                h3::libc::sprintf(h3_TextBuffer, "%s", defName.String());
                // data.defButton->SendCommand(9, (int)"rain.def");//reinterpret_cast<DWORD>(h3_TextBuffer));

                data.defButton->Show();
                // load new picture taken from json
                *reinterpret_cast<H3LoadedDef **>(reinterpret_cast<char *>(data.defButton) + 0x30) =
                    H3LoadedDef::Load(defName.String());
                def = data.defButton->GetDef();

                data.framesNumber = def->groups[0]->count;
                defsToDraw++;
            }
            else
            { // leave the loop cause json key is empty
                break;
            }
        }
    }

    return defsToDraw;
}

void TownManager::AnimationDrawInfo::Update(LPCSTR townPrefixName, const UINT thisTownTime)
{
    defDrawData.clear();
    defsToDraw = 0;
    // first get def ptrs
    for (size_t i = 0; i < 5; i++)
    {
        // H3String buttonTagName = H3String::Format(buttonNameTagFormat, i).String();
        if (const UINT defBttnId = Era::GetButtonID(H3String::Format(buttonNameTagFormat, i).String()))
        {
            if (auto bttn = P_TownMgr->dlg->GetDefButton(defBttnId))
            {
                defDrawData.emplace_back(DefDrawData{bttn, 0});
                bttn->HideDeactivate();
            }
        }
        else
        {
            break;
        }
    }

    // try to load for that time only
    if (!LoadFromJsonByTime(townPrefixName, thisTownTime))
    {
        // otherwise load for dfault time
        LoadFromJsonByTime(townPrefixName, -1);
    }
}
TownManager::TownManager() : IGamePatch(_PI)
{
    this->CreatePatches();
}
TownManager &TownManager::Get()
{
    static TownManager instance;

    return instance;
    // TODO: insert return statement here
}

void TownManager::CreatePatches()
{

    if (!m_isInited)
    {
        _pi->WriteLoHook(0x05C6E19, TownMgrDlg_SwitchTown);

        _pi->WriteHiHook(0x5C6E52, CDECL_, TownMgrDlg_CreateBackgroundImageName);
        _pi->WriteHiHook(0x05D37EC, THISCALL_, TownMgrDlg_DrawBackgroundImageNameInProc);
        _pi->WriteHiHook(0x5D3843, THISCALL_, TownMgrDlg_BuildingsWindowsRedraw);

        //_pi->WriteHiHook(0x5CEB0F, THISCALL_, MageGuildDlg_Create);

        // Era::RegisterHandler(OnAfterErmInstructions, "OnAfterErmInstructions");
        m_isInited = true;
    }
}
} // namespace timezone
