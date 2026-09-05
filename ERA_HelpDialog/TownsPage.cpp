#include "TownsPage.h"

namespace main
{

TownCategoriesPage::TownCategoriesPage(H3Dlg *dialog) : HelpPage(dialog)
{
}

TownsPage::TownsPage(H3Dlg *dialog) : HelpPage(dialog)
{
}

TownsSection::TownsSection(H3Dlg *dialog) : categoriesPage(dialog), contentPage(dialog)
{
}

void TownsSection::SetVisible(const BOOL state) noexcept
{
    categoriesPage.SetVisible(state);
    contentPage.SetVisible(state);
}

BOOL TownsSection::ProcessMessage(H3Msg &msg)
{
    return FALSE;
}

void TownsSection::Redraw()
{
}

} // namespace main
