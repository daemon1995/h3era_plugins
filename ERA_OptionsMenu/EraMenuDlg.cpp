#include "EraMenuDlg.h"
#include "ObjectBansDlg.h"
#include "OptionsMenuUI.h"
#include "GameContext.h"
#include "WoGOptionsBridge.h"
#include "OptionsMenuApi.h"

#include <algorithm>

namespace
{
constexpr int SearchId = 10;
constexpr int ClearId = 11;
constexpr int DefaultsId = 12;
constexpr int ErrorsId = 13;
constexpr int CloseId = 14;
constexpr int ExportId = 15;
constexpr int ResizeId = 16;
constexpr int BansId = 17;
constexpr int ModScrollId = 20;
constexpr int PageScrollBase = 5000;
constexpr size_t MaxCachedScrollbars = 64;
constexpr int ModRowBase = 100;
constexpr int OptionNameBase = 1000;
constexpr int CheckboxBase = 3000;
constexpr int RadioBase = 4000;
constexpr int SidebarTop = 88;
constexpr int ListTop = 114;
constexpr int ModRowHeight = 42;
constexpr int MinimumRowHeight = 24;

unsigned GameCodePage()
{
    return Era::GetCodePage();
}

const char *UiText(const char *key, const char *fallback)
{
    const std::string path = std::string("era_options.ui.") + key;
    const char *value = Era::tr(path.c_str());
    return value && path != value ? value : fallback;
}

H3DlgCaptionButton *Button(H3Dlg *dlg, int x, int y, int width, int height, int id, const char *text)
{
    auto *button = H3DlgCaptionButton::Create(x, y, width, height, id, "OVBUTN3.def", text,
        NH3Dlg::Text::SMALL, 0, 0, false, 0, eTextColor::REGULAR);
    if (button)
    {
        button->SetClickFrame(1);
        dlg->AddItem(button);
    }
    return button;
}

class AdjustableScrollbar : public H3DlgScrollbar
{
  public:
    void SetRange(int count) const { vSetTickCount(count); }
};

int UpdateScroll(H3DlgScrollbar *scroll, int maximum, int requested, bool visible)
{
    const int value = (std::max)(0, (std::min)(requested, maximum));
    if (!scroll) return value;
    if (maximum > 0)
    {
        // Do not reset the range from every native tick callback.
        if (scroll->GetTicksCount() != maximum + 1)
            reinterpret_cast<AdjustableScrollbar *>(scroll)->SetRange(maximum + 1);
        scroll->SetTick(value);
        if (visible)
        {
            scroll->SetButtonPosition();
            scroll->ShowActivate();
        }
        else scroll->HideDeactivate();
    }
    else
    {
        // Native thumb positioning divides by tickCount - 1.
        scroll->SetTick(0);
        scroll->HideDeactivate();
    }
    return value;
}

using era_options::TiledBackground;

// Same artwork, font, sizing and hold-RMB lifetime as WoG Native Dialogs.
class OptionPreview final : public era_options::OptionsDialog
{
  public:
    OptionPreview(int width, int height, const std::string &text, const char *fontName) :
        OptionsDialog(width, height)
    {
        bool skin = true;
        for (const char *asset : {"WOGODARK.PCX", "WOGOPT.PCX", "WOGOPB.PCX", "WOGOPL.PCX", "WOGOPR.PCX",
                                 "WOGOTL.PCX", "WOGOTR.PCX", "WOGOBL.PCX", "WOGOBR.PCX"})
            if (skin && !Era::PcxPngExists(asset)) skin = false;
        if (skin)
        {
            CreatePcx(0, 0, width, height, 1, "WOGODARK.PCX");
            CreatePcx(0, 0, width, 8, 2, "WOGOPT.PCX");
            CreatePcx(0, height - 8, width, 8, 3, "WOGOPB.PCX");
            CreatePcx(0, 0, 8, height, 4, "WOGOPL.PCX");
            CreatePcx(width - 8, 0, 8, height, 5, "WOGOPR.PCX");
            CreatePcx(0, 0, 46, 46, 6, "WOGOTL.PCX");
            CreatePcx(width - 46, 0, 46, 46, 7, "WOGOTR.PCX");
            CreatePcx(0, height - 46, 46, 46, 8, "WOGOBL.PCX");
            CreatePcx(width - 46, height - 46, 46, 46, 9, "WOGOBR.PCX");
        }
        else
        {
            background = TiledBackground(width, height);
            if (background)
            {
                FrameRegion(0, 0, width, height, FALSE, 0);
                if (auto *item = H3DlgPcx16::Create(0, 0, width, height, 0, nullptr))
                {
                    item->SetPcx(background);
                    AddItem(item);
                }
            }
        }
        CreateText(20, 20, width - 40, height - 40, text.c_str(), fontName,
            eTextColor::REGULAR, 10, eTextAlignment::MIDDLE_CENTER);
    }
};

const char *ReadOnlyReason(bool currentMap)
{
    return currentMap ? UiText("map_option_read_only", "This option can only be changed before starting a map.") :
        UiText("read_only", "This dialog was opened for viewing only.");
}

void ShowOptionPreview(const era_options::EOption &option, int value, bool allowEditing, bool currentMap)
{
    const auto &definition = option.Definition();
    std::string text = "{" + definition.name + "}\n\n" +
        (definition.popup.empty() ? definition.hint : definition.popup);
    if (value < 0) text += "\n\n" + std::string(UiText("value_unavailable", "Cannot read the current value."));
    else if (definition.type == era_options::OptionType::Choice)
        text += "\n\n{" + definition.choices[value] + "}";
    text += "\n\n" + std::string(definition.changeDuringGame ?
        UiText("change_during_game", "Changes during game: supported by the mod") :
        UiText("before_game_only", "Changes during game: not supported by the mod"));
    if (!allowEditing) text += "\n\n" + std::string(ReadOnlyReason(currentMap));
    else if (!era_options::IsOptionEditable(option, currentMap))
    {
        const auto reason = era_options::OptionDependencyHint(option, currentMap);
        text += "\n\n" + (reason.empty() ? std::string(currentMap && !definition.changeDuringGame ?
            ReadOnlyReason(true) : UiText("locked", "This option cannot be changed.")) : reason);
    }
    H3FontLoader font("medfont2.fnt");
    H3FontLoader fallback(NH3Dlg::Text::MEDIUM);
    H3Font *previewFont = font.Get() ? font.Get() : fallback.Get();
    const int screenWidth = H3GameWidth::Get() - 16;
    const int screenHeight = H3GameHeight::Get() - 16;
    int width = (std::min)(400, screenWidth);
    int lines = previewFont ? previewFont->GetLinesCountInText(text.c_str(), width - 40) : 5;
    if (lines > 30)
    {
        width = (std::min)(580, screenWidth);
        lines = previewFont ? previewFont->GetLinesCountInText(text.c_str(), width - 40) : lines;
    }
    if (lines * 16 + 40 > (std::min)(580, screenHeight))
    {
        width = (std::min)(780, screenWidth);
        lines = previewFont ? previewFont->GetLinesCountInText(text.c_str(), width - 40) : lines;
    }
    const int height = (std::min)((std::min)(580, screenHeight), (std::max)(80, lines * 16) + 40);
    OptionPreview preview(width, height, text, font.Get() ? "medfont2.fnt" : NH3Dlg::Text::MEDIUM);
    preview.RMB_Show();
}
} // namespace

EraMenuDlg::EraMenuDlg(int width, int height, bool fullScreen, const era_options::OptionMenuModel &state,
    int sidebarPosition, bool allowEditing, bool currentMap) : OptionsDialog(width, height), model(state),
    firstMod(sidebarPosition), fullScreen(fullScreen), allowEditing(allowEditing), currentMap(currentMap)
{
    CreateBackground();
    hintBar = CreateHint();
    CreateWidgets();
    RebuildOptions();
    RefreshMods();
}

int EraMenuDlg::DisplayValue(const era_options::EOption &option) const
{
    return era_options::ReadOptionValue(option, currentMap);
}

void EraMenuDlg::CreateBackground()
{
    background = TiledBackground(widthDlg, heightDlg);
    if (!background) return;
    FrameRegion(0, 0, widthDlg, heightDlg, TRUE, 0);
    if (auto *item = H3DlgPcx16::Create(0, 0, widthDlg, heightDlg, 0, nullptr))
    {
        item->SetPcx(background);
        AddItem(item);
    }
}

void EraMenuDlg::CreateWidgets()
{
    CreateText(18, 12, widthDlg - 486, 26, UiText("title", "ERA Options"),
        NH3Dlg::Text::BIG, eTextColor::GOLD, -1, eTextAlignment::MIDDLE_LEFT);
    Button(this, widthDlg - 466, 12, 122, 28, BansId, UiText("object_bans", "Object bans"));
    Button(this, widthDlg - 334, 12, 104, 28, ResizeId, fullScreen ?
        UiText("windowed", "Window (F11)") : UiText("fullscreen", "Full screen (F11)"));
    if (auto *defaults = Button(this, widthDlg - 220, 12, 110, 28, DefaultsId, UiText("defaults", "Defaults")))
        if (!allowEditing) { defaults->Shade(); defaults->DeActivate(); }
    Button(this, widthDlg - 100, 12, 80, 28, CloseId, "OK");
    CreateText(18, 43, widthDlg - 36, 16, UiText("search", "Search all mods by name, key, ID or tags (Ctrl+F)"),
        NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
    search = CreateEdit(18, 60, widthDlg - 138, 22, 255, "", NH3Dlg::Text::SMALL,
        eTextColor::REGULAR, eTextAlignment::MIDDLE_LEFT, nullptr, SearchId, TRUE, 2, 2);
    if (search) search->SetAutoredraw(TRUE);
    if (search) search->SetText(model.Query().c_str());
    Button(this, widthDlg - 110, 58, 90, 26, ClearId, UiText("clear", "Clear"));
    listHeight = heightDlg - ListTop - 88;
    sidebarHeight = heightDlg - SidebarTop - 88;
    sidebarWidth = (std::max)(160, widthDlg / 4);
    optionX = sidebarWidth + 30;
    columnWidth = (widthDlg - optionX - 42) / 2;
    textWidth = columnWidth - 40;
    CreateFrame(sidebarWidth + 20, SidebarTop, 1, sidebarHeight, -1, H3RGB565::Black());
    pageTitle = CreateText(optionX + 8, SidebarTop, widthDlg - optionX - 42, 22, "",
        NH3Dlg::Text::SMALL, eTextColor::GOLD, -1, eTextAlignment::MIDDLE_LEFT);
    for (int slot = 0; slot < (std::max)(1, sidebarHeight / ModRowHeight); ++slot)
        modRows.push_back(Button(this, 18, SidebarTop + slot * ModRowHeight,
            sidebarWidth - 24, ModRowHeight - 4, ModRowBase + slot, ""));
    modScroll = H3DlgScrollbar::Create(sidebarWidth - 2, SidebarTop, 16, sidebarHeight,
        ModScrollId, 2, ModScrollProc, false, 1, true);
    if (modScroll) AddItem(modScroll);

    // Each column packs independently; only visible cells need native controls.
    const int slots = 2 * ((std::max)(1, listHeight / MinimumRowHeight) + 1);
    optionRows.reserve(slots);
    for (int slot = 0; slot < slots; ++slot)
    {
        OptionRow row;
        const int x = optionX + (slot % 2) * columnWidth;
        row.name = CreateText(x + 32, ListTop, textWidth, 20, "", NH3Dlg::Text::SMALL,
            eTextColor::REGULAR, OptionNameBase + slot, eTextAlignment::TOP_LEFT);
        row.checkbox = H3DlgDefButton::Create(x + 8, ListTop + 4, CheckboxBase + slot,
            "checkbox.def", 0, 1, false, 0);
        if (row.checkbox) AddItem(row.checkbox);
        row.radio = H3DlgDefButton::Create(x + 8, ListTop + 2, RadioBase + slot,
            "radiobttn.def", 0, 1, false, 0);
        if (row.radio) AddItem(row.radio);
        optionRows.push_back(row);
    }
    empty = CreateText(optionX + 8, ListTop + 12, widthDlg - optionX - 52, listHeight - 24,
        UiText("no_results", "No options found."), NH3Dlg::Text::MEDIUM,
        eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_CENTER);
    summary = CreateText(18, heightDlg - 70, widthDlg - 320, 28, "", NH3Dlg::Text::SMALL,
        eTextColor::GOLD, -1, eTextAlignment::MIDDLE_LEFT);
    Button(this, widthDlg - 152, heightDlg - 70, 132, 28, ErrorsId, UiText("errors", "JSON errors"));
    Button(this, widthDlg - 294, heightDlg - 70, 132, 28, ExportId, UiText("export", "Export JSON"));
    if (auto *cancel = CreateCancelButton()) cancel->HideDeactivate();
}

void EraMenuDlg::RebuildOptions()
{
    rowHeights.clear();
    H3Font *font = optionRows.empty() || !optionRows.front().name ? nullptr : optionRows.front().name->GetFont();
    const int lineHeight = font ? static_cast<int>(font->height) : 16;
    grid = era_options::BuildOptionGrid(model.Categories(), [&](const era_options::OptionCell &cell) {
        const bool category = cell.kind == era_options::OptionCellKind::Category;
        const int width = category || cell.kind == era_options::OptionCellKind::Heading ? columnWidth - 16 : textWidth;
        std::string text = era_options::OptionCellName(cell);
        if (category) text = std::string(cell.category->collapsed ? "[+] " : "[-] ") +
            (text.empty() ? UiText("general", "General") : text);
        const int lines = font ? font->GetLinesCountInText(text.c_str(), width) : 1;
        return (std::max)(MinimumRowHeight, (std::max)(1, lines) * lineHeight + 4);
    }, listHeight);
    for (const auto &row : grid) rowHeights.push_back(row.height);
    lastOptionStart = era_options::LastGridStart(rowHeights, listHeight);
    SelectPageScrollbar();
    firstOption = model.ScrollPosition();
    if (pageTitle)
    {
        std::string title;
        if (model.IsSearching()) title = UiText("search_results", "Search results: all mods");
        else if (model.SelectedMod() >= 0)
        {
            const auto &mod = model.Mods()[model.SelectedMod()];
            const auto &page = mod.pages[model.SelectedPage()].name;
            title = mod.name + " / " + (page.empty() ? UiText("general", "General") : page);
        }
        pageTitle->SetText(title.c_str());
    }
    RefreshOptions();
}

void EraMenuDlg::SelectPageScrollbar()
{
    if (optionScroll) optionScroll->HideDeactivate();
    optionScroll = nullptr;
    if (lastOptionStart == 0) return;
    const std::pair<int, int> page = model.IsSearching() ? std::make_pair(-1, -1) :
        std::make_pair(model.SelectedMod(), model.SelectedPage());
    const auto found = pageScrollbars.find(page);
    if (found != pageScrollbars.end()) { optionScroll = found->second; return; }
    // A page owns its native scrollbar while cached; page positions live
    // independently in the model even if an unusually large tree evicts it.
    if (pageScrollbars.size() == MaxCachedScrollbars)
    {
        auto cached = pageScrollbars.begin();
        optionScroll = cached->second;
        pageScrollbars.erase(cached);
    }
    else
    {
        optionScroll = H3DlgScrollbar::Create(widthDlg - 32, ListTop, 16, listHeight,
            PageScrollBase + static_cast<int>(pageScrollbars.size()), 2, OptionScrollProc, false, 1, true);
        if (optionScroll) AddItem(optionScroll);
    }
    if (optionScroll) pageScrollbars.emplace(page, optionScroll);
}

void EraMenuDlg::RefreshMods(bool revealSelection)
{
    sidebarEntries.clear();
    for (int mod = 0; mod < static_cast<int>(model.Mods().size()); ++mod)
    {
        sidebarEntries.push_back({mod, -1});
        if (model.Mods()[mod].expanded)
            for (int page = 0; page < static_cast<int>(model.Mods()[mod].pages.size()); ++page)
                sidebarEntries.push_back({mod, page});
    }
    if (revealSelection)
    {
        const bool expanded = model.SelectedMod() >= 0 && model.Mods()[model.SelectedMod()].expanded;
        for (int index = 0; index < static_cast<int>(sidebarEntries.size()); ++index)
            if (sidebarEntries[index].mod == model.SelectedMod() &&
                sidebarEntries[index].page == (expanded ? model.SelectedPage() : -1))
            {
                if (index < firstMod) firstMod = index;
                if (index >= firstMod + static_cast<int>(modRows.size()))
                    firstMod = index - static_cast<int>(modRows.size()) + 1;
                break;
            }
    }
    firstMod = UpdateScroll(modScroll, (std::max)(0, static_cast<int>(sidebarEntries.size()) -
        static_cast<int>(modRows.size())), firstMod, P_WindowManager->lastDlg == this);
    for (size_t slot = 0; slot < modRows.size(); ++slot)
    {
        auto *button = modRows[slot];
        if (!button) continue;
        const int index = firstMod + static_cast<int>(slot);
        if (index >= static_cast<int>(sidebarEntries.size())) { button->HideDeactivate(); continue; }
        const auto &entry = sidebarEntries[index];
        const auto &mod = model.Mods()[entry.mod];
        std::string label;
        bool selected = false;
        if (entry.page < 0)
        {
            label = std::string(mod.expanded ? "[-] " : "[+] ") + mod.name;
            selected = !model.IsSearching() && entry.mod == model.SelectedMod();
        }
        else
        {
            const auto &name = mod.pages[entry.page].name;
            label = name.empty() ? UiText("general", "General") : name;
            selected = !model.IsSearching() && entry.mod == model.SelectedMod() && entry.page == model.SelectedPage();
        }
        const int indent = entry.page < 0 ? 0 : 14;
        button->SetX(18 + indent);
        button->SetWidth(static_cast<UINT16>(sidebarWidth - 24 - indent));
        button->SetText(label.c_str());
        button->SetFrame(selected ? 1 : 0);
        button->ShowActivate();
    }
    UpdateScrollActivity();
}

void EraMenuDlg::RefreshOptions()
{
    firstOption = UpdateScroll(optionScroll, lastOptionStart, firstOption, P_WindowManager->lastDlg == this);
    model.SetScrollPosition(firstOption);
    for (auto &row : optionRows)
    {
        row.cell = {};
        for (H3DlgItem *item : {static_cast<H3DlgItem *>(row.name), static_cast<H3DlgItem *>(row.checkbox),
                               static_cast<H3DlgItem *>(row.radio)})
            if (item) item->HideDeactivate();
    }
    size_t slot = 0;
    int y = ListTop;
    for (int index = firstOption; index < static_cast<int>(grid.size()) && y < ListTop + listHeight; ++index)
    {
        for (int column = 0; column < 2; ++column)
        {
            const auto &cell = column ? grid[index].right : grid[index].left;
            const int cellHeight = column ? grid[index].rightHeight : grid[index].leftHeight;
            if (cell.kind == era_options::OptionCellKind::Empty || y + cellHeight > ListTop + listHeight ||
                slot >= optionRows.size()) continue;
            auto &row = optionRows[slot++];
            row.cell = cell;
            const bool category = cell.kind == era_options::OptionCellKind::Category;
            const bool heading = category || cell.kind == era_options::OptionCellKind::Heading;
            const bool editable = category || (allowEditing && era_options::IsOptionEditable(*cell.option, currentMap));
            const int x = optionX + column * columnWidth;
            if (row.name)
            {
                row.name->SetX(x + (heading ? 8 : 28));
                row.name->SetWidth(static_cast<UINT16>(heading ? columnWidth - 16 : textWidth));
                row.name->SetY(y + 2);
                row.name->SetHeight(static_cast<UINT16>(cellHeight - 4));
                std::string name = era_options::OptionCellName(cell);
                if (category) name = std::string(cell.category->collapsed ? "[+] " : "[-] ") +
                    (name.empty() ? UiText("general", "General") : name);
                row.name->SetText(heading ? ("{" + name + "}").c_str() : name.c_str());
                row.name->ShowActivate();
                if (!editable) row.name->Shade(); else row.name->UnShade();
            }
            H3DlgDefButton *button = cell.kind == era_options::OptionCellKind::Radio ? row.radio :
                cell.kind == era_options::OptionCellKind::Checkbox ? row.checkbox : nullptr;
            if (button)
            {
                const int checked = cell.kind == era_options::OptionCellKind::Radio ?
                    DisplayValue(*cell.option) == cell.choice : (std::max)(0, DisplayValue(*cell.option));
                const int frame = editable ? checked * 2 : 4 + checked;
                button->SetX(x + 8);
                button->SetY(y + 2);
                button->SetFrame(frame);
                button->SetClickFrame(editable ? frame + 1 : frame);
                button->ShowActivate();
            }
        }
        y += grid[index].height;
    }
    if (empty)
    {
        if (model.Options().empty()) empty->ShowActivate(); else empty->HideDeactivate();
    }
    if (summary)
    {
        std::string text = std::to_string(model.Options().size()) + " / " +
            std::to_string(era_options::Registry().Options().size());
        if (!allowEditing) text += " | " + std::string(currentMap ?
            UiText("map_view", "Current map: read only") : UiText("view", "Read only"));
        else if (currentMap) text += " | " + std::string(UiText("map_live", "Current map: only supported options can be changed"));
        summary->SetText(text.c_str());
    }
    UpdateScrollActivity();
    if (P_WindowManager->lastDlg == this) Redraw();
}

const EraMenuDlg::OptionRow *EraMenuDlg::RowAtControl(int controlId) const
{
    for (size_t slot = 0; slot < optionRows.size(); ++slot)
        if (controlId == OptionNameBase + slot || controlId == CheckboxBase + slot || controlId == RadioBase + slot)
            return &optionRows[slot];
    return nullptr;
}

era_options::EOption *EraMenuDlg::OptionAtControl(int controlId) const
{
    const auto *row = RowAtControl(controlId);
    return row ? row->cell.option : nullptr;
}

BOOL EraMenuDlg::OnLeftClick(INT itemId, H3Msg &)
{
    if (itemId == CloseId) { SaveAndClose(); return FALSE; }
    if (itemId == ResizeId) { resizeRequested = true; Stop(); return FALSE; }
    if (itemId == BansId)
    {
        if (!currentMap) era_options::ReloadObjectBanPreferences();
        if (!currentMap && !era_options::ObjectBanLoadError().empty())
            H3Messagebox((era_options::ObjectBanPreferencesPath() + "\n\n" + era_options::ObjectBanLoadError()).c_str());
        else if (SaveAndClose()) bansRequested = true;
        return FALSE;
    }
    if (itemId == ClearId)
    {
        if (search) search->SetText("");
        SyncSearch();
        return FALSE;
    }
    if (itemId == ErrorsId) { ShowErrors(); return FALSE; }
    if (itemId == DefaultsId)
    {
        if (!allowEditing) return FALSE;
        for (const auto *option : model.Options())
            era_options::ChangeOptionValue(option->GetId(), option->Definition().defaultValue, currentMap);
        RefreshOptions();
        return FALSE;
    }
    if (itemId == ExportId)
    {
        std::string error;
        if (era_options::ExportOptionsJson(era_options::DefaultExportPath(), error, allowEditing, currentMap))
            H3Messagebox(UiText("export_done", "All options exported to ERA_OptionsMenu.export.json in the game directory."));
        else
            H3Messagebox((std::string(UiText("export_error", "Could not export options: ")) + error).c_str());
        return FALSE;
    }
    const int slot = itemId - ModRowBase;
    if (slot >= 0 && slot < static_cast<int>(modRows.size()) && firstMod + slot < static_cast<int>(sidebarEntries.size()))
    {
        const auto entry = sidebarEntries[firstMod + slot];
        if (entry.page < 0) model.ClickMod(entry.mod);
        else model.SelectPage(entry.mod, entry.page);
        if (search) { search->SetText(""); search->SetFocus(FALSE); }
        RebuildOptions();
        RefreshMods(true);
        Redraw();
        return FALSE;
    }
    if (const auto *row = RowAtControl(itemId))
    {
        const auto &cell = row->cell;
        if (cell.kind == era_options::OptionCellKind::Category)
        {
            model.ToggleCategory(cell.categoryIndex);
            RebuildOptions();
            return FALSE;
        }
        if (!allowEditing || !cell.option || cell.kind == era_options::OptionCellKind::Heading ||
            !era_options::IsOptionEditable(*cell.option, currentMap)) return FALSE;
        const int next = cell.kind == era_options::OptionCellKind::Radio ? cell.choice : 1 - DisplayValue(*cell.option);
        if (!era_options::ChangeOptionValue(cell.option->GetId(), next, currentMap) && !era_options::LastChangeError().empty())
            H3Messagebox(era_options::LastChangeError().c_str());
        RefreshOptions();
        return FALSE;
    }
    return TRUE;
}

BOOL EraMenuDlg::OnRightClick(H3DlgItem *item)
{
    if (item)
        if (const auto *option = OptionAtControl(item->GetID()))
        {
            ShowOptionPreview(*option, DisplayValue(*option), allowEditing, currentMap);
            return FALSE;
        }
    return TRUE;
}

BOOL EraMenuDlg::OnMouseHover(H3DlgItem *item)
{
    if (GetHintBar())
    {
        const auto *option = item ? OptionAtControl(item->GetID()) : nullptr;
        std::string hint = option ? option->Definition().hint : "";
        if (option)
        {
            if (DisplayValue(*option) < 0) hint += " | " + std::string(UiText("value_unavailable", "Cannot read the current value."));
            if (!allowEditing) hint += " | " + std::string(ReadOnlyReason(currentMap));
            else
            {
                const auto dependency = era_options::OptionDependencyHint(*option, currentMap);
                if (!dependency.empty()) hint = dependency;
                else if (currentMap && !option->Definition().changeDuringGame) hint += " | " + std::string(ReadOnlyReason(true));
            }
        }
        GetHintBar()->ShowMessage(hint.c_str());
    }
    return TRUE;
}

void EraMenuDlg::UpdateScrollActivity()
{
    const auto cursor = H3POINT::GetCursorPosition();
    const auto area = P_WindowManager->lastDlg == this ? era_options::ScrollAreaAt(
        cursor.x - xDlg, cursor.y - yDlg, sidebarWidth, SidebarTop, sidebarHeight,
        optionX, ListTop, widthDlg - optionX - 16, listHeight) : era_options::ScrollArea::None;
    if (modScroll && modScroll->IsVisible())
    {
        if (area == era_options::ScrollArea::Mods) modScroll->Activate();
        else modScroll->DeActivate();
    }
    if (optionScroll && optionScroll->IsVisible())
    {
        if (area == era_options::ScrollArea::Options) optionScroll->Activate();
        else optionScroll->DeActivate();
    }
}

INT EraMenuDlg::vPreProcess(H3Msg &msg)
{
    UpdateScrollActivity();
    return H3Dlg::vPreProcess(msg);
}

BOOL EraMenuDlg::OnMouseWheel(INT)
{
    // Native routing scrolls only the active panel; do not scroll a second time.
    UpdateScrollActivity();
    return TRUE;
}

bool EraMenuDlg::SaveAndClose()
{
    if (allowEditing && !currentMap && !era_options::SaveOptionValues())
    {
        const std::string text = std::string(UiText("save_error", "Could not save option values or the WoG profile.")) +
            "\n\n" + era_options::LastSaveError();
        H3Messagebox(text.c_str());
        return false;
    }
    Stop();
    return true;
}

void EraMenuDlg::OnClose(INT) { SaveAndClose(); }

void EraMenuDlg::SyncSearch()
{
    const std::string current = search && search->GetText() ? search->GetText() : "";
    if (current == model.Query()) return;
    model.SetQuery(current);
    RebuildOptions();
    RefreshMods();
    if (P_WindowManager->lastDlg == this) Redraw();
}

INT EraMenuDlg::vDialogProc(H3Msg &msg)
{
    if (!activated && P_WindowManager->lastDlg == this)
    {
        activated = true;
        RefreshMods();
        RefreshOptions();
    }
    const INT result = H3Dlg::vDialogProc(msg);
    // DefaultProc edits the text. Read AFTER it so the last character appears
    // immediately, without waiting for a mouse move or another key event.
    if (!endDialog) SyncSearch();
    return result;
}

BOOL EraMenuDlg::OnKeyPress(eVKey key, eMsgFlag)
{
    if (key == eVKey::H3VK_F11) { resizeRequested = true; Stop(); return FALSE; }
    if (key == eVKey::H3VK_ESCAPE)
    {
        if (model.IsSearching())
        {
            if (search) search->SetText("");
            SyncSearch();
        }
        else SaveAndClose();
        return FALSE;
    }
    if (key == eVKey::H3VK_F && (GetKeyState(VK_CONTROL) & 0x8000))
    {
        if (search) search->SetFocus(TRUE);
        return FALSE;
    }
    return TRUE;
}

void EraMenuDlg::ShowErrors()
{
    std::string text;
    if (!era_options::ObjectBanLoadError().empty())
        text = era_options::ObjectBanPreferencesPath() + "\n" + era_options::ObjectBanLoadError() + "\n\n";
    for (const auto &issue : era_options::LastLoadResult().issues)
        text += issue.path + "\n" + issue.message + "\n\n";
    H3Messagebox(text.empty() ? "JSON: OK" : text.c_str());
}

void __fastcall EraMenuDlg::ModScrollProc(INT32 tick, H3BaseDlg *dlg)
{
    auto *self = static_cast<EraMenuDlg *>(dlg);
    self->firstMod = tick;
    self->RefreshMods();
    self->Redraw();
}

void __fastcall EraMenuDlg::OptionScrollProc(INT32 tick, H3BaseDlg *dlg)
{
    auto *self = static_cast<EraMenuDlg *>(dlg);
    self->firstOption = tick;
    self->RefreshOptions();
}

BOOL EraMenuDlg::CreateAndRun(bool showBans, bool allowEditing)
{
    // Menu transitions reuse this loop rather than opening another instance.
    static bool running = false;
    if (running) return FALSE;
    struct RunningGuard
    {
        bool &flag;
        explicit RunningGuard(bool &value) : flag(value) { flag = true; }
        ~RunningGuard() { flag = false; }
    } runningGuard(running);
    struct ResultGuard
    {
        int saved = P_WindowManager->resultItemID;
        ~ResultGuard() { P_WindowManager->resultItemID = saved; }
    } resultGuard;
    const bool currentMap = era_options::IsOnMap();
    // ERA owns the VFS/locale merge and translation references. Reload before
    // constructing any controls so descriptions reflect edits made during play.
    Era::ReloadLanguageData();
    era_options::InvalidateOptionTexts();
    era_options::EnsureOptionsLoaded(allowEditing && !currentMap);
    if (currentMap) era_options::BeginCurrentMapOptions();
    if (!currentMap) era_options::ReloadObjectBanPreferences();
    if (showBans && !currentMap && !era_options::ObjectBanLoadError().empty())
    {
        H3Messagebox((era_options::ObjectBanPreferencesPath() + "\n\n" + era_options::ObjectBanLoadError()).c_str());
        return FALSE;
    }
    era_options::OptionMenuModel state(era_options::Registry(), GameCodePage());
    const auto &settingsPath = era_options::ValuesFilePath();
    bool full = GetPrivateProfileIntA("Dialog", "FullScreen", 0, settingsPath.c_str()) != 0;
    int sidebarPosition = 0;
    ObjectBansDlg::State bansState;
    bool resize = false, switchMenu = false, visitedOptions = false;
    do
    {
        resize = switchMenu = false;
        const auto size = era_options::OptionsDialogSize(H3GameWidth::Get(), H3GameHeight::Get(), full);
        if (showBans)
        {
            ObjectBansDlg dlg(size.width, size.height, bansState, allowEditing, currentMap);
            dlg.Start();
            bansState = dlg.GetState();
            switchMenu = dlg.OptionsRequested();
            if (switchMenu) showBans = false;
        }
        else
        {
            visitedOptions = true;
            if (allowEditing) era_options::RefreshWogValues(currentMap);
            EraMenuDlg dlg(size.width, size.height, full, state, sidebarPosition, allowEditing, currentMap);
            dlg.Start();
            state = dlg.model;
            sidebarPosition = dlg.firstMod;
            resize = dlg.resizeRequested;
            switchMenu = dlg.bansRequested;
            if (switchMenu) showBans = true;
        }
        if (resize)
        {
            full = !full;
            if (!WritePrivateProfileStringA("Dialog", "FullScreen", full ? "1" : "0", settingsPath.c_str()))
                H3Messagebox(UiText("resize_save_error", "Could not save the dialog size setting."));
        }
    } while (resize || switchMenu);
    if (!allowEditing || currentMap) return TRUE;
    std::string error;
    const bool saved = visitedOptions ? era_options::SaveOptionValues() : era_options::SaveObjectBanPreferences(error);
    if (!saved) H3Messagebox((std::string(UiText("save_error", "Could not save settings.")) + "\n\n" +
        (visitedOptions ? era_options::LastSaveError() : error)).c_str());
    return saved ? TRUE : FALSE;
}

BOOL CreateAndRunEraMenuDlg() { return EraMenuDlg::CreateAndRun(); }
BOOL CreateAndRunObjectBansDlg() { return EraMenuDlg::CreateAndRun(true); }

#pragma comment(linker, "/EXPORT:EraOptions_ShowDialog=_EraOptions_ShowDialog@8")
extern "C" int32_t __stdcall EraOptions_ShowDialog(int32_t showBans, int32_t allowEditing)
{
    try { return EraMenuDlg::CreateAndRun(showBans != 0, allowEditing != 0); }
    catch (...) { return 0; }
}
