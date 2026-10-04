#include "CataloguePage.h"
#include "ScrollbarUtils.h"

namespace main
{
CatalogueSection *CatalogueSection::visibleSection = nullptr;
namespace
{
constexpr int kCategoryScroll = 40;
constexpr int kContentScroll = 41;
constexpr int kQuery = 42;
constexpr int kReset = 43;
constexpr int kSort = 44;
constexpr int kFacetFirst = 50;
constexpr int kLevelFirst = 60;
constexpr int kObjectFirst = 100;
const eHelpPage kCataloguePages[] = {eHelpPage::CREATURES, eHelpPage::ARTIFACTS,        eHelpPage::HEROES,
                                     eHelpPage::SPELLS,    eHelpPage::SECONDARY_SKILLS, eHelpPage::TOWNS};
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

    categoryRows = std::min(std::max(1, (height - 12) / 28), static_cast<int>(catalogue.categories.size()));
    for (int row = 0; row < categoryRows; ++row)
    {
        auto *button =
            helpdlg::Button(nullptr, categoriesX + 6, y + 6 + row * 28, categoriesWidth - 32, 26, idBase + row, "");
        AddItem(button);
        categoryButtons.push_back(button);
    }
    categoryScroll = H3DlgScrollbar::Create(categoriesX + categoriesWidth - 22, y + 6, 16, categoryRows * 28,
                                            idBase + kCategoryScroll, 2, CategoryScrollProc, false, 1, true);
    AddItem(categoryScroll);

    AddItem(H3DlgText::Create(x + 8, y + 6, 48, 22, helpdlg::Text("help.ui.filter", "Filter:"), NH3Dlg::Text::SMALL,
                              eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT));
    queryEdit = H3DlgEdit::Create(x + 58, y + 6, width - 304, 22, 128, "", NH3Dlg::Text::SMALL, eTextColor::REGULAR,
                                  eTextAlignment::MIDDLE_LEFT, nullptr, idBase + kQuery, true, 2, 2);
    AddItem(queryEdit);
    if (queryEdit)
        queryEdit->SetHints(helpdlg::Text("help.ui.filter_hint", "Filter titles only."), nullptr, false);
    sortButton = helpdlg::Button(nullptr, x + width - 238, y + 6, 126, 22, idBase + kSort, "");
    AddItem(sortButton);
    AddItem(helpdlg::Button(nullptr, x + width - 108, y + 6, 100, 22, idBase + kReset,
                            helpdlg::Text("help.ui.reset_filters", "Reset filters")));

    const int facetColumns = std::max(1, std::min(static_cast<int>(catalogue.facets.size()), (width - 12) / 154));
    const int facetWidth = (width - 16) / facetColumns;
    const int facetRows = (static_cast<int>(catalogue.facets.size()) + facetColumns - 1) / facetColumns;
    for (size_t facet = 0; facet < catalogue.facets.size(); ++facet)
    {
        auto *button = helpdlg::Button(nullptr, x + 8 + (static_cast<int>(facet) % facetColumns) * facetWidth,
                                       y + 30 + (static_cast<int>(facet) / facetColumns) * 24, facetWidth - 2, 22,
                                       idBase + kFacetFirst + static_cast<int>(facet), "");
        AddItem(button);
        if (button)
            button->SetHints(helpdlg::Text("help.ui.filter_cycle_hint", "Left: next; right: previous."), nullptr,
                             false);
        facetButtons.push_back(button);
    }
    const int levelY = y + 30 + facetRows * 24;
    constexpr int levelWidth = 28;
    if (catalogue.levelCount)
    {
        AddItem(helpdlg::Button(nullptr, x + 8, levelY, 70, 22, idBase + kLevelFirst,
                                helpdlg::Text("help.ui.all_levels", "All levels")));
        for (int level = 0; level < catalogue.levelCount; ++level)
        {
            auto *button =
                helpdlg::Button(nullptr, x + 82 + level * (levelWidth + 2), levelY, levelWidth, 22,
                                idBase + kLevelFirst + level + 1, std::to_string(catalogue.firstLevel + level).c_str());
            if (button)
                button->SetHints(helpdlg::Text("help.ui.level_hint", "Click to toggle a level."), nullptr, false);
            AddItem(button);
            levelButtons.push_back(button);
        }
    }
    // Share the level row with the result count instead of reserving another row.
    const int countX = catalogue.levelCount ? 90 + catalogue.levelCount * (levelWidth + 2) : 8;
    countText = H3DlgText::Create(x + countX, levelY, width - countX - 8, 22, "", NH3Dlg::Text::SMALL,
                                  eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
    AddItem(countText);
    const int gridY = levelY + 26;
    LPCSTR defName = nullptr;
    if (!catalogue.entries.empty())
        defName = catalogue.entries.front().def;
    H3DefLoader def(defName ? defName : NH3Dlg::Assets::ARTIFACT_DEF);
    const int portraitWidth = defName && def.Get() ? def->widthDEF : 58;
    const int portraitHeight = defName && def.Get() ? def->heightDEF : 64;
    const auto grid = helpdlg::FitCatalogueGrid(width, y + height - gridY - 6, portraitWidth, portraitHeight,
                                                data.page == eHelpPage::SECONDARY_SKILLS);
    const int cellWidth = grid.cellWidth, cellHeight = grid.cellHeight;
    columns = grid.columns;
    rows = grid.rows;
    const bool supportsBans = data.page == eHelpPage::ARTIFACTS || data.page == eHelpPage::SPELLS ||
                              data.page == eHelpPage::HEROES || data.page == eHelpPage::SECONDARY_SKILLS;
    H3DefLoader banDef("tpthchk.def");
    const bool hasBanDef = supportsBans && banDef.Get() && banDef->groups && banDef->groupsCount > 0 &&
                           banDef->groups[0] && banDef->groups[0]->count > 2 && banDef->widthDEF > 0 &&
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
            AddItem(sprite);
            AddItem(pcx);
            defPortraits.push_back(sprite);
            pcxPortraits.push_back(pcx);
            auto *button = helpdlg::Button(nullptr, left, top + portraitHeight + 2, cellWidth - 4, 30, itemId, "");
            AddItem(button);
            objectButtons.push_back(button);
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
                                                     top + portraitHeight - banDef->heightDEF, -1, "tpthchk.def", 2)
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
    emptyText =
        H3DlgText::Create(x + 10, gridY + 16, width - 40, 50, helpdlg::Text("help.ui.no_results", "No matches."),
                          NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1);
    AddItem(emptyText);
    Rebuild();
}

CatalogueSection::~CatalogueSection()
{
    for (auto *portrait : pcxPortraits)
        if (portrait)
            portrait->SetPcx(static_cast<H3LoadedPcx *>(nullptr));
    for (auto *portrait : portraitCache)
        if (portrait)
            portrait->Dereference();
    if (visibleSection == this)
        visibleSection = nullptr;
}

void CatalogueSection::SetFilter(const helpdlg::CatalogueFilter &state)
{
    filter = state;
    filter.category = helpdlg::Bound(filter.category, 0, static_cast<int>(catalogue.categories.size()) - 1);
    filter.sort = helpdlg::Bound(filter.sort, 0, 2);
    const unsigned validLevels = catalogue.levelCount ? (1u << catalogue.levelCount) - 1u : 0u;
    filter.levels &= validLevels;
    for (size_t facet = 0; facet < catalogue.facets.size(); ++facet)
        filter.facets[facet] =
            helpdlg::Bound(filter.facets[facet], 0, static_cast<int>(catalogue.facets[facet].options.size()) - 1);
    if (queryEdit)
        queryEdit->SetText(filter.query.c_str());
    activeSubtype = filter.category;
    categoryFirst = helpdlg::Bound(filter.category - categoryRows + 1, 0,
                                   static_cast<int>(catalogue.categories.size()) - categoryRows);
    Rebuild(false);
}

void CatalogueSection::SetSubtype(int subtype)
{
    filter.category = helpdlg::Bound(subtype, 0, static_cast<int>(catalogue.categories.size()) - 1);
    activeSubtype = filter.category;
    categoryFirst = helpdlg::Bound(filter.category - categoryRows + 1, 0,
                                   static_cast<int>(catalogue.categories.size()) - categoryRows);
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
        if (filter.sort == 2 && a.filter.level != b.filter.level)
            return a.filter.level < b.filter.level;
        const std::string aName = helpdlg::Lower(a.name), bName = helpdlg::Lower(b.name);
        return aName == bName ? a.id < b.id : aName < bName;
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
            facetButtons[facet]->SetText(
                (catalogue.facets[facet].name + ": " + catalogue.facets[facet].options[filter.facets[facet]]).c_str());
    for (size_t level = 0; level < levelButtons.size(); ++level)
        if (levelButtons[level])
            levelButtons[level]->SetFrame(filter.levels & (1u << level) ? 1 : 0);
    if (catalogue.levelCount)
        if (auto *all = static_cast<H3DlgCaptionButton *>(dialog->GetH3DlgItem(idBase + kLevelFirst)))
            all->SetFrame(filter.levels ? 0 : 1);
    const LPCSTR sortNames[] = {"Sort: name", "Sort: ID", "Sort: level / name"};
    if (sortButton)
        sortButton->SetText(sortNames[filter.sort]);
    if (countText)
        countText->SetText(H3String::Format("%d / %d %s", static_cast<int>(filtered.size()),
                                            static_cast<int>(catalogue.entries.size()),
                                            helpdlg::Text("help.ui.results", "results"))
                               .String());
}

void CatalogueSection::RefreshCategories()
{
    categoryFirst = helpdlg::UpdateScrollbar(
        categoryScroll, static_cast<int>(catalogue.categories.size()) - categoryRows, categoryFirst, isVisible);
    for (size_t row = 0; row < categoryButtons.size(); ++row)
    {
        auto *button = categoryButtons[row];
        if (!button)
            continue;
        const int category = categoryFirst + static_cast<int>(row);
        if (isVisible && category < static_cast<int>(catalogue.categories.size()))
        {
            auto preview = filter;
            preview.category = category;
            int count = 0;
            for (const auto &entry : catalogue.entries)
                count += helpdlg::MatchesFilter(entry.filter, preview) ? 1 : 0;
            button->SetText((catalogue.categories[category] + " (" + std::to_string(count) + ")").c_str());
            button->SetFrame(filter.category == category ? 1 : 0);
            button->ShowActivate();
        }
        else
            button->HideDeactivate();
    }
}

void CatalogueSection::Redraw()
{
    const int totalRows = (static_cast<int>(filtered.size()) + columns - 1) / columns;
    filter.firstRow = helpdlg::UpdateScrollbar(contentScroll, totalRows - rows, filter.firstRow, isVisible);
    for (size_t slot = 0; slot < objectButtons.size(); ++slot)
    {
        auto *button = objectButtons[slot];
        auto *sprite = defPortraits[slot];
        auto *pcx = pcxPortraits[slot];
        auto *selection = selectionFrames[slot];
        const int index = filter.firstRow * columns + static_cast<int>(slot);
        const bool visible = isVisible && index < static_cast<int>(filtered.size());
        if (selection)
        {
            if (visible && catalogue.entries[filtered[index]].id == filter.selectedId)
                selection->Show();
            else
                selection->HideDeactivate();
        }
        if (visible)
        {
            const int record = filtered[index];
            const auto &entry = catalogue.entries[record];
            if (button)
            {
                button->SetText(entry.name.c_str());
                button->SetFrame(entry.id == filter.selectedId ? 1 : 0);
                const std::string hint =
                    entry.name + " - " +
                    helpdlg::Text("help.ui.object_hint", "Left: open details. Right: hold for preview.");
                button->SetHints(hint.c_str(), nullptr, true);
                button->ShowActivate();
            }
            if (sprite)
            {
                sprite->SetHints(
                    button ? button->GetHint()
                           : helpdlg::Text("help.ui.object_hint", "Left: open details. Right: hold for preview."),
                    nullptr, true);
                auto *def = sprite->GetDef();
                if (def && def->groups && def->groupsCount > 0 && def->groups[0] && entry.frame >= 0 &&
                    entry.frame < def->groups[0]->count)
                {
                    sprite->SetFrame(entry.frame);
                    sprite->ShowActivate();
                }
                else
                    sprite->HideDeactivate();
            }
            if (pcx)
            {
                pcx->SetHints(
                    button ? button->GetHint()
                           : helpdlg::Text("help.ui.object_hint", "Left: open details. Right: hold for preview."),
                    nullptr, true);
                auto *&cached = portraitCache[record];
                if (!cached && !entry.pcx.empty())
                    cached = H3LoadedPcx::Load(entry.pcx.c_str());
                pcx->SetPcx(cached);
                if (cached)
                    pcx->ShowActivate();
                else
                    pcx->HideDeactivate();
            }
        }
        else
        {
            if (button)
                button->HideDeactivate();
            if (sprite)
                sprite->HideDeactivate();
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
                    underlay->Show();
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
                banned ? marker->Show() : marker->HideDeactivate();
                changed = true;
            }
        }
    return changed;
}

void CatalogueSection::FocusObject(int objectId, bool openDetails)
{
    for (size_t index = 0; index < catalogue.entries.size(); ++index)
        if (catalogue.entries[index].id == objectId)
        {
            filter = helpdlg::CatalogueFilter{};
            filter.selectedId = objectId;
            if (queryEdit)
                queryEdit->SetText("");
            activeSubtype = 0;
            Rebuild();
            const auto position = std::find(filtered.begin(), filtered.end(), static_cast<int>(index));
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
        RefreshCategories();
        Redraw();
    }
    else if (visibleSection == this)
        visibleSection = nullptr;
    if (!state && queryEdit && queryEdit->IsFocused())
        queryEdit->SetFocus(FALSE);
}

void CatalogueSection::UpdateMousePosition(const H3Msg &msg) noexcept
{
    if (!isVisible || !msg.IsMouseOver())
        return;
    // Native wheel routing uses active scrollbars. Activate only the panel
    // under the pointer; the content scrollbar must not steal category input.
    const int mouseX = msg.GetX() - dialog->GetX();
    const int mouseY = msg.GetY() - dialog->GetY();
    const bool insideY = mouseY >= y && mouseY < y + height;
    if (categoryScroll && categoryScroll->IsVisible())
    {
        if (insideY && mouseX < x)
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
    // Re-read live state while this page is open, including script changes.
    if (RefreshBanMarkers())
        RedrawDialog();
    const std::string query = queryEdit ? helpdlg::Safe(queryEdit->GetText()) : "";
    if (query != filter.query)
    {
        filter.query = query;
        Rebuild();
    }
    if (!msg.IsLeftClick() && !msg.IsRightClick())
        return FALSE;
    const int item = msg.itemId - idBase;
    if (item >= kObjectFirst && item < kObjectFirst + static_cast<int>(objectButtons.size()))
    {
        const int index = filter.firstRow * columns + item - kObjectFirst;
        if (index < static_cast<int>(filtered.size()))
        {
            const auto &entry = catalogue.entries[filtered[index]];
            if (!msg.IsRightClick())
            {
                filter.selectedId = entry.id;
                Redraw();
            }
            helpdlg::ShowObjectDetails(entry, msg.IsRightClick() != FALSE);
        }
        return TRUE;
    }
    if (item >= kFacetFirst && item < kFacetFirst + static_cast<int>(catalogue.facets.size()))
    {
        const int facet = item - kFacetFirst;
        filter.facets[facet] = helpdlg::CycleOption(
            filter.facets[facet], static_cast<int>(catalogue.facets[facet].options.size()), msg.IsRightClick());
        Rebuild();
        return TRUE;
    }
    if (!msg.IsLeftClick())
        return FALSE;
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
    if (item == kSort)
    {
        filter.sort = (filter.sort + 1) % 3;
        Rebuild();
        return TRUE;
    }
    if (item == kReset)
    {
        SetFilter(helpdlg::CatalogueFilter{});
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
void CatalogueSection::ReleaseInputFocus(int keepItemId) noexcept
{
    auto *edit = queryEdit;
    if (edit && edit->IsFocused() && edit->GetID() != keepItemId)
        edit->SetFocus(FALSE);
}
} // namespace main
