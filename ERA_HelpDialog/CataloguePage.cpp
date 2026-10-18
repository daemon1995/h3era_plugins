#include "CataloguePage.h"
#include "ScrollbarUtils.h"

namespace main
{
CatalogueSection *CatalogueSection::visibleSection = nullptr;
namespace
{
constexpr int kCategoryScroll = 40;
constexpr int kContentScroll = 41;
constexpr int kSort = 44;
constexpr int kResetFilters = 45;
constexpr int kSortArrow = 46;
constexpr int kFacetFirst = 50;
constexpr int kFacetArrowFirst = 70;
constexpr int kLevelFirst = 60;
constexpr int kObjectFirst = 100;
constexpr int kChoiceFirst = 29000;
constexpr int kChoiceScroll = 28999;
constexpr int kChoiceRowHeight = 24;
constexpr LPCSTR kResetFiltersDef = "ERAHDRST.DEF";
const eHelpPage kCataloguePages[] = {eHelpPage::CREATURES, eHelpPage::ARTIFACTS,        eHelpPage::HEROES,
                                     eHelpPage::SPELLS,    eHelpPage::SECONDARY_SKILLS, eHelpPage::TOWNS};

class ChoiceListDlg final : public H3Dlg
{
    std::vector<LPCSTR> options;
    std::vector<H3DlgTextPcx *> buttons;
    H3DlgFrame *hoverFrame = nullptr;
    H3DlgScrollbar *scroll = nullptr;
    int selected = 0;
    int result = -1;
    int first = 0;
    int rows = 1;
    int hoverRow = -1;
    static ChoiceListDlg *instance;

    void RedrawRows(int requested)
    {
        const int maxFirst = std::max(0, static_cast<int>(options.size()) - rows);
        first = helpdlg::UpdateScrollbar(scroll, maxFirst, requested, TRUE);
        for (size_t row = 0; row < buttons.size(); ++row)
        {
            auto *button = buttons[row];
            const int index = first + static_cast<int>(row);
            if (button && index < static_cast<int>(options.size()))
            {
                button->SetText(options[index]);
                button->ShowActivate();
            }
            else if (button)
                button->HideDeactivate();
        }
        helpdlg::PlaceSelectorFrame(
            hoverFrame, hoverRow >= 0 && hoverRow < static_cast<int>(buttons.size()) ? buttons[hoverRow] : nullptr);
        if (P_WindowManager->lastDlg == this)
            Redraw();
    }
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *)
    {
        if (instance)
            instance->RedrawRows(tick);
    }

  public:
    ChoiceListDlg(int x, int y, int width, int visibleRows, std::vector<LPCSTR> values, int current)
        : H3Dlg(width, std::max(1, visibleRows) * kChoiceRowHeight, x, y, false, false), options(std::move(values)),
          selected(helpdlg::Bound(current, 0, std::max(0, static_cast<int>(options.size()) - 1))),
          rows(std::max(1, visibleRows))
    {
        instance = this;
        flags &= ~16;
        helpdlg::AddSafeBackground(*this, false, false);
        const bool needsScroll = static_cast<int>(options.size()) > rows;
        const int rowWidth = widthDlg - (needsScroll ? 16 : 0);
        for (int row = 0; row < rows; ++row)
        {
            auto *text =
                helpdlg::SelectorText(0, row * kChoiceRowHeight, rowWidth, kChoiceRowHeight, kChoiceFirst + row);
            if (text)
                AddItem(text);
            buttons.push_back(text);
        }
        if (needsScroll)
        {
            scroll = H3DlgScrollbar::Create(widthDlg - 16, 0, 16, heightDlg, kChoiceScroll,
                                            static_cast<int>(options.size()) - rows + 1, ScrollProc, false, 1, true);
            AddItem(scroll);
        }
        first = helpdlg::Bound(selected - rows + 1, 0, std::max(0, static_cast<int>(options.size()) - rows));
        helpdlg::GoldFrame(*this, 0, 0, widthDlg, heightDlg);
        hoverFrame = helpdlg::GoldFrame(*this, 0, 0, rowWidth, kChoiceRowHeight);
        if (hoverFrame)
            hoverFrame->HideDeactivate();
        RedrawRows(first);
    }
    virtual ~ChoiceListDlg()
    {
        for (auto *button : buttons)
            helpdlg::ReleaseSelectorText(button);
        if (instance == this)
            instance = nullptr;
    }
    int Result() const noexcept
    {
        return result;
    }
    BOOL DialogProc(H3Msg &msg) override
    {
        if (msg.IsMouseOver())
        {
            const int mouseX = msg.GetX() - GetX(), mouseY = msg.GetY() - GetY();
            const int rowWidth = widthDlg - (scroll ? 16 : 0);
            const int row =
                mouseX >= 0 && mouseX < rowWidth && mouseY >= 0 && mouseY < heightDlg ? mouseY / kChoiceRowHeight : -1;
            if (hoverRow != row)
            {
                hoverRow = row;
                if (row >= 0)
                    selected = first + row;
                RedrawRows(first);
            }
            return FALSE;
        }
        if (msg.IsKeyPress())
        {
            if (msg.GetKey() == eVKey::H3VK_ESCAPE)
            {
                Stop();
                return FALSE;
            }
            if (msg.GetKey() == eVKey::H3VK_ENTER)
            {
                result = selected;
                Stop();
                return FALSE;
            }
            if (msg.GetKey() == eVKey::H3VK_UP || msg.GetKey() == eVKey::H3VK_DOWN)
            {
                selected = helpdlg::Bound(selected + (msg.GetKey() == eVKey::H3VK_UP ? -1 : 1), 0,
                                          std::max(0, static_cast<int>(options.size()) - 1));
                if (selected < first)
                    first = selected;
                else if (selected >= first + rows)
                    first = selected - rows + 1;
                hoverRow = selected - first;
                RedrawRows(first);
                return FALSE;
            }
        }
        if (msg.ClickOutside())
        {
            Stop();
            return FALSE;
        }
        const int row = msg.itemId - kChoiceFirst;
        if (msg.IsLeftDown() && row >= 0 && row < rows && first + row < static_cast<int>(options.size()))
        {
            result = first + row;
            Stop();
            return FALSE;
        }
        return TRUE;
    }
};
ChoiceListDlg *ChoiceListDlg::instance = nullptr;

H3DlgFrame *AddDecorationFrame(H3Dlg *dialog, std::vector<H3DlgItem *> &decorations, int x, int y, int width,
                               int height, H3RGB565 color)
{
    auto *frame = H3DlgFrame::Create(x, y, width, height, -1, color);
    if (frame)
    {
        dialog->AddItem(frame);
        decorations.push_back(frame);
        frame->DeActivate();
        frame->HideDeactivate();
    }
    return frame;
}
} // namespace

CatalogueSection::CatalogueSection(int categoriesX, int categoriesY, int categoriesWidth, int categoriesHeight,
                                   int contentX, int contentWidth, H3Dlg *parent, const helpdlg::Catalogue &data)
    : DlgPage(parent), catalogue(data), x(contentX), y(categoriesY), width(contentWidth), height(categoriesHeight)
{
    const int index = static_cast<int>(std::find(std::begin(kCataloguePages), std::end(kCataloguePages), data.page) -
                                       std::begin(kCataloguePages));
    idBase = 3000 + index * 2000;
    portraitCache.resize(catalogue.entries.size(), nullptr);
    AddFrame(categoriesX, y, categoriesWidth, height);
    AddFrame(x, y, width, height);

    const int categoryStep = helpdlg::CategoryButtonStep();
    categoryRows =
        std::min(std::max(1, (height - 12) / categoryStep), std::max(1, static_cast<int>(catalogue.categories.size())));
    for (int row = 0; row < categoryRows; ++row)
    {
        auto *button =
            helpdlg::CategoryButton(categoriesX + 6, y + 6 + row * categoryStep, idBase + row, h3_NullString);
        AddItem(button);
        categoryButtons.push_back(button);
    }
    categoryScroll = H3DlgScrollbar::Create(categoriesX + categoriesWidth - 22, y + 2, 16, std::max(16, height - 4),
                                            idBase + kCategoryScroll, 2, CategoryScrollProc, false, 1, true);
    AddItem(categoryScroll);

    const int controlsY = y + 6;
    const int controlsHeight = 24;
    const bool hasFilters = catalogue.levelCount > 0 || !catalogue.facets.empty();
    const int controlsLeft = x + 8;
    const int sortBlockWidth = std::min(180, std::max(130, width / 4));
    const int sortX = controlsLeft;
    H3FontLoader controlFont(NH3Dlg::Text::SMALL);
    const auto labelWidth = [&](LPCSTR text, int maxWidth) {
        const int measured = controlFont.Get() ? controlFont->GetMaxLineWidth(text) : 42;
        return helpdlg::Bound(measured + 2, 16, maxWidth);
    };
    const int sortLabelWidth = labelWidth(helpdlg::Text("help.ui.sort"), 42);
    const auto createSelector = [&](int left, int top, int controlWidth, int id, int arrowId) {
        const int arrowWidth = helpdlg::kSelectorArrowWidth;
        auto *text = helpdlg::SelectorText(left, top, std::max(1, controlWidth - arrowWidth), controlsHeight, id);
        AddItem(text);
        H3DefLoader arrowDef("OVBUTN2.def");
        if (arrowDef.Get() && arrowDef->widthDEF > 0 && arrowDef->heightDEF > 0)
        {
            auto *arrow = H3DlgDef::Create(left + controlWidth - arrowWidth + (arrowWidth - arrowDef->widthDEF) / 2,
                                           top + (controlsHeight - arrowDef->heightDEF) / 2, arrowId, "OVBUTN2.def", 2);
            AddItem(arrow);
            if (arrow)
                arrow->SetHints(helpdlg::Text("help.ui.dropdown_hint"), nullptr, false);
        }
        AddDecorationFrame(dialog, decorations, left, top, controlWidth, controlsHeight,
                           H3RGB565(H3RGB888::Highlight()));
        AddDecorationFrame(dialog, decorations, left + controlWidth - arrowWidth, top, 1, controlsHeight,
                           H3RGB565(H3RGB888::Highlight()));
        if (text)
            text->SetHints(helpdlg::Text("help.ui.dropdown_hint"), nullptr, false);
        return text;
    };
    sortLabel = H3DlgText::Create(sortX, controlsY, sortLabelWidth, controlsHeight, helpdlg::Text("help.ui.sort"),
                                  NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_RIGHT);
    AddItem(sortLabel);
    sortButton = createSelector(sortX + sortLabelWidth + 4, controlsY, sortBlockWidth - sortLabelWidth - 4,
                                idBase + kSort, idBase + kSortArrow);

    constexpr int resetWidth = 24;
    const int resetX = sortX + sortBlockWidth + 6;
    if (hasFilters)
    {
        resetButton = helpdlg::IconButton(nullptr, resetX, controlsY, resetWidth, controlsHeight,
                                          idBase + kResetFilters, kResetFiltersDef, h3_NullString);
        AddItem(resetButton);
        if (resetButton)
            resetButton->SetHints(helpdlg::Text("help.ui.reset_filters"), nullptr, false);
    }

    // Latest layout order: sort dropdown, reset icon, then page-specific filters.
    const int facetsLeft = hasFilters ? resetX + resetWidth + 8 : sortX + sortBlockWidth + 8;
    const int facetsRight = x + width - 8;
    const int facetsWidth = std::max(0, facetsRight - facetsLeft);
    const int facetCount = static_cast<int>(catalogue.facets.size());
    const int facetBlockWidth = facetCount ? facetsWidth / facetCount : 0;
    for (int facet = 0; facet < facetCount; ++facet)
    {
        const int left = facetsLeft + facet * facetBlockWidth;
        const int blockWidth = facet == facetCount - 1 ? facetsRight - left : facetBlockWidth;
        const int captionWidth = labelWidth(catalogue.facets[facet].name, std::min(78, std::max(16, blockWidth / 2)));
        auto *label = H3DlgText::Create(left, controlsY, captionWidth, controlsHeight, catalogue.facets[facet].name,
                                        NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_RIGHT);
        AddItem(label);
        facetLabels.push_back(label);
        auto *button = createSelector(left + captionWidth + 4, controlsY, std::max(28, blockWidth - captionWidth - 8),
                                      idBase + kFacetFirst + facet, idBase + kFacetArrowFirst + facet);
        facetButtons.push_back(button);
    }

    const int levelY = y + 36;
    int countX = x + 8;
    if (catalogue.levelCount)
    {
        const int allWidth = 54;
        const int levelWidth = 28;
        const int levelHeight = 22;
        auto createLevel = [&](int left, int width, int id, LPCSTR text) {
            auto *label = H3DlgText::Create(left, levelY, width, levelHeight, text, NH3Dlg::Text::SMALL,
                                            eTextColor::REGULAR, id, eTextAlignment::MIDDLE_CENTER);
            AddItem(label);
            if (label)
                label->SetHints(helpdlg::Text("help.ui.level_hint"), nullptr, false);
            levelTexts.push_back(label);
            AddDecorationFrame(dialog, decorations, left, levelY, width, levelHeight, H3RGB565(H3RGB888(116, 100, 70)));
            auto *outer = AddDecorationFrame(dialog, decorations, left - 1, levelY - 1, width + 2, levelHeight + 2,
                                             H3RGB565(H3RGB888::Highlight()));
            auto *inner = AddDecorationFrame(dialog, decorations, left, levelY, width, levelHeight,
                                             H3RGB565(H3RGB888::Highlight()));
            levelSelectionFrames.push_back({outer, inner});
        };
        createLevel(x + 8, allWidth, idBase + kLevelFirst, helpdlg::Text("help.ui.all"));
        for (int level = 0; level < catalogue.levelCount; ++level)
        {
            const int left = x + 8 + allWidth + 4 + level * (levelWidth + 3);
            createLevel(left, levelWidth, idBase + kLevelFirst + level + 1,
                        std::to_string(catalogue.firstLevel + level).c_str());
        }
        countX = x + 8 + allWidth + 4 + catalogue.levelCount * (levelWidth + 3) + 8;
    }
    countText = H3DlgText::Create(countX, levelY, std::max(40, x + width - countX - 8), 22, h3_NullString,
                                  NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
    AddItem(countText);

    const int gridY = levelY + 28;
    LPCSTR defName = nullptr;
    if (!catalogue.entries.empty())
        defName = catalogue.entries.front().def;
    H3DefLoader def(defName ? defName : NH3Dlg::Assets::ARTIFACT_DEF);
    const int portraitWidth = defName && def.Get() ? def->widthDEF : 58;
    const int portraitHeight = defName && def.Get() ? def->heightDEF : 64;
    int layoutPortraitHeight = portraitHeight;
    if (data.page == eHelpPage::CREATURES || data.page == eHelpPage::SPELLS)
    {
        // spells.def is 65 px high while TwCrPort.def is 64 px. Use one shared
        // row stride so row N starts at exactly the same Y on both pages.
        H3DefLoader creatureDef(NH3Dlg::Assets::CREATURE_LARGE);
        H3DefLoader spellDef(NH3Dlg::Assets::SPELLS_DEF);
        if (creatureDef.Get() && creatureDef->heightDEF > 0)
            layoutPortraitHeight = std::max(layoutPortraitHeight, creatureDef->heightDEF);
        if (spellDef.Get() && spellDef->heightDEF > 0)
            layoutPortraitHeight = std::max(layoutPortraitHeight, spellDef->heightDEF);
    }
    const auto grid =
        helpdlg::FitCatalogueGrid(width, y + height - gridY - 6, portraitWidth, layoutPortraitHeight,
                                  data.page == eHelpPage::SECONDARY_SKILLS, static_cast<int>(catalogue.entries.size()));
    const int cellWidth = grid.cellWidth, cellHeight = grid.cellHeight;
    columns = grid.columns;
    rows = grid.rows;
    const bool supportsBans = data.page == eHelpPage::ARTIFACTS || data.page == eHelpPage::SPELLS ||
                              data.page == eHelpPage::HEROES || data.page == eHelpPage::SECONDARY_SKILLS;
    H3DefLoader banDef("tpthchk.def");
    const bool hasBanDef = supportsBans && banDef.Get() && banDef->groups && banDef->groupsCount > 0 &&
                           banDef->groups[0] && banDef->groups[0]->count > 1 && banDef->widthDEF > 0 &&
                           banDef->heightDEF > 0 && banDef->widthDEF <= portraitWidth &&
                           banDef->heightDEF <= portraitHeight;
    for (int row = 0; row < rows; ++row)
        for (int column = 0; column < columns; ++column)
        {
            const int slot = row * columns + column;
            const int left = x + 8 + column * cellWidth;
            const int top = gridY + row * cellHeight;
            const int itemId = idBase + kObjectFirst + slot;
            std::vector<H3DlgDef *> underlays;
            if (data.page == eHelpPage::SPELLS && def.Get())
                for (int school = 0; school < 4; ++school)
                {
                    auto *underlay = helpdlg::CreateSpellSchoolUnderlay(left + (cellWidth - portraitWidth) / 2, top,
                                                                        portraitWidth, portraitHeight, school);
                    underlays.push_back(underlay);
                    if (underlay)
                    {
                        dialog->AddItem(underlay);
                        decorations.push_back(underlay);
                        underlay->HideDeactivate();
                    }
                }
            spellUnderlays.push_back(std::move(underlays));
            H3DlgDef *sprite = defName && def.Get()
                                   ? H3DlgDef::Create(left + (cellWidth - portraitWidth) / 2, top, itemId, defName, 0)
                                   : nullptr;
            H3DlgPcx *pcx =
                !defName ? H3DlgPcx::Create(left + (cellWidth - 58) / 2, top, 58, 64, itemId, nullptr) : nullptr;
            H3DlgDef *fallbackCreature =
                data.page == eHelpPage::SPELLS
                    ? H3DlgDef::Create(left + (cellWidth - 58) / 2, top, idBase + kObjectFirst + rows * columns + slot,
                                       NH3Dlg::Assets::CREATURE_LARGE, 0)
                    : nullptr;
            AddItem(sprite);
            AddItem(pcx);
            AddItem(fallbackCreature);
            defPortraits.push_back(sprite);
            fallbackCreaturePortraits.push_back(fallbackCreature);
            pcxPortraits.push_back(pcx);

            const int labelY = top + layoutPortraitHeight + 2;
            const int labelWidth = cellWidth - 4;
            auto *label =
                H3DlgText::Create(left, labelY, labelWidth, helpdlg::kCatalogueCaptionHeight, h3_NullString,
                                  NH3Dlg::Text::SMALL, eTextColor::REGULAR, itemId, eTextAlignment::MIDDLE_CENTER);
            AddItem(label);
            objectLabels.push_back(label);
            auto *labelFrame = AddDecorationFrame(dialog, decorations, left, labelY, labelWidth,
                                                  helpdlg::kCatalogueCaptionHeight, H3RGB565(H3RGB888(116, 100, 70)));
            objectLabelFrames.push_back(labelFrame);

            auto *selection =
                H3DlgFrame::Create(left, top, cellWidth - 4, cellHeight - 2, -1, H3RGB565(H3RGB888::Highlight()));
            if (selection)
            {
                dialog->AddItem(selection);
                decorations.push_back(selection);
                selection->HideDeactivate();
            }
            selectionFrames.push_back(selection);
            auto *banMarker = hasBanDef
                                  ? H3DlgDef::Create(left + (cellWidth + portraitWidth) / 2 - banDef->widthDEF,
                                                     top + portraitHeight - banDef->heightDEF, -1, "tpthchk.def", 1)
                                  : nullptr;
            if (banMarker)
            {
                dialog->AddItem(banMarker);
                decorations.push_back(banMarker);
                banMarker->HideDeactivate();
            }
            banMarkers.push_back(banMarker);
        }
    contentScroll = H3DlgScrollbar::Create(x + width - 22, gridY, 16, height - (gridY - y) - 6, idBase + kContentScroll,
                                           2, ContentScrollProc, false, 1, true);
    AddItem(contentScroll);
    emptyText = H3DlgText::Create(x + 10, gridY + 16, width - 40, 50, helpdlg::Text("help.ui.no_results"),
                                  NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1);
    AddItem(emptyText);
    Rebuild();
}

CatalogueSection::~CatalogueSection()
{
    helpdlg::ReleaseSelectorText(sortButton);
    for (auto *button : facetButtons)
        helpdlg::ReleaseSelectorText(button);
    for (auto *portrait : pcxPortraits)
        if (portrait)
            portrait->SetPcx(static_cast<H3LoadedPcx *>(nullptr));
    for (auto *portrait : portraitCache)
        if (portrait)
            portrait->Dereference();
    if (visibleSection == this)
        visibleSection = nullptr;
}

int CatalogueSection::SortOptionCount() const noexcept
{
    return catalogue.page == eHelpPage::SECONDARY_SKILLS || catalogue.page == eHelpPage::TOWNS ? 2 : 3;
}
LPCSTR CatalogueSection::SortOptionName(int option) const noexcept
{
    if (option == 0)
        return helpdlg::Text("help.sort.name");
    if (option == 1)
        return helpdlg::Text("help.sort.id");
    return helpdlg::Text(catalogue.page == eHelpPage::HEROES      ? "help.sort.hero"
                         : catalogue.page == eHelpPage::ARTIFACTS ? "help.sort.artifact"
                                                                  : "help.sort.level");
}

void CatalogueSection::SetFilter(const helpdlg::CatalogueFilter &state)
{
    filter = state;
    filter.category =
        helpdlg::Bound(filter.category, 0, std::max(0, static_cast<int>(catalogue.categories.size()) - 1));
    filter.sort = helpdlg::Bound(filter.sort, 0, SortOptionCount() - 1);
    const unsigned validLevels = catalogue.levelCount ? (1u << catalogue.levelCount) - 1u : 0u;
    filter.levels &= validLevels;
    for (size_t facet = 0; facet < catalogue.facets.size(); ++facet)
        filter.facets[facet] =
            helpdlg::Bound(filter.facets[facet], 0, static_cast<int>(catalogue.facets[facet].options.size()) - 1);
    for (size_t facet = catalogue.facets.size(); facet < 4; ++facet)
        filter.facets[facet] = 0;
    searchFocusedId = -1;
    activeSubtype = filter.category;
    categoryFirst = helpdlg::Bound(filter.category - categoryRows + 1, 0,
                                   std::max(0, static_cast<int>(catalogue.categories.size()) - categoryRows));
    Rebuild(false);
}

void CatalogueSection::EnsureCategoryVisible(int category) noexcept
{
    const int maxFirst = std::max(0, static_cast<int>(catalogue.categories.size()) - categoryRows);
    if (category < categoryFirst)
        categoryFirst = category;
    else if (category >= categoryFirst + categoryRows)
        categoryFirst = category - categoryRows + 1;
    categoryFirst = helpdlg::Bound(categoryFirst, 0, maxFirst);
}

void CatalogueSection::SetSubtype(int subtype)
{
    searchFocusedId = -1;
    filter.category = helpdlg::Bound(subtype, 0, std::max(0, static_cast<int>(catalogue.categories.size()) - 1));
    activeSubtype = filter.category;
    // A click on an already visible category must not jump the category list
    // back to zero. Only move the window when the requested category is outside it.
    EnsureCategoryVisible(filter.category);
    Rebuild();
}

void CatalogueSection::Rebuild(bool resetScroll)
{
    filtered.clear();
    for (size_t index = 0; index < catalogue.entries.size(); ++index)
        if (helpdlg::MatchesFilter(catalogue.entries[index].filter, filter))
            filtered.push_back(static_cast<int>(index));
    std::stable_sort(filtered.begin(), filtered.end(), [this](int left, int right) {
        const auto &a = catalogue.entries[left];
        const auto &b = catalogue.entries[right];
        if (filter.sort == 1)
            return a.id < b.id;
        if (filter.sort == 2)
        {
            int aGroup = 0, bGroup = 0;
            if (catalogue.page == eHelpPage::HEROES)
                aGroup = a.filter.facets[0], bGroup = b.filter.facets[0];
            else if (catalogue.page == eHelpPage::ARTIFACTS)
                aGroup = a.filter.facets[0], bGroup = b.filter.facets[0];
            else
                aGroup = a.filter.level, bGroup = b.filter.level;
            if (aGroup != bGroup)
                return aGroup < bGroup;
        }
        const int order = helpdlg::CompareNames(a.name, b.name);
        return order ? order < 0 : a.id < b.id;
    });
    if (resetScroll)
        filter.firstRow = 0;
    RefreshControls();
    RefreshCategories();
    Redraw();
}

void CatalogueSection::RefreshControls()
{
    for (size_t facet = 0; facet < facetButtons.size(); ++facet)
        if (facetButtons[facet])
            facetButtons[facet]->SetText(catalogue.facets[facet].options[filter.facets[facet]]);

    for (size_t index = 0; index < levelSelectionFrames.size(); ++index)
    {
        const bool active = index == 0 ? filter.levels == 0 : (filter.levels & (1u << (index - 1))) != 0;
        for (auto *frame : {levelSelectionFrames[index].first, levelSelectionFrames[index].second})
            if (frame)
                if (active && isVisible)
                    frame->Show()->DeActivate();
                else
                    frame->HideDeactivate();
    }
    if (sortButton)
        sortButton->SetText(SortOptionName(filter.sort));
    if (countText)
        countText->SetText(helpdlg::FormatText("help.formats.catalogue_results", static_cast<int>(filtered.size()),
                                               static_cast<int>(catalogue.entries.size()))
                               .c_str());
}

std::string CatalogueSection::CategoryCaption(int category, int count) const
{
    if (category < 0 || category >= static_cast<int>(catalogue.categories.size()))
        return h3_NullString;
    LPCSTR name = catalogue.categories[category];
    std::string prefix;
    if (category >= 1 && category <= catalogue.factionCategoryCount)
    {
        name = helpdlg::TownName(category - 1);
        prefix = H3String::Format("{~>itpa.def:0:%d} ", (category - 1) * 2 + 2).String();
    }
    else if (catalogue.page == eHelpPage::SPELLS && category >= 1 && category <= 4)
    {
        const int secondarySkill = 14 + category - 1; // Fire, Air, Water, Earth Magic.
        prefix = H3String::Format("{~>SECSK32.def:0:%d} ", secondarySkill * 3 + 5).String();
    }

    const auto caption = helpdlg::FormatText("help.formats.category_count", name, count);
    return prefix.empty() ? caption : "{~text align=left}" + prefix + caption + "}";
}

void CatalogueSection::RefreshCategories()
{
    const int maxFirst = std::max(0, static_cast<int>(catalogue.categories.size()) - categoryRows);
    categoryFirst = helpdlg::UpdateScrollbar(categoryScroll, maxFirst, categoryFirst, isVisible, TRUE);
    for (size_t row = 0; row < categoryButtons.size(); ++row)
    {
        auto *button = categoryButtons[row];
        if (!button)
            continue;
        const int category = categoryFirst + static_cast<int>(row);
        const bool state = filter.category == category;
        if (isVisible && category < static_cast<int>(catalogue.categories.size()))
        {
            auto preview = filter;
            preview.category = category;
            int count = 0;
            for (const auto &entry : catalogue.entries)
                count += helpdlg::MatchesFilter(entry.filter, preview) ? 1 : 0;
            const std::string &caption = CategoryCaption(category, count);
            button->SetText(caption.c_str());
            button->ShowActivate();
            helpdlg::SetCategoryState(button, state);
        }
        else
            button->HideDeactivate();
    }
}

void CatalogueSection::Redraw()
{
    const int totalRows = (static_cast<int>(filtered.size()) + columns - 1) / columns;
    const int overflowRows = std::max(0, totalRows - rows);
    // Each native tick advances exactly one non-fitting content row.
    filter.firstRow = helpdlg::UpdateScrollbar(contentScroll, overflowRows, filter.firstRow, isVisible);
    for (size_t slot = 0; slot < objectLabels.size(); ++slot)
    {
        auto *label = objectLabels[slot];
        auto *labelFrame = objectLabelFrames[slot];
        auto *sprite = defPortraits[slot];
        auto *fallbackCreaturePortrait =
            slot < fallbackCreaturePortraits.size() ? fallbackCreaturePortraits[slot] : nullptr;
        auto *pcx = pcxPortraits[slot];
        auto *selection = selectionFrames[slot];
        const int index = filter.firstRow * columns + static_cast<int>(slot);
        const bool visible = isVisible && index < static_cast<int>(filtered.size());
        if (selection)
        {
            if (visible && catalogue.entries[filtered[index]].id == searchFocusedId)
                selection->Show()->DeActivate();
            else
                selection->HideDeactivate();
        }
        if (labelFrame)
            if (visible)
                labelFrame->Show()->DeActivate();
            else
                labelFrame->HideDeactivate();
        if (visible)
        {
            const int record = filtered[index];
            const auto &entry = catalogue.entries[record];
            const std::string hint =
                helpdlg::FormatText("help.formats.object_hint", helpdlg::ObjectTitle(entry.name, entry.id).c_str(),
                                    helpdlg::Text("help.ui.object_hint"));
            if (label)
            {
                label->SetText(entry.name);
                label->SetHints(hint.c_str(), nullptr, true);
                label->ShowActivate();
            }
            bool spellFrameVisible = false;
            if (sprite)
            {
                sprite->SetHints(hint.c_str(), nullptr, true);
                auto *loaded = sprite->GetDef();
                if (loaded && loaded->groups && loaded->groupsCount > 0 && loaded->groups[0] && entry.frame >= 0 &&
                    entry.frame < loaded->groups[0]->count)
                {
                    sprite->SetFrame(entry.frame);
                    sprite->ShowActivate();
                    spellFrameVisible = true;
                }
                else
                    sprite->HideDeactivate();
            }
            const int fallbackCreature = spellFrameVisible ? -1 : FallbackCreature(static_cast<int>(slot), entry);
            if (fallbackCreaturePortrait)
            {
                auto *loaded = fallbackCreaturePortrait->GetDef();
                const int frame = fallbackCreature + 2;
                if (fallbackCreature >= 0 && loaded && loaded->groups && loaded->groupsCount > 0 && loaded->groups[0] &&
                    frame >= 0 && frame < loaded->groups[0]->count)
                {
                    fallbackCreaturePortrait->SetFrame(frame);
                    fallbackCreaturePortrait->SetHints(hint.c_str(), nullptr, true);
                    fallbackCreaturePortrait->ShowActivate();
                }
                else
                    fallbackCreaturePortrait->HideDeactivate();
            }
            if (pcx)
            {
                pcx->SetHints(hint.c_str(), nullptr, true);
                auto *&cached = portraitCache[record];
                if (!cached && *entry.pcx)
                    cached = H3LoadedPcx::Load(entry.pcx);
                pcx->SetPcx(cached);
                cached ? pcx->ShowActivate() : pcx->HideDeactivate();
            }
        }
        else
        {
            if (label)
                label->HideDeactivate();
            if (sprite)
                sprite->HideDeactivate();
            if (fallbackCreaturePortrait)
                fallbackCreaturePortrait->HideDeactivate();
            if (pcx)
            {
                pcx->SetPcx(static_cast<H3LoadedPcx *>(nullptr));
                pcx->HideDeactivate();
            }
        }
        const int school = visible ? catalogue.entries[filtered[index]].spellSchool : -1;
        for (size_t background = 0; background < spellUnderlays[slot].size(); ++background)
            if (auto *underlay = spellUnderlays[slot][background])
            {
                if (visible && sprite && sprite->IsVisible() && school == static_cast<int>(background))
                    underlay->Show()->DeActivate();
                else
                    underlay->HideDeactivate();
            }
    }
    RefreshBanMarkers();
    if (emptyText)
    {
        if (isVisible && filtered.empty())
            emptyText->Show();
        else
            emptyText->HideDeactivate();
    }
    RedrawDialog();
}

int CatalogueSection::FallbackCreature(int slot, const helpdlg::CatalogueEntry &entry) const noexcept
{
    if (catalogue.page != eHelpPage::SPELLS || entry.id < 70 || entry.id > 80)
        return -1;
    if (slot >= 0 && slot < static_cast<int>(defPortraits.size()))
        if (auto *sprite = defPortraits[slot])
            if (auto *loaded = sprite->GetDef())
                if (loaded->groups && loaded->groupsCount > 0 && loaded->groups[0] && entry.frame >= 0 &&
                    entry.frame < loaded->groups[0]->count)
                    return -1;
    const int creature = helpdlg::CreatureSpellFallback(entry.id);
    return creature >= 0 && creature < static_cast<int>(H3CreatureCount::Get()) ? creature : -1;
}

bool CatalogueSection::RefreshBanMarkers()
{
    bool changed = false;
    for (size_t slot = 0; slot < banMarkers.size(); ++slot)
        if (auto *marker = banMarkers[slot])
        {
            const int index = filter.firstRow * columns + static_cast<int>(slot);
            const bool banned = isVisible && index >= 0 && index < static_cast<int>(filtered.size()) &&
                                helpdlg::IsObjectBanned(catalogue.page, catalogue.entries[filtered[index]].id);
            if (banned != (marker->IsVisible() != FALSE))
            {
                if (banned)
                    marker->Show()->DeActivate();
                else
                    marker->HideDeactivate();
                changed = true;
            }
        }
    return changed;
}

int CatalogueSection::ChooseOption(H3DlgTextPcx *anchor, const std::vector<LPCSTR> &options, int selected) const
{
    if (!anchor || options.empty())
        return selected;
    const int popupWidth = anchor->GetWidth() + helpdlg::kSelectorArrowWidth;
    const int anchorX = anchor->GetAbsoluteX();
    const int anchorY = anchor->GetAbsoluteY();
    const int below = std::max(0, H3GameHeight::Get() - (anchorY + anchor->GetHeight()) - 2);
    const int above = std::max(0, anchorY - 2);
    const bool openBelow = below >= std::min<int>(8, options.size()) * kChoiceRowHeight || below >= above;
    const int available = openBelow ? below : above;
    const int visibleRows =
        helpdlg::Bound(available / kChoiceRowHeight, 1, std::min<int>(10, static_cast<int>(options.size())));
    const int popupHeight = visibleRows * kChoiceRowHeight;
    const int popupX = helpdlg::Bound(anchorX, 0, std::max(0, H3GameWidth::Get() - popupWidth));
    const int popupY = openBelow ? anchorY + anchor->GetHeight() + 2 : anchorY - popupHeight - 2;
    ChoiceListDlg picker(popupX, helpdlg::Bound(popupY, 0, std::max(0, H3GameHeight::Get() - popupHeight)), popupWidth,
                         visibleRows, options, selected);
    picker.Start();
    return picker.Result() >= 0 ? picker.Result() : selected;
}

void CatalogueSection::FocusObject(int objectId, bool openDetails, bool fromSearch)
{
    for (size_t index = 0; index < catalogue.entries.size(); ++index)
        if (catalogue.entries[index].id == objectId)
        {
            filter = helpdlg::CatalogueFilter{};
            filter.sort = helpdlg::kCatalogueIdSort;
            searchFocusedId = fromSearch ? objectId : -1;
            activeSubtype = 0;
            Rebuild();
            const auto position = std::find(filtered.begin(), filtered.end(), static_cast<int>(index));
            if (position != filtered.end())
                filter.firstRow = static_cast<int>(position - filtered.begin()) / columns;
            Redraw();
            if (openDetails)
                helpdlg::ShowObjectDetails(catalogue.entries[index]);
            return;
        }
}

void CatalogueSection::SetVisible(BOOL state) noexcept
{
    DlgPage::SetVisible(state);
    if (state)
    {
        visibleSection = this;
        RefreshControls();
        RefreshCategories();
        Redraw();
    }
    else if (visibleSection == this)
        visibleSection = nullptr;
}

void CatalogueSection::UpdateMousePosition(const H3Msg &msg) noexcept
{
    if (!isVisible || !msg.IsMouseOver())
        return;
    const int mouseX = msg.GetX() - dialog->GetX();
    const int mouseY = msg.GetY() - dialog->GetY();
    const bool insideY = mouseY >= y && mouseY < y + height;
    const int categoryMax = std::max(0, static_cast<int>(catalogue.categories.size()) - categoryRows);
    if (categoryScroll && categoryScroll->IsVisible())
    {
        if (categoryMax > 0 && insideY && mouseX < x)
            categoryScroll->Activate();
        else
            categoryScroll->DeActivate();
    }
    if (contentScroll && contentScroll->IsVisible())
    {
        if (insideY && mouseX >= x && mouseX < x + width)
            contentScroll->Activate();
        else
            contentScroll->DeActivate();
    }
}

BOOL CatalogueSection::ProcessMessage(H3Msg &msg)
{
    if (!isVisible)
        return FALSE;
    if (!msg.IsLeftDown() && !msg.IsLeftClick() && !msg.IsRightClick())
        return FALSE;
    if (RefreshBanMarkers())
        RedrawDialog();
    int item = msg.itemId - idBase;
    if (item == kSortArrow)
        item = kSort;
    else if (item >= kFacetArrowFirst && item < kFacetArrowFirst + static_cast<int>(catalogue.facets.size()))
        item = kFacetFirst + item - kFacetArrowFirst;
    const bool selector =
        item == kSort || (item >= kFacetFirst && item < kFacetFirst + static_cast<int>(catalogue.facets.size()));
    if (selector)
    {
        if (msg.IsLeftDown())
        {
            P_SoundManager->ClickSound();
            if (item == kSort)
            {
                std::vector<LPCSTR> options;
                for (int option = 0; option < SortOptionCount(); ++option)
                    options.push_back(SortOptionName(option));
                filter.sort = ChooseOption(sortButton, options, filter.sort);
            }
            else
            {
                const int facet = item - kFacetFirst;
                filter.facets[facet] =
                    ChooseOption(facetButtons[facet], catalogue.facets[facet].options, filter.facets[facet]);
            }
            Rebuild();
        }
        return TRUE;
    }
    const bool filterControl = item == kResetFilters || (catalogue.levelCount && item >= kLevelFirst &&
                                                         item <= kLevelFirst + catalogue.levelCount);
    if (filterControl && !msg.IsLeftDown())
        return TRUE;
    if (msg.IsLeftDown() && !filterControl)
        return FALSE;
    const int objectSlots = static_cast<int>(objectLabels.size());
    const int fallbackFirst = kObjectFirst + objectSlots;
    int objectSlot = -1;
    if (item >= kObjectFirst && item < kObjectFirst + objectSlots)
        objectSlot = item - kObjectFirst;
    else if (item >= fallbackFirst && item < fallbackFirst + objectSlots)
        objectSlot = item - fallbackFirst;
    if (objectSlot >= 0)
    {
        const int index = filter.firstRow * columns + objectSlot;
        if (index < static_cast<int>(filtered.size()))
        {
            const auto &entry = catalogue.entries[filtered[index]];
            const int fallbackCreature = FallbackCreature(objectSlot, entry);
            if (fallbackCreature >= 0)
            {
                // LMB uses the native creature window; RMB must use exactly the
                // same compact popup as the Creatures catalogue page.
                helpdlg::ShowCreatureDetails(fallbackCreature, msg.IsRightClick() != FALSE);
            }
            else
                helpdlg::ShowObjectDetails(entry, msg.IsRightClick() != FALSE);
        }
        return TRUE;
    }
    if (!msg.IsLeftClick() && !filterControl)
        return FALSE;
    if (item == kResetFilters)
    {
        filter.sort = helpdlg::kCatalogueIdSort;
        filter.levels = 0;
        for (auto &facet : filter.facets)
            facet = 0;
        Rebuild();
        return TRUE;
    }
    if (item >= 0 && item < categoryRows)
    {
        SetSubtype(categoryFirst + item);
        return TRUE;
    }
    if (catalogue.levelCount && item >= kLevelFirst && item <= kLevelFirst + catalogue.levelCount)
    {
        if (item == kLevelFirst)
            filter.levels = 0;
        else
            filter.levels ^= 1u << (item - kLevelFirst - 1);
        Rebuild();
        return TRUE;
    }
    return FALSE;
}

void __fastcall CatalogueSection::CategoryScrollProc(INT32 tick, H3BaseDlg *)
{
    if (visibleSection)
    {
        visibleSection->categoryFirst = tick;
        visibleSection->RefreshCategories();
        visibleSection->RedrawDialog();
    }
}
void __fastcall CatalogueSection::ContentScrollProc(INT32 tick, H3BaseDlg *)
{
    if (visibleSection)
    {
        visibleSection->filter.firstRow = tick;
        visibleSection->Redraw();
    }
}
} // namespace main
