#pragma once

#include "HelpUI.h"

namespace helpdlg
{
struct CatalogueEntry
{
    main::eHelpPage page = main::eHelpPage::MODS;
    int id = -1;
    LPCSTR name = h3_NullString;
    std::string summary;
    std::string description;
    LPCSTR notes = h3_NullString;
    std::vector<MasteryRow> masteryRows;
    std::vector<CardRow> stats, cardRows;
    LPCSTR statsLabel = h3_NullString;
    bool compactDetails = false;
    bool detailsLoaded = false;
    LPCSTR pcx = h3_NullString;
    LPCSTR def = nullptr;
    int frame = 0;
    int spellSchool = -1;
    int faction = -1;
    FilterRecord filter;
};

struct Facet
{
    LPCSTR name = h3_NullString;
    std::vector<LPCSTR> options;
};

struct Catalogue
{
    main::eHelpPage page;
    std::vector<LPCSTR> categories;
    int factionCategoryCount = 0;
    std::vector<Facet> facets;
    int firstLevel = 0;
    int levelCount = 0;
    std::vector<CatalogueEntry> entries;
};

LPCSTR TownName(int town);
Catalogue BuildCatalogue(main::eHelpPage page);
bool TryBuildObjectEntry(main::eHelpPage page, int id, CatalogueEntry &entry, bool details = true);
bool IsObjectBanned(main::eHelpPage page, int id);
int CreatureSpellFallback(int spellId) noexcept;
bool ShowCreatureDetails(int creatureId, bool popup = false);
H3DlgDef *CreateSpellSchoolUnderlay(int x, int y, int width, int height, int school);
bool ShowObjectDetails(const CatalogueEntry &entry, bool popup = false);
} // namespace helpdlg
