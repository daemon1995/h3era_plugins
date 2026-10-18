#include "HotkeysPage.h"
#include "HelpUI.h"
#include "CardLayout.h"
#include "ScrollbarUtils.h"

namespace main
{
namespace
{
const int kContexts[] = {-2, -1, 1, 2, 3, 4, 5, 6};
constexpr int kContextFirst = 26000;
constexpr int kCategoryScroll = 25998;
constexpr int kContentScroll = 25999;
constexpr int kRowHeight = 24;
} // namespace
HotkeysSection *HotkeysSection::visibleSection = nullptr;

HotkeysSection::HotkeysSection(int cx, int cy, int cw, int ch, int x, int y, int w, int h, H3Dlg *parent,
                               const std::vector<ModInformation *> &source)
    : DlgPage(parent), mods(source), categoriesX(cx), categoriesY(cy), categoriesWidth(cw), categoriesHeight(ch),
      contentX(x), contentY(y), contentWidth(w), contentHeight(h)
{
    AddFrame(cx, cy, cw, ch);
    AddFrame(x, y, w, h);
    const int categoryStep = helpdlg::CategoryButtonStep();
    categoryRows = std::min(8, std::max(1, (ch - 12) / categoryStep));
    for (int row = 0; row < categoryRows; ++row)
    {
        auto *button = helpdlg::CategoryButton(cx + 6, cy + 6 + row * categoryStep, kContextFirst + row, h3_NullString);
        AddItem(button);
        contexts.push_back(button);
    }
    categoryScroll = H3DlgScrollbar::Create(cx + cw - 22, cy + 2, 16, std::max(16, ch - 4), kCategoryScroll, 2,
                                            CategoryScrollProc, false, 1, true);
    AddItem(categoryScroll);

    const int textLeft = x + 10;
    const int textWidth = w - 40;
    const int keyWidth = std::min(220, std::max(120, textWidth * 42 / 100));
    visibleRows = std::max(1, (h - 16) / kRowHeight);
    for (int row = 0; row < visibleRows; ++row)
    {
        const int top = y + 8 + row * kRowHeight;
        auto *heading = H3DlgText::Create(textLeft + 4, top + 2, textWidth - 4, kRowHeight - 2, h3_NullString,
                                          NH3Dlg::Text::MEDIUM, eTextColor::GOLD, -1, eTextAlignment::MIDDLE_LEFT);
        auto *key = H3DlgText::Create(textLeft + 8, top, keyWidth - 8, kRowHeight, h3_NullString, NH3Dlg::Text::MEDIUM,
                                      eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
        auto *action = H3DlgText::Create(textLeft + keyWidth, top, textWidth - keyWidth, kRowHeight, h3_NullString,
                                         P_SmallFont->GetName(), eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
        AddItem(heading);
        AddItem(key);
        AddItem(action);
        headingRows.push_back(heading);
        keyRows.push_back(key);
        actionRows.push_back(action);

        auto *separator = H3DlgFrame::Create(textLeft, top, textWidth, 1, -1, H3RGB565(H3RGB888(190, 151, 70)));
        if (separator)
        {
            dialog->AddItem(separator);
            decorations.push_back(separator);
            separator->HideDeactivate();
        }
        rowSeparators.push_back(separator);
    }
    contentScroll =
        H3DlgScrollbar::Create(x + w - 22, y + 6, 16, h - 12, kContentScroll, 2, ContentScrollProc, false, 1, true);
    AddItem(contentScroll);
    Rebuild();
}

void HotkeysSection::SetVisible(BOOL state) noexcept
{
    DlgPage::SetVisible(state);
    if (state)
    {
        visibleSection = this;
        RefreshCategories();
        RenderRows();
    }
    else if (visibleSection == this)
        visibleSection = nullptr;
}

void HotkeysSection::SetSubtype(int subtype)
{
    activeSubtype = helpdlg::Bound(subtype, 0, 7);
    if (activeSubtype < categoryFirst)
        categoryFirst = activeSubtype;
    else if (activeSubtype >= categoryFirst + categoryRows)
        categoryFirst = activeSubtype - categoryRows + 1;
    categoryFirst = helpdlg::Bound(categoryFirst, 0, std::max(0, 8 - categoryRows));
    firstLine = 0;
    Rebuild();
}

helpdlg::HotkeyFilter HotkeysSection::Filter() const
{
    helpdlg::HotkeyFilter filter;
    filter.context = activeSubtype;
    filter.firstRow = firstLine;
    return filter;
}

void HotkeysSection::SetFilter(const helpdlg::HotkeyFilter &filter)
{
    activeSubtype = helpdlg::Bound(filter.context, 0, 7);
    categoryFirst = helpdlg::Bound(activeSubtype - categoryRows + 1, 0, 8 - categoryRows);
    Rebuild();
    firstLine = helpdlg::Bound(filter.firstRow, 0, std::max(0, static_cast<int>(lines.size()) - visibleRows));
    RenderRows();
}

void HotkeysSection::FocusHotkey(LPCSTR folder, int context, int hotkeyId)
{
    if (context == hkcategories::NONE)
        context = hkcategories::OTHER_DLG;
    const auto position = std::find(std::begin(kContexts), std::end(kContexts), context);
    activeSubtype = position == std::end(kContexts) ? 0 : static_cast<int>(position - std::begin(kContexts));
    categoryFirst = helpdlg::Bound(activeSubtype - categoryRows + 1, 0, 8 - categoryRows);
    for (auto *mod : mods)
        if (mod && mod->hotkeysCategory && helpdlg::Safe(mod->path.String()) == helpdlg::Safe(folder))
        {
            Rebuild(static_cast<int>(mod->id), hotkeyId);
            return;
        }
    Rebuild();
}

void HotkeysSection::Rebuild(int focusModId, int focusHotkeyId)
{
    std::vector<helpdlg::HotkeyLine> entries;
    for (auto *mod : mods)
    {
        if (!mod || !mod->hotkeysCategory)
            continue;
        const LPCSTR source = mod->name;
        for (const auto &key : mod->hotkeysCategory->hotkeys)
        {
            const int context = helpdlg::HotkeyContext(key.type);
            if (activeSubtype && context != kContexts[activeSubtype])
                continue;
            entries.push_back(
                {context, source, key.keys, key.name, static_cast<int>(mod->id), key.id, key.description});
        }
    }
    std::stable_sort(entries.begin(), entries.end(), [](const helpdlg::HotkeyLine &a, const helpdlg::HotkeyLine &b) {
        if (a.context != b.context)
            return a.context < b.context;
        return a.sourceId != b.sourceId ? a.sourceId < b.sourceId : a.id < b.id;
    });

    lines.clear();
    int previousContext = -1000;
    int focusLine = -1;
    for (const auto &entry : entries)
    {
        if (entry.context != previousContext)
        {
            DisplayLine heading;
            heading.heading = true;
            heading.left = helpdlg::ContextName(entry.context);
            lines.push_back(std::move(heading));
            previousContext = entry.context;
        }
        DisplayLine row;
        row.left = std::string("[") + entry.keys + "]";
        row.right = entry.name;
        if (*entry.source)
            row.right += std::string(" (") + entry.source + ")";
        row.sourceId = entry.sourceId;
        row.hotkeyId = entry.id;
        if (entry.sourceId == focusModId && entry.id == focusHotkeyId)
            focusLine = static_cast<int>(lines.size());
        lines.push_back(std::move(row));
    }
    if (lines.empty())
    {
        DisplayLine row;
        row.right = helpdlg::Text("help.ui.no_hotkeys");
        lines.push_back(std::move(row));
    }
    if (focusLine >= 0)
        firstLine = helpdlg::Bound(focusLine - 1, 0, std::max(0, static_cast<int>(lines.size()) - visibleRows));
    else
        firstLine = helpdlg::Bound(firstLine, 0, std::max(0, static_cast<int>(lines.size()) - visibleRows));
    RefreshCategories();
    RenderRows();
}

void HotkeysSection::RefreshCategories()
{
    const int maxFirst = std::max(0, 8 - categoryRows);
    categoryFirst = helpdlg::UpdateScrollbar(categoryScroll, maxFirst, categoryFirst, isVisible, TRUE);
    if (categoryScroll)
    {
        if (isVisible)
        {
            categoryScroll->Show();
            maxFirst > 0 ? categoryScroll->Activate() : categoryScroll->DeActivate();
        }
        else
            categoryScroll->HideDeactivate();
    }
    for (int row = 0; row < categoryRows; ++row)
    {
        auto *button = contexts[row];
        const int index = categoryFirst + row;
        if (!button)
            continue;
        if (index < 8 && isVisible)
        {
            const LPCSTR caption = index == 0 ? helpdlg::Text("help.ui.all") : helpdlg::ContextName(kContexts[index]);
            button->SetText(caption);
            button->ShowActivate();
            helpdlg::SetCategoryState(button, index == activeSubtype);
        }
        else
        {
            button->HideDeactivate();
        }
    }
}

void HotkeysSection::RenderRows()
{
    const int maxFirst = std::max(0, static_cast<int>(lines.size()) - visibleRows);
    firstLine = helpdlg::UpdateScrollbar(contentScroll, maxFirst, firstLine, isVisible, TRUE);
    for (int row = 0; row < visibleRows; ++row)
    {
        const int index = firstLine + row;
        const bool visible = isVisible && index < static_cast<int>(lines.size());
        auto *heading = headingRows[row];
        auto *key = keyRows[row];
        auto *action = actionRows[row];
        auto *separator = rowSeparators[row];
        if (!visible)
        {
            if (heading)
                heading->HideDeactivate();
            if (key)
                key->HideDeactivate();
            if (action)
                action->HideDeactivate();
            if (separator)
                separator->HideDeactivate();
            continue;
        }
        const auto &line = lines[index];
        if (line.heading)
        {
            if (heading)
            {
                heading->SetText(line.left.c_str());
                heading->ShowActivate();
            }
            if (key)
                key->HideDeactivate();
            if (action)
                action->HideDeactivate();
            if (separator)
            {
                if (index > 0)
                    separator->Show()->DeActivate();
                else
                    separator->HideDeactivate();
            }
        }
        else
        {
            if (heading)
                heading->HideDeactivate();
            if (key)
            {
                key->SetText(line.left.c_str());
                key->ShowActivate();
            }
            if (action)
            {
                action->SetText(line.right.c_str());
                action->ShowActivate();
            }
            if (separator)
                separator->HideDeactivate();
        }
    }
    RedrawDialog();
}

void HotkeysSection::UpdateMousePosition(const H3Msg &msg) noexcept
{
    if (!isVisible || !msg.IsMouseOver() || !dialog)
        return;
    const int mx = msg.GetX() - dialog->GetX();
    const int my = msg.GetY() - dialog->GetY();
    const bool inCategories = mx >= categoriesX && mx < categoriesX + categoriesWidth && my >= categoriesY &&
                              my < categoriesY + categoriesHeight;
    const bool inContent =
        mx >= contentX && mx < contentX + contentWidth && my >= contentY && my < contentY + contentHeight;
    if (categoryScroll)
        inCategories && 8 > categoryRows ? categoryScroll->Activate() : categoryScroll->DeActivate();
    if (contentScroll)
        inContent &&static_cast<int>(lines.size()) > visibleRows ? contentScroll->Activate()
                                                                 : contentScroll->DeActivate();
}

BOOL HotkeysSection::ProcessMessage(H3Msg &msg)
{
    if (!isVisible || !msg.IsLeftClick())
        return FALSE;
    const int row = msg.itemId - kContextFirst;
    if (row >= 0 && row < categoryRows && categoryFirst + row < 8)
    {
        SetSubtype(categoryFirst + row);
        return TRUE;
    }
    return FALSE;
}

void HotkeysSection::Redraw()
{
    RefreshCategories();
    RenderRows();
}

void __fastcall HotkeysSection::CategoryScrollProc(INT32 tick, H3BaseDlg *)
{
    if (visibleSection)
    {
        visibleSection->categoryFirst = tick;
        visibleSection->RefreshCategories();
        visibleSection->RedrawDialog();
    }
}
void __fastcall HotkeysSection::ContentScrollProc(INT32 tick, H3BaseDlg *)
{
    if (visibleSection)
    {
        visibleSection->firstLine = tick;
        visibleSection->RenderRows();
    }
}
} // namespace main

namespace helpdlg
{
bool ShowHotkeyPreview(const HotKey &key, LPCSTR modName)
{
    const int width = std::min(560, H3GameWidth::Get() - 20);
    const int maxHeight = std::min(520, H3GameHeight::Get() - 20);
    const int left = 18, contentWidth = width - 36;
    H3BinaryLoader<H3Font> keyFont(NH3Dlg::Text::BIG);
    H3BinaryLoader<H3Font> titleFont(NH3Dlg::Text::MEDIUM);
    H3BinaryLoader<H3Font> sourceFont(NH3Dlg::Text::SMALL);
    const LPCSTR binding = key.keys;
    const LPCSTR action = key.name;
    const LPCSTR description = key.description;
    const std::string source = Safe(modName) + " / " + ContextName(HotkeyContext(key.type));
    const int keyHeight = std::max(36, CardTextHeight(keyFont.Get(), binding, contentWidth - 24));
    const int boxHeight = keyHeight + 16;
    const int titleY = 18 + boxHeight + 12;
    const int titleHeight = std::max(26, CardTextHeight(titleFont.Get(), action, contentWidth));
    const int sourceY = titleY + titleHeight + 6;
    const int sourceHeight = CardTextHeight(sourceFont.Get(), source, contentWidth);
    const int bodyY = sourceY + sourceHeight + 14;
    const int naturalHeight = !*description ? 0 : CardTextHeight(titleFont.Get(), description, contentWidth - 18);
    const int bodyHeight = std::min(naturalHeight, std::max(0, maxHeight - bodyY - 18));
    H3Dlg dlg(width, bodyY + bodyHeight + 18, -1, -1, false, false);
    if (!AddSafeBackground(dlg))
        return false;
    dlg.CreateFrame(left, 18, contentWidth, boxHeight, -1, H3RGB565(H3RGB888(190, 151, 70)));
    dlg.CreateFrame(left + 1, 19, contentWidth - 2, boxHeight - 2, -1, H3RGB565(H3RGB888(89, 65, 31)));
    dlg.CreateText(left + 12, 26, contentWidth - 24, keyHeight, binding, NH3Dlg::Text::BIG, eTextColor::GOLD, -1);
    dlg.CreateText(left, titleY, contentWidth, titleHeight, action, NH3Dlg::Text::MEDIUM, eTextColor::WHITE, -1);
    dlg.CreateText(left, sourceY, contentWidth, sourceHeight, source.c_str(), NH3Dlg::Text::SMALL, eTextColor::GOLD,
                   -1);
    if (bodyHeight > 0)
    {
        dlg.CreateFrame(left, bodyY - 7, contentWidth, 1, -1, H3RGB565(H3RGB888(116, 100, 70)));
        if (naturalHeight <= bodyHeight)
            dlg.CreateText(left, bodyY, contentWidth, bodyHeight, description, NH3Dlg::Text::MEDIUM,
                           eTextColor::REGULAR, -1, eTextAlignment::TOP_LEFT);
        else if (auto *text = H3DlgScrollableText::Create(description, left, bodyY, contentWidth, bodyHeight,
                                                          NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, false))
            dlg.AddItem(text);
    }
    dlg.RMB_Show();
    return true;
}
} // namespace helpdlg
