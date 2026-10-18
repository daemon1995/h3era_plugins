#include "ModPage.h"
#include "ScrollbarUtils.h"
#include "HelpUI.h"

#include <algorithm>
#include <string>

namespace main
{
namespace
{
constexpr int kModHotkeyRowHeight = 24;
}
ModCategoriesPage *ModCategoriesPage::instance = nullptr;
ModContentPage *ModContentPage::hotkeyInstance = nullptr;

ModCategoriesPage::~ModCategoriesPage()
{
    if (instance == this)
        instance = nullptr;
}

ModCategoriesPage::ModCategoriesPage(const int x, const int y, const int width, const int height, H3Dlg *dialog)
    : DlgPage(dialog), pageX(x), pageY(y), pageWidth(width), pageHeight(height)
{
    instance = this;
    AddFrame(pageX, pageY, pageWidth, pageHeight);
    categoryStep = helpdlg::CategoryButtonStep();
    visibleCount = std::max(1, (pageHeight - 8) / categoryStep);
    buttons.reserve(visibleCount);
    for (int row = 0; row < visibleCount; ++row)
    {
        auto *button = helpdlg::CategoryButton(pageX + 6, pageY + 4 + row * categoryStep, modpage::CATEGORY_FIRST + row,
                                               h3_NullString);
        AddItem(button);
        buttons.push_back(button);
    }
    scrollBar = H3DlgScrollbar::Create(pageX + pageWidth - 20, pageY + 2, 16, std::max(16, pageHeight - 4),
                                       modpage::CATEGORY_SCROLLBAR, 2, ScrollProc, false, 1, true);
    AddItem(scrollBar);
}

void ModCategoriesPage::SetMod(const ModInformation *mod)
{
    activeMod = mod;
    firstIndex = 0;
    SetActiveCategory(0);
}

BOOL ModCategoriesPage::IsCategory(const int itemId) const noexcept
{
    const int row = itemId - modpage::CATEGORY_FIRST;
    return isVisible && activeMod && row >= 0 && row < visibleCount &&
           firstIndex + row < static_cast<int>(activeMod->categories.size());
}

int ModCategoriesPage::CategoryIndex(const int itemId) const noexcept
{
    return firstIndex + itemId - modpage::CATEGORY_FIRST;
}

void ModCategoriesPage::SetActiveCategory(const int index)
{
    const int count = activeMod ? static_cast<int>(activeMod->categories.size()) : 0;
    activeCategory = helpdlg::Bound(index, 0, std::max(0, count - 1));
    if (activeCategory < firstIndex)
        firstIndex = activeCategory;
    if (activeCategory >= firstIndex + visibleCount)
        firstIndex = activeCategory - visibleCount + 1;
    RedrawItems(firstIndex);
}

void ModCategoriesPage::UpdateScrollbarActivation(const H3Msg &msg) noexcept
{
    if (!scrollBar || !scrollBar->IsVisible() || !msg.IsMouseOver() || !dialog)
        return;

    const int left = dialog->GetX() + pageX;
    const int top = dialog->GetY() + pageY;
    const bool inside =
        msg.GetX() >= left && msg.GetX() < left + pageWidth && msg.GetY() >= top && msg.GetY() < top + pageHeight;
    const int count = activeMod ? static_cast<int>(activeMod->categories.size()) : 0;
    if (inside && count > visibleCount)
        scrollBar->Activate();
    else
        scrollBar->DeActivate();
}

void ModCategoriesPage::Refresh()
{
    RedrawItems(firstIndex);
}

void ModCategoriesPage::RedrawItems(const int requestedFirstIndex)
{
    const int itemCount = activeMod ? static_cast<int>(activeMod->categories.size()) : 0;
    const int maxFirst = std::max(0, itemCount - visibleCount);
    firstIndex = helpdlg::UpdateScrollbar(scrollBar, maxFirst, requestedFirstIndex, isVisible, TRUE);
    if (scrollBar)
    {
        if (isVisible)
        {
            scrollBar->Show();
            maxFirst > 0 ? scrollBar->Activate() : scrollBar->DeActivate();
        }
        else
            scrollBar->HideDeactivate();
    }
    for (size_t row = 0; row < buttons.size(); ++row)
    {
        auto *button = buttons[row];
        const int index = firstIndex + static_cast<int>(row);
        if (!button)
            continue;
        if (isVisible && index < itemCount)
        {
            const Category *category = activeMod->categories[index];
            button->SetText(category->name);
            button->ShowActivate();
            helpdlg::SetCategoryState(button, index == activeCategory);
        }
        else
            button->HideDeactivate();
    }
    RedrawDialog();
}

void __fastcall ModCategoriesPage::ScrollProc(const INT32 tick, H3BaseDlg *)
{
    if (instance)
        instance->RedrawItems(tick);
}

ModContentPage::ModContentPage(const int x, const int y, const int width, const int height, H3Dlg *dialog)
    : DlgPage(dialog), pageX(x), pageY(y), pageWidth(width), pageHeight(height)
{
    hotkeyInstance = this;
    AddFrame(pageX, pageY, pageWidth, pageHeight);
    textScroll = H3DlgScrollableText::Create(h3_NullString, pageX + 10, pageY + 8, pageWidth - 28, pageHeight - 16,
                                             NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, false);
    AddScrollableText(textScroll);

    const int textLeft = pageX + 10;
    const int textWidth = pageWidth - 40;
    const int keyWidth = std::min(220, std::max(120, textWidth * 42 / 100));
    hotkeyVisibleRows = std::max(1, (pageHeight - 16) / kModHotkeyRowHeight);
    for (int row = 0; row < hotkeyVisibleRows; ++row)
    {
        const int top = pageY + 8 + row * kModHotkeyRowHeight;
        auto *heading = H3DlgText::Create(textLeft + 4, top + 2, textWidth - 4, kModHotkeyRowHeight - 2, h3_NullString,
                                          NH3Dlg::Text::MEDIUM, eTextColor::GOLD, -1, eTextAlignment::MIDDLE_LEFT);
        auto *key = H3DlgText::Create(textLeft + 8, top, keyWidth - 8, kModHotkeyRowHeight, h3_NullString,
                                      NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
        auto *action =
            H3DlgText::Create(textLeft + keyWidth, top, textWidth - keyWidth, kModHotkeyRowHeight, h3_NullString,
                              P_SmallFont->GetName(), eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
        AddItem(heading);
        AddItem(key);
        AddItem(action);
        hotkeyHeadingRows.push_back(heading);
        hotkeyKeyRows.push_back(key);
        hotkeyActionRows.push_back(action);
        auto *separator = H3DlgFrame::Create(textLeft, top, textWidth, 1, -1, H3RGB565(H3RGB888(190, 151, 70)));
        AddItem(separator);
        if (separator)
            separator->DeActivate();
        hotkeySeparators.push_back(separator);
    }
    hotkeyScroll = H3DlgScrollbar::Create(pageX + pageWidth - 22, pageY + 6, 16, pageHeight - 12,
                                          modpage::HOTKEY_CONTENT_SCROLLBAR, 2, HotkeyScrollProc, false, 1, true);
    AddItem(hotkeyScroll);
}

ModContentPage::~ModContentPage()
{
    if (hotkeyInstance == this)
        hotkeyInstance = nullptr;
}

void ModContentPage::SetMod(const ModInformation *mod)
{
    activeMod = mod;
    SetCategory(0);
}

void ModContentPage::SetCategory(const int index)
{
    if (renderedCategory)
        scrollPositions[renderedCategory] = ScrollPosition();
    const Category *previous = renderedCategory;
    activeCategory = std::max(0, index);
    RebuildText();
    if (renderedCategory && renderedCategory != previous)
        SetScrollPosition(scrollPositions[renderedCategory]);
}

void ModContentPage::RefreshVisibility()
{
    SetCategory(activeCategory);
}

void ModContentPage::RebuildText()
{
    const Category *category = activeMod && activeCategory < static_cast<int>(activeMod->categories.size())
                                   ? activeMod->categories[activeCategory]
                                   : nullptr;
    if (renderedCategory == category)
    {
        RefreshModeVisibility();
        return;
    }
    hotkeyMode = activeMod && category == activeMod->hotkeysCategory;
    RebuildObjects(category);
    if (hotkeyMode)
        RebuildHotkeyRows();
    else
    {
        LPCSTR text = category && category->content ? category->content->text : h3_NullString;
        std::string combined;
        if (category && category->content)
            for (const auto &object : category->content->objects)
                if (object.kind == eHelpObjectKind::Text && !object.overlay && *object.value)
                {
                    if (combined.empty())
                        combined = text;
                    if (!combined.empty())
                        combined += "\n\n";
                    combined += object.value;
                }
        if (!combined.empty())
            text = combined.c_str();
        hasArticleText = *text || !activeObjects || activeObjects->items.empty();
        SetScrollableText(textScroll, text);
    }
    RefreshModeVisibility();
    if (renderedCategory)
        SetScrollPosition(scrollPositions[renderedCategory]);
}

void ModContentPage::RebuildObjects(const Category *category)
{
    if (activeObjects)
        for (auto *item : activeObjects->items)
            if (item)
                item->HideDeactivate();
    activeObjects = nullptr;
    pendingAction = h3_NullString;
    renderedCategory = category;
    if (!category || !category->content)
        return;
    const auto cached = objectCache.emplace(category, ObjectControls{});
    activeObjects = &cached.first->second;
    if (!cached.second)
        return;
    auto &objectItems = activeObjects->items;
    auto &actions = activeObjects->actions;
    // Controls belong to H3Dlg. IDs grow within a private range, and only the
    // current category's action table is considered during dispatch.
    for (const auto &object : category->content->objects)
    {
        if (nextObjectId >= 25000)
            break;
        if (object.kind == eHelpObjectKind::Text && !object.overlay)
            continue;
        const int left = pageX + helpdlg::Bound(object.x, 8, pageWidth - 32);
        const int top = pageY + helpdlg::Bound(object.y, 8, pageHeight - 36);
        const int width = helpdlg::Bound(object.width ? object.width : 180, 20, pageX + pageWidth - left - 8);
        const int height = helpdlg::Bound(object.height ? object.height : 28, 20, pageY + pageHeight - top - 8);
        const int id = nextObjectId++;
        H3DlgItem *item = nullptr;
        if (object.kind == eHelpObjectKind::Text)
            item = H3DlgText::Create(left, top, width, height, object.value, NH3Dlg::Text::SMALL, eTextColor::REGULAR,
                                     id, eTextAlignment::TOP_LEFT);
        else if (object.kind == eHelpObjectKind::Image)
        {
            const std::string resource = helpdlg::Lower(object.value);
            if (resource.size() >= 4 && resource.substr(resource.size() - 4) == ".def")
            {
                H3DefLoader def(object.value);
                if (def.Get() && def->groupsCount > 0 && def->groups && def->groups[0] && object.frame >= 0 &&
                    object.frame < def->groups[0]->count && def->widthDEF <= width && def->heightDEF <= height)
                    item = H3DlgDef::Create(left, top, id, object.value, object.frame);
            }
            else
            {
                H3PcxLoader pcx(object.value);
                if (pcx.Get() && pcx->width <= width && pcx->height <= height)
                    item = H3DlgPcx::Create(left, top, width, height, id, object.value);
            }
        }
        else
        {
            item = helpdlg::Button(nullptr, left, top, width, height, id, object.value, PAGE_BUTTON_DEF);
            if (item && *object.action)
                actions.emplace_back(id, &object);
        }
        if (item)
        {
            AddItem(item);
            objectItems.push_back(item);
            if (isVisible && !hotkeyMode)
                item->ShowActivate();
        }
    }
}

void ModContentPage::RebuildHotkeyRows()
{
    std::vector<helpdlg::HotkeyLine> entries;
    if (activeMod && activeMod->hotkeysCategory)
    {
        const LPCSTR source = activeMod->name;
        for (const auto &hotkey : activeMod->hotkeysCategory->hotkeys)
            entries.push_back({helpdlg::HotkeyContext(hotkey.type), source, hotkey.keys, hotkey.name,
                               static_cast<int>(activeMod->id), hotkey.id, hotkey.description});
    }
    std::stable_sort(entries.begin(), entries.end(), [](const helpdlg::HotkeyLine &a, const helpdlg::HotkeyLine &b) {
        if (a.context != b.context)
            return a.context < b.context;
        return a.id < b.id;
    });
    hotkeyLines.clear();
    int previousContext = -1000;
    for (const auto &entry : entries)
    {
        if (entry.context != previousContext)
        {
            HotkeyDisplayLine heading;
            heading.heading = true;
            heading.left = helpdlg::ContextName(entry.context);
            hotkeyLines.push_back(std::move(heading));
            previousContext = entry.context;
        }
        HotkeyDisplayLine row;
        row.left = std::string("[") + entry.keys + "]";
        row.right = entry.name;
        if (*entry.source)
            row.right += std::string(" (") + entry.source + ")";
        hotkeyLines.push_back(std::move(row));
    }
    if (hotkeyLines.empty())
    {
        HotkeyDisplayLine row;
        row.right = helpdlg::Text("help.ui.no_hotkeys");
        hotkeyLines.push_back(std::move(row));
    }
    hotkeyFirstLine =
        helpdlg::Bound(hotkeyFirstLine, 0, std::max(0, static_cast<int>(hotkeyLines.size()) - hotkeyVisibleRows));
}

void ModContentPage::RenderHotkeyRows()
{
    const BOOL visibleMode = isVisible && hotkeyMode;
    const int maxFirst = std::max(0, static_cast<int>(hotkeyLines.size()) - hotkeyVisibleRows);
    hotkeyFirstLine = helpdlg::UpdateScrollbar(hotkeyScroll, maxFirst, hotkeyFirstLine, visibleMode, TRUE);
    for (int row = 0; row < hotkeyVisibleRows; ++row)
    {
        const int index = hotkeyFirstLine + row;
        const bool visible = visibleMode && index < static_cast<int>(hotkeyLines.size());
        auto *heading = hotkeyHeadingRows[row];
        auto *key = hotkeyKeyRows[row];
        auto *action = hotkeyActionRows[row];
        auto *separator = hotkeySeparators[row];
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
        const auto &line = hotkeyLines[index];
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
}

void ModContentPage::SetTextModeVisible(BOOL state) noexcept
{
    if (!textScroll)
        return;
    if (state)
        textScroll->ShowActivate();
    else
        textScroll->HideDeactivate();
    if (auto *textItems = textScroll->GetItems())
        for (auto *item : *textItems)
            if (item)
                if (state)
                    item->ShowActivate();
                else
                    item->HideDeactivate();
    if (auto *scroll = textScroll->GetTextScrollBar())
    {
        if (state && scroll->GetTicksCount() > 1)
            scroll->ShowActivate();
        else
            scroll->HideDeactivate();
    }
}

void ModContentPage::RefreshModeVisibility() noexcept
{
    SetTextModeVisible(isVisible && !hotkeyMode && hasArticleText);
    if (activeObjects)
        for (auto *item : activeObjects->items)
            if (item)
            {
                if (isVisible && !hotkeyMode)
                    item->ShowActivate();
                else
                    item->HideDeactivate();
            }
    if (hotkeyMode)
        RenderHotkeyRows();
    else
    {
        if (hotkeyScroll)
            hotkeyScroll->HideDeactivate();
        for (auto *item : hotkeyHeadingRows)
            if (item)
                item->HideDeactivate();
        for (auto *item : hotkeyKeyRows)
            if (item)
                item->HideDeactivate();
        for (auto *item : hotkeyActionRows)
            if (item)
                item->HideDeactivate();
        for (auto *item : hotkeySeparators)
            if (item)
                item->HideDeactivate();
    }
}

int ModContentPage::ScrollPosition() const
{
    return hotkeyMode ? hotkeyFirstLine : hasArticleText ? TextScrollPosition(textScroll) : 0;
}
void ModContentPage::SetScrollPosition(int position)
{
    if (hotkeyMode)
    {
        hotkeyFirstLine =
            helpdlg::Bound(position, 0, std::max(0, static_cast<int>(hotkeyLines.size()) - hotkeyVisibleRows));
        RenderHotkeyRows();
    }
    else if (hasArticleText)
        SetTextScrollPosition(textScroll, position);
}

void ModContentPage::SetVisible(BOOL state) noexcept
{
    DlgPage::SetVisible(state);
    // DlgPage tracks controls from every previously rendered mod category.
    // Re-apply the current rendering mode immediately after its broad show/hide pass.
    for (auto *item : items)
        if (item)
            item->HideDeactivate();
    RefreshModeVisibility();
}

void ModContentPage::UpdateScrollbarActivation(const H3Msg &msg) noexcept
{
    if (!msg.IsMouseOver() || !dialog)
        return;
    const int left = dialog->GetX() + pageX, top = dialog->GetY() + pageY;
    const bool inside =
        msg.GetX() >= left && msg.GetX() < left + pageWidth && msg.GetY() >= top && msg.GetY() < top + pageHeight;
    if (hotkeyMode)
    {
        if (hotkeyScroll && hotkeyScroll->IsVisible())
        {
            const bool canScroll = static_cast<int>(hotkeyLines.size()) > hotkeyVisibleRows;
            (inside && canScroll) ? hotkeyScroll->Activate() : hotkeyScroll->DeActivate();
        }
        return;
    }
    FlushTextScrollPositions();
    if (!textScroll)
        return;
    auto *scroll = textScroll->GetTextScrollBar();
    if (!scroll || !scroll->IsVisible())
        return;
    inside ? scroll->Activate() : scroll->DeActivate();
}

BOOL ModContentPage::ProcessMessage(H3Msg &msg)
{
    if (!isVisible || hotkeyMode || !activeObjects || !msg.IsLeftClick())
        return FALSE;
    const int mouseX = msg.GetX() - dialog->GetX(), mouseY = msg.GetY() - dialog->GetY();
    // The native article's line widgets can be the reported item under an
    // overlay. Resolve the visible action by its rectangle, in reverse z-order.
    for (auto action = activeObjects->actions.rbegin(); action != activeObjects->actions.rend(); ++action)
    {
        auto *item = dialog->GetH3DlgItem(action->first);
        if (!item || !item->IsVisible())
            continue;
        const bool inside = mouseX >= item->GetX() && mouseX < item->GetX() + item->GetWidth() &&
                            mouseY >= item->GetY() && mouseY < item->GetY() + item->GetHeight();
        if (action->first == msg.itemId || inside)
        {
            const auto &object = *action->second;
            pendingAction = object.action;
            if (object.kind == eHelpObjectKind::ErmFunction && strncmp(object.action, "erm:", 4))
                pendingAction = H3String::Format("erm:%s", object.action);
            return TRUE;
        }
    }
    return FALSE;
}

void __fastcall ModContentPage::HotkeyScrollProc(INT32 tick, H3BaseDlg *)
{
    if (hotkeyInstance && hotkeyInstance->hotkeyMode)
    {
        hotkeyInstance->hotkeyFirstLine = tick;
        hotkeyInstance->RenderHotkeyRows();
        hotkeyInstance->RedrawDialog();
    }
}

H3String ModContentPage::TakeAction()
{
    H3String result(pendingAction);
    pendingAction = h3_NullString;
    return result;
}

ModSection::ModSection(const int categoriesX, const int categoriesY, const int categoriesWidth,
                       const int categoriesHeight, const int contentX, const int contentY, const int contentWidth,
                       const int contentHeight, H3Dlg *dialog)
    : categoriesPage(categoriesX, categoriesY, categoriesWidth, categoriesHeight, dialog),
      contentPage(contentX, contentY, contentWidth, contentHeight, dialog)
{
}

void ModSection::SetMod(const ModInformation *mod)
{
    activeMod = mod;
    categoriesPage.SetMod(mod);
    contentPage.SetMod(mod);
}

void ModSection::SetSubtype(const int subtype)
{
    const int categoryCount = activeMod ? static_cast<int>(activeMod->categories.size()) : 0;
    if (categoryCount <= 0)
    {
        activeSubtype = 0;
        return;
    }

    activeSubtype = std::max(0, std::min(subtype, categoryCount - 1));
    categoriesPage.SetActiveCategory(activeSubtype);
    contentPage.SetCategory(activeSubtype);
}

void ModSection::SetVisible(const BOOL state) noexcept
{
    categoriesPage.SetVisible(state);
    contentPage.SetVisible(state);
    if (state)
    {
        categoriesPage.Refresh();
        contentPage.RefreshVisibility();
    }
}

void ModSection::UpdateMousePosition(const H3Msg &msg) noexcept
{
    categoriesPage.UpdateScrollbarActivation(msg);
    contentPage.UpdateScrollbarActivation(msg);
}

BOOL ModSection::ProcessMessage(H3Msg &msg)
{
    if (contentPage.ProcessMessage(msg))
        return TRUE;
    if (!msg.IsLeftClick() || !categoriesPage.IsCategory(msg.itemId))
        return FALSE;
    const int index = categoriesPage.CategoryIndex(msg.itemId);
    activeSubtype = index;
    categoriesPage.SetActiveCategory(index);
    contentPage.SetCategory(index);
    return TRUE;
}

void ModSection::Redraw()
{
    categoriesPage.Refresh();
    contentPage.SetCategory(activeSubtype);
}

} // namespace main
