#include "ModPage.h"
#include "ScrollbarUtils.h"
#include "HelpUI.h"

#include <algorithm>
#include <string>

namespace main
{
ModCategoriesPage *ModCategoriesPage::instance = nullptr;

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
    visibleCount = std::max(1, (pageHeight - 8) / 34);
}

void ModCategoriesPage::SetMod(const ModInformation *mod)
{
    activeMod = mod;
    if (activeMod && buttons.size() < activeMod->categories.size())
        CreateForMod(activeMod);
    if (activeMod)
    {
        const size_t count = activeMod->categories.size();
        for (size_t i = 0; i < buttons.size(); ++i)
        {
            if (buttons[i])
            {
                if (i < count)
                {
                    const H3String &categoryName = activeMod->categories[i]->name;
                    buttons[i]->SetText(categoryName.Empty() ? "Category" : categoryName.String());
                    buttons[i]->ShowActivate();
                }
                else
                    buttons[i]->HideDeactivate();
            }
        }
    }
    firstIndex = 0;
    SetActiveCategory(0);
    RedrawItems(0);
}

void ModCategoriesPage::CreateForMod(const ModInformation *mod)
{
    const int count = static_cast<int>(mod->categories.size());
    const int firstNewIndex = static_cast<int>(buttons.size());
    for (int index = firstNewIndex; index < count; ++index)
    {
        Category *category = mod->categories[index];
        const LPCSTR categoryName = category && !category->name.Empty() ? category->name.String() : "Category";
        auto *button = H3DlgCaptionButton::Create(pageX + 4, pageY + 4 + index * 34, modpage::CATEGORY_FIRST + index,
                                                  "OVBUTN3.def", categoryName, NH3Dlg::Text::SMALL, 0, 0, false,
                                                  static_cast<eVKey>(0), eTextColor::REGULAR);
        if (button)
        {
            button->SetWidth(pageWidth - 26);
            button->SetHeight(30);
            button->SetClickFrame(1);
        }
        AddItem(button);
        buttons.emplace_back(button);
    }
    if (!scrollBar && count > visibleCount)
    {
        scrollBar =
            H3DlgScrollbar::Create(pageX + pageWidth - 20, pageY + 4, 16, pageHeight - 8, modpage::CATEGORY_SCROLLBAR,
                                   count - visibleCount + 1, ScrollProc, false, 1, true);
        AddItem(scrollBar);
    }
}

BOOL ModCategoriesPage::IsCategory(const int itemId) const noexcept
{
    return itemId >= modpage::CATEGORY_FIRST && itemId < modpage::CATEGORY_FIRST + static_cast<int>(buttons.size());
}

int ModCategoriesPage::CategoryIndex(const int itemId) const noexcept
{
    return itemId - modpage::CATEGORY_FIRST;
}

void ModCategoriesPage::SetActiveCategory(const int index)
{
    if (index < firstIndex)
        firstIndex = index;
    if (index >= firstIndex + visibleCount)
        firstIndex = index - visibleCount + 1;
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        if (buttons[i])
            buttons[i]->SetFrame(static_cast<int>(i) == index ? 1 : 0);
    }
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
    if (inside)
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
    firstIndex = helpdlg::UpdateScrollbar(scrollBar, maxFirst, requestedFirstIndex, isVisible);
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        auto *button = buttons[i];
        const int row = static_cast<int>(i) - firstIndex;
        if (button && isVisible && static_cast<int>(i) < itemCount && row >= 0 && row < visibleCount)
        {
            button->SetY(pageY + 4 + row * 34);
            button->ShowActivate();
        }
        else if (button)
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
    AddFrame(pageX, pageY, pageWidth, pageHeight);
    textScroll = H3DlgScrollableText::Create(h3_NullString, pageX + 10, pageY + 8, pageWidth - 28, pageHeight - 16,
                                             NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, false);
    AddScrollableText(textScroll);
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
    const H3String previousText(renderedText);
    renderedText = h3_NullString;
    if (activeMod && activeCategory < static_cast<int>(activeMod->categories.size()))
    {
        Category *category = activeMod->categories[activeCategory];
        if (category)
        {
            if (category == activeMod->hotkeysCategory)
            {
                std::vector<helpdlg::HotkeyLine> entries;
                for (const auto &hotkey : activeMod->hotkeysCategory->hotkeys)
                    entries.push_back({hotkey.type, "", helpdlg::Safe(hotkey.keys.String()),
                                       hotkey.name.Empty() ? "Unnamed hotkey" : helpdlg::Safe(hotkey.name.String()),
                                       static_cast<int>(activeMod->id), hotkey.id,
                                       helpdlg::Safe(hotkey.description.String())});
                renderedText = helpdlg::GroupHotkeys(entries, helpdlg::ContextName).c_str();
            }
            else if (category->content)
            {
                renderedText = category->content->text;
                for (const auto &object : category->content->objects)
                    if (object.kind == eHelpObjectKind::Text && !object.overlay)
                    {
                        renderedText.Append("\n\n");
                        renderedText.Append(object.value);
                    }
            }
            if (renderedCategory != category)
                RebuildObjects(category);
        }
    }
    if (renderedText.Empty())
        renderedText = helpdlg::Text("help.ui.empty_category",
                                     "This category has no text. Interactive objects may be available below.");
    if (strcmp(previousText.String(), renderedText.String()))
    {
        SetScrollableText(textScroll, renderedText.String());
        if (renderedCategory)
            SetScrollPosition(scrollPositions[renderedCategory]);
    }
}

void ModContentPage::RebuildObjects(const Category *category)
{
    for (auto *item : objectItems)
        if (item)
            item->HideDeactivate();
    objectItems.clear();
    actions.clear();
    pendingAction = h3_NullString;
    renderedCategory = category;
    if (!category || !category->content)
        return;
    auto cached = objectCache.find(category);
    if (cached != objectCache.end())
    {
        objectItems = cached->second.items;
        actions = cached->second.actions;
        if (isVisible)
            for (auto *item : objectItems)
                if (item)
                    item->ShowActivate();
        return;
    }
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
            item = H3DlgText::Create(left, top, width, height, object.value.String(), NH3Dlg::Text::SMALL,
                                     eTextColor::REGULAR, id, eTextAlignment::TOP_LEFT);
        else if (object.kind == eHelpObjectKind::Image)
        {
            const std::string resource = helpdlg::Lower(object.value.String());
            if (resource.size() >= 4 && resource.substr(resource.size() - 4) == ".def")
            {
                H3DefLoader def(object.value.String());
                if (def.Get() && def->groupsCount > 0 && def->groups && def->groups[0] && object.frame >= 0 &&
                    object.frame < def->groups[0]->count && def->widthDEF <= width && def->heightDEF <= height)
                    item = H3DlgDef::Create(left, top, id, object.value.String(), object.frame);
            }
            else
            {
                H3PcxLoader pcx(object.value.String());
                if (pcx.Get() && pcx->width <= width && pcx->height <= height)
                    item = H3DlgPcx::Create(left, top, width, height, id, object.value.String());
            }
        }
        else
        {
            item = helpdlg::Button(nullptr, left, top, width, height, id, object.value.String());
            H3String action(object.action);
            if (object.kind == eHelpObjectKind::ErmFunction && !action.Empty() &&
                std::string(action.String()).compare(0, 4, "erm:") != 0)
                action = H3String::Format("erm:%s", action.String());
            if (!action.Empty())
                actions.emplace_back(id, action);
        }
        if (item)
        {
            AddItem(item);
            objectItems.push_back(item);
            if (isVisible)
                item->ShowActivate();
        }
    }
    objectCache.emplace(category, ObjectControls{objectItems, actions});
}

int ModContentPage::ScrollPosition() const
{
    return TextScrollPosition(textScroll);
}
void ModContentPage::SetScrollPosition(int position)
{
    SetTextScrollPosition(textScroll, position);
}

void ModContentPage::SetVisible(BOOL state) noexcept
{
    DlgPage::SetVisible(state);
    // DlgPage also tracks controls from previous categories. Restore only the
    // current category; retired controls remain hidden in the native dialog.
    for (auto *item : items)
        if (item)
            item->HideDeactivate();
    for (auto *item : objectItems)
        if (item && state)
            item->ShowActivate();
}

void ModContentPage::UpdateScrollbarActivation(const H3Msg &msg) noexcept
{
    FlushTextScrollPositions();
    if (!textScroll || !msg.IsMouseOver())
        return;
    auto *scroll = textScroll->GetTextScrollBar();
    if (!scroll || !scroll->IsVisible())
        return;
    const int left = dialog->GetX() + pageX, top = dialog->GetY() + pageY;
    if (msg.GetX() >= left && msg.GetX() < left + pageWidth && msg.GetY() >= top && msg.GetY() < top + pageHeight)
        scroll->Activate();
    else
        scroll->DeActivate();
}

BOOL ModContentPage::ProcessMessage(H3Msg &msg)
{
    if (!isVisible || !msg.IsLeftClick())
        return FALSE;
    for (const auto &action : actions)
        if (action.first == msg.itemId)
        {
            pendingAction = action.second;
            return TRUE;
        }
    return FALSE;
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
    contentPage.SetCategory(activeSubtype);
}

} // namespace main
