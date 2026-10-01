#include "HeroesPage.h"
#include "ScrollbarUtils.h"

#include <algorithm>

namespace main
{
namespace
{
constexpr LPCSTR kCategoryNames[buttons::HERO_CATEGORY_COUNT] = {
    "All", "Castle", "Rampart", "Tower", "Inferno", "Necropolis", "Dungeon", "Stronghold", "Fortress", "Conflux"};

LPCSTR ReadCategoryName(const int index) noexcept
{
    bool success = false;
    LPCSTR value = EraJS::read(H3String::Format("help.heroes.categories.%d.name", index).String(), success);
    return success && value ? value : kCategoryNames[index];
}
} // namespace

HeroCategoriesPage *HeroCategoriesPage::instance = nullptr;
HeroesPage *HeroesPage::instance = nullptr;

HeroCategoriesPage::HeroCategoriesPage(const int x, const int y, const int width, const int height, H3Dlg *dialog)
    : DlgPage(dialog), pageX(x), pageY(y), pageWidth(width), pageHeight(height)
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
    for (int index = 0; index < buttons::HERO_CATEGORY_COUNT; ++index)
    {
        const int townId = index == 0 ? -1 : index - 1;
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
            pageX + buttonX, pageY + margin, buttonWidth, buttonHeight, buttons::HERO_CATEGORY_FIRST + index,
            "RMGmenbt.def", ReadCategoryName(index), NH3Dlg::Text::SMALL, 0, 0, false, 0, eTextColor::REGULAR);
        if (button)
        {
            button->SetClickFrame(1);
        }
        AddItem(button);
        categoryButtons.emplace_back(button);
    }

    const int maxFirstCategory = std::max(0, buttons::HERO_CATEGORY_COUNT - visibleCategoryCount);
    if (maxFirstCategory > 0)
    {
        scrollBar =
            H3DlgScrollbar::Create(pageX + pageWidth - scrollBarWidth, pageY + margin, 16, pageHeight - margin * 2,
                                   buttons::HERO_CATEGORY_SCROLLBAR, maxFirstCategory + 1, ScrollProc, false, 1, true);
        AddItem(scrollBar);
    }
}

HeroCategoriesPage::~HeroCategoriesPage()
{
    if (instance == this)
        instance = nullptr;
}

void HeroCategoriesPage::Redraw(const int requestedFirstCategory)
{
    constexpr int iconHeight = 48;
    constexpr int buttonHeight = 32;
    constexpr int rowGap = 2;
    constexpr int margin = 2;
    const int maxFirst = std::max(0, buttons::HERO_CATEGORY_COUNT - visibleCategoryCount);
    firstCategoryIndex = helpdlg::UpdateScrollbar(scrollBar, maxFirst, requestedFirstCategory, isVisible);
    for (int index = 0; index < buttons::HERO_CATEGORY_COUNT; ++index)
    {
        const int row = index - firstCategoryIndex;
        const bool visible = isVisible && row >= 0 && row < visibleCategoryCount;
        auto *button = categoryButtons[index];
        auto *icon = categoryIcons[index];
        if (!visible)
        {
            if (button)
                button->HideDeactivate();
            if (icon)
                icon->HideDeactivate();
            continue;
        }
        const int y = pageY + margin + row * (iconHeight + rowGap);
        if (button)
        {
            button->SetY(y + (iconHeight - buttonHeight) / 2);
            button->ShowActivate();
        }
        if (icon)
        {
            icon->SetY(y + (iconHeight - icon->GetHeight()) / 2);
            icon->ShowActivate();
        }
    }
    RedrawDialog();
}

void HeroCategoriesPage::SetActiveCategory(const int categoryIndex) noexcept
{
    activeCategoryIndex = std::max(0, std::min(categoryIndex, buttons::HERO_CATEGORY_COUNT - 1));
    for (int index = 0; index < static_cast<int>(categoryButtons.size()); ++index)
    {
        if (categoryButtons[index])
            categoryButtons[index]->SetFrame(index == activeCategoryIndex ? 1 : 0);
    }
    RedrawDialog();
}

void HeroCategoriesPage::UpdateScrollbarActivation(const H3Msg &msg) noexcept
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

void __fastcall HeroCategoriesPage::ScrollProc(const INT32 tick, H3BaseDlg *)
{
    if (instance)
        instance->Redraw(tick);
}

HeroesPage::HeroesPage(const int x, const int y, const int width, const int height, H3Dlg *dialog)
    : DlgPage(dialog), pageX(x), pageY(y), pageWidth(width), pageHeight(height)
{
    instance = this;
    constexpr int margin = 8;
    constexpr int columnGap = 6;
    constexpr int rowGap = 6;
    constexpr int portraitWidth = 58;
    constexpr int portraitHeight = 64;
    constexpr int nameHeight = 22;
    constexpr int scrollBarWidth = 18;
    const int availableWidth = pageWidth - margin * 2 - scrollBarWidth;
    const int availableHeight = pageHeight - margin * 2;
    const int cellWidth = portraitWidth + 28;
    const int cellHeight = portraitHeight + nameHeight + rowGap;
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

    const int gridY = pageY + margin;
    for (int row = 0; row < rows; ++row)
    {
        for (int column = 0; column < columns; ++column)
        {
            const int cellX = pageX + margin + column * (cellWidth + columnGap);
            const int portraitX = cellX + (cellWidth - portraitWidth) / 2;
            const int portraitY = gridY + row * cellHeight;
            const int itemId = buttons::HERO_ITEM_FIRST + row * columns + column;
            auto *portrait = H3DlgPcx::Create(portraitX, portraitY, portraitWidth, portraitHeight, itemId, nullptr);
            AddItem(portrait);
            heroPortraits.emplace_back(portrait);
            auto *name =
                H3DlgText::Create(cellX, portraitY + portraitHeight, cellWidth, nameHeight, h3_NullString,
                                  NH3Dlg::Text::SMALL, eTextColor::REGULAR, itemId, eTextAlignment::MIDDLE_CENTER, -1);
            AddItem(name);
            heroNames.emplace_back(name);
        }
    }

    const int heroCount = std::max(0, std::min(h3::limits::HEROES, h3::limits::TOTAL_HEROES));
    loadedHeroPortraits.resize(heroCount, nullptr);
    const int initialHeroRows = (heroCount + columns - 1) / columns;
    const int initialContentRows = std::max(0, initialHeroRows - rows);
    if (initialContentRows > 0)
    {
        scrollBar =
            H3DlgScrollbar::Create(pageX + pageWidth - scrollBarWidth - margin, gridY, 16, availableHeight,
                                   buttons::HERO_CONTENT_SCROLLBAR, initialContentRows + 1, ScrollProc, false, 1, true);
        AddItem(scrollBar);
    }
    RebuildHeroList();
}

void HeroesPage::CallCustomFunction(const int heroId, const BOOL) noexcept
{

    H3PcxLoader back = H3LoadedPcx::Load("HStInf.pcx");
    if (!back)
        return;

    struct _HeroType_
    {
        int Belong2Town;
        char *TypeName;
        int Agression;
        char PSkillStart[4];
        char ProbPSkillToLvl9[4];
        char ProbPSkillAfterLvl10[4];
        char ProbSSkill[28];
        char ProbInTown[9];
        char field_3D[3];
    };

    const auto &typesTable = reinterpret_cast<_HeroType_ *>(0x067D868);
    volatile int itemId = 0;
    const int width = back->width;
    const int height = back->height;
    const auto &heroInfo = P_HeroInfo[heroId];
    const auto &specInfo = P_HeroSpecialty[heroId];
    const auto &typeInfo = typesTable[heroInfo.heroClass];

    H3Dlg dlg(width, height, -1, -1, 0, 0, IntAt(0x69CCF4));
    auto backWidget = dlg.CreatePcx(0, 0, itemId++, back->GetName());

    constexpr int leftPartX = 18;

    // create portrait
    dlg.CreatePcx(leftPartX, 19, itemId++, heroInfo.largePortrait);
    dlg.CreateText(85, 22, 200, 32, heroInfo.name, NH3Dlg::Text::BIG, eTextColor::GOLD, itemId++);
    dlg.CreateText(85, 52, 200, 32, typeInfo.TypeName, NH3Dlg::Text::MEDIUM, 1, itemId++);

    constexpr int frameIds[4] = {0, 1, 2, 5};
    // primary skills
    for (size_t i = 0; i < 4; i++)
    {
        const int x = 31 + i * (29 + 42);
        const int y = 111;
        constexpr int textWidth = 71;
        dlg.CreateText(textWidth * i + 16, 90, textWidth, 16, P_PrimarySkillName[i], NH3Dlg::Text::SMALL,
                       eTextColor::REGULAR, itemId++);

        dlg.CreateDef(x, y, itemId++, NH3Dlg::Assets::PSKILL_42, frameIds[i]);

        libc::sprintf(h3_TextBuffer, "%d", typeInfo.PSkillStart[i]);
        dlg.CreateText(textWidth * i + 16, 156, textWidth, 16, h3_TextBuffer, NH3Dlg::Text::SMALL, eTextColor::REGULAR,
                       itemId++);
    }

    // hero bio
    auto bioText = H3HeroDefaultBiography::Get()[heroId];
    // bioText = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20";
    dlg.CreateText(leftPartX + 6, 182, 270, 148, bioText, NH3Dlg::Text::SMALL, eTextColor::REGULAR, itemId++,
                   eTextAlignment::TOP_LEFT);

    // specialization
    dlg.CreateDef(leftPartX, 325, itemId++, NH3Dlg::Assets::UN44_DEF, heroId);

    std::string specializationText(specInfo.spDescr);
    const size_t pos = specializationText.rfind('\n');
    if (pos != std::string::npos)
        specializationText.erase(0, pos + 1);

    dlg.CreateText(leftPartX + 56, 324, 200, 44, specInfo.spFull, NH3Dlg::Text::SMALL, eTextColor::REGULAR, itemId++,
                   eTextAlignment::TOP_LEFT);
    dlg.CreateText(leftPartX + 6, 376, 270, 84, specializationText.c_str(), NH3Dlg::Text::SMALL, eTextColor::REGULAR,
                   itemId++, eTextAlignment::TOP_LEFT);

    // RIGHT DLG PART
    constexpr int rightPartX = 318;
    constexpr int textX = rightPartX + 54;

    int y = 53;

    for (size_t i = 0; i < std::size(heroInfo.sskills); i++)
    {
        auto &skillInfo = heroInfo.sskills[i];
        if (skillInfo.type == -1 || skillInfo.level < 1)
            continue;
        const int frameId = 3 + 3 * skillInfo.type + skillInfo.level - 1;
        dlg.CreateDef(rightPartX, y, itemId++, NH3Dlg::Assets::SSKILL_44, frameId);
        dlg.CreateText(textX, y, 64, 20, H3SecondarySkillLevel::Get()[skillInfo.level], NH3Dlg::Text::SMALL,
                       eTextColor::REGULAR, itemId++, eTextAlignment::MIDDLE_LEFT);
        dlg.CreateText(textX, y + 20, 64, 20, P_SecondarySkillInfo[skillInfo.type].name, NH3Dlg::Text::SMALL,
                       eTextColor::REGULAR, itemId++, eTextAlignment::MIDDLE_LEFT);

        y += 51;
    }
    y = 200;

    int wmMonId = -1;

    int usedMonX = 0;
    for (size_t i = 0; i < std::size(heroInfo.armyType); i++)
    {

        const int monId = heroInfo.armyType[i];
        if (monId < 0)
            continue;
        const int minMon = heroInfo.creatureAmount[i].lowAmount;
        const int maxMon = heroInfo.creatureAmount[i].highAmount;

        if (minMon <= 0 && maxMon <= 0)
            continue;

        int x = rightPartX + usedMonX * 42;
        dlg.CreateDef(rightPartX, y, itemId++, NH3Dlg::Assets::CREATURE_SMALL, monId + 2);
        typeInfo;
        usedMonX++;
    }
    if (wmMonId != -1)
    {
        y = 200;
        dlg.CreateDef(rightPartX, y, itemId++, NH3Dlg::Assets::UN44_DEF, wmMonId);
        dlg.CreateText(textX, y, 64, 20, P_CreatureInformation[wmMonId].nameSingular, NH3Dlg::Text::SMALL,
                       eTextColor::REGULAR, itemId++, eTextAlignment::MIDDLE_LEFT);
    }

    //
    // secondary skills
    // const int x = 31 + 4 * (29 + 42);

    // starting army

    // magic && book
    if (heroInfo.hasSpellbook)
    {
        y = 374;

        dlg.CreateDef(rightPartX + 4, y, itemId++, NH3Dlg::Assets::ARTIFACT_DEF, 0);
        dlg.CreateText(textX, y, 64, 44, P_ArtifactSetup[0].name, NH3Dlg::Text::SMALL, eTextColor::REGULAR, itemId++,
                       eTextAlignment::MIDDLE_LEFT);

        const int spellId = heroInfo.startingSpell;

        y = 425;
        dlg.CreateDef(rightPartX, y, itemId++, NH3Dlg::Assets::SPELL_SMALL, spellId + 1);
        dlg.CreateText(textX, y, 64, 32, P_Spell[spellId].name, NH3Dlg::Text::SMALL, eTextColor::REGULAR, itemId++,
                       eTextAlignment::MIDDLE_LEFT);
    }
    //   backWidget->SetPcx(back);
    //  H3DlgPcx =
    dlg.RMB_Show();
}

void HeroesPage::OnLeftClick(const int heroId) noexcept
{
    CallCustomFunction(heroId, FALSE);
}

void HeroesPage::OnRightClick(const int heroId) noexcept
{
    CallCustomFunction(heroId, TRUE);
}

BOOL HeroesPage::ProcessItemMessage(H3Msg &msg)
{
    if (!msg.IsLeftClick() && !msg.IsRightClick())
        return FALSE;
    if (msg.itemId < buttons::HERO_ITEM_FIRST || msg.itemId > buttons::HERO_ITEM_LAST)
        return FALSE;

    const int slot = msg.itemId - buttons::HERO_ITEM_FIRST;
    const int heroIndex = firstRow * columns + slot;
    if (!isVisible || slot < 0 || slot >= static_cast<int>(heroPortraits.size()) || heroIndex < 0 ||
        heroIndex >= static_cast<int>(heroIds.size()))
        return TRUE;

    if (msg.IsRightClick())
        OnRightClick(heroIds[heroIndex]);
    else
        OnLeftClick(heroIds[heroIndex]);
    return TRUE;
}

HeroesPage::~HeroesPage()
{
    // H3DlgPcx does not own the references cached by this page. Detach every
    // slot before releasing them so the dialog cannot retain a stale PCX.
    for (auto *portrait : heroPortraits)
    {
        if (portrait)
            portrait->SetPcx(static_cast<H3LoadedPcx *>(nullptr));
    }
    for (auto *portrait : loadedHeroPortraits)
    {
        if (portrait)
            portrait->Dereference();
    }

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

BOOL HeroesPage::MatchesCategory(const int heroId) const noexcept
{
    if (activeCategory == buttons::HERO_CATEGORY_ALL)
        return TRUE;
    if (activeCategory < buttons::HERO_CATEGORY_TOWN_FIRST || activeCategory > buttons::HERO_CATEGORY_TOWN_LAST)
        return FALSE;
    const int townId = P_HeroInfo[heroId].heroClass / 2;
    return townId == activeCategory - buttons::HERO_CATEGORY_TOWN_FIRST;
}

H3LoadedPcx *HeroesPage::GetHeroPortrait(const int heroId) noexcept
{
    if (heroId < 0 || heroId >= static_cast<int>(loadedHeroPortraits.size()))
        return nullptr;

    auto *&loadedPortrait = loadedHeroPortraits[heroId];
    if (!loadedPortrait)
    {
        LPCSTR portraitName = P_HeroInfo[heroId].largePortrait;
        if (portraitName && *portraitName)
            loadedPortrait = H3LoadedPcx::Load(portraitName);
    }
    return loadedPortrait;
}

void HeroesPage::RebuildHeroList()
{
    heroIds.clear();
    const int heroCount = std::max(0, std::min(h3::limits::HEROES, h3::limits::TOTAL_HEROES));
    for (int heroId = 0; heroId < heroCount; ++heroId)
    {
        if (MatchesCategory(heroId))
            heroIds.emplace_back(heroId);
    }
    std::sort(heroIds.begin(), heroIds.end());
    firstRow = 0;
}

void HeroesPage::SetCategory(const int categoryIndex)
{
    activeCategory =
        buttons::HERO_CATEGORY_FIRST + std::max(0, std::min(categoryIndex, buttons::HERO_CATEGORY_COUNT - 1));
    RebuildHeroList();
    Redraw();
}

void HeroesPage::Redraw(const int requestedFirstRow)
{
    const int totalRows = (static_cast<int>(heroIds.size()) + columns - 1) / columns;
    const int maxFirstRow = std::max(0, totalRows - rows);
    firstRow = helpdlg::UpdateScrollbar(scrollBar, maxFirstRow, requestedFirstRow, isVisible);
    const int firstItem = firstRow * columns;
    for (size_t slot = 0; slot < heroPortraits.size(); ++slot)
    {
        auto *portrait = heroPortraits[slot];
        auto *name = slot < heroNames.size() ? heroNames[slot] : nullptr;
        const int heroIndex = firstItem + static_cast<int>(slot);
        if (portrait && isVisible && heroIndex >= 0 && heroIndex < static_cast<int>(heroIds.size()))
        {
            const int heroId = heroIds[heroIndex];
            const auto &heroInfo = P_HeroInfo[heroId];
            if (auto *loadedPortrait = GetHeroPortrait(heroId))
            {
                // Use the pointer overload. The name overload sends a dialog
                // command and may leave the previous resource in the slot if
                // loading fails, producing a mismatched final portrait.
                portrait->SetPcx(loadedPortrait);
                // A slot can have been hidden on a previous scroll/filter
                // pass. Setting a new PCX does not reactivate the dialog
                // item, so explicitly show it after assigning the portrait.
                portrait->ShowActivate();
            }
            else
            {
                portrait->SetPcx(static_cast<H3LoadedPcx *>(nullptr));
                portrait->HideDeactivate();
            }
            if (name)
            {
                name->SetText(heroInfo.name ? heroInfo.name : h3_NullString);
                name->Show();
            }
        }
        else
        {
            if (portrait)
            {
                portrait->SetPcx(static_cast<H3LoadedPcx *>(nullptr));
                portrait->HideDeactivate();
            }
            if (name)
                name->HideDeactivate();
        }
    }
    RedrawDialog();
}

void __fastcall HeroesPage::ScrollProc(const INT32 tick, H3BaseDlg *)
{
    if (instance)
        instance->Redraw(tick);
}

HeroesSection::HeroesSection(const int categoriesX, const int categoriesY, const int categoriesWidth,
                             const int categoriesHeight, const int contentX, const int contentY, const int contentWidth,
                             const int contentHeight, H3Dlg *dialog)
    : categoriesPage(categoriesX, categoriesY, categoriesWidth, categoriesHeight, dialog),
      contentPage(contentX, contentY, contentWidth, contentHeight, dialog)
{
    categoriesPage.SetActiveCategory(0);
    contentPage.SetCategory(0);
}

void HeroesSection::SetVisible(const BOOL state) noexcept
{
    categoriesPage.SetVisible(state);
    contentPage.SetVisible(state);
}

void HeroesSection::SetSubtype(const int subtype)
{
    activeSubtype = std::max(0, std::min(subtype, buttons::HERO_CATEGORY_COUNT - 1));
    categoriesPage.SetActiveCategory(activeSubtype);
    contentPage.SetCategory(activeSubtype);
}

void HeroesSection::UpdateMousePosition(const H3Msg &msg) noexcept
{
    categoriesPage.UpdateScrollbarActivation(msg);
}

BOOL HeroesSection::ProcessMessage(H3Msg &msg)
{
    if (contentPage.ProcessItemMessage(msg))
        return TRUE;
    if (!msg.IsLeftClick())
        return FALSE;
    if (msg.itemId >= buttons::HERO_CATEGORY_FIRST && msg.itemId <= buttons::HERO_CATEGORY_LAST)
    {
        const int categoryIndex = msg.itemId - buttons::HERO_CATEGORY_FIRST;
        activeSubtype = categoryIndex;
        categoriesPage.SetActiveCategory(categoryIndex);
        contentPage.SetCategory(categoryIndex);
        return TRUE;
    }
    return FALSE;
}

void HeroesSection::Redraw()
{
    categoriesPage.Redraw();
    contentPage.Redraw();
}

} // namespace main
