#include <thread>
#include <unordered_map>

#include "pch.h"

/**
@TODO:
    -- Add categories for the all pages as dropdown list
*/

namespace itemIds
{

constexpr INT16 FIRST = 1221;
constexpr INT16 DLG_SETTINGS_BUTTON_ID = FIRST;
constexpr INT16 RMG_SETTINGS_TEXT_ID = FIRST + 1;
constexpr INT16 RMG_SEED_TEXTEDIT_ID = FIRST + 2;
constexpr INT16 RMG_SEED_TEXT_ID = FIRST + 3;
constexpr INT16 RMG_RANDOMIZE_BUTTON_ID = FIRST + 4;
constexpr INT16 LAST = RMG_RANDOMIZE_BUTTON_ID;

} // namespace itemIds

namespace rmgdlg
{
UINT rmgdlg::RMG_SettingsDlg::ObjectsPanel::id = 0;

namespace sorting
{
enum eSorting : char
{
    NONE = -1,
    BY_CB_ID,
    BY_ENABLING,
    BY_TYPE,
    BY_NAME,
    BY_MAP,
    BY_ZONE,
    BY_VALUE,
    BY_DENSITY
};

void SortRmgObjects(std::vector<RMGDlgObject> &objVector, const eSorting sortingType = BY_TYPE, bool isReverse = false)
{
    // sort map rmgDlgObjects by dfferent types

    std::sort(objVector.begin(), objVector.end(), [&](const RMGDlgObject &first, const RMGDlgObject &second) -> bool {
        const int cbIdFirst =
            cbanks::CreatureBanksExtender::GetCreatureBankType(first.objectInfo.type, first.objectInfo.subtype);
        const int cbIdSecond =
            cbanks::CreatureBanksExtender::GetCreatureBankType(second.objectInfo.type, second.objectInfo.subtype);

        H3String str, str2;
        switch (sortingType)
        {
        case BY_ENABLING:
            return first.objectInfo.enabled < second.objectInfo.enabled;
        case BY_TYPE:
            // for creature banks get own sort type
            // if (cbIdFirst != -1	|| cbIdSecond != -1)
            //	return cbIdFirst < cbIdSecond;
            if (first.objectInfo.type == second.objectInfo.type)
            {
                return first.objectInfo.subtype < second.objectInfo.subtype;
            }
            return first.objectInfo.type < second.objectInfo.type;

            // return first.objectInfo.subtype < second.objectInfo.subtype;

        case BY_NAME:
            return libc::strcmpi(first.objectInfo.GetName(), second.objectInfo.GetName()) < 0;

        case BY_MAP:
            return first.objectInfo.mapLimit < second.objectInfo.mapLimit;
        case BY_ZONE:
            return first.objectInfo.zoneLimit < second.objectInfo.zoneLimit;
        case BY_VALUE:
            return first.objectInfo.value < second.objectInfo.value;
        case BY_DENSITY:
            return first.objectInfo.density < second.objectInfo.density;
        default:
            break;
        }
        return first.objectInfo.subtype < second.objectInfo.subtype;
    });

    if (!isReverse)
        std::reverse(objVector.begin(), objVector.end());
}
} // namespace sorting

namespace NItemIDs
{

enum eControl : INT
{
    NONE = -1,
    PAGE_FIRST = 30000,
    PAGE_0 = PAGE_FIRST,
    PAGE_1,
    PAGE_2,
    PAGE_3,
    PAGE_LAST = PAGE_3,
    BUTTON_FIRST = 30100,
    CANCEL = BUTTON_FIRST, // = 30721,
    OK,                    // = 30722,
    DEFAULT,               // = 30723,
    RANDOM,                // = 30724,
    HELP,                  //= 30722
    BUTTON_LAST = HELP
};
enum eHeader : INT
{
    //			NONE = -1,
    HEADER_FIRST = 2018,
    TYPE = HEADER_FIRST,
    NAME,
    MAP,
    ZONE,
    VALUE,
    DENSITY,
    HEADER_LAST = DENSITY
};

} // namespace NItemIDs

enum eZoneType : INT
{
    HUMAN,
    COMPUTER,
    TREASURE,
    JUNCTION
};

DWORD RMG_SettingsDlg::userRandSeed = 0;

std::vector<GraphicalAttributes> RMG_SettingsDlg::m_creatureBanks, RMG_SettingsDlg::m_commonObjects,
    RMG_SettingsDlg::m_creatureGenerators, RMG_SettingsDlg::m_wogObjects;

const std::vector<std::vector<GraphicalAttributes> *> RMG_SettingsDlg::m_objectAttributes = {
    &m_creatureBanks, &m_commonObjects, &m_creatureGenerators, &m_wogObjects};

DllExport BOOL RMGObjectSupportsGeneration(const int objType, const int objSubtype = -1)
{
    BOOL result = false;
    if (objType > eObject::NO_OBJ && objType < H3_MAX_OBJECTS)
    {
        const auto vec = RMG_SettingsDlg::GetObjectAttributesVector(eObject(objType));
        for (auto &prop : *vec)
        {
            if (prop.attributes->type == objType)
            {
                return true;
            }
        }
    }

    return false;
}

RMG_SettingsDlg *RMG_SettingsDlg::Page::dlg = nullptr;
RMG_SettingsDlg *RMG_SettingsDlg::instance = nullptr;
BOOL RMG_SettingsDlg::isDlgTextEditInput = false;

const std::vector<std::vector<GraphicalAttributes> *> &RMG_SettingsDlg::GetObjectAttributes() noexcept
{
    return m_objectAttributes;
}

DWORD RMG_SettingsDlg::GetUserRandSeedInput() noexcept
{

    return userRandSeed;
}

RMG_SettingsDlg::RMG_SettingsDlg(int width, int height, int x = -1, int y = -1)
    : H3Dlg(width, height, x, y, false, false), m_currentPage(nullptr)
{

    RMG_SettingsDlg::ObjectsPanel::id = 0;

    // THISCALL_1(void, 0x5063F0, &P_Game->mainSetup);
    this->flags = 2;

    AddBackground(0, 0, width, height, false, false, 1, true);
    if (background)
    {
        // background->FrameRegion(0, 0, width, height, true, 1, false);
        // CreateHint();
        //  blue
    }

    blockLettersInput = true;

    Page::dlg = this;
    instance = this;

    //	auto okBttn = this->CreateOKButton();

    headerPcx = H3DlgPcx16::Create(25, 6, widthDlg - 50, 45, -1, "RMG_back.pcx");
    AddItem(headerPcx);
    // this->background->DarkenArea(25, 6, widthDlg - 50, 45, 80);

    constexpr int bttnWidth = 55; // okBttn->GetWidth();

    constexpr struct
    {
        const POINT pos; // = { 0,-50 };
        LPCSTR defName;
        LPCSTR jsonName;
        INT hotkeys[2]; // {};
        BOOL createFrame;
        BOOL closeDlg; // = false;

    } buttosData[5] = {
        {{-25 - bttnWidth, -50}, "RMG_cncl.def", "cancel", {eVKey::H3VK_ESCAPE, eVKey::H3VK_S}, true, false},
        {{-33 - bttnWidth * 2, -50}, "RMG_okay.def", "ok", {eVKey::H3VK_ENTER, eVKey::H3VK_SPACEBAR}, true, false},
        {{25 + 8 + bttnWidth, -50}, "RMG_dflt.def", "default", {eVKey::H3VK_D, 0}, true, false},
        {{25, -50}, "RanRand.def", "random", {eVKey::H3VK_R, 0}, true, false},
        {{-39 - bttnWidth, 13}, "RMG_help.def", "help", {eVKey::H3VK_H, 0}, false, false}};

    H3DlgDefButton *bttns[5]{}; // = nullptr;

    H3RGB565 color(H3RGB888::Highlight());
    //	H3RGB565 color(H3RGB888(0xA8, 0x8D, 0x44));

    for (size_t i = 0; i < 5; ++i)
    {
        auto &data = buttosData[i];

        const int _x = data.pos.x < 0 ? widthDlg + data.pos.x : data.pos.x;
        const int _y = data.pos.y < 0 ? heightDlg + data.pos.y : data.pos.y;

        auto *bttn = H3DlgDefButton::Create(_x, _y, NItemIDs::BUTTON_FIRST + i, data.defName, 0, 1, data.closeDlg,
                                            data.hotkeys[0]);

        bttns[i] = bttn;

        if (bttn)
        {
            // add additionall hotkeyes
            if (int additionalHotkey = data.hotkeys[1])
            {
                bttn->AddHotkey(additionalHotkey);
            }

            // create frame
            if (data.createFrame)
            {
                this->CreateFrame(bttn, color, 0, 1);
            }
            // set hints
            bttn->SetHints(EraJS::read(H3String::Format("RMG.text.dlg.buttons.%s.hint", data.jsonName).String()),
                           EraJS::read(H3String::Format("RMG.text.dlg.buttons.%s.rmc", data.jsonName).String()), false);

            // add item
            AddItem(bttn);
        }
    }

    // create page selectors

    constexpr size_t pagesNum = 4;
    captionButtons.resize(pagesNum);
    for (size_t i = 0; i < pagesNum; i++)
    {
        libc::sprintf(h3_TextBuffer, "RMG.text.dlg.pages.%d.name", i);

        const int _x = 29 + 167 * i;
        constexpr int _y = 10;

        H3DlgCaptionButton *captionbttn =
            H3DlgCaptionButton::Create(_x, _y, NItemIDs::PAGE_FIRST + i, "RMGmenbt.def", EraJS::read(h3_TextBuffer),
                                       NH3Dlg::Text::MEDIUM, 0, 0, 0, eVKey::H3VK_1 + i, eTextColor::HIGHLIGHT);

        captionbttn->SetHints(EraJS::read(H3String::Format("RMG.text.dlg.pages.%d.hint", i).String()),
                              EraJS::read(H3String::Format("RMG.text.dlg.pages.%d.rmc", i).String()), false);

        captionbttn->SetClickFrame(1);
        AddItem(captionbttn);
        captionButtons[i] = (captionbttn);
    }

    if (bttns[1] && bttns[2])
    {

        const int _x = bttns[2]->GetX() + bttns[2]->GetWidth() + 8;
        const int _y = bttns[2]->GetY() - 1;
        const int _w = bttns[1]->GetX() - _x - 8;
        const int _h = bttns[2]->GetHeight() + 2;

        hintBar = H3ObjectAllocator<H3DlgHintBar>().allocate(1);
        if (hintBar)
        {
            THISCALL_12(H3DlgHintBar *, 0x5BCB70, hintBar, _x, _y, _w, _h, h3_NullString, NH3Dlg::Text::SMALL,
                        "rmghntbr.pcx", eTextColor::REGULAR, 0, eTextAlignment::MIDDLE_CENTER, 8);
            AddItem(hintBar);
        }
    }

    m_pages.emplace_back(new BanksPage{captionButtons[0], m_creatureBanks});

    // create with ignored subtypes to display less data but should affect rmgDlgObjects anyway
    m_pages.emplace_back(new ObjectsPage{captionButtons[1], m_commonObjects, false});
    m_pages.emplace_back(new ObjectsPage{captionButtons[2], m_creatureGenerators, false});
    m_pages.emplace_back(new ObjectsPage{captionButtons[3], m_wogObjects, false});
    ReadIniDlgSettings();
}

int __fastcall HelpButtonProc(H3Msg *msg)
{
    if (msg->IsLeftClick())
    {

        const BOOL isOk = H3Messagebox::Choice(EraJS::read("RMG.text.dlg.buttons.help.question"));
        if (isOk)
        {
            H3String shellExecutePath = EraJS::read("RMG.text.dlg.buttons.help.url");

            // call ShellExecuteA from exe to open github download page
            INT_PTR intRes =
                STDCALL_6(INT_PTR, PtrAt(0x63A250), NULL, "open", shellExecutePath.String(), NULL, NULL, SW_SHOWNORMAL);
            if (intRes <= 32)
            {

                LPSTR messageBuffer = nullptr;

                // Форматирование сообщения об ошибке
                size_t size = FormatMessageA(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr, intRes, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

                std::string message(messageBuffer, size);
                H3Messagebox(message.c_str());

                // Освобождение буфера
                LocalFree(messageBuffer);
            }
        }

        return 1;
    }

    return false;
}
VOID RMG_SettingsDlg::OnHelp() const noexcept
{
    H3String mes = EraJS::read("RMG.text.dlg.buttons.help.help");

    // mes.Append
    for (size_t i = 0; i < m_pages.size(); i++)
    {
        mes += "\n\n";
        mes += EraJS::read(H3String::Format("RMG.text.dlg.pages.%d.help", i).String());
    }
    if (mes.Empty())
    {
        mes = EraJS::read("RMG.text.dlg.notImplemented");
    }

    const int helpWidth = this->widthDlg * 0.95;
    const int helpHeight = this->heightDlg * 0.95;
    H3Dlg dlg(helpWidth, helpHeight, -1, -1, 0, 1);

    H3DefLoader okBttnDef(NH3Dlg::Assets::OKAY32_DEF);
    H3DefLoader siteBttn("wogcurse.def");

    const int offset = 20;
    const int textWidth = helpWidth - 2 * offset;

    const int okBttnY = helpHeight - okBttnDef->heightDEF - offset;
    const int okBttnX = (helpWidth - okBttnDef->widthDEF) / 2;
    auto okBttn = dlg.CreateOK32Button(helpWidth - offset - okBttnDef->widthDEF - 1, okBttnY);
    okBttn->AddHotkey(eVKey::H3VK_ESCAPE);
    // okBttn->SetX(helpWidth - offset - okBttnDef->widthDEF - 1);
    auto customBttn = dlg.CreateCustomButton(offset, okBttnY, 1, siteBttn->GetName(), HelpButtonProc, 0, 0);

    auto text = dlg.CreateText(offset, okBttnY, helpWidth - offset - okBttnX, okBttnDef->heightDEF,
                               EraJS::read("RMG.text.dlg.buttons.help.name"), NH3Dlg::Text::MEDIUM, eTextColor::REGULAR,
                               0, eTextAlignment::MIDDLE_CENTER, -1);

    customBttn->SetWidth(text->GetWidth());
    customBttn->SetHeight(text->GetHeight());
    H3RGB565 color(H3RGB888::Highlight());

    auto frame = dlg.CreateFrame(text, color, 0, 1);
    frame->DeActivate();
    text->DeActivate();

    const int textHeight = helpHeight - 2 * (offset + 1) - okBttnDef->heightDEF;

    dlg.CreateScrollableText(mes.String(), offset, offset, textWidth, textHeight, NH3Dlg::Text::MEDIUM,
                             eTextColor::SILVER);

    return dlg.Start();
}

BOOL RMG_SettingsDlg::DialogProc(H3Msg &msg)
{
    const int itemId = msg.itemId;

    if (m_currentPage)
    {

        if (m_currentPage->Proc(msg))
        {
            return 0;
        }
    }

    if (msg.IsLeftClick())
    {
        auto *mapObjectsPage = dynamic_cast<ObjectsPage *>(m_currentPage);

        const UINT pageId = msg.itemId - NItemIDs::PAGE_0;
        switch (msg.itemId)
        {

            // operate page clicks
        case NItemIDs::PAGE_0:
        case NItemIDs::PAGE_1:
        case NItemIDs::PAGE_2:
        case NItemIDs::PAGE_3:

            if (pageId < m_pages.size())
            {
                if (SetActivePage(m_pages[pageId]))
                {
                    Redraw();
                }
            }
            break;

        case NItemIDs::CANCEL:
            this->OnCancel();
            break;

        case NItemIDs::OK:
            this->OnOK();
            break;

        case NItemIDs::DEFAULT:
            if (mapObjectsPage)
            {
                m_currentPage->SetDefault();
                mapObjectsPage->ResetSortingState();
                Redraw();
            }
            break;

        case NItemIDs::RANDOM:
            // call virtual randomizer
            if (mapObjectsPage)
            {
                m_currentPage->SetRandom(msg);
                mapObjectsPage->ResetSortingState();
                Redraw();
            }
            break;
        case NItemIDs::HELP:
            this->OnHelp();
            break;

        default:
            break;
        }
    }
    else if (msg.IsLeftDown())
    {
        // if (RemoveEditsFocus(true))
        {
        }
    }

    // show message box at rmc for items which have hints

    if (msg.IsRightClick())
    {
        if (H3DlgItem *item = GetH3DlgItem(msg.itemId))
        {
            if (LPCSTR rmcHint = *reinterpret_cast<LPCSTR *>(reinterpret_cast<char *>(item) + 0x24))
            {
                H3Messagebox::RMB(rmcHint);
            }
        }
    }
    if (msg.IsAltPressed())
    {
        // rei msg.GetDlg().onok
    }

    // hintbar procedure

    if (hintBar && hintBar->IsVisible())
    {
        hintBar->ShowHint(&msg);
    }

    return 0;
}

BOOL RMG_SettingsDlg::RemoveEditsFocus(const BOOL save) const noexcept
{
    if (auto objPage = dynamic_cast<ObjectsPage *>(m_currentPage))
    {
        // loop all the panels
        for (auto panel : objPage->objectsPanels)
        {
            // if any of them was foucused
            if (panel->UnfocusEdits(save))
            {
                // return that as flag
                return true;
            }
        }
    }
    // this->m_currentPage;
    return false;
}
VOID RMG_SettingsDlg::OnOK()
{

    RemoveEditsFocus(true);

    if (!SaveRMGObjectsInfo(true))
        H3Messagebox(EraJS::read("RMG.text.dlg.iniError"));
    WriteIniDlgSettings();

    this->Stop();
}

VOID RMG_SettingsDlg::OnCancel()
{
    RemoveEditsFocus(false);
    WriteIniDlgSettings();
    this->Stop();
}
BOOL RMG_SettingsDlg::ReadIniDlgSettings() noexcept
{
    BOOL result = Era::ReadStrFromIni("lastPageId", SETTINGS_INI_SECTION, INI_FILE_PATH, h3_TextBuffer);
    if (result)
    {
        m_lastPageId = atoi(h3_TextBuffer);
    }
    //  Era::ReadStrFromIni("DlgSettings", "settingsVersion", INI_FILE_PATH, h3_TextBuffer);
    //  {
    //      float localVersion = atof(h3_TextBuffer);
    //      float dlgSettingsVersion = EraJS::readFloat("RMG.dlg.settingsVersion");
    // if (localVersion != dlgSettingsVersion)
    //{
    //	result = false;
    //}
    //  }
    for (size_t i = 0; i < 4; i++)
    {
        // if (Era::ReadStrFromIni("DlgSettings", "lastPageId", m_iniPath, h3_TextBuffer))
        //{
        //	m_lastPageId = atoi(h3_TextBuffer);
        // }
    }

    return result;
}
BOOL RMG_SettingsDlg::WriteIniDlgSettings() const noexcept
{
    BOOL result =
        Era::WriteStrToIni("lastPageId", std::to_string(m_lastPageId).c_str(), SETTINGS_INI_SECTION, INI_FILE_PATH);
    if (result)
    {
        result = Era::SaveIni(INI_FILE_PATH);
    }

    return result;
}

BOOL RMG_SettingsDlg::SaveRMGObjectsInfo(const BOOL saveIni) const noexcept
{

    constexpr int SIZE = 5;

    bool success = true;

    const int zoneType = 0;

    if (saveIni)
    {
        Era::ClearIniCache(RMGObjectInfo::INI_FILE_PATH);
        DeleteFileA(RMGObjectInfo::INI_FILE_PATH);
    }

    for (auto &page : m_pages)
    {
        // iterate pages
        if (auto *mapObjectPage = dynamic_cast<ObjectsPage *>(page))
        { // try to cast to map object page

            for (RMGDlgObject &object : mapObjectPage->rmgDlgObjects)
            {
                auto &info = object.objectInfo;
                info.Clamp();
                info.MakeReal();

                if (saveIni)
                {
                    success = info.WriteToINI();
                }
            }
        }
    }

    return saveIni ? Era::SaveIni(RMGObjectInfo::INI_FILE_PATH) : success;
}

void RMG_SettingsDlg::ObjectsPage::CreateVerticalScrollBar()
{
    const int layoutHeight = RMG_SettingsDlg::instance->heightDlg - 40;
    constexpr int itemId = 100;

    const int PANELS_NUM = objectsPanels.size();
    const int DATA_NUM = rmgDlgObjects.size();
    // if needed m_size > space -> create custom scroll bar
    if (PANELS_NUM < DATA_NUM)
    {
        auto lastPanel = objectsPanels.back();
        const int scrollBarY = objectsPanels.front()->backgroundPcx->GetY();
        verticalScrollBar = H3DlgScrollbar::Create(
            dlg->widthDlg - 24, scrollBarY, 16,
            lastPanel->backgroundPcx->GetY() + lastPanel->backgroundPcx->GetHeight() - scrollBarY, itemId,
            DATA_NUM + 1 - PANELS_NUM, VerticalScrollBarProc, true, 1, true);
        if (verticalScrollBar)
        {
            dlg->AddItem(verticalScrollBar);
        }
    }
}

void RMG_SettingsDlg::ObjectsPage::CreateHorizontalScrollBar()
{
    // if we have some panels
    if (objectsPanels.size())
    {
        // if panels have > 4 input edits
        auto &firstPanel = objectsPanels.front();
        int editsNum = 0;
        for (const auto edit : firstPanel->edits)
        {
            editsNum++;
        }
        if (editsNum > 4)
        {
            constexpr int padding = 22;
            const int x = firstPanel->objectNameItem->GetX() + firstPanel->objectNameItem->GetWidth() + padding;
            auto &lastPanel = objectsPanels.back();

            const int y = lastPanel->objectNameItem->GetY() + lastPanel->objectNameItem->GetHeight() + 5;

            horizontalScrollBar = H3DlgScrollbar::Create(x, y, lastPanel->backgroundPcx->GetWidth() - x + padding, 16,
                                                         101, 5, HorizontalScrollBarProc, true, 1, true);
            if (horizontalScrollBar)
            {
                dlg->AddItem(horizontalScrollBar);
            }
        }
    }
}

RMG_SettingsDlg::BanksPage::BanksPage(H3DlgCaptionButton *captionbttn, const std::vector<GraphicalAttributes> &data,
                                      const BOOL ignoreSubtypes)
    : ObjectsPage(captionbttn, data, ignoreSubtypes)
{
}

RMG_SettingsDlg::ObjectsPanel::ObjectsPanel(const int x, const int y, Page *parent)
    : x(x), y(y), visible(false), parentPage(parent)
{
    auto *dlg = parentPage->dlg;
    id++;
    int itemId = 100 * (id + 1);

    const int FIELD_WIDTH = dlg->widthDlg - x * 2;

    constexpr int FIELD_HEIGHT = 50;
    H3RGB565 color(H3RGB888::Highlight());

    constexpr int PCX_WIDTH = 44;
    constexpr int PCX_HEIGHT = 44;
    // return;
    constexpr int columns = 2;

    // First Ceate panel Background + frame
    // add item into page list

    backgroundPcx = H3DlgPcx16::Create(x, y, FIELD_WIDTH, FIELD_HEIGHT, itemId++, "RMG_back.pcx");
    if (backgroundPcx)
    {
        items.emplace_back(backgroundPcx);
        items.emplace_back(H3DlgFrame::Create(backgroundPcx, color, itemId++));
    }

    LPCSTR chbxName = "RMG_chbx.def";

    enabledCheckBox = H3DlgDef::Create(x + 3, y + 8, itemId++, chbxName, 0, 1);
    enabledCheckBox->SetHint(EraJS::read("RMG.text.dlg.objectPanel.checkbox.hint"));
    items.emplace_back(enabledCheckBox);

    const int DLG_OBJ_PCX_X_OFFSET = enabledCheckBox->GetWidth() + enabledCheckBox->GetX();

    items.emplace_back(H3DlgFrame::Create(DLG_OBJ_PCX_X_OFFSET + 3, y + 4, 1, FIELD_HEIGHT - 8, itemId++, color));

    pictureItem = H3DlgPcx16::Create(DLG_OBJ_PCX_X_OFFSET + 10, y + 3, PCX_WIDTH, PCX_HEIGHT, itemId++,
                                     0); // create dlg pcx w/o picture
    items.emplace_back(pictureItem);

    const int DLG_OBJ_NAME_X_OFFSET = PCX_WIDTH + pictureItem->GetX();

    constexpr int textPadding = 20;
    objectNameItem = H3DlgText::Create(DLG_OBJ_NAME_X_OFFSET + 7, y, (FIELD_WIDTH - PCX_WIDTH) / 3 - textPadding,
                                       FIELD_HEIGHT, h3_NullString, h3::NH3Dlg::Text::MEDIUM, 1, itemId++);
    items.emplace_back(objectNameItem);

    const int DLG_EDITS_X_OFFSET = objectNameItem->GetX() + objectNameItem->GetWidth() + textPadding;

    items.emplace_back(H3DlgFrame::Create(DLG_EDITS_X_OFFSET - 5, y, 4, FIELD_HEIGHT, itemId++, color));

    constexpr int MAX_TEXT_LENGTH = 6;
    constexpr int EDITS_SIZE = 4;

    bool readSucces = false;

    for (size_t i = 0; i < EDITS_SIZE; i++)
    {
        if (edits[i] = H3DlgEdit::Create(DLG_EDITS_X_OFFSET + 100 * i + 22, y + 15, 52, 22, MAX_TEXT_LENGTH,
                                         h3_NullString, h3::NH3Dlg::Text::MEDIUM, 1, eTextAlignment::MIDDLE_CENTER,
                                         "RMG_edit.pcx", itemId++, 4, 1, 1))
            items.emplace_back(edits[i]);
    }
    const int defaultButtonX = items.back()->GetX() + items.back()->GetWidth() + 8;
    //    const int defaultButtonX = items.back()->GetX() + 100;
    defaultButton = H3DlgDefButton::Create(defaultButtonX, y + 15, itemId++, "RMG_dfmn.def", 0, 1, false, false);
    defaultButton->SetHints(EraJS::read("RMG.text.dlg.objectPanel.default.hint"),
                            EraJS::read("RMG.text.dlg.objectPanel.default.rmc"), false);

    // items.emplace_back(H3DlgFrame::Create(setDefaultButton, color, itemId++, 1));

    items.emplace_back(defaultButton);
    for (auto &wgt : items)
        dlg->AddItem(wgt);
}

RMG_SettingsDlg::ObjectsPanel::~ObjectsPanel()
{
    id--;
    // remove temporary created resized pcx from object cause they are destoyed in the ~Objects()
    pictureItem->SetPcx(nullptr);
}

void RMG_SettingsDlg::ObjectsPanel::SetVisible(const BOOL state) noexcept
{
    visible = state;
    for (auto &wgt : items)
    {
        visible ? wgt->ShowActivate() : wgt->HideDeactivate();
    }
    for (auto &edit : edits)
    {
        edit->SetAutoredraw(state);
    }
}

void RMG_SettingsDlg::ObjectsPanel::SetObject(RMGDlgObject *mapObject) noexcept
{
    if (mapObject != nullptr)
    {
        rmgObject = mapObject;
        pictureItem->SetPcx(rmgObject->graphicalAttributes->objectPcx);
        objectNameItem->SetText(rmgObject->objectInfo.GetName());
    }

    // refresh displayed info
    ObjectInfoToPanelInfo();
}

const BOOL RMG_SettingsDlg::ObjectsPanel::UnfocusEdits(const BOOL saveChanges) noexcept
{

    for (auto edit : edits)
    {
        if (*reinterpret_cast<char *>(reinterpret_cast<char *>(edit) + 0x6D))
        {
            *reinterpret_cast<char *>(reinterpret_cast<char *>(edit) + 0x6D) = false;

            // edit->SetFocus(false);
            if (saveChanges)
            {
                PanelInfoToObjectInfo();
            }
            else
            {
                ObjectInfoToPanelInfo();
            }

            // Redraw();
            return true;
        }
    }
    return false;
}
void RMG_SettingsDlg::ObjectsPanel::ObjectInfoToPanelInfo() noexcept
{
    // if there is assigned data
    if (rmgObject != nullptr)
    {
        enabledCheckBox->SetFrame(rmgObject->objectInfo.enabled);

        constexpr int SIZE = 4;
        for (size_t i = 0; i < SIZE; ++i)
        {
            if (edits[i])
            {
                edits[i]->SetText(std::to_string(rmgObject->objectInfo.data[i + 1]).c_str());
            }
        }
    }
}
void RMG_SettingsDlg::ObjectsPanel::PanelInfoToObjectInfo() noexcept
{
    if (rmgObject != nullptr)
    {
        auto &objectInfo = rmgObject->objectInfo;
        // objectInfo.enabled = enabledCheckBox->GetFrame();

        constexpr int SIZE = 4;
        for (size_t i = 0; i < SIZE; ++i)
        {
            if (edits[i])
            {
                objectInfo.data[i + 1] = edits[i]->GetString()->ToSigned();
            }
        }
        objectInfo.Clamp();
    }
}

RMG_SettingsDlg::Page::~Page()
{
}

RMG_SettingsDlg::ObjectsPage::~ObjectsPage()
{
    // call all the items dtors and clear vectors
    for (auto &panel : objectsPanels)
    {

        delete panel;
        panel = nullptr;
    }

    delete pageHeader;
    objectsPanels.clear();

    //// clear rmgDlgObjects pcx
    // for (auto &object : rmgDlgObjects)
    //{
    //     // if (object.objectPictures.size())
    //     //{
    //     //	for (auto& pcx : object.objectPictures)
    //     //	{
    //     //		pcx.objectPcx->Destroy();
    //     //		pcx.objectPcx = nullptr;

    //    //	}
    //    //}

    //    if (auto &pcx = object.graphicalAttributes->objectPcx)
    //    {
    //        //  pcx = nullptr;
    //    }
    //}

    rmgDlgObjects.clear();
}

H3Msg *__stdcall RMG_SettingsDlg::H3DlgEdit__TranslateInputKey(HiHook *h, H3InputManager *inpt, H3Msg *msg)
{
    auto result = THISCALL_2(H3Msg *, h->GetDefaultFunc(), inpt, msg);

    if (instance && instance->blockLettersInput)
    {
        isDlgTextEditInput = true;
        // block inputing any text except numbers and backspaces
        if (msg->subtype != 127 && (msg->subtype < 48 || msg->subtype > 57)
            //	|| msg->subtype > 60 && msg->subtype < 122
            //	|| msg->subtype >255
        )

            // replace with 0
            msg->subtype = h3::eMsgSubtype(0);
    }

    return msg;
}
int __fastcall RMG_SettingsDlg::RMGDlgOptionsButtonProc(void *msg)
{
    if (H3Msg *h3msg = static_cast<H3Msg *>(msg))
    {
        if (h3msg->IsLeftClick())
        {
            RMG_SettingsDlg dlg(800, 600);
            dlg.Start();
        }
    }

    return true;
}
RMG_SettingsDlg::~RMG_SettingsDlg()
{

    blockLettersInput = false;

    for (auto &p : m_pages)
    {
        delete p;
        p = nullptr;
    }
    m_pages.clear();

    RMG_SettingsDlg::Page::dlg = nullptr;
    instance = nullptr;
}

void RMG_SettingsDlg::ObjectsPage::SetVisible(bool state)
{
    // hide items step by step
    pageHeader->SetVisible(state);

    if (verticalScrollBar)
    {
        state ? verticalScrollBar->ShowActivate() : verticalScrollBar->HideDeactivate();
    }
    if (horizontalScrollBar)
    {
        state ? horizontalScrollBar->ShowActivate() : horizontalScrollBar->HideDeactivate();
    }

    auto time = GetTime();
    for (auto &p : objectsPanels)
    {
        p->SetVisible(state);
        p->lastChangedPictureTime = time;
    }

    visible = state;
}

void RMG_SettingsDlg::ObjectsPage::SaveData()
{
    for (auto &panel : objectsPanels)
    {
        panel->rmgObject->objectInfo.Clamp();

        panel->rmgObject->objectInfo.enabled = panel->enabledCheckBox->GetFrame();

        for (size_t i = 0; i < 4; i++)
        {
            panel->rmgObject->objectInfo.data[i + 1] = panel->edits[i]->GetH3String().ToSigned();
        }
    }
}

void RMG_SettingsDlg::ObjectsPage::SetRandom(const H3Msg &msg)
{

    const BOOL affectOnlyEnabled = msg.IsCtrlPressed();
    if (msg.ShiftPressed() || msg.AltPressed() || msg.CtrlPressed() || msg.IsAltPressed() || msg.IsCtrlPressed() ||
        msg.IsShiftPressed())
    {

        // ECHO();
    }
    for (auto &rmgObject : rmgDlgObjects)
    {
        rmgObject.objectInfo.SetRandom();
    }
    //
    FillObjects(firstItemCount);
}

void RMG_SettingsDlg::ObjectsPage::SetDefault()
{
    for (auto &object : rmgDlgObjects)
    {
        object.objectInfo.RestoreDefault();
    }
    FillObjects(firstItemCount);
}

RMG_SettingsDlg::ObjectsPage::ObjectsPage(H3DlgCaptionButton *captionbttn,
                                          const std::vector<GraphicalAttributes> &attributes, const BOOL ignoreSubtypes)
    : Page(captionbttn), ignoreSubtypes(ignoreSubtypes), objectAttributes(&attributes)

{

    this->ignoreSubtypes = false;

    // resizedPcx16.RemoveAll();

    const size_t SIZE = attributes.size();

    rmgDlgObjects.reserve(SIZE);

    // Crreate Object Parameteres

    // create local rmgDlgObjects
    for (size_t i = 0; i < SIZE; i++)
    {
        auto *attr = const_cast<GraphicalAttributes *>(&attributes[i]);
        rmgDlgObjects.emplace_back(attr);
    }
    rmgDlgObjects.shrink_to_fit();

    sorting::SortRmgObjects(rmgDlgObjects, sorting::eSorting::BY_TYPE);
    lastSorting.type = sorting::eSorting::BY_TYPE;

    // auto* dlg = dlg;
    constexpr int x = 24;
    constexpr int y = 55;

    const int PANEL_WINDTH = dlg->widthDlg - x * 2;

    constexpr int HEADER_HEIGHT = 38;

    // create page header
    pageHeader = new PageHeader(x, y, PANEL_WINDTH, HEADER_HEIGHT, rmgDlgObjects.size());

    // later add all its items into main dlg
    for (auto it : pageHeader->items)
        dlg->AddItem(it);

    // prepare creating page dlg Items aka as CreateDlgPanels
    const size_t objectsNum = rmgDlgObjects.size();

    const size_t panelsNum = Clamp(0, objectsNum, 8);
    for (size_t i = 0; i < panelsNum; i++)
    {
        objectsPanels.emplace_back(new ObjectsPanel(x, i * 55 + 100, this));
    }

    CreateVerticalScrollBar();
    CreateHorizontalScrollBar();
}

void RMG_SettingsDlg::ObjectsPage::FillObjects(int firstItem)
{
    const size_t PANELS_NUM = objectsPanels.size();
    const size_t DATA_NUM = rmgDlgObjects.size();

    for (size_t i = 0; i < PANELS_NUM; i++)
    {
        const size_t dataIndex = i + firstItem;
        const bool inRange = dataIndex < DATA_NUM;

        // set new data if object is in range
        if (inRange)
        {
            objectsPanels[i]->UnfocusEdits(false);
            objectsPanels[i]->SetObject(&rmgDlgObjects[dataIndex]);
        }
    }
}

BOOL RMG_SettingsDlg::ObjectsPage::ShowObjectExtendedInfo(const ObjectsPanel *panel, const H3Msg &msg) const noexcept
{
    auto *clickedItem = dlg->GetH3DlgItem(msg.itemId);

    if (clickedItem == panel->pictureItem || clickedItem == panel->objectNameItem)
    {
        const auto &rmgObject = panel->rmgObject;

        H3String str;

        const bool result = extender::ObjectExtenderManager::ShowObjectExtendedInfo(
            rmgObject->objectInfo, rmgObject->graphicalAttributes->attributes, str);

        if (!result)
        {
            H3Messagebox::RMB(str.String());
        }

        return true;
    }

    return false;
    // H3PictureCategories pic();
}
BOOL RMG_SettingsDlg::BanksPage::ShowObjectExtendedInfo(const ObjectsPanel *panel, const H3Msg &msg) const noexcept
{
    BOOL result = ObjectsPage::ShowObjectExtendedInfo(panel, msg);
    return result;
    /**/
    BOOL resultA = true;
    const auto rmgObject = panel->rmgObject;
    const int cbID =
        cbanks::CreatureBanksExtender::GetCreatureBankType(rmgObject->objectInfo.type, rmgObject->objectInfo.subtype);

    if (cbID != eObject::NO_OBJ)
    {
        const auto &cbSetup = H3CreatureBankSetup::Get()[cbID];

        H3Dlg ddl(500, 500);

        for (size_t i = 0; i < 4; i++)
        {
            if (cbSetup.states[i].chance > 0)
            {
                const int _y = 80 * i + 20;
                for (size_t j = 0; j < 7; j++)
                {
                    const int montype = cbSetup.states[i].guardians.type[j];
                    const int monNum = cbSetup.states[i].guardians.count[j];
                    if (montype != eCreature::UNDEFINED)
                    {
                        const int _x = 60 * j + 20;
                        auto def = ddl.CreateDef(_x, _y, -1, h3::NH3Dlg::Assets::CREATURE_SMALL, montype + 2);
                        ddl.CreateText(_x, _y + def->GetHeight() + 2, def->GetWidth(), 20,
                                       Era::IntToStr(monNum).c_str(), NH3Dlg::Text::SMALL, eTextColor::WHITE, -1);
                    }
                }
            }
        }
        ddl.CreateText(50, 400, 400, 20, rmgObject->objectInfo.GetName(), NH3Dlg::Text::SMALL, eTextColor::WHITE, -1);

        ddl.RMB_Show();
    }
    return resultA;
}

RMG_SettingsDlg::BanksPage::~BanksPage()
{
}

BOOL RMG_SettingsDlg::ObjectsPage::Proc(H3Msg &msg)
{
    const int itemId = msg.itemId;
    int needDlgRedraw = false;
    //	Era::ExecErmCmd("IF:L^^");

    BOOL result = false;

    const DWORD time = GetTime();
    for (auto &objectsPanel : objectsPanels)
    {
        if (objectsPanel->visible && objectsPanel->rmgObject)
        {
            auto rmgDlgObject = objectsPanel->rmgObject;
            // if pcx needs redraw
            if (rmgDlgObject && time - objectsPanel->lastChangedPictureTime > REFRESH_RATE_FREQUENCY)
            {
                objectsPanel->lastChangedPictureTime = time;

                if (rmgDlgObject->SwitchToNextPicture())
                {
                    auto &dlgPcx = objectsPanel->pictureItem;
                    dlgPcx->SetPcx(rmgDlgObject->graphicalAttributes->objectPcx);
                    {
                        dlgPcx->Draw();
                        dlgPcx->Refresh();
                    }
                }
            }
        }
    }

    if (itemId && msg.command == eMsgCommand::ITEM_COMMAND)
    {

        // store clicked item * in the var
        auto *clickedItem = dlg->ItemAtPosition(msg);

        // check if it is a Header Click
        const bool itemIsInHeader =
            std::find(pageHeader->items.begin(), pageHeader->items.end(), clickedItem) != pageHeader->items.end();
        //	H3Messagebox(Era::IntToStr(int(pageHeader)).c_str());

        if (itemIsInHeader)
        {

            // for left click (not release)
            if (msg.IsLeftDown())
            {
                P_SoundManager->ClickSound();
                sorting::eSorting sort = sorting::NONE;

                if (itemId >= NItemIDs::TYPE && itemId <= NItemIDs::HEADER_LAST)
                {
                    sort = sorting::eSorting(sorting::BY_TYPE + itemId - NItemIDs::TYPE);
                    bool reverse = lastSorting.type == sort;
                    lastSorting.type = reverse ? -1 : sort;
                    lastSorting.isReverse = reverse;

                    for (auto *arrow : pageHeader->arrows)
                    {
                        const UINT frameId = arrow->GetID() != clickedItem->GetID() ? 2 : reverse ? 0 : 2;
                        arrow->SetFrame(frameId);
                    }

                    sorting::SortRmgObjects(rmgDlgObjects, sort, reverse);

                    FillObjects(firstItemCount);
                    needDlgRedraw = true;
                }
            }
        }
        // otherwise loop all the panels
        else
        {
            const int doMassEnable = msg.CtrlPressed();
            const int doMassReverse = msg.AltPressed();
            int needMassRedraw = false;
            // iterate all "Page" dlgPanels
            for (const auto &objectsPanel : objectsPanels)
            {
                // check if item is in dlg panel

                // if that panel is visible
                if (objectsPanel->visible)
                {

                    // check if panel has data
                    // check if clicked item is from that panel

                    if (objectsPanel->rmgObject &&
                        objectsPanel->items.end() !=
                            std::find(objectsPanel->items.begin(), objectsPanel->items.end(), clickedItem))
                    {
                        if (msg.IsRightClick())
                        {
                            result = this->ShowObjectExtendedInfo(objectsPanel, msg);
                            break;
                        }
                        if (msg.subtype == eMsgSubtype::LBUTTON_DOWN &&
                            (objectsPanel->enabledCheckBox == clickedItem || objectsPanel->pictureItem == clickedItem ||
                             objectsPanel->objectNameItem == clickedItem))
                        {
                            P_SoundManager->ClickSound();

                            const int newSetup = objectsPanel->rmgObject->objectInfo.enabled ^= true;
                            if (doMassEnable || doMassReverse)
                            {
                                for (auto &obj : this->rmgDlgObjects)
                                {
                                    obj.objectInfo.enabled = doMassReverse ? obj.objectInfo.enabled ^ true : newSetup;
                                }
                                needMassRedraw = true;
                                if (doMassReverse)
                                {
                                    objectsPanel->rmgObject->objectInfo.enabled ^= true;
                                }
                            }
                            else
                            {

                                objectsPanel->UnfocusEdits(true);
                                objectsPanel->ObjectInfoToPanelInfo();
                                //	objectsPanel->PanelInfoToObjectInfo();
                            }
                            needMassRedraw = true;

                            needDlgRedraw = true;
                        }

                        //	if (objectsPanel->mapLimitEdit == clickedItem)
                        constexpr int SIZE = 4;

                        for (size_t i = 0; i < SIZE; ++i)
                        {
                            //	objectsPanel->rmgObject->objectInfo.data[i + 1] =
                            // objectsPanel->edits[i]->GetH3String().ToSigned();
                        }
                        if (msg.subtype == eMsgSubtype::LBUTTON_CLICK && clickedItem == objectsPanel->defaultButton)
                        {

                            objectsPanel->UnfocusEdits(true);
                            objectsPanel->rmgObject->objectInfo.RestoreDefault();
                            objectsPanel->ObjectInfoToPanelInfo();
                            needDlgRedraw = true;
                        }
                    }
                    else if (isDlgTextEditInput)
                    {
                        // objectsPanel->UnfocusEdits(1);

                        constexpr int SIZE = 4;
                        for (auto &i : objectsPanel->edits)
                        {
                            if (*reinterpret_cast<char *>(reinterpret_cast<char *>(i) + 0x6D))
                            {
                                //   needDlgRedraw = true;
                                objectsPanel->PanelInfoToObjectInfo();
                                objectsPanel->ObjectInfoToPanelInfo();

                                //  Era::ExecErmCmd("IF:L^^");
                                isDlgTextEditInput = false;
                                needDlgRedraw = true;
                                break;
                            }
                        }
                        //  Era::NotifyError(Era::IntToStr(int(clickedItem)).c_str());
                    }
                    if (needDlgRedraw)
                    {
                        break;
                    }
                }

                // break;
            }
            if (needMassRedraw)
            {
                for (const auto &dlgPanel : objectsPanels)
                {
                    dlgPanel->UnfocusEdits(true);

                    dlgPanel->ObjectInfoToPanelInfo();
                    //	break;
                }
            }
        }
    }

    if (needDlgRedraw)
    {
        dlg->Redraw();
    }

    return result;
}

// H3Dlgsc
RMG_SettingsDlg::Page::Page(H3DlgCaptionButton *captionbttn) : captionbttn(captionbttn), verticalScrollBar(nullptr)
{
    // resizedPcx16.RemoveAll();
    id = dlg->m_pages.size();
}

void RMG_SettingsDlg::ObjectsPage::ResetSortingState()
{
    for (auto *arrow : pageHeader->arrows)
    {
        arrow->Cast<H3DlgDef>()->SetFrame(2);
    }
    lastSorting.type = sorting::NONE;
    lastSorting.isReverse = false;
}

void __fastcall RMG_SettingsDlg::ObjectsPage::VerticalScrollBarProc(INT32 tick, H3BaseDlg *dlg)
{

    if (auto *objDlg = dynamic_cast<RMG_SettingsDlg *>(dlg))
    {
        if (auto currentPage = objDlg->m_currentPage)
        {
            currentPage->firstItemCount = tick;
            currentPage->FillObjects(tick);
            objDlg->Redraw();
        }
    }
}
void __fastcall RMG_SettingsDlg::ObjectsPage::HorizontalScrollBarProc(INT32 tick, H3BaseDlg *dlg)
{

    if (auto *objDlg = dynamic_cast<RMG_SettingsDlg *>(dlg))
    {
        // if (auto currentPage = objDlg->m_currentPage)
        //{
        //	currentPage->firstItemCount = tick;
        //	currentPage->FillObjects(tick);
        //	objDlg->Redraw();
        // }
    }
}

BOOL RMG_SettingsDlg::OnCreate()
{
    const size_t size = m_pages.size();

    if (size)
    {

        for (Page *page : m_pages)
        {
            page->FillObjects();
            page->SetVisible(false);
        }

        Page *page = m_lastPageId < size ? m_pages[m_lastPageId] : *m_pages.begin();
        SetActivePage(page);
    }

    // remove local settings for the objects that have same data as in default
    Era::ReadStrFromIni("version", SETTINGS_INI_SECTION, INI_FILE_PATH, h3_TextBuffer);
    {
        float localVersion = atof(h3_TextBuffer);

        if (localVersion != SETTINGS_VERSION)
        {
            //     H3Messagebox();
            // return false;
        }
    }

    return true;
}
BOOL RMG_SettingsDlg::SetActivePage(Page *page) noexcept
{
    bool result = page != m_currentPage;
    if (result)
    {

        for (auto &caption : captionButtons)
        {
            caption->SetFrame(0);
            caption->SetClickFrame(1);
        }

        if (m_currentPage)
        {
            RemoveEditsFocus(true);
            m_currentPage->SetVisible(false);
        }

        m_currentPage = page;
        if (page)
        {
            page->SetVisible(true);
            page->captionbttn->SetFrame(2);
            page->captionbttn->SetClickFrame(3);
            m_lastPageId = page->id;
        }
        else
        {
            m_lastPageId = 0;
        }
    }

    return result;
}

RMGDlgObject::RMGDlgObject(GraphicalAttributes *graphicalAttributes) : graphicalAttributes(graphicalAttributes)
{
    // Get data from global array

    objectInfo = RMGObjectInfo::CurrentObjectInfo(graphicalAttributes->attributes->type,
                                                  graphicalAttributes->attributes->subtype);
}

BOOL RMGDlgObject::SwitchToNextPicture() noexcept
{
    if (graphicalAttributes && graphicalAttributes != graphicalAttributes->next)
    {
        graphicalAttributes = graphicalAttributes->next;
        return true;
    }

    return false;
}

RMG_SettingsDlg::ObjectsPage::PageHeader::PageHeader(const int x, const int y, const int width, const int height,
                                                     const int objectsNum)
{
    items.clear();
    items.reserve(15);
    auto dlg = RMG_SettingsDlg::Page::dlg;

    // First create columns back
    backgroundPcx = H3DlgPcx16::Create(x, y, width, height, -1, "RMG_back.pcx");

    items.emplace_back(backgroundPcx);

    H3RGB565 color(H3RGB888::Highlight());
    items.emplace_back(H3DlgFrame::Create(backgroundPcx, color));
    // dlg->AddItem(H3DlgFrame::Create(backgroundPcx, color));

    // LPCSTR(0x6839DC);
    LPCSTR defName = 1 ? LPCSTR(0x6839DC) : "ScnrBDn.def";

    // create frame and description for the map rmgDlgObjects fields

    constexpr int _x[] = {20, 88, 350, 450, 550, 650};
    constexpr int _w[] = {65, 238, 65, 65, 65, 65};
    constexpr int _arrowOffset[] = {16, 20, 5, 5, 5, 5};

    // H3RGB565 color(H3RGB888::Highlight());

    for (size_t i = 0; i < 6; i++)
    {

        H3DlgText *text = H3DlgText::Create(_x[i] + x, y + 2, _w[i], height - 4,
                                            EraJS::read(H3String::Format("RMG.text.dlg.header.%d.name", i).String()),
                                            NH3Dlg::Text::SMALL, 1, rmgdlg::NItemIDs::HEADER_FIRST + i);
        if (i == 1)
        {
            //		H3String modifiedText = text->GetH3String();
            //		modifiedText.Append(H3String::Format("%s (%d)", text->GetH3String().String(), objectsNum));
            text->SetText(H3String::Format("%s (%d)", text->GetH3String().String(), objectsNum));
        }
        text->SetHints(EraJS::read(H3String::Format("RMG.text.dlg.header.%d.hint", i).String()),
                       EraJS::read(H3String::Format("RMG.text.dlg.header.%d.rmc", i).String()), false);
        auto *frame = H3DlgFrame::Create(text, color, 0, 1);
        items.emplace_back(frame);
        items.emplace_back(text);

        H3DlgDef *arrow = H3DlgDef::Create(text->GetX() + text->GetWidth() - _arrowOffset[i],
                                           y + 4 + ((height - 16) >> 1), text->GetID(), defName, 2);
        arrows.emplace_back(arrow);
        items.emplace_back(arrow);
    }
}

void RMG_SettingsDlg::ObjectsPage::PageHeader::SetVisible(bool state)
{
    for (auto &p : items)
        state ? p->ShowActivate() : p->HideDeactivate();

    visible = state;
}

RMG_SettingsDlg::MiscPage::MiscPage(H3DlgCaptionButton *captionbttn, const std::vector<GraphicalAttributes> &data)
    : ObjectsPage(captionbttn, data)
{
}

RMG_SettingsDlg::MiscPage::~MiscPage()
{
}

RMG_SettingsDlg::DwellingsPage::DwellingsPage(H3DlgCaptionButton *captionbttn,
                                              const std::vector<GraphicalAttributes> &data)
    : ObjectsPage(captionbttn, data)
{
}

RMG_SettingsDlg::DwellingsPage::~DwellingsPage()
{
}

std::vector<GraphicalAttributes> *RMG_SettingsDlg::GetObjectAttributesVector(const int objectType) noexcept
{

    // spacing is added to show not adjusted object types
    switch (objectType)
    {
    case eObject::CREATURE_BANK:
    case eObject::DERELICT_SHIP:
    case eObject::DRAGON_UTOPIA:
    case eObject::CRYPT:
    case eObject::SHIPWRECK:
        return &m_creatureBanks;
    case eObject::ARENA:
        /*
         * eObject::KEYMASTER, @todo: place into another page
         */
    case eObject::BLACK_MARKET:

    case eObject::CARTOGRAPHER:
    case eObject::SWAN_POND:

    case eObject::CORPSE:
    case eObject::MARLETTO_TOWER:

    case eObject::FAERIE_RING:

    case eObject::GARDEN_OF_REVELATION:
    case eObject::IDOL_OF_FORTUNE:

    case eObject::LIBRARY_OF_ENLIGHTENMENT:

    case eObject::SCHOOL_OF_MAGIC:

    case eObject::MAGIC_WELL:

    case eObject::MERCENARY_CAMP:
    case eObject::STAR_AXIS:

    case eObject::FOUNTAIN_OF_FORTUNE:
    case eObject::FOUNTAIN_OF_YOUTH:

    case eObject::HILL_FORT:

    case eObject::LEAN_TO:
    case eObject::MAGIC_SPRING:

    case eObject::MYSTICAL_GARDEN:
    case eObject::OASIS:
    case eObject::OBELISK:
    case eObject::REDWOOD_OBSERVATORY:

    case eObject::PILLAR_OF_FIRE:

    case eObject::RALLY_FLAG:

    case eObject::REFUGEE_CAMP:

    case eObject::SANCTUARY:
    case eObject::SCHOLAR:
        // case eObject::SEA_CHEST:

    case eObject::SEER_HUT:

        // case eObject::SHIPWRECK_SURVIVOR:

    case eObject::SHRINE_OF_MAGIC_INCANTATION:
    case eObject::SHRINE_OF_MAGIC_GESTURE:
    case eObject::SHRINE_OF_MAGIC_THOUGHT:

    case eObject::SIRENS:

    case eObject::STABLES:
    case eObject::TAVERN:
    case eObject::TEMPLE:
    case eObject::DEN_OF_THIEVES:
    case eObject::TRADING_POST:
    case eObject::LEARNING_STONE:

    case eObject::TREE_OF_KNOWLEDGE:

    case eObject::UNIVERSITY:
    case eObject::WAGON:
    case eObject::WAR_MACHINE_FACTORY:
    case eObject::SCHOOL_OF_WAR:
    case eObject::WARRIORS_TOMB:
    case eObject::WATER_WHEEL:
    case eObject::WATERING_HOLE:

    case eObject::WINDMILL:
    case eObject::WITCH_HUT:

    case warehouses::WAREHOUSE_OBJECT_TYPE:

    case extender::HOTA_OBJECT_TYPE:
    case extender::HOTA_PICKUPABLE_OBJECT_TYPE:
    case extender::HOTA_UNREACHABLE_OBJECT_TYPE:

    case eObject::FREELANCERS_GUILD:
        // doesn't work properly cause game generates TRADING_POST and but puts TRADING_POST_2
    case eObject::TRADING_POST_SNOW:
        return &m_commonObjects;

    case eObject::CREATURE_GENERATOR1:
    case eObject::CREATURE_GENERATOR4:
        return &m_creatureGenerators;
    case eObject::PYRAMID:
        return &m_wogObjects;

    default:
        return nullptr;

        break;
    }
    return nullptr;
}

void RMG_SettingsDlg::CopyOriginalObjectDefsIntoPcx16()
{
    for (auto &vec : m_objectAttributes)
    {
        for (auto &attributes : *vec)
        {
            auto *workingAttributes = &attributes;

            do
            {
                H3LoadedDef *def = H3LoadedDef::Load(workingAttributes->attributes->defName.String());
                const auto frame = def->GetGroupFrame(0, 0);

                //    create temp pcx to draw def there
                if (H3LoadedPcx16 *tempPcx = H3LoadedPcx16::Create(frame->width, frame->height))
                {
                    // fill with black color
                    libc::memset(tempPcx->buffer, 0, tempPcx->buffSize);
                    // copy def to temp pcx

                    frame->DrawToPcx16(frame->marginLeft, frame->marginTop, frame->frameWidth, frame->height, tempPcx,
                                       0, 0, def->palette565);
                    // def->DrawTransparent(0, tempPcx, 52,52, 1,1);
                    // def->DrawToPcx16(0, 0, tempPcx, 0, 0);
                    workingAttributes->objectPcx = tempPcx;
                }
                def->Dereference();
                // if we have more graphics
            } while (workingAttributes = workingAttributes->next);
        }
    }
}

void AssignPrototypeToObjectGens(const H3RmgObjectGenerator *objGen, std::vector<GraphicalAttributes> *workingVector,
                                 std::unordered_map<DWORD, size_t> &uniqueObjectsIndex, const int objectTypeAlias)
{
    // const int objectType = objGen->type;
    auto &objectTypeVector = P_Game->mainSetup.objectLists[objectTypeAlias];

    const int objectSubtype = objGen->subtype;

    const DWORD pair = (objGen->type << 16) | objectSubtype;

    for (auto &attributes : objectTypeVector)
    {
        if (attributes.subtype == objectSubtype)
        {
            auto itIndex = uniqueObjectsIndex.find(pair);

            if (itIndex == uniqueObjectsIndex.end())
            {
                workingVector->emplace_back(GraphicalAttributes{&attributes});
                uniqueObjectsIndex[pair] = workingVector->size() - 1;
            }
            else
            {
                const size_t addedInTheListItemId = uniqueObjectsIndex[pair];

                GraphicalAttributes *tail = &(*workingVector)[addedInTheListItemId];
                while (tail->next)
                {
                    tail = tail->next;
                }
                // create linked list of graphical attributes for same type/subtype objects
                // last item points to the original attributes
                // ATTENTION
                // access set only via loop
                // and deleting only when the game ends
                tail->next = new GraphicalAttributes{&attributes};
            }
        }
    }
}

BOOL RMG_SettingsDlg::CreateObjectPrototypesLists(const H3Vector<H3RmgObjectGenerator *> *objectGenerators)
{
    if (objectGenerators == nullptr)
    {
        return false;
    }

    // Create object prototypes copies from the loaded generators list

    std::vector<GraphicalAttributes> *workingVector = nullptr;

    std::unordered_map<DWORD, size_t> uniqueObjectsIndex;

    // we iterate all the RMG prototypes and store copies of H3ObjectAttributes into local array taken from game
    // objects list
    for (const auto &objGen : *objectGenerators)
    {
        const int objectType = objGen->type;

        if (workingVector = GetObjectAttributesVector(objectType))
        {

            AssignPrototypeToObjectGens(objGen, workingVector, uniqueObjectsIndex, objectType);
            if (objectType == eObject::TRADING_POST)
            {
                AssignPrototypeToObjectGens(objGen, workingVector, uniqueObjectsIndex, eObject::TRADING_POST_SNOW);
            }
        }
    }

    return true;
}

_LHF_(RMG_SettingsDlg::Dlg_SelectScenario_Proc)
{
    //	THISCALL_1(int, h->GetDefaultFunc(), msg);

    if (H3Msg *msg = reinterpret_cast<H3Msg *>(c->ebx))
    {
        const int itemId = msg->itemId;
        const int subtype = msg->subtype;

        if (itemId == itemIds::DLG_SETTINGS_BUTTON_ID && subtype == eMsgSubtype::LBUTTON_CLICK)
        {
            RMG_SettingsDlg dlg(800, 600);
            dlg.Start();
        }

        if (auto dlg = msg->GetDlg())
        {

            if (auto edit = dlg->GetEdit(itemIds::RMG_SEED_TEXTEDIT_ID))
            {
                //                userRandSeed = atol(edit->GetText());
                const long inputNumber = atol(edit->GetText());

                if (itemId == itemIds::RMG_SEED_TEXTEDIT_ID)
                {
                    if (!inputNumber)
                    {
                        edit->SetText(h3_NullString);
                    }
                    edit->SetFocus(true);
                }
                else
                {
                    if (!inputNumber)
                    {
                        edit->SetText(EraJS::read("RMG.text.seed.edit"));
                    }
                    else
                    {
                        edit->SetText(Era::IntToStr(inputNumber).c_str());
                    }
                    edit->SetFocus(false);
                }
            }
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(RMG_SettingsDlg::H3SelectScenarioDialog_StartButtonClick)
{

    auto dlg = reinterpret_cast<H3SelectScenarioDialog *>(c->ebx);

    if (auto edit = dlg->GetEdit(itemIds::RMG_SEED_TEXTEDIT_ID))
    {
        userRandSeed = atol(edit->GetText());
    }
    return EXEC_DEFAULT;
}

_LHF_(RMG_SettingsDlg::H3SelectScenarioDialog_ShowRandomMapsSettings)
{
    H3Dlg *dlg = reinterpret_cast<H3Dlg *>(c->esi);
    if (dlg)
    {
        for (INT16 i = itemIds::FIRST; i <= itemIds::LAST; i++)
        {
            if (H3DlgItem *it = dlg->GetH3DlgItem(i))
                it->ShowActivate();
        }
        auto edit = dlg->GetEdit(itemIds::RMG_SEED_TEXTEDIT_ID);
        if (edit)
        {
            const long inputNumber = atol(edit->GetText());
            if (!inputNumber)
            {
                edit->SetText(EraJS::read("RMG.text.seed.edit"));
            }
            else
            {
                edit->SetText(Era::IntToStr(inputNumber).c_str());
            }
        }
    }

    return EXEC_DEFAULT;
}
_LHF_(RMG_SettingsDlg::H3SelectScenarioDialog_HideRandomMapsSettings)
{
    H3Dlg *dlg = reinterpret_cast<H3Dlg *>(c->esi);
    if (dlg)
    {
        for (INT16 i = itemIds::FIRST; i <= itemIds::LAST; i++)
        {
            if (H3DlgItem *it = dlg->GetH3DlgItem(i))
                it->HideDeactivate();
        }
    }
    return EXEC_DEFAULT;
}

void __stdcall RMG_SetRandSeed(HiHook *h, DWORD *rmgSeed)
{

    CDECL_1(void, h->GetDefaultFunc(), rmgSeed);
    if (const DWORD userSeed = RMG_SettingsDlg::GetUserRandSeedInput())
    {
        *rmgSeed = userSeed;
    }
}

int __fastcall SelectScenarioDlgRandomizeProc(H3Msg *msg)
{
    if (msg->IsLeftClick())
    {
        auto *randomGameButton = msg->GetDlg()->ItemAtPosition(msg)->Cast<H3DlgCustomButton>();
        if (randomGameButton)
        {
            const int currentFrame = randomGameButton->GetFrame();
            randomGameButton->SetFrame(currentFrame ^ 2);
            const bool enableFullRandom = randomGameButton->GetFrame() == 2;
            randomGameButton->Draw();
            randomGameButton->ParentRedraw();

            if (currentFrame == 0)
            {
                H3Messagebox::Show(EraJS::read("RMG.text.buttons.random.help"));
            }

            Era::WriteStrToIni(RMG_SettingsDlg::INI_ALWAYS_RANDOM, Era::IntToStr(enableFullRandom).c_str(),
                               RMG_SettingsDlg::SETTINGS_INI_SECTION, RMG_SettingsDlg::INI_FILE_PATH);
            Era::SaveIni(RMG_SettingsDlg::INI_FILE_PATH);
        }
    }

    return 0;
}

void __stdcall RMG_SettingsDlg::NewScenarioDlg_Create(HiHook *hook, H3SelectScenarioDialog *dlg,
                                                      const DWORD dlgCallType)
{
    THISCALL_2(int, hook->GetDefaultFunc(), dlg, dlgCallType);
    if (dlgCallType == 0) // 0 new/ 1 load/2 from map
    {
        // create text over button near at team setups (HD mod only)
        if (H3DlgText *text =
                dlg->CreateText(248, 264, 128, 20, EraJS::read(DLG_TEXT_JSON_KEY), h3::NH3Dlg::Text::SMALL,
                                eTextColor::REGULAR, itemIds::RMG_SETTINGS_TEXT_ID, eTextAlignment::MIDDLE_RIGHT))
        {
            // hide button cause we assume it is not in the list of the items to display
            text->HideDeactivate();
        }

        // create clicl button that calls the dlg
        if (H3DlgCaptionButton *bttn = dlg->CreateCaptionButton(
                246, 291, 128, 20, itemIds::DLG_SETTINGS_BUTTON_ID, *reinterpret_cast<char **>(0x57A93B + 1),
                EraJS::read("RMG.text.buttons.setup.name"), h3::NH3Dlg::Text::SMALL, 0, 0, 0, 0, eTextColor::HIGHLIGHT))
        {
            bttn->SetClickFrame(1);
            bttn->SetHint(EraJS::read("RMG.text.buttons.setup.rmc"));

            bttn->AddHotkey(h3::eVKey::H3VK_S);

            // hide button cause we assume it is not in the list of the items to display
            bttn->HideDeactivate();
        }

        // create randomize button
        H3DefLoader def("RMG_Dice.def");
        if (H3DlgCustomButton *randomGameButton =
                H3DlgCustomButton::Create(208, 286, def->widthDEF, def->heightDEF, itemIds::RMG_RANDOMIZE_BUTTON_ID,
                                          def->GetName(), SelectScenarioDlgRandomizeProc, 0, 1))
        {
            randomGameButton->SetHint(EraJS::read("RMG.text.buttons.random.rmc"));

            BOOL result = Era::ReadStrFromIni(INI_ALWAYS_RANDOM, SETTINGS_INI_SECTION, INI_FILE_PATH, h3_TextBuffer);
            if (result && atoi(h3_TextBuffer))
            {
                randomGameButton->SetFrame(2);
            }
            randomGameButton->HideDeactivate();
            dlg->AddItem(randomGameButton);
        }

        // create text over button near at team setups (HD mod only)
        if (H3DlgText *text = dlg->CreateText(210, 328, def->widthDEF, 20, EraJS::read("RMG.text.seed.title"),
                                              h3::NH3Dlg::Text::SMALL, eTextColor::REGULAR, itemIds::RMG_SEED_TEXT_ID,
                                              eTextAlignment::MIDDLE_CENTER))
        {
            text->HideDeactivate();
        }
        if (H3DlgEdit *textEdit = dlg->CreateEdit(
                246, 328, 128, 20, 11, EraJS::read("RMG.text.seed.edit"), h3::NH3Dlg::Text::SMALL, eTextColor::REGULAR,
                eTextAlignment::MIDDLE_CENTER, "rmg_sedt.pcx", itemIds::RMG_SEED_TEXTEDIT_ID, true, 1, 1))
        {
            textEdit->SetAutoredraw(true);
            textEdit->SetHint(EraJS::read("RMG.text.seed.rmc"));

            textEdit->HideDeactivate();
        }
    }
    userRandSeed = 0;
}

void CreateResizedObjectPcx()
{

    constexpr int PCX_WIDTH = 44;
    constexpr int PCX_HEIGHT = 44;

    auto &objectAttributes = RMG_SettingsDlg::GetObjectAttributes();

    for (auto &vec : objectAttributes)
    {
        for (auto &attributes : *vec)
        {
            auto workingAttributes = &attributes;

            do
            {
                if (H3LoadedPcx16 *tempPcx = workingAttributes->objectPcx)
                {
                    // create pcx to draw resized temp pcx there
                    if (H3LoadedPcx16 *resizedPcx = H3LoadedPcx16::Create(PCX_WIDTH, PCX_HEIGHT))
                    {
                        memset(resizedPcx->buffer, 0, resizedPcx->buffSize);

                        const int srcWidth = tempPcx->width;
                        const int srcHeight = tempPcx->height;

                        // place picture into "Square" thanks to @Berserker ...
                        const int maxDim = std::max(srcWidth, srcHeight);

                        const int dstWidth = static_cast<int>(static_cast<double>(srcWidth) / maxDim * PCX_WIDTH);
                        const int dstHeight = static_cast<int>(static_cast<double>(srcHeight) / maxDim * PCX_HEIGHT);

                        const int dstX = (PCX_WIDTH - dstWidth) >> 1;
                        const int dstY = (PCX_HEIGHT - dstHeight) >> 1;

                        resized::H3LoadedPcx16Resized::DrawPcx16ResizedBicubic(resizedPcx, tempPcx, srcWidth, srcHeight,
                                                                               dstX, dstY, dstWidth, dstHeight);

                        workingAttributes->objectPcx = resizedPcx;
                    }

                    tempPcx->Destroy();
                }
                // if we have more graphics
                if (workingAttributes->next)
                {
                    // repeat creation pictures for them
                    workingAttributes = workingAttributes->next;
                }
                else
                {
                    // otherwise break the loop
                    workingAttributes->next = &attributes;
                    break;
                }
            } while (workingAttributes);
        }
    }
}

// create resized graphics for dialog
void __stdcall GameStart(HiHook *hook, const DWORD a1)
{
    THISCALL_1(int, hook->GetDefaultFunc(), a1);

    hook->Undo();
    const auto genList = editor::RMGObjectsEditor::Get().GetObjectGeneratorsList();
    if (RMG_SettingsDlg::CreateObjectPrototypesLists(genList))
    {
        RMG_SettingsDlg::CopyOriginalObjectDefsIntoPcx16();
        // CreateResizedObjectPcx();
        std::thread th(CreateResizedObjectPcx);
        th.detach();
    }
}
_ERH_(RMG_SettingsDlg::OnAfterReloadLanguageData)
{
    mainmenu::MainMenu_SetDialogButtonText(MAIN_MENU_WIDGET_UUID, EraJS::read(MAIN_MENU_JSON_KEY));
}

void RMG_SettingsDlg::SetPatches(PatcherInstance *_pi)
{
    if (_pi)
    {
        // H3DLL wndPlugin = h3::H3DLL::H3DLL("wog native dialogs.era")
        if (GetModuleHandleA("HD_WOG.dll"))
        {
            _pi->WriteHiHook(0x579CE0, THISCALL_, NewScenarioDlg_Create);

            _pi->WriteHiHook(0x0536630, CDECL_, RMG_SetRandSeed);
            _pi->WriteLoHook(0x58698B, H3SelectScenarioDialog_StartButtonClick);

            _pi->WriteLoHook(0x5806D4, H3SelectScenarioDialog_ShowRandomMapsSettings);
            _pi->WriteLoHook(0x58207F, H3SelectScenarioDialog_HideRandomMapsSettings);
            //_pi->WriteLoHook(0x58207F, H3SelectScenarioDialog_HideRandomMapsSettings);
            _pi->WriteLoHook(0x0588469, Dlg_SelectScenario_Proc);

            _pi->WriteHiHook(0x5BB1D1, THISCALL_, H3DlgEdit__TranslateInputKey);

            Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");
        }
        _pi->WriteHiHook(0x4FB930, THISCALL_, GameStart);

        using namespace mainmenu;
        MenuWidgetInfo widgetInfo;
        widgetInfo.name = MAIN_MENU_WIDGET_UUID;
        widgetInfo.customProc = RMG_SettingsDlg::RMGDlgOptionsButtonProc;
        const eMenuFlags flags = static_cast<eMenuFlags>(eMenuFlags::NEW_GAME | eMenuFlags::AT_BOTTOM);

        widgetInfo.menuList = flags;
        widgetInfo.text = EraJS::read(MAIN_MENU_JSON_KEY);

        mainmenu::MainMenu_RegisterWidget(widgetInfo);
    }
}

} // namespace rmgdlg
