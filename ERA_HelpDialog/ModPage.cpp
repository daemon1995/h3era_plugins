#include "ModPage.h"
#include "ScrollbarUtils.h"

#include <algorithm>
#include <string>

namespace main
{
namespace
{
LPCSTR HotkeyTypeName(const hkcategories::eType type) noexcept
{
    switch (type)
    {
    case hkcategories::ANY_DLG:
        return "everywhere";
    case hkcategories::ADV_MAP_DLG:
        return "map";
    case hkcategories::HERO_DLG:
        return "hero";
    case hkcategories::TOWN_DLG:
        return "town";
    case hkcategories::COMBAT_DLG:
        return "battle";
    default:
        return "other";
    }
}
}

ModCategoriesPage *ModCategoriesPage::instance = nullptr;

ModCategoriesPage::ModCategoriesPage(const int x, const int y, const int width, const int height, H3Dlg *dialog)
    : HelpPage(dialog), pageX(x), pageY(y), pageWidth(width), pageHeight(height)
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
                                                  "OVBUTN3.def", categoryName,
                                                  NH3Dlg::Text::SMALL, 0, 0, false, static_cast<eVKey>(0),
                                                  eTextColor::REGULAR);
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
        scrollBar = H3DlgScrollbar::Create(pageX + pageWidth - 20, pageY + 4, 16, pageHeight - 8,
                                            modpage::CATEGORY_SCROLLBAR, count - visibleCount + 1,
                                            ScrollProc, false, 1, true);
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
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        if (buttons[i])
            buttons[i]->SetFrame(static_cast<int>(i) == index ? 1 : 0);
    }
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
        if (button && isVisible && row >= 0 && row < visibleCount)
        {
            button->SetY(pageY + 4 + row * 34);
            button->ShowActivate();
        }
        else if (button)
            button->HideDeactivate();
    }
}

void __fastcall ModCategoriesPage::ScrollProc(const INT32 tick, H3BaseDlg *)
{
    if (instance)
        instance->RedrawItems(tick);
}

ModContentPage::ModContentPage(const int x, const int y, const int width, const int height, H3Dlg *dialog)
    : HelpPage(dialog), pageX(x), pageY(y), pageWidth(width), pageHeight(height)
{
    AddFrame(pageX, pageY, pageWidth, pageHeight);
    textScroll = H3DlgScrollableText::Create(h3_NullString, pageX + 10, pageY + 8, pageWidth - 28,
                                              pageHeight - 16, NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, true);
    AddScrollableText(textScroll);
}

void ModContentPage::SetMod(const ModInformation *mod)
{
    activeMod = mod;
    SetCategory(0);
}

void ModContentPage::SetCategory(const int index)
{
    activeCategory = std::max(0, index);
    RebuildText();
}

void ModContentPage::RefreshVisibility()
{
    SetCategory(activeCategory);
}

void ModContentPage::RebuildText()
{
    renderedText = h3_NullString;
    if (activeMod && activeCategory < static_cast<int>(activeMod->categories.size()))
    {
        Category *category = activeMod->categories[activeCategory];
        if (category)
        {
            if (category == activeMod->hotkeysCategory)
            {
                std::string text;
                text = "[hotkeys][STD][HD_MOD][MODS][ALL]\n\n";
                hkcategories::eType previousType = static_cast<hkcategories::eType>(127);
                for (const auto &hotkey : activeMod->hotkeysCategory->hotkeys)
                {
                    if (hotkey.type != previousType)
                    {
                        text += "(";
                        text += HotkeyTypeName(hotkey.type);
                        text += ")\n";
                        previousType = hotkey.type;
                    }
                    text += "[" + std::string(hotkey.keys.String()) + "]  ";
                    text += hotkey.name.Empty() ? "Unnamed hotkey" : hotkey.name.String();
                    text += "\n";
                    if (!hotkey.description.Empty())
                        text += std::string(hotkey.description.String()) + "\n";
                    text += "\n";
                }
                renderedText = text.c_str();
            }
            else if (category->content)
                renderedText = category->content->text;
        }
    }
    if (renderedText.Empty())
        renderedText = "This category has no content yet.";
    SetScrollableText(textScroll, renderedText.String());
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

BOOL ModSection::ProcessMessage(H3Msg &msg)
{
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
