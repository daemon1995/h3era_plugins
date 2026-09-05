#include "ArtifactsPage.h"
#include "ScrollbarUtils.h"

#include <algorithm>

namespace main
{
namespace
{
constexpr LPCSTR kCategoryNames[artifacts::CATEGORY_COUNT] = {"All", "Treasure", "Minor", "Major", "Relics", "Other"};
}

ArtifactsPage *ArtifactsPage::instance = nullptr;

ArtifactCategoriesPage::ArtifactCategoriesPage(const int x, const int y, const int width, const int height,
                                               H3Dlg *dialog)
    : HelpPage(dialog)
{
    AddFrame(x, y, width, height);
    for (int index = 0; index < artifacts::CATEGORY_COUNT; ++index)
    {
        auto *button = H3DlgCaptionButton::Create(x + 4, y + 4 + index * 34, artifacts::CATEGORY_FIRST + index,
                                                  "OVBUTN3.def", kCategoryNames[index], NH3Dlg::Text::SMALL, 0, 0,
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
    SetActiveCategory(artifacts::CATEGORY_ALL);
}

BOOL ArtifactCategoriesPage::IsCategory(const int itemId) const noexcept
{
    return itemId >= artifacts::CATEGORY_FIRST && itemId <= artifacts::CATEGORY_LAST;
}

void ArtifactCategoriesPage::SetActiveCategory(const int itemId) noexcept
{
    if (!IsCategory(itemId))
        return;
    activeCategory = itemId;
    for (int index = 0; index < artifacts::CATEGORY_COUNT; ++index)
    {
        if (buttons[index])
            buttons[index]->SetFrame(buttons[index]->GetID() == activeCategory ? 1 : 0);
    }
    RedrawDialog();
}

ArtifactsPage::ArtifactsPage(const int x, const int y, const int width, const int height, H3Dlg *dialog)
    : HelpPage(dialog), pageX(x), pageY(y), pageWidth(width), pageHeight(height)
{
    instance = this;
    H3DefLoader def(NH3Dlg::Assets::ARTIFACT_DEF);
    if (!def.Get())
        return;

    constexpr int margin = 8;
    constexpr int columnGap = 6;
    constexpr int rowGap = 6;
    constexpr int nameHeight = 22;
    constexpr int scrollBarWidth = 18;
    const int availableWidth = pageWidth - margin * 2 - scrollBarWidth;
    const int availableHeight = pageHeight - margin * 2;
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

    const int gridY = pageY + margin;
    for (int row = 0; row < rows; ++row)
    {
        for (int column = 0; column < columns; ++column)
        {
            const int cellX = pageX + margin + column * (cellWidth + columnGap);
            const int portraitX = cellX + (cellWidth - def->widthDEF) / 2;
            const int portraitY = gridY + row * cellHeight;
            const int itemId = artifacts::ITEM_FIRST + row * columns + column;
            auto *portrait = H3DlgDef::Create(portraitX, portraitY, itemId, NH3Dlg::Assets::ARTIFACT_DEF, 0);
            AddItem(portrait);
            portraits.emplace_back(portrait);
            auto *name = H3DlgText::Create(cellX, portraitY + def->heightDEF, cellWidth, nameHeight, h3_NullString,
                                           NH3Dlg::Text::SMALL, eTextColor::REGULAR, itemId,
                                           eTextAlignment::MIDDLE_CENTER, -1);
            AddItem(name);
            names.emplace_back(name);
        }
    }
    const int initialArtifactRows = (std::max(0, H3ArtifactCount::Get()) + columns - 1) / columns;
    const int initialContentRows = std::max(0, initialArtifactRows - rows);
    if (initialContentRows > 0)
    {
        scrollBar = H3DlgScrollbar::Create(pageX + pageWidth - scrollBarWidth - margin, gridY, 16, availableHeight,
                                            artifacts::CONTENT_SCROLLBAR, initialContentRows + 1, ScrollProc, false, 1,
                                            true);
        AddItem(scrollBar);
    }
    RebuildArtifactList();
}

void ArtifactsPage::CallCustomFunction(const int artifactId, const BOOL) noexcept
{


}

void ArtifactsPage::OnLeftClick(const int artifactId) noexcept
{
    CallCustomFunction(artifactId, FALSE);
}

void ArtifactsPage::OnRightClick(const int artifactId) noexcept
{
    CallCustomFunction(artifactId, TRUE);
}

BOOL ArtifactsPage::ProcessItemMessage(H3Msg &msg)
{
    if (!msg.IsLeftClick() && !msg.IsRightClick())
        return FALSE;
    if (msg.itemId < artifacts::ITEM_FIRST || msg.itemId > artifacts::ITEM_LAST)
        return FALSE;

    const int slot = msg.itemId - artifacts::ITEM_FIRST;
    const int artifactIndex = firstRow * columns + slot;
    if (!isVisible || slot < 0 || slot >= static_cast<int>(portraits.size()) ||
        artifactIndex < 0 || artifactIndex >= static_cast<int>(artifactIds.size()))
        return TRUE;

    if (msg.IsRightClick())
        OnRightClick(artifactIds[artifactIndex]);
    else
        OnLeftClick(artifactIds[artifactIndex]);
    return TRUE;
}

ArtifactsPage::~ArtifactsPage()
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

BOOL ArtifactsPage::MatchesCategory(const int artifactId) const noexcept
{
    const eArtifactType type = H3Artifact(static_cast<eArtifact>(artifactId)).GetType();
    switch (activeCategory)
    {
    case artifacts::CATEGORY_ALL:
        return TRUE;
    case artifacts::CATEGORY_TREASURE:
        return (type & eArtifactType::TREASURE) != 0;
    case artifacts::CATEGORY_MINOR:
        return (type & eArtifactType::MINOR) != 0;
    case artifacts::CATEGORY_MAJOR:
        return (type & eArtifactType::MAJOR) != 0;
    case artifacts::CATEGORY_RELIC:
        return (type & eArtifactType::RELIC) != 0;
    case artifacts::CATEGORY_OTHER:
        return (type & eArtifactType::SPECIAL) != 0 || type == eArtifactType(0);
    default:
        return FALSE;
    }
}

void ArtifactsPage::RebuildArtifactList()
{
    artifactIds.clear();
    const int count = std::max(0, H3ArtifactCount::Get());
    for (int id = 0; id < count; ++id)
    {
        if (MatchesCategory(id))
            artifactIds.emplace_back(id);
    }
    firstRow = 0;
}

void ArtifactsPage::SetCategory(const int categoryId)
{
    if (categoryId < artifacts::CATEGORY_FIRST || categoryId > artifacts::CATEGORY_LAST)
        return;
    activeCategory = categoryId;
    RebuildArtifactList();
    Redraw();
}

void ArtifactsPage::Redraw(const int requestedFirstRow)
{
    const int totalRows = (static_cast<int>(artifactIds.size()) + columns - 1) / columns;
    const int maxFirstRow = std::max(0, totalRows - rows);
    firstRow = helpdlg::UpdateScrollbar(scrollBar, maxFirstRow, requestedFirstRow, isVisible);
    const int firstItem = firstRow * columns;
    for (size_t slot = 0; slot < portraits.size(); ++slot)
    {
        auto *portrait = portraits[slot];
        auto *name = slot < names.size() ? names[slot] : nullptr;
        const int artifactIndex = firstItem + static_cast<int>(slot);
        if (portrait && isVisible && artifactIndex < static_cast<int>(artifactIds.size()))
        {
            const int artifactId = artifactIds[artifactIndex];
            portrait->SetFrame(artifactId);
            portrait->ShowActivate();
            if (name)
            {
                const LPCSTR artifactName = H3Artifact(static_cast<eArtifact>(artifactId)).GetName();
                name->SetText(artifactName ? artifactName : h3_NullString);
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

void __fastcall ArtifactsPage::ScrollProc(INT32 tick, H3BaseDlg *)
{
    if (instance)
        instance->Redraw(tick);
}

ArtifactsSection::ArtifactsSection(const int categoriesX, const int categoriesY, const int categoriesWidth,
                                   const int categoriesHeight, const int contentX, const int contentY,
                                   const int contentWidth, const int contentHeight, H3Dlg *dialog)
    : categoriesPage(categoriesX, categoriesY, categoriesWidth, categoriesHeight, dialog),
      contentPage(contentX, contentY, contentWidth, contentHeight, dialog)
{
    categoriesPage.SetActiveCategory(artifacts::CATEGORY_ALL);
    contentPage.SetCategory(artifacts::CATEGORY_ALL);
}

void ArtifactsSection::SetVisible(const BOOL state) noexcept
{
    categoriesPage.SetVisible(state);
    contentPage.SetVisible(state);
}

void ArtifactsSection::SetSubtype(const int subtype)
{
    activeSubtype = std::max(0, std::min(subtype, artifacts::CATEGORY_COUNT - 1));
    const int category = artifacts::CATEGORY_FIRST + activeSubtype;
    categoriesPage.SetActiveCategory(category);
    contentPage.SetCategory(category);
}

BOOL ArtifactsSection::ProcessMessage(H3Msg &msg)
{
    if (contentPage.ProcessItemMessage(msg))
        return TRUE;
    if (!msg.IsLeftClick() || !categoriesPage.IsCategory(msg.itemId))
        return FALSE;
    activeSubtype = msg.itemId - artifacts::CATEGORY_FIRST;
    categoriesPage.SetActiveCategory(msg.itemId);
    contentPage.SetCategory(msg.itemId);
    return TRUE;
}

void ArtifactsSection::Redraw()
{
    contentPage.Redraw();
}

} // namespace main
