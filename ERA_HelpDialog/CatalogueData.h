#pragma once

#include "HelpUI.h"

namespace helpdlg
{
struct CatalogueEntry
{
    main::eHelpPage page;
    int id = -1;
    std::string name;
    std::string summary;
    std::string description;
    std::string notes;
    std::vector<MasteryRow> masteryRows;
    std::vector<CardRow> stats, cardRows;
    std::string statsLabel;
    bool compactDetails = false;
    std::string pcx;
    LPCSTR def = nullptr;
    int frame = 0;
    int spellSchool = -1;
    FilterRecord filter;
};

struct Facet
{
    std::string name;
    std::vector<std::string> options;
};

struct Catalogue
{
    main::eHelpPage page;
    std::vector<std::string> categories;
    std::vector<Facet> facets;
    int firstLevel = 0;
    int levelCount = 0;
    std::vector<CatalogueEntry> entries;
};

LPCSTR TownName(int town);
Catalogue BuildCatalogue(main::eHelpPage page);
bool TryBuildObjectEntry(main::eHelpPage page, int id, CatalogueEntry &entry);
bool IsObjectBanned(main::eHelpPage page, int id);
H3DlgDef *CreateSpellSchoolUnderlay(int x, int y, int width, int height, int school);
bool ShowObjectDetails(const CatalogueEntry &entry, bool popup = false);
} // namespace helpdlg
