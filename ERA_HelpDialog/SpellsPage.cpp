#include "SpellsPage.h"
#include "ScrollbarUtils.h"

#include <algorithm>

namespace main
{
namespace
{
constexpr LPCSTR kCategoryNames[spells::CATEGORY_COUNT] = {"All", "Fire", "Air", "Water", "Earth"};
constexpr LPCSTR kLevelNames[spells::LEVEL_COUNT] = {"0", "1", "2", "3", "4", "5"};

LPCSTR ReadCategoryName(const int index) noexcept
{
    bool success = false;
    LPCSTR value = EraJS::read(H3String::Format("help.spells.categories.%d.name", index).String(), success);
    return success && value ? value : kCategoryNames[index];
}

int SchoolMaskForCategory(const int categoryId) noexcept
{
    switch (categoryId)
    {
    case spells::CATEGORY_FIRE:
        return h3::NH3Spells::NSchool::FIRE;
    case spells::CATEGORY_AIR:
        return h3::NH3Spells::NSchool::AIR;
    case spells::CATEGORY_WATER:
        return h3::NH3Spells::NSchool::WATER;
    case spells::CATEGORY_EARTH:
        return h3::NH3Spells::NSchool::EARTH;
    default:
        return h3::NH3Spells::NSchool::ALL;
    }
}
} // namespace

SpellsPage *SpellsPage::instance = nullptr;

SpellCategoriesPage::SpellCategoriesPage(const int x, const int y, const int width, const int height, H3Dlg *dialog)
    : HelpPage(dialog)
{
    AddFrame(x, y, width, height);
    for (int index = 0; index < spells::CATEGORY_COUNT; ++index)
    {
        auto *button = H3DlgCaptionButton::Create(x + 4, y + 4 + index * 34, spells::CATEGORY_FIRST + index,
                                                  "OVBUTN3.def", ReadCategoryName(index), NH3Dlg::Text::SMALL, 0, 0,
                                                  false, static_cast<eVKey>(0), eTextColor::REGULAR);
        if (button)
        {
            button->SetWidth(width - 26);
            button->SetHeight(30);
            button->SetClickFrame(1);
        }
        AddItem(button);
        buttons[index] = button;
    }
    SetActiveCategory(spells::CATEGORY_ALL);
}

BOOL SpellCategoriesPage::IsCategory(const int itemId) const noexcept
{
    return itemId >= spells::CATEGORY_FIRST && itemId <= spells::CATEGORY_LAST;
}

void SpellCategoriesPage::SetActiveCategory(const int itemId) noexcept
{
    if (!IsCategory(itemId))
        return;
    activeCategory = itemId;
    for (int index = 0; index < spells::CATEGORY_COUNT; ++index)
    {
        if (buttons[index])
            buttons[index]->SetFrame(buttons[index]->GetID() == activeCategory ? 1 : 0);
    }
    RedrawDialog();
}

SpellsPage::SpellsPage(const int x, const int y, const int width, const int height, H3Dlg *dialog)
    : HelpPage(dialog), pageX(x), pageY(y), pageWidth(width), pageHeight(height)
{
    instance = this;
    H3DefLoader def(NH3Dlg::Assets::SPELLS_DEF);
    if (!def.Get())
        return;

    constexpr int margin = 8;
    constexpr int columnGap = 6;
    constexpr int rowGap = 6;
    constexpr int nameHeight = 22;
    constexpr int levelPanelHeight = 40;
    constexpr int scrollBarWidth = 18;
    const int availableWidth = pageWidth - margin * 2 - scrollBarWidth;
    const int availableHeight = pageHeight - levelPanelHeight - margin * 2;
    const int cellWidth = def->widthDEF + 28;
    const int cellHeight = def->heightDEF + nameHeight + rowGap;
    columns = std::max(1, (availableWidth + columnGap) / (cellWidth + columnGap));
    rows = std::max(1, availableHeight / cellHeight);

    backPcx = H3DlgPcx16::Create(pageX + 1, pageY + 1, pageWidth - 2, pageHeight - 2, -1, nullptr);
    if (auto *background = H3LoadedPcx16::Create(pageWidth - 2, pageHeight - 2))
    {
        memset(background->buffer, 14, background->buffSize);
        if (backPcx)
            backPcx->SetPcx(background);
        else
            background->Destroy();
    }
    AddItem(backPcx);
    AddFrame(pageX, pageY, pageWidth, pageHeight);
    AddFrame(pageX + margin / 2, pageY + margin / 2, pageWidth - margin, levelPanelHeight - margin);

    int levelButtonX = pageX + margin;
    for (int level = 0; level < spells::LEVEL_COUNT; ++level)
    {
        auto *button = H3DlgCaptionButton::Create(levelButtonX, pageY + 4, spells::LEVEL_FIRST + level,
                                                  "OVBUTN3.def", kLevelNames[level], NH3Dlg::Text::SMALL, 0, 0,
                                                  false, static_cast<eVKey>(0), eTextColor::REGULAR);
        if (button)
        {
            button->SetWidth(28);
            button->SetHeight(30);
            button->SetClickFrame(1);
            levelButtonX += button->GetWidth() + 4;
        }
        AddItem(button);
        levelButtons.emplace_back(button);
    }

    const int gridY = pageY + levelPanelHeight + margin;
    for (int row = 0; row < rows; ++row)
    {
        for (int column = 0; column < columns; ++column)
        {
            const int cellX = pageX + margin + column * (cellWidth + columnGap);
            const int portraitX = cellX + (cellWidth - def->widthDEF) / 2;
            const int portraitY = gridY + row * cellHeight;
            const int itemId = spells::ITEM_FIRST + row * columns + column;
            auto *portrait = H3DlgDef::Create(portraitX, portraitY, itemId, NH3Dlg::Assets::SPELLS_DEF, 0);
            AddItem(portrait);
            portraits.emplace_back(portrait);
            auto *name = H3DlgText::Create(cellX, portraitY + def->heightDEF, cellWidth, nameHeight, h3_NullString,
                                           NH3Dlg::Text::SMALL, eTextColor::REGULAR, itemId,
                                           eTextAlignment::MIDDLE_CENTER, -1);
            AddItem(name);
            names.emplace_back(name);
        }
    }
    const int availableSpells = std::max(0, std::min(static_cast<int>(H3SpellCount::Get()), h3::limits::TOTAL_SPELLS));
    const int initialSpellRows = (availableSpells + columns - 1) /
                                 columns;
    const int initialContentRows = std::max(0, initialSpellRows - rows);
    if (initialContentRows > 0)
    {
        scrollBar = H3DlgScrollbar::Create(pageX + pageWidth - scrollBarWidth - margin, gridY, 16, availableHeight,
                                            spells::CONTENT_SCROLLBAR, initialContentRows + 1, ScrollProc, false, 1,
                                            true);
        AddItem(scrollBar);
    }
    RebuildSpellList();
}

SpellsPage::~SpellsPage()
{
    if (backPcx)
    {
        if (auto *pcx = backPcx->GetPcx())
        {
            pcx->Destroy();
            backPcx->SetPcx(nullptr);
        }
    }
    if (instance == this)
        instance = nullptr;
}

BOOL SpellsPage::MatchesCategory(const int spellId) const noexcept
{
    if (activeCategory == spells::CATEGORY_ALL)
        return TRUE;
    const int school = static_cast<int>(P_Spell[spellId].school);
    return (school & SchoolMaskForCategory(activeCategory)) != 0;
}

void SpellsPage::RebuildSpellList()
{
    spellIds.clear();
    const int count = std::max(0, std::min(static_cast<int>(H3SpellCount::Get()), h3::limits::TOTAL_SPELLS));
    for (int spellId = 0; spellId < count; ++spellId)
    {
        if (MatchesCategory(spellId) && (activeLevel < 0 || P_Spell[spellId].level == activeLevel))
            spellIds.emplace_back(spellId);
    }
    std::sort(spellIds.begin(), spellIds.end(), [](const int first, const int second) {
        const int firstLevel = P_Spell[first].level;
        const int secondLevel = P_Spell[second].level;
        return firstLevel == secondLevel ? first < second : firstLevel < secondLevel;
    });
    firstRow = 0;
}

void SpellsPage::SetCategory(const int categoryId)
{
    if (categoryId < spells::CATEGORY_FIRST || categoryId > spells::CATEGORY_LAST)
        return;
    activeCategory = categoryId;
    RebuildSpellList();
    Redraw();
}

void SpellsPage::SetLevel(const int level)
{
    const int requestedLevel = std::max(0, std::min(level, spells::LEVEL_COUNT - 1));
    activeLevel = activeLevel == requestedLevel ? -1 : requestedLevel;
    for (int index = 0; index < static_cast<int>(levelButtons.size()); ++index)
    {
        if (levelButtons[index])
            levelButtons[index]->SetFrame(index == activeLevel ? 1 : 0);
    }
    RebuildSpellList();
    Redraw();
}

void SpellsPage::Redraw(const int requestedFirstRow)
{
    const int totalRows = (static_cast<int>(spellIds.size()) + columns - 1) / columns;
    const int maxFirstRow = std::max(0, totalRows - rows);
    firstRow = helpdlg::UpdateScrollbar(scrollBar, maxFirstRow, requestedFirstRow, isVisible);
    const int firstItem = firstRow * columns;
    for (size_t slot = 0; slot < portraits.size(); ++slot)
    {
        auto *portrait = portraits[slot];
        auto *name = slot < names.size() ? names[slot] : nullptr;
        const int spellIndex = firstItem + static_cast<int>(slot);
        if (portrait && isVisible && spellIndex >= 0 && spellIndex < static_cast<int>(spellIds.size()))
        {
            const int spellId = spellIds[spellIndex];
            portrait->SetFrame(spellId);
            portrait->ShowActivate();
            if (name)
            {
                name->SetText(P_Spell[spellId].name ? P_Spell[spellId].name : h3_NullString);
                name->Show();
            }
        }
        else
        {
            if (portrait)
                portrait->HideDeactivate();
            if (name)
                name->HideDeactivate();
        }
    }
    RedrawDialog();
}

void __fastcall SpellsPage::ScrollProc(const INT32 tick, H3BaseDlg *)
{
    if (instance)
        instance->Redraw(tick);
}

SpellsSection::SpellsSection(const int categoriesX, const int categoriesY, const int categoriesWidth,
                             const int categoriesHeight, const int contentX, const int contentY,
                             const int contentWidth, const int contentHeight, H3Dlg *dialog)
    : categoriesPage(categoriesX, categoriesY, categoriesWidth, categoriesHeight, dialog),
      contentPage(contentX, contentY, contentWidth, contentHeight, dialog)
{
    categoriesPage.SetActiveCategory(spells::CATEGORY_ALL);
    contentPage.SetCategory(spells::CATEGORY_ALL);
}

void SpellsSection::SetVisible(const BOOL state) noexcept
{
    categoriesPage.SetVisible(state);
    contentPage.SetVisible(state);
}

void SpellsSection::SetSubtype(const int subtype)
{
    activeSubtype = std::max(0, std::min(subtype, spells::CATEGORY_COUNT - 1));
    const int category = spells::CATEGORY_FIRST + activeSubtype;
    categoriesPage.SetActiveCategory(category);
    contentPage.SetCategory(category);
}

BOOL SpellsSection::ProcessMessage(H3Msg &msg)
{
    if (!msg.IsLeftClick())
        return FALSE;
    if (categoriesPage.IsCategory(msg.itemId))
    {
        activeSubtype = msg.itemId - spells::CATEGORY_FIRST;
        categoriesPage.SetActiveCategory(msg.itemId);
        contentPage.SetCategory(msg.itemId);
        return TRUE;
    }
    if (msg.itemId >= spells::LEVEL_FIRST && msg.itemId <= spells::LEVEL_LAST)
    {
        contentPage.SetLevel(msg.itemId - spells::LEVEL_FIRST);
        return TRUE;
    }
    return FALSE;
}

void SpellsSection::Redraw()
{
    contentPage.Redraw();
}

} // namespace main
