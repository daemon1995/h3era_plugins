#include "HotkeysPage.h"
#include "HelpUI.h"
#include "CardLayout.h"

namespace main
{
namespace
{
const int kContexts[] = {-2, -1, 1, 2, 3, 4, 5, 6};
}
HotkeysSection::HotkeysSection(int cx, int cy, int cw, int ch, int x, int y, int w, int h, H3Dlg *parent,
                               const std::vector<ModInformation *> &source)
    : DlgPage(parent), mods(source)
{
    AddFrame(cx, cy, cw, ch);
    AddFrame(x, y, w, h);
    for (int index = 0; index < 8; ++index)
    {
        const LPCSTR label = index == 0 ? helpdlg::Text("help.ui.all", "All") : helpdlg::ContextName(kContexts[index]);
        auto *button = helpdlg::Button(nullptr, cx + 6, cy + 6 + index * 32, cw - 12, 28, 26000 + index, label);
        AddItem(button);
        contexts.push_back(button);
    }
    AddItem(H3DlgText::Create(x + 8, y + 6, 48, 22, helpdlg::Text("help.ui.filter", "Filter:"), NH3Dlg::Text::SMALL,
                              eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT));
    queryEdit = H3DlgEdit::Create(x + 58, y + 6, w - 66, 22, 128, "", NH3Dlg::Text::SMALL, eTextColor::REGULAR,
                                  eTextAlignment::MIDDLE_LEFT, nullptr, 26011, true, 2, 2);
    AddItem(queryEdit);
    if (queryEdit)
        queryEdit->SetHints(helpdlg::Text("help.ui.filter_hint", "Filter titles only."), nullptr, false);
    text = H3DlgScrollableText::Create("", x + 10, y + 30, w - 30, h - 38, NH3Dlg::Text::MEDIUM, eTextColor::REGULAR,
                                       false);
    AddScrollableText(text);
    Rebuild();
}
void HotkeysSection::SetVisible(BOOL state) noexcept
{
    DlgPage::SetVisible(state);
    if (!state && queryEdit && queryEdit->IsFocused())
        queryEdit->SetFocus(FALSE);
}
void HotkeysSection::SetSubtype(int subtype)
{
    activeSubtype = helpdlg::Bound(subtype, 0, 7);
    Rebuild();
}
helpdlg::HotkeyFilter HotkeysSection::Filter() const
{
    helpdlg::HotkeyFilter filter;
    filter.context = activeSubtype;
    filter.query = query;
    filter.firstRow = TextScrollPosition(text);
    return filter;
}
void HotkeysSection::SetFilter(const helpdlg::HotkeyFilter &filter)
{
    activeSubtype = helpdlg::Bound(filter.context, 0, 7);
    query = filter.query;
    if (queryEdit)
        queryEdit->SetText(query.c_str());
    Rebuild();
    SetTextScrollPosition(text, filter.firstRow);
}
void HotkeysSection::FocusHotkey(LPCSTR folder, int context, const std::string &search)
{
    helpdlg::HotkeyFilter filter;
    filter.query = search + " " + helpdlg::Safe(folder);
    if (context == hkcategories::NONE)
        context = hkcategories::OTHER_DLG;
    const auto position = std::find(std::begin(kContexts), std::end(kContexts), context);
    filter.context = position == std::end(kContexts) ? 0 : static_cast<int>(position - std::begin(kContexts));
    SetFilter(filter);
}
void HotkeysSection::Rebuild()
{
    std::vector<helpdlg::HotkeyLine> entries;
    for (auto *mod : mods)
    {
        if (!mod || !mod->hotkeysCategory)
            continue;
        const auto source = helpdlg::Safe(mod->path.String());
        for (const auto &key : mod->hotkeysCategory->hotkeys)
        {
            const int context = helpdlg::HotkeyContext(key.type);
            if (activeSubtype && context != kContexts[activeSubtype])
                continue;
            if (helpdlg::MatchesWords(source + " " + key.keys.String() + " " + key.name.String(), query))
                entries.push_back({context, source, helpdlg::Safe(key.keys.String()), helpdlg::Safe(key.name.String()),
                                   static_cast<int>(mod->id), key.id, helpdlg::Safe(key.description.String())});
        }
    }
    std::string output = helpdlg::GroupHotkeys(entries, helpdlg::ContextName);
    if (entries.empty())
        output += helpdlg::Text("help.ui.no_hotkeys", "No matching hotkeys.");
    if (output != rendered.String())
    {
        rendered = output.c_str();
        SetScrollableText(text, rendered.String());
    }
    for (size_t index = 0; index < contexts.size(); ++index)
        if (contexts[index])
            contexts[index]->SetFrame(index == static_cast<size_t>(activeSubtype) ? 1 : 0);
    RedrawDialog();
}
void HotkeysSection::UpdateMousePosition(const H3Msg &) noexcept {}
BOOL HotkeysSection::ProcessMessage(H3Msg &msg)
{
    FlushTextScrollPositions();
    const std::string current = queryEdit ? helpdlg::Safe(queryEdit->GetText()) : "";
    if (current != query)
    {
        query = current;
        Rebuild();
    }
    if (msg.IsLeftClick() && msg.itemId >= 26000 && msg.itemId < 26008)
    {
        SetSubtype(msg.itemId - 26000);
        return TRUE;
    }
    return FALSE;
}
void HotkeysSection::Redraw()
{
    RedrawDialog();
}
void HotkeysSection::ReleaseInputFocus(int keepItemId) noexcept
{
    auto *edit = queryEdit;
    if (edit && edit->IsFocused() && edit->GetID() != keepItemId)
        edit->SetFocus(FALSE);
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
    const auto binding = Safe(key.keys.String());
    const auto action = key.name.Empty() ? std::string(Text("help.ui.hotkey", "Hotkey")) : Safe(key.name.String());
    const auto description = Safe(key.description.String());
    const std::string source = Safe(modName) + " / " + ContextName(HotkeyContext(key.type));
    const int keyHeight = std::max(36, CardTextHeight(keyFont.Get(), binding, contentWidth - 24));
    const int boxHeight = keyHeight + 16;
    const int titleY = 18 + boxHeight + 12;
    const int titleHeight = std::max(26, CardTextHeight(titleFont.Get(), action, contentWidth));
    const int sourceY = titleY + titleHeight + 6;
    const int sourceHeight = CardTextHeight(sourceFont.Get(), source, contentWidth);
    const int bodyY = sourceY + sourceHeight + 14;
    const int naturalHeight = description.empty() ? 0 : CardTextHeight(titleFont.Get(), description, contentWidth - 18);
    const int bodyHeight = std::min(naturalHeight, std::max(0, maxHeight - bodyY - 18));
    H3Dlg dlg(width, bodyY + bodyHeight + 18, -1, -1, false, false);
    if (!AddSafeBackground(dlg))
        return false;
    dlg.CreateFrame(left, 18, contentWidth, boxHeight, -1, H3RGB565(H3RGB888(190, 151, 70)));
    dlg.CreateFrame(left + 1, 19, contentWidth - 2, boxHeight - 2, -1, H3RGB565(H3RGB888(89, 65, 31)));
    dlg.CreateText(left + 12, 26, contentWidth - 24, keyHeight, binding.c_str(), NH3Dlg::Text::BIG, eTextColor::GOLD,
                   -1);
    dlg.CreateText(left, titleY, contentWidth, titleHeight, action.c_str(), NH3Dlg::Text::MEDIUM, eTextColor::WHITE,
                   -1);
    dlg.CreateText(left, sourceY, contentWidth, sourceHeight, source.c_str(), NH3Dlg::Text::SMALL, eTextColor::GOLD,
                   -1);
    if (bodyHeight > 0)
    {
        dlg.CreateFrame(left, bodyY - 7, contentWidth, 1, -1, H3RGB565(H3RGB888(116, 100, 70)));
        if (naturalHeight <= bodyHeight)
            dlg.CreateText(left, bodyY, contentWidth, bodyHeight, description.c_str(), NH3Dlg::Text::MEDIUM,
                           eTextColor::REGULAR, -1, eTextAlignment::TOP_LEFT);
        else if (auto *text = H3DlgScrollableText::Create(description.c_str(), left, bodyY, contentWidth, bodyHeight,
                                                          NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, false))
            dlg.AddItem(text);
    }
    dlg.RMB_Show();
    return true;
}
} // namespace helpdlg
