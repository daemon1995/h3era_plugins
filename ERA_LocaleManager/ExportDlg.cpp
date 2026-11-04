#include "ExportManager.h"
#include "../headers/EraPluginsAPI/ConfigExportUIAPI.h"
#ifdef CREATE_TEXT_JSON_EXPORTS
#include <algorithm>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

namespace
{
constexpr LPCSTR COLUMN_KEYS[] = {"original", "mods", "text", "data"};
constexpr LPCSTR OPTION_KEYS[] = {"options.merge", "options.ignoreDefaults"};
LPCSTR ExportText(const std::string &key)
{
    bool found = false;
    const char *value = EraJS::read("era.configExport." + key, found);
    return found ? value : EraJS::read("era.locale.dlg.export." + key);
}
LPCSTR CategoryText(const configExport::Category &category)
{
    bool found = false;
    const char *text = EraJS::read("era.configExport.items." + category.key, found);
    if (!found) text = EraJS::read("era.locale.dlg.export.items." + category.key, found);
    return found ? text : category.fileStem.c_str();
}
}

ExportDlg::Layout ExportDlg::Measure(const std::vector<configExport::Category> &categories)
{
    H3FontLoader font(NH3Dlg::Text::MEDIUM);
    const int lineHeight = font.Get() ? font->height : 16;
    const auto textWidth = [&](LPCSTR text) { return font.Get() ? font->GetMaxLineWidth(text) : 160; };
    const auto textHeight = [&](LPCSTR text, int width) {
        return (font.Get() ? (std::max)(1, font->GetLinesCountInText(text, width)) : 1) * lineHeight;
    };
    int labelWidth = textWidth(ExportText("item")) + 16;
    int columnWidth = 88;
    for (const char *column : COLUMN_KEYS) columnWidth = (std::max)(columnWidth, textWidth(ExportText(std::string("columns.") + column)) + 16);
    for (const auto &category : categories)
    {
        labelWidth = (std::max)(labelWidth, textWidth(CategoryText(category)) + 16);
        columnWidth = (std::max)(columnWidth, textWidth(std::to_string(category.recordCount).c_str()) + 44);
    }
    const int availableWidth = H3GameWidth::Get() - 24;
    labelWidth = (std::min)(labelWidth, (std::max)(140, availableWidth - 2 * PADDING - 4 * columnWidth));
    const int width = (std::min)(availableWidth, 2 * PADDING + labelWidth + 4 * columnWidth);
    columnWidth = (width - 2 * PADDING - labelWidth) / 4;
    int rowHeight = (std::max)(30, lineHeight + 12);
    for (const auto &category : categories)
        rowHeight = (std::max)(rowHeight, textHeight(CategoryText(category), labelWidth - 12) + 8);
    int headerHeight = (std::max)(26, textHeight(ExportText("item"), labelWidth - 12) + 8);
    for (const char *column : COLUMN_KEYS)
        headerHeight = (std::max)(headerHeight, textHeight(ExportText(std::string("columns.") + column), columnWidth - 8) + 8);
    const int noteHeight = textHeight(ExportText("note"), width - 2 * PADDING) + 8;
    std::array<int, 2> optionHeights{};
    for (size_t option = 0; option < optionHeights.size(); ++option)
        optionHeights[option] = (std::max)(30, textHeight(ExportText(OPTION_KEYS[option]), width - 2 * PADDING - 44) + 8);
    const int fixed = 2 * PADDING + 32 + headerHeight + optionHeights[0] + optionHeights[1] + 10 + noteHeight + 54;
    const int maxRows = (std::max)(1, (H3GameHeight::Get() - 24 - fixed - 28) / rowHeight);
    const int visibleRows = (std::max)(1, (std::min)(int(categories.size()), maxRows));
    const int height = fixed + visibleRows * rowHeight + (categories.size() > unsigned(visibleRows) ? 28 : 0);
    return {width, height, labelWidth, columnWidth, rowHeight, headerHeight, noteHeight, visibleRows, optionHeights};
}
ExportDlg::ExportDlg(unsigned mode) : ExportDlg(ExportManager::Collect(), mode) {}
ExportDlg::ExportDlg(std::vector<configExport::Category> categories, unsigned mode) : ExportDlg(categories, Measure(categories), mode) {}
ExportDlg::ExportDlg(const std::vector<configExport::Category> &categories, const Layout &layout, unsigned mode)
    : H3Dlg(layout.width, layout.height, -1, -1, false, false), categories(categories), layout(layout), mode(mode)
{
    AddBackground(true, false, ePlayer::RED);
    flags ^= 0x10;
    CreateDlgItems();
}
void ExportDlg::CreateDlgItems()
{
    CreateText(PADDING, PADDING, widthDlg - 2 * PADDING, 24, ExportText(mode == era_config_ui::Translations ? "translationsTitle" : "title"), NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1);
    const int tableY = PADDING + 32;
    const int tableWidth = layout.labelWidth + 4 * layout.columnWidth;
    CreateText(PADDING + 6, tableY, layout.labelWidth - 12, layout.headerHeight, ExportText("item"), NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
    for (int column = 0; column < 4; ++column)
        CreateText(PADDING + layout.labelWidth + column * layout.columnWidth, tableY, layout.columnWidth, layout.headerHeight,
                   ExportText(std::string("columns.") + COLUMN_KEYS[column]), NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1);
    const int rowsY = tableY + layout.headerHeight;
    const int tableBottom = rowsY + layout.visibleRows * layout.rowHeight;
    // The game's hero/creature content dialogs use this same separator asset.
    for (int index = 0; index <= layout.visibleRows; ++index)
        if (auto *separator = CreateLineSeparator(PADDING, rowsY + static_cast<int>(index) * layout.rowHeight, tableWidth)) separator->DeActivate();
    if (auto *background = GetBackgroundPcx())
        for (int column = 0; column < 4; ++column)
        {
            const int x = PADDING + layout.labelWidth + column * layout.columnWidth;
            background->DarkenArea(x, tableY, 1, tableBottom - tableY, 50);
            background->LightenArea(x + 1, tableY, 1, tableBottom - tableY, 50);
        }
    rows.resize(categories.size());
    for (size_t index = 0; index < categories.size(); ++index)
    {
        auto &row = rows[index];
        const auto available = categories[index].Counts({}, false);
        row.enabled = {available[0] > 0, available[1] > 0, available[2] > 0, categories[index].supportsData && available[3] > 0};
        row.selection = {row.enabled[0], row.enabled[1], row.enabled[2], row.enabled[3]};
        if (mode == era_config_ui::Translations) row.selection.data = false;
        const int y = rowsY + int(index % layout.visibleRows) * layout.rowHeight;
        row.label = CreateText(PADDING + 6, y + 2, layout.labelWidth - 12, layout.rowHeight - 4,
                   CategoryText(categories[index]), NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
        for (int column = 0; column < 4; ++column)
        {
            const int x = PADDING + layout.labelWidth + column * layout.columnWidth;
            const int checkX = x + layout.columnWidth - 32;
            const bool selected[] = {row.selection.original != 0, row.selection.mods != 0, row.selection.text != 0, row.selection.data != 0};
            auto *checkbox = CreateDef(checkX, y + 4, FIRST_CHECKBOX_ID + static_cast<int>(index) * 4 + column, NH3Dlg::Assets::ON_OFF_CHECKBOX, selected[column] ? 1 : 0);
            row.checkboxes[column] = checkbox;
            if (checkbox)
            {
                checkbox->SetY(y + (layout.rowHeight - checkbox->GetHeight()) / 2);
                if (!row.enabled[column]) { checkbox->Shade(); checkbox->DeActivate(); }
            }
            row.counts[column] = CreateText(x + 4, y + 2, checkX - x - 8, layout.rowHeight - 4, "0", NH3Dlg::Text::MEDIUM,
                                            eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_RIGHT);
        }
        RefreshRow(index);
    }
    int optionY = tableBottom + 8;
    if (categories.size() > unsigned(layout.visibleRows))
    {
        CreateText(PADDING, optionY, 32, 24, "<", NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, 61000)->Activate();
        CreateText(widthDlg - PADDING - 32, optionY, 32, 24, ">", NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, 61001)->Activate();
        const auto pages = (categories.size() + layout.visibleRows - 1) / layout.visibleRows;
        const auto text = "1 / " + std::to_string(pages);
        CreateText(PADDING + 40, optionY, widthDlg - 2 * PADDING - 80, 24, text.c_str(), NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, 61002);
        optionY += 28;
    }
    for (int option = 0; option < 2; ++option)
    {
        const int id = option == 0 ? MERGE_OPTION_ID : IGNORE_DEFAULTS_OPTION_ID;
        const bool selected = option == 0 ? options.mergeTextAndData : options.ignoreRmgDefaults;
        CreateText(PADDING + 6, optionY, widthDlg - 2 * PADDING - 44, layout.optionHeights[option],
                   ExportText(OPTION_KEYS[option]), NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, id + 10, eTextAlignment::MIDDLE_LEFT);
        auto *checkbox = CreateDef(widthDlg - PADDING - 32, optionY, id, NH3Dlg::Assets::ON_OFF_CHECKBOX, selected ? 1 : 0);
        optionCheckboxes[option] = checkbox;
        if (checkbox) checkbox->SetY(optionY + (layout.optionHeights[option] - checkbox->GetHeight()) / 2);
        optionY += layout.optionHeights[option];
    }
    CreateText(PADDING, optionY + 10, widthDlg - 2 * PADDING, layout.noteHeight, ExportText("note"), NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1);
    CreateOKButton();
    CreateCancelButton();
}
void ExportDlg::RefreshRow(size_t index)
{
    auto &row = rows[index];
    const auto counts = categories[index].Counts(row.selection, options.ignoreRmgDefaults);
    const bool selected[] = {row.selection.original != 0, row.selection.mods != 0, row.selection.text != 0, row.selection.data != 0};
    const bool visible = index / layout.visibleRows == page;
    if (row.label) { if (visible) row.label->Show(); else row.label->Hide(); }
    for (int column = 0; column < 4; ++column)
    {
        if (row.checkboxes[column]) row.checkboxes[column]->SetFrame(selected[column] ? 1 : 0);
        if (row.counts[column]) row.counts[column]->SetText(std::to_string(counts[column]).c_str());
        if (row.counts[column]) { if (visible) row.counts[column]->Show(); else row.counts[column]->Hide(); }
        if (row.checkboxes[column])
        {
            if (!visible) row.checkboxes[column]->HideDeactivate();
            else { row.checkboxes[column]->Show(); if (row.enabled[column]) row.checkboxes[column]->Activate(); }
        }
    }
}
BOOL ExportDlg::DialogProc(H3Msg &msg)
{
    // Native controls sign-extend their 16-bit IDs when creating mouse events.
    const int itemId = static_cast<UINT16>(msg.itemId);
    if (msg.IsLeftDown())
    {
        if (itemId == 61000 || itemId == 61001)
        {
            const unsigned pages = unsigned((categories.size() + layout.visibleRows - 1) / layout.visibleRows);
            page = itemId == 61000 ? (page + pages - 1) % pages : (page + 1) % pages;
            for (size_t i = 0; i < rows.size(); ++i) RefreshRow(i);
            GetText(61002)->SetText((std::to_string(page + 1) + " / " + std::to_string(pages)).c_str());
            Redraw();
            return 1;
        }
        const int option = itemId == MERGE_OPTION_ID || itemId == MERGE_OPTION_ID + 10 ? 0 :
            itemId == IGNORE_DEFAULTS_OPTION_ID || itemId == IGNORE_DEFAULTS_OPTION_ID + 10 ? 1 : -1;
        if (option >= 0)
        {
            int &selected = option == 0 ? options.mergeTextAndData : options.ignoreRmgDefaults;
            selected = !selected;
            if (optionCheckboxes[option]) optionCheckboxes[option]->SetFrame(selected ? 1 : 0);
            if (option == 1)
                for (size_t index = 0; index < rows.size(); ++index) RefreshRow(index);
            P_SoundManager->ClickSound();
            Redraw();
            return 1;
        }
    }
    if (msg.IsLeftDown() && itemId >= FIRST_CHECKBOX_ID && itemId < FIRST_CHECKBOX_ID + static_cast<int>(rows.size()) * 4)
    {
        const int index = (itemId - FIRST_CHECKBOX_ID) / 4;
        const int column = (itemId - FIRST_CHECKBOX_ID) % 4;
        auto &row = rows[index];
        if (!row.enabled[column]) return 0;
        int *values[] = {&row.selection.original, &row.selection.mods, &row.selection.text, &row.selection.data};
        *values[column] = !*values[column];
        P_SoundManager->ClickSound();
        RefreshRow(index);
        Redraw();
        return 1;
    }
    if (msg.IsRightClick()) H3Messagebox::RMB(ExportText("help"));
    return 0;
}
VOID ExportDlg::OnOK()
{
    const std::string exportRoot = std::string(Era::z[1]) + "Runtime\\Exports\\";
    std::string textPath = exportRoot;
    std::string combinedPath = exportRoot + "Combined\\";
    if (Era::era_str language = Era::GetLanguage())
    {
        const std::string locale = language;
        Era::MemFree(language);
        if (EraJS::LanguageNameIsValid(locale))
        {
            textPath += locale + "\\";
            combinedPath += locale + "\\";
        }
    }
    std::string results;
    std::string outputDirectory;
    bool multipleDirectories = false;
    for (size_t index = 0; index < categories.size(); ++index)
    {
        const auto &category = categories[index];
        try
        {
            for (const auto &output : category.Outputs(rows[index].selection, options))
            {
                const auto directory = output.kind == era_config::CombinedFile ? combinedPath :
                    output.kind == era_config::DataFile ? exportRoot + "Data\\" : textPath;
                const std::string filePath = directory + output.fileName;
                try
                {
                    const bool written = ExportManager::WriteJsonFile(filePath, output.content) != FALSE;
                    if (written)
                    {
                        if (outputDirectory.empty()) outputDirectory = directory;
                        else if (outputDirectory != directory) multipleDirectories = true;
                    }
                    const char *message = ExportText(written ? "success" : "error");
                    results += "\n" + EraJS::FormatText(message, {filePath});
                }
                catch (const std::exception &error)
                {
                    results += "\n" + filePath + ": " + error.what();
                }
            }
        }
        catch (const std::exception &error)
        {
            results += "\n" + std::string(category.fileStem) + ": " + error.what();
        }
    }
    if (results.empty()) H3Messagebox::Show(ExportText("empty"));
    else if (H3Messagebox::Choice(EraJS::FormatText(ExportText("completed"), {results}).c_str()))
        ShellExecuteA(nullptr, "open", !multipleDirectories && !outputDirectory.empty() ? outputDirectory.c_str() : exportRoot.c_str(),
                      nullptr, nullptr, SW_SHOWNORMAL);
}
#endif
