#include "HotkeysPage.h"

#include <algorithm>
#include <cstring>
#include <string>

namespace main
{
namespace
{
constexpr int kCategoryCount = hotkeys::CATEGORY_COUNT;
constexpr int kButtonHeight = 42;
constexpr int kButtonGap = 4;
constexpr LPCSTR kCategoryNames[kCategoryCount] = {"All", "Everywhere", "Adventure Map", "Hero Window",
                                                    "Town Window", "Battle Window", "Other"};
constexpr LPCSTR kCategoryHints[kCategoryCount] = {"Show all registered hotkeys.", "Hotkeys available everywhere.",
                                                    "Hotkeys for the adventure map.", "Hotkeys for the hero window.",
                                                    "Hotkeys for the town window.", "Hotkeys for combat.",
                                                    "Hotkeys with an unknown or unsupported context."};

bool IsKnownType(hkcategories::eType type) noexcept
{
    return type >= hkcategories::ANY_DLG && type <= hkcategories::OTHER_DLG;
}

LPCSTR TypeName(const hkcategories::eType type) noexcept
{
    switch (type)
    {
    case hkcategories::ANY_DLG:
        return "Everywhere";
    case hkcategories::ADV_MAP_DLG:
        return "Adventure Map";
    case hkcategories::HERO_DLG:
        return "Hero Window";
    case hkcategories::TOWN_DLG:
        return "Town Window";
    case hkcategories::COMBAT_DLG:
        return "Battle Window";
    default:
        return "Other";
    }
}
}

HotkeysCategoriesPage::HotkeysCategoriesPage(const int x, const int y, const int width, const int height, H3Dlg *dialog)
    : HelpPage(dialog)
{
    AddFrame(x, y, width, height);
    constexpr LPCSTR defName = "RMGmenbt.def";
    H3DefLoader def(defName);
    const int buttonWidth = std::max(1, def->widthDEF * 2 / 3);
    for (int index = 0; index < kCategoryCount; ++index)
    {
        const int itemId = hotkeys::CATEGORY_FIRST + index;
        auto *button = H3DlgCaptionButton::Create(x + 4, y + 4 + index * (kButtonHeight + kButtonGap), itemId, defName,
                                                  kCategoryNames[index], NH3Dlg::Text::MEDIUM, 0, 0, false,
                                                  static_cast<eVKey>(0), 0);
        if (!button)
            continue;
        button->SetWidth(buttonWidth);
        button->SetHeight(kButtonHeight);
        button->SetClickFrame(1);
        button->SetHints(kCategoryHints[index], "Select a hotkey context category.", false);
        buttons[index] = button;
        AddItem(button);
    }
    SetActiveCategory(hotkeys::CATEGORY_ALL);
}

BOOL HotkeysCategoriesPage::IsCategory(const int itemId) const noexcept
{
    return itemId >= hotkeys::CATEGORY_FIRST && itemId <= hotkeys::CATEGORY_LAST;
}

int HotkeysCategoriesPage::ActiveCategory() const noexcept
{
    return activeCategory;
}

void HotkeysCategoriesPage::SetActiveCategory(const int itemId) noexcept
{
    if (!IsCategory(itemId))
        return;
    activeCategory = itemId;
    for (int index = 0; index < kCategoryCount; ++index)
    {
        if (buttons[index])
            buttons[index]->SetFrame(hotkeys::CATEGORY_FIRST + index == activeCategory ? 1 : 0);
    }
    RedrawDialog();
}

HotkeysPage::HotkeysPage(const int x, const int y, const int width, const int height, H3Dlg *dialog,
                         const std::vector<ModInformation *> &mods)
    : HelpPage(dialog), mods(mods)
{
    AddFrame(x, y, width, height);
    textScroll = H3DlgScrollableText::Create(h3_NullString, x + 10, y + 8, width - 28, height - 16,
                                              NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, true);
    AddScrollableText(textScroll);
    SetCategory(hotkeys::CATEGORY_ALL);
}

void HotkeysPage::SetCategory(const int categoryId)
{
    if (categoryId < hotkeys::CATEGORY_FIRST || categoryId > hotkeys::CATEGORY_LAST)
        return;
    activeCategory = categoryId;
    RebuildText();
    RedrawDialog();
}

bool HotkeysPage::MatchesCategory(const HotKey &hotkey) const noexcept
{
    switch (activeCategory)
    {
    case hotkeys::CATEGORY_ALL:
        return true;
    case hotkeys::CATEGORY_EVERYWHERE:
        return hotkey.type == hkcategories::ANY_DLG;
    case hotkeys::CATEGORY_ADVENTURE:
        return hotkey.type == hkcategories::ADV_MAP_DLG;
    case hotkeys::CATEGORY_HERO:
        return hotkey.type == hkcategories::HERO_DLG;
    case hotkeys::CATEGORY_TOWN:
        return hotkey.type == hkcategories::TOWN_DLG;
    case hotkeys::CATEGORY_COMBAT:
        return hotkey.type == hkcategories::COMBAT_DLG;
    case hotkeys::CATEGORY_OTHER:
        return !IsKnownType(hotkey.type) || hotkey.type == hkcategories::NONE || hotkey.type == hkcategories::OTHER_DLG;
    default:
        return false;
    }
}

void HotkeysPage::RebuildText()
{
    std::string text;
    std::vector<std::pair<const ModInformation *, const HotKey *>> entries;
    for (const auto *mod : mods)
    {
        if (!mod || !mod->hotkeysCategory)
            continue;
        for (const auto &hotkey : mod->hotkeysCategory->hotkeys)
        {
            if (!MatchesCategory(hotkey))
                continue;
            entries.emplace_back(mod, &hotkey);
        }
    }
    std::sort(entries.begin(), entries.end(), [](const auto &left, const auto &right) {
        if (left.second->type != right.second->type)
            return left.second->type < right.second->type;
        const LPCSTR leftModName = left.first->name.Empty() ? "" : left.first->name.String();
        const LPCSTR rightModName = right.first->name.Empty() ? "" : right.first->name.String();
        const int modCompare = std::strcmp(leftModName, rightModName);
        if (modCompare != 0)
            return modCompare < 0;
        const LPCSTR leftName = left.second->name.Empty() ? "" : left.second->name.String();
        const LPCSTR rightName = right.second->name.Empty() ? "" : right.second->name.String();
        return std::strcmp(leftName, rightName) < 0;
    });

    hkcategories::eType previousType = static_cast<hkcategories::eType>(127);
    const ModInformation *previousMod = nullptr;
    for (const auto &entry : entries)
    {
        const ModInformation *mod = entry.first;
        const HotKey *hotkey = entry.second;
        if (hotkey->type != previousType)
        {
            text += "\n== ";
            text += TypeName(hotkey->type);
            text += " ==\n";
            previousType = hotkey->type;
            previousMod = nullptr;
        }
        if (mod != previousMod)
        {
            const LPCSTR modName = mod->name.Empty() ? "Unknown mod" : mod->name.String();
            text += "\n" + std::string(modName) + "\n";
            previousMod = mod;
        }
        text += "----------------------------------------\n[";
            text += hotkey->keys.Empty() ? "" : hotkey->keys.String();
        text += "]  ";
        text += hotkey->name.Empty() ? "Unnamed hotkey" : hotkey->name.String();
        text += "\n";
        if (!hotkey->description.Empty())
            text += std::string(hotkey->description.String()) + "\n";
        text += "\n";
    }
    if (entries.empty())
        text = "No hotkeys are registered for this category.";
    renderedText = text.c_str();
    SetScrollableText(textScroll, renderedText.String());
}

HotkeysSection::HotkeysSection(const int categoriesX, const int categoriesY, const int categoriesWidth,
                               const int categoriesHeight, const int contentX, const int contentY,
                               const int contentWidth, const int contentHeight, H3Dlg *dialog,
                               const std::vector<ModInformation *> &mods)
    : categoriesPage(categoriesX, categoriesY, categoriesWidth, categoriesHeight, dialog),
      contentPage(contentX, contentY, contentWidth, contentHeight, dialog, mods)
{
}

void HotkeysSection::SetVisible(const BOOL state) noexcept
{
    categoriesPage.SetVisible(state);
    contentPage.SetVisible(state);
}

void HotkeysSection::SetSubtype(const int subtype)
{
    activeSubtype = std::max(0, std::min(subtype, hotkeys::CATEGORY_COUNT - 1));
    const int category = hotkeys::CATEGORY_FIRST + activeSubtype;
    categoriesPage.SetActiveCategory(category);
    contentPage.SetCategory(category);
}

BOOL HotkeysSection::ProcessMessage(H3Msg &msg)
{
    if (!msg.IsLeftClick() || !categoriesPage.IsCategory(msg.itemId))
        return FALSE;
    activeSubtype = msg.itemId - hotkeys::CATEGORY_FIRST;
    categoriesPage.SetActiveCategory(msg.itemId);
    contentPage.SetCategory(msg.itemId);
    return TRUE;
}

void HotkeysSection::Redraw()
{
}

} // namespace main
