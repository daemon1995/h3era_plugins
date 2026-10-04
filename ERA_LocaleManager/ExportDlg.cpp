#include "ExportManager.h"
#ifdef CREATE_TEXT_JSON_EXPORTS

void ExportDlg::CreateDlgItems()
{
    CreateOKButton();
    CreateCancelButton();

    BOOL (*selectionPanelExportFunctions[])(LPCSTR, const BOOL, const BOOL) = {
        ExportManager::CreateMonstersJson,      ExportManager::CreateArtifactsJson,
        ExportManager::CreateObjectsJson,       ExportManager::CreateCreatureBanksJson,
        ExportManager::CreateTownBuildingsJson, ExportManager::CreateHeroesJson};
    LPCSTR panelPaths[] = {ExportManager::MonsterInfo::DEFAULT_PATH,      ExportManager::ArtifactInfo::DEFAULT_PATH,
                           ExportManager::MapObjectInfo::DEFAULT_PATH,    ExportManager::CreatureBankInfo::DEFAULT_PATH,
                           ExportManager::TownBuildingInfo::DEFAULT_PATH, ExportManager::HeroInfo::DEFAULT_PATH};
    constexpr size_t panelsNum = std::size(selectionPanelExportFunctions);
    constexpr int startId = 1;
    selectionPanels.reserve(panelsNum);

    constexpr int borderPadding = 20;

    for (size_t i = 0; i < panelsNum; i++)
    {

        const int y = borderPadding + 30 + i * PANELS_PADDING;
        libc::sprintf(h3_TextBuffer, PANEL_NAME_FORMAT, i);
        if (auto panel =
                CreateSelectionPanel(borderPadding, y, EraJS::read(h3_TextBuffer), startId + i * ITEMS_PER_PANEL,
                                     panelPaths[i], selectionPanelExportFunctions[i]))
        {
            selectionPanels.emplace_back(panel);
        }
    }
    if (selectionPanels.size())
    {
        LPCSTR text = EraJS::read("era.locale.dlg.export.item");
        CreateText(borderPadding, borderPadding, PANEL_TEXT_WIDTH, 20, text, NH3Dlg::Text::MEDIUM, eTextColor::REGULAR,
                   -1);
        text = EraJS::read("era.locale.dlg.export.original");
        CreateText(borderPadding + PANEL_TEXT_WIDTH, borderPadding, PANEL_TEXT_WIDTH, 20, text, NH3Dlg::Text::MEDIUM,
                   eTextColor::REGULAR, -1);
        text = EraJS::read("era.locale.dlg.export.additional");
        CreateText(borderPadding + PANEL_TEXT_WIDTH + CHECKBOX_PADDING, borderPadding, PANEL_TEXT_WIDTH, 20, text,
                   NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1);
    }
}
ExportDlg::SelectionPanel *ExportDlg::CreateSelectionPanel(const int x, const int y, LPCSTR text, const int startId,
                                                           LPCSTR path,
                                                           BOOL (*exportFunc)(LPCSTR, const BOOL, const BOOL))

{
    SelectionPanel *panel = nullptr;
    if (panel = new SelectionPanel())
    {
        constexpr int panelTextW = PANEL_TEXT_WIDTH;

        panel->panelText = CreateText(x, y + 3, panelTextW, 20, text, NH3Dlg::Text::MEDIUM, eTextColor::REGULAR,
                                      startId, eTextAlignment::MIDDLE_LEFT);
        const int panelTextX = panel->panelText->GetX();

        const int originalCheckBoxX = panelTextW + CHECKBOX_PADDING / 2;
        panel->originalDataCheckBox = CreateDef(originalCheckBoxX, y, startId + 1, NH3Dlg::Assets::ON_OFF_CHECKBOX, 1);
        panel->additionalDataCheckBox =
            CreateDef(originalCheckBoxX + CHECKBOX_PADDING, y, startId + 2, NH3Dlg::Assets::ON_OFF_CHECKBOX, 1);
        panel->exportPath = path;
        panel->exportFunction = exportFunc;
    }
    return panel;
}

ExportDlg::ExportDlg(const int width, const int height, const int x, const int y)
    : H3Dlg(width, height, x, y, false, false)
{
    this->AddBackground(true, false, ePlayer::RED);
    this->flags ^= 0x10; // disable dlg shadow
    CreateDlgItems();
}

ExportDlg::~ExportDlg()
{
    for (auto &i : selectionPanels)
    {
        delete i;
    }
}

BOOL ExportDlg::DialogProc(H3Msg &msg)
{
    if (msg.IsLeftDown())
    {
        if (auto checkBox = GetDef(msg.itemId))
        {
            P_SoundManager->ClickSound();
            checkBox->SetFrame(checkBox->GetFrame() ^ 1);
            checkBox->Draw();
            checkBox->Refresh();
            return 1;
        }
    }

    return 0;
}
// Creating json here
VOID ExportDlg::OnOK()
{
    // bool exportSuccess = false;
    std::string message, list;

    std::string exportPath = Era::z[1] + std::string(SUBFOLDER_NAME);

    if (Era::era_str eraLocale = Era::GetLanguage())
    {
        std::string currentLanguage = eraLocale;
        Era::MemFree(eraLocale);
        if (!currentLanguage.empty())
        {
            exportPath += currentLanguage + "\\";
        }
    }

    for (auto &i : selectionPanels)
    {
        if (i->exportFunction)
        {
            const BOOL originalData = i->originalDataCheckBox->GetFrame() == 1;
            const BOOL additionalData = i->additionalDataCheckBox->GetFrame() == 1;
            if (originalData || additionalData)
            {

                std::string filePath = exportPath + i->exportPath;
                try
                {
                    LPCSTR key = i->exportFunction(filePath.c_str(), originalData, additionalData)
                                     ? EraJS::read("era.locale.dlg.export.success")
                                     : EraJS::read("era.locale.dlg.export.error");
                    list += "\n" + EraJS::FormatText(key, {filePath});
                }
                catch (const std::exception &error)
                {
                    list += "\n" + filePath + ": " + error.what();
                }
            }
        }
    }
    if (!list.empty())
    {
        message = EraJS::read("era.locale.dlg.export.completed");
        message = EraJS::FormatText(message.c_str(), {list});
        if (H3Messagebox::Choice(message.c_str()))
        {
            INT_PTR intRes =
                STDCALL_6(INT_PTR, PtrAt(0x63A250), NULL, "open", exportPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
    }
    else
    {
        message = EraJS::read("era.locale.dlg.export.empty");
        H3Messagebox::Show(message.c_str());
    }
}
#endif // CREATE_TEXT_JSON_EXPORTS
