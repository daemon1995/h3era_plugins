#pragma once

#include "DlgPage.h"

namespace main
{

class TownCategoriesPage final : public DlgPage
{
  public:
    explicit TownCategoriesPage(H3Dlg *dialog);
};

class TownsPage final : public DlgPage
{
  public:
    explicit TownsPage(H3Dlg *dialog);
};

class TownsSection final : public DlgSection
{
    TownCategoriesPage categoriesPage;
    TownsPage contentPage;

  public:
    explicit TownsSection(H3Dlg *dialog);

    void SetVisible(BOOL state) noexcept override;
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
};

} // namespace main
