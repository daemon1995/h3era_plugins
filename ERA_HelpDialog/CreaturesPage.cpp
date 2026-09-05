#include "CreaturesPage.h"
#include "ScrollbarUtils.h"

namespace main
{

CreatureCategoriesPage *CreatureCategoriesPage::instance = nullptr;
CreaturesPage *CreaturesPage::instance = nullptr;

CreatureCategoriesPage::CreatureCategoriesPage(const int x, const int y, const int width, const int height,
                                               H3Dlg *dialog)
    : HelpPage(dialog), pageX(x), pageY(y), pageWidth(width), pageHeight(height)
{
    instance = this;
    constexpr int iconHeight = 48;
    constexpr int buttonHeight = 32;
    constexpr int rowGap = 2;
    constexpr int margin = 2;
    constexpr int iconWidth = 48;
    constexpr int scrollBarWidth = 18;

    visibleCategoryCount = std::max(1, (pageHeight - margin * 2 + rowGap) / (iconHeight + rowGap));
    AddFrame(pageX, pageY, pageWidth, pageHeight);
    for (int index = 0; index < buttons::CREATURE_CATEGORY_COUNT; ++index)
    {
        const int townId = index >= buttons::CREATURE_CATEGORY_TOWN_FIRST - buttons::CREATURE_CATEGORY_FIRST &&
                                   index <= buttons::CREATURE_CATEGORY_TOWN_LAST - buttons::CREATURE_CATEGORY_FIRST
                               ? index - 1
                               : -1;
        H3DlgDef *icon = nullptr;
        if (townId >= 0)
        {
            icon = H3DlgDef::Create(pageX + margin, pageY + margin, -1, NH3Dlg::Assets::TOWN_SMALL, townId * 2 + 2,
                                    townId);
        }
        AddItem(icon);
        categoryIcons.emplace_back(icon);

        const int buttonX = townId >= 0 ? margin + iconWidth + 4 : margin;
        const int buttonWidth = pageWidth - buttonX - scrollBarWidth - margin;
        auto *button = H3DlgCaptionButton::Create(
            pageX + buttonX, pageY + margin, buttonWidth, buttonHeight, buttons::CREATURE_CATEGORY_FIRST + index,
            "RMGmenbt.def", EraJS::read(H3String::Format("help.creatures.categories.%d.name", index).String()),
            NH3Dlg::Text::SMALL, 0, 0, false, 0, eTextColor::REGULAR);
        if (button)
        {
            button->SetClickFrame(1);
            button->SetHints(EraJS::read(H3String::Format("help.creatures.categories.%d.hint", index).String()),
                             EraJS::read(H3String::Format("help.creatures.categories.%d.rmc", index).String()), false);
        }
        AddItem(button);
        categoryButtons.emplace_back(button);
    }

    const int maxFirstCategory = std::max(0, buttons::CREATURE_CATEGORY_COUNT - visibleCategoryCount);
    if (maxFirstCategory > 0)
    {
        scrollBar =
            H3DlgScrollbar::Create(pageX + pageWidth - scrollBarWidth, pageY + margin, 16, pageHeight - margin * 2,
                                   buttons::CREATURE_CATEGORY_SCROLLBAR, maxFirstCategory + 1, ScrollProc, false, 1, true);
        AddItem(scrollBar);
    }
}

CreatureCategoriesPage::~CreatureCategoriesPage()
{
    if (instance == this)
    {
        instance = nullptr;
    }
}

void CreatureCategoriesPage::Redraw(const int requestedFirstCategory)
{
    if (categoryButtons.empty())
    {
        return;
    }

    constexpr int iconHeight = 48;
    constexpr int buttonHeight = 32;
    constexpr int rowGap = 2;
    constexpr int margin = 2;
    const int maxFirst = std::max(0, buttons::CREATURE_CATEGORY_COUNT - visibleCategoryCount);
    firstCategoryIndex = helpdlg::UpdateScrollbar(scrollBar, maxFirst, requestedFirstCategory, isVisible);

    for (int index = 0; index < buttons::CREATURE_CATEGORY_COUNT; ++index)
    {
        const int row = index - firstCategoryIndex;
        const bool visible = isVisible && row >= 0 && row < visibleCategoryCount;
        auto *button = categoryButtons[index];
        auto *icon = categoryIcons[index];
        if (!button || !visible)
        {
            if (button)
            {
                button->HideDeactivate();
            }
            if (icon)
            {
                icon->HideDeactivate();
            }
            continue;
        }

        const int y = pageY + margin + row * (iconHeight + rowGap);
        button->SetY(y + (iconHeight - buttonHeight) / 2);
        button->ShowActivate();
        if (icon)
        {
            icon->SetY(y + (iconHeight - icon->GetHeight()) / 2);
            icon->ShowActivate();
        }
    }

    RedrawDialog();
}

void CreatureCategoriesPage::SetActiveCategory(const int categoryIndex) noexcept
{
    activeCategoryIndex = std::max(0, std::min(categoryIndex, buttons::CREATURE_CATEGORY_COUNT - 1));
    for (int index = 0; index < static_cast<int>(categoryButtons.size()); ++index)
    {
        if (categoryButtons[index])
        {
            categoryButtons[index]->SetFrame(index == activeCategoryIndex ? 1 : 0);
        }
    }
    RedrawDialog();
}

void __fastcall CreatureCategoriesPage::ScrollProc(INT32 tick, H3BaseDlg *dlg)
{
    if (instance)
    {
        instance->Redraw(tick);
    }
}

CreaturesPage::CreaturesPage(const int x, const int y, const int width, const int height, H3Dlg *dialog)
    : HelpPage(dialog), pageX(x), pageY(y), pageWidth(width), pageHeight(height)
{
    instance = this;
    H3DefLoader def(NH3Dlg::Assets::CREATURE_LARGE);
    if (!def.Get())
    {
        return;
    }

    constexpr int margin = 8;
    constexpr int columnGap = 6;
    constexpr int rowGap = 6;
    constexpr int nameHeight = 22;
    constexpr int filterPanelHeight = 40;
    constexpr int scrollBarWidth = 18;
    const int availableWidth = pageWidth - margin * 2 - scrollBarWidth;
    const int availableHeight = pageHeight - filterPanelHeight - margin * 2;
    const int cellWidth = def->widthDEF + 34;
    const int cellHeight = def->heightDEF + nameHeight + rowGap;
    columns = std::max(1, (availableWidth + columnGap) / (cellWidth + columnGap));
    rows = std::max(1, availableHeight / cellHeight);

    backPcx = H3DlgPcx16::Create(pageX + 1, pageY + 1, pageWidth - 2, pageHeight - 2, -1, nullptr);
    if (auto *backgroundPcx = H3LoadedPcx16::Create(pageWidth - 2, pageHeight - 2))
    {
        memset(backgroundPcx->buffer, 14, backgroundPcx->buffSize);
        if (backPcx)
        {
            backPcx->SetPcx(backgroundPcx);
        }
        else
        {
            backgroundPcx->Destroy();
        }
    }
    AddItem(backPcx);
    AddFrame(pageX, pageY, pageWidth, pageHeight);
    AddFrame(pageX + margin / 2, pageY + margin / 2, pageWidth - margin, filterPanelHeight - margin);

    int levelButtonX = pageX + margin;
    for (int level = 0; level < buttons::CREATURE_LEVEL_COUNT; ++level)
    {
        const H3String defName = H3String::Format("RanNum%d.def", level + 1);
        H3DefLoader levelDef(defName.String());
        if (!levelDef.Get())
        {
            levelButtons.emplace_back(nullptr);
            continue;
        }

        const int levelButtonY = pageY + (filterPanelHeight - levelDef->heightDEF) / 2;
        auto *button = H3DlgDefButton::Create(levelButtonX, levelButtonY, buttons::CREATURE_LEVEL_FIRST + level,
                                              defName.String(), 0, 1, false, 0);
        if (button)
        {
            button->SetHints(EraJS::read(H3String::Format("help.creatures.levels.%d.hint", level).String()),
                             EraJS::read(H3String::Format("help.creatures.levels.%d.rmc", level).String()), false);
            levelButtonX += button->GetWidth() + 4;
        }
        levelButtons.emplace_back(button);
        AddItem(button);
    }

    const int gridY = pageY + filterPanelHeight + margin;
    for (int row = 0; row < rows; ++row)
    {
        for (int column = 0; column < columns; ++column)
        {
            const int cellX = pageX + margin + column * (cellWidth + columnGap);
            const int portraitX = cellX + (cellWidth - def->widthDEF) / 2;
            const int portraitY = gridY + row * cellHeight;
            const int itemId = buttons::CREATURE_ITEM_FIRST + row * columns + column;
            auto *portrait = H3DlgDef::Create(portraitX, portraitY, itemId, NH3Dlg::Assets::CREATURE_LARGE, 0);
            AddItem(portrait);
            creaturePortraits.emplace_back(portrait);

            auto *name =
                H3DlgText::Create(cellX, portraitY + def->heightDEF, cellWidth, nameHeight, h3_NullString,
                                  NH3Dlg::Text::SMALL, eTextColor::REGULAR, itemId, eTextAlignment::MIDDLE_CENTER, -1);
            AddItem(name);
            creatureNames.emplace_back(name);
        }
    }

    const int initialCreatureRows = (std::max(0, static_cast<int>(P_CreatureCount)) + columns - 1) / columns;
    const int initialContentRows = std::max(0, initialCreatureRows - rows);
    if (initialContentRows > 0)
    {
        contentScrollBar = H3DlgScrollbar::Create(pageX + pageWidth - scrollBarWidth - margin, gridY, 16,
                                                   availableHeight, buttons::CREATURE_CONTENT_SCROLLBAR,
                                                   initialContentRows + 1, ScrollProc, false, 1, true);
        if (contentScrollBar)
            AddItem(contentScrollBar);
    }

    RebuildCreatureList();
}

void CreaturesPage::CallCustomFunction(const int, const BOOL) noexcept
{
}

void CreaturesPage::OnLeftClick(const int creatureId) noexcept
{
    CallCustomFunction(creatureId, FALSE);
}

void CreaturesPage::OnRightClick(const int creatureId) noexcept
{
    CallCustomFunction(creatureId, TRUE);
}

BOOL CreaturesPage::ProcessItemMessage(H3Msg &msg)
{
    if (!msg.IsLeftClick() && !msg.IsRightClick())
        return FALSE;
    if (msg.itemId < buttons::CREATURE_ITEM_FIRST || msg.itemId > buttons::CREATURE_ITEM_LAST)
        return FALSE;

    const int slot = msg.itemId - buttons::CREATURE_ITEM_FIRST;
    const int creatureIndex = firstRow * columns + slot;
    if (!isVisible || slot < 0 || slot >= static_cast<int>(creaturePortraits.size()) ||
        creatureIndex < 0 || creatureIndex >= static_cast<int>(creatureIds.size()))
        return TRUE;

    if (msg.IsRightClick())
        OnRightClick(creatureIds[creatureIndex]);
    else
        OnLeftClick(creatureIds[creatureIndex]);
    return TRUE;
}

CreaturesPage::~CreaturesPage()
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
    {
        instance = nullptr;
    }
}

BOOL CreaturesPage::MatchesCategory(const int creatureId) const noexcept
{
    if (activeCategory == buttons::CREATURE_CATEGORY_ALL)
    {
        return TRUE;
    }

    const auto &creature = P_CreatureInformation[creatureId];
    const BOOL isCommander = Era::IsCommanderId(creatureId);
    const BOOL isWarMachine = THISCALL_1(BOOL8, 0x47AAB0, creatureId);
    if (activeCategory == buttons::CREATURE_CATEGORY_WAR_MACHINES)
    {
        return isWarMachine;
    }
    if (activeCategory == buttons::CREATURE_CATEGORY_COMMANDERS)
    {
        return isCommander;
    }
    if (isCommander || isWarMachine)
    {
        return FALSE;
    }
    if (activeCategory == buttons::CREATURE_CATEGORY_NEUTRAL)
    {
        return creature.town < 0;// || creature.town > 8;
    }
    if (activeCategory >= buttons::CREATURE_CATEGORY_TOWN_FIRST &&
        activeCategory <= buttons::CREATURE_CATEGORY_TOWN_LAST)
    {
        return creature.town == activeCategory - buttons::CREATURE_CATEGORY_TOWN_FIRST;
    }
    return FALSE;
}

void CreaturesPage::RebuildCreatureList()
{
    creatureIds.clear();
    const int creatureCount = std::max(0, static_cast<int>(P_CreatureCount));
    for (int creatureId = 0; creatureId < creatureCount; ++creatureId)
    {
        const auto &creature = P_CreatureInformation[creatureId];
        if (MatchesCategory(creatureId) && (activeLevel < 0 || creature.level == activeLevel))
        {
            creatureIds.emplace_back(creatureId);
        }
    }

    std::sort(creatureIds.begin(), creatureIds.end(), [](const int first, const int second) {
        const int firstLevel = P_CreatureInformation[first].level;
        const int secondLevel = P_CreatureInformation[second].level;
        return firstLevel == secondLevel ? first < second : firstLevel < secondLevel;
    });
    firstRow = 0;
}

void CreaturesPage::SetCategory(const int categoryIndex)
{
    activeCategory =
        buttons::CREATURE_CATEGORY_FIRST + std::max(0, std::min(categoryIndex, buttons::CREATURE_CATEGORY_COUNT - 1));
    RebuildCreatureList();
    Redraw();
}

void CreaturesPage::SetLevel(const int level)
{
    const int requestedLevel = std::max(0, std::min(level, buttons::CREATURE_LEVEL_COUNT - 1));
    activeLevel = activeLevel == requestedLevel ? -1 : requestedLevel;
    for (int index = 0; index < static_cast<int>(levelButtons.size()); ++index)
    {
        if (levelButtons[index])
        {
            levelButtons[index]->SetFrame(index == activeLevel ? 1 : 0);
        }
    }
    RebuildCreatureList();
    Redraw();
}

void CreaturesPage::Redraw(const int requestedFirstRow)
{
    const int totalRows = (static_cast<int>(creatureIds.size()) + columns - 1) / columns;
    const int maxFirstRow = std::max(0, totalRows - rows);
    firstRow = helpdlg::UpdateScrollbar(contentScrollBar, maxFirstRow, requestedFirstRow, isVisible);
    const int firstItem = firstRow * columns;

    for (size_t slot = 0; slot < creaturePortraits.size(); ++slot)
    {
        auto *portrait = creaturePortraits[slot];
        auto *name = slot < creatureNames.size() ? creatureNames[slot] : nullptr;
        const int creatureIndex = firstItem + static_cast<int>(slot);
        if (portrait && isVisible && creatureIndex >= 0 && creatureIndex < static_cast<int>(creatureIds.size()))
        {
            const int creatureId = creatureIds[creatureIndex];
            portrait->SetFrame(creatureId + 2);
            portrait->ShowActivate();
            if (name)
            {
                const LPCSTR creatureName = P_CreatureInformation[creatureId].nameSingular;
                name->SetText(creatureName ? creatureName : h3_NullString);
                name->Show();
            }
        }
        else
        {
            if (portrait)
            {
                portrait->HideDeactivate();
            }
            if (name)
            {
                name->HideDeactivate();
            }
        }
    }

    RedrawDialog();
}

void __fastcall CreaturesPage::ScrollProc(INT32 tick, H3BaseDlg *dlg)
{
    if (instance)
    {
        instance->Redraw(tick);
    }
}

CreaturesSection::CreaturesSection(const int categoriesX, const int categoriesY, const int categoriesWidth,
                                   const int categoriesHeight, const int contentX, const int contentY,
                                   const int contentWidth, const int contentHeight, H3Dlg *dialog)
    : categoriesPage(categoriesX, categoriesY, categoriesWidth, categoriesHeight, dialog),
      contentPage(contentX, contentY, contentWidth, contentHeight, dialog)
{
    categoriesPage.SetActiveCategory(0);
    contentPage.SetCategory(0);
}

void CreaturesSection::SetVisible(const BOOL state) noexcept
{
    categoriesPage.SetVisible(state);
    contentPage.SetVisible(state);
}

void CreaturesSection::SetSubtype(const int subtype)
{
    activeSubtype = std::max(0, std::min(subtype, buttons::CREATURE_CATEGORY_COUNT - 1));
    categoriesPage.SetActiveCategory(activeSubtype);
    contentPage.SetCategory(activeSubtype);
}

BOOL CreaturesSection::ProcessMessage(H3Msg &msg)
{
    if (contentPage.ProcessItemMessage(msg))
        return TRUE;
    if (!msg.IsLeftClick())
    {
        return FALSE;
    }
    if (msg.itemId >= buttons::CREATURE_CATEGORY_FIRST && msg.itemId <= buttons::CREATURE_CATEGORY_LAST)
    {
        const int categoryIndex = msg.itemId - buttons::CREATURE_CATEGORY_FIRST;
        activeSubtype = categoryIndex;
        categoriesPage.SetActiveCategory(categoryIndex);
        contentPage.SetCategory(categoryIndex);
        return TRUE;
    }
    if (msg.itemId >= buttons::CREATURE_LEVEL_FIRST && msg.itemId <= buttons::CREATURE_LEVEL_LAST)
    {
        contentPage.SetLevel(msg.itemId - buttons::CREATURE_LEVEL_FIRST);
        return TRUE;
    }
    return FALSE;
}

void CreaturesSection::Redraw()
{
    categoriesPage.Redraw();
    contentPage.Redraw();
}

} // namespace main
