#include "ObjectCard.h"
#include "CardLayout.h"
#include "ScrollbarUtils.h"

namespace helpdlg
{
namespace
{
class ObjectCardDialog : public H3Dlg
{
public:
    std::vector<int> artifacts;
    std::vector<H3DlgPcx16 *> captions;
    std::vector<H3DlgPcx16 *> standaloneCaptions;
    std::vector<H3DlgDef *> componentSprites;
    std::vector<H3DlgPcx *> componentBackgrounds;
    int componentBase = 0;
    std::vector<CardIcon> components;
    H3DlgScrollbar *componentScroll = nullptr;
    int componentColumns = 1, componentFirst = 0, componentRows = 0;
    virtual ~ObjectCardDialog()
    {
        for (auto *caption : captions) ReleaseObjectCaption(caption);
        for (auto *caption : standaloneCaptions) ReleaseObjectCaption(caption);
    }
    void RefreshComponents(int first)
    {
        const int totalRows = (static_cast<int>(components.size()) + componentColumns - 1) / componentColumns;
        componentFirst = UpdateScrollbar(componentScroll, std::max(0, totalRows - componentRows), first, TRUE);
        for (size_t slot = 0; slot < componentSprites.size(); ++slot)
        {
            const int index = componentFirst * componentColumns + static_cast<int>(slot);
            if (index < static_cast<int>(components.size()))
            {
                artifacts[componentBase + slot] = components[index].artifactId;
                if (componentBackgrounds[slot]) componentBackgrounds[slot]->Show()->DeActivate();
                componentSprites[slot]->SetFrame(components[index].frame);
                componentSprites[slot]->ShowActivate();
                SetObjectCaptionText(*this, captions[slot], components[index].label.c_str());
                captions[slot]->ShowActivate();
            }
            else
            {
                artifacts[componentBase + slot] = -1;
                if (componentBackgrounds[slot]) componentBackgrounds[slot]->HideDeactivate();
                componentSprites[slot]->HideDeactivate();
                captions[slot]->HideDeactivate();
            }
        }
        if (P_WindowManager->lastDlg == this) Redraw();
    }
    static void __fastcall ComponentScrollProc(INT32 tick, H3BaseDlg *dialog)
    {
        static_cast<ObjectCardDialog *>(dialog)->RefreshComponents(tick);
    }
    BOOL OnMouseWheel(INT direction) override
    {
        if (!componentScroll) return FALSE;
        RefreshComponents(componentFirst + (direction > 0 ? -1 : 1));
        return TRUE;
    }
    ObjectCardDialog(int width, int height) : H3Dlg(width, height, -1, -1, false, false) {}
    BOOL OnRightClick(H3DlgItem *item) override
    {
        const int index = item ? static_cast<int>(item->GetID()) - 1000 : -1;
        if (index >= 0 && index < static_cast<int>(artifacts.size()) && artifacts[index] >= 0)
        {
            CatalogueEntry entry;
            if (TryBuildObjectEntry(main::eHelpPage::ARTIFACTS, artifacts[index], entry))
                ShowObjectDetails(entry, true);
            return TRUE;
        }
        return FALSE;
    }
};
struct IconSize
{
    int width = 0, height = 0;
};
IconSize CardIconSize(const CardIcon &icon)
{
    H3DefLoader def(icon.def);
    if (!def.Get() || !def->groups || def->groupsCount <= 0 || !def->groups[0] || icon.frame < 0 ||
        icon.frame >= def->groups[0]->count || def->widthDEF <= 0 || def->heightDEF <= 0 || def->widthDEF > 64 ||
        def->heightDEF > 64)
        return {};
    return {def->widthDEF, def->heightDEF};
}
void AddIconRow(ObjectCardDialog &dlg, H3Font *font, const CardRow &row, int x, int y, int width, int height)
{
    const int columns = static_cast<int>(row.icons.size()), cellWidth = width / columns;
    for (int index = 0; index < columns; ++index)
    {
        const auto &icon = row.icons[index];
        const auto size = CardIconSize(icon);
        const int cellX = x + index * cellWidth;
        const int actualWidth = index == columns - 1 ? width - index * cellWidth : cellWidth;
        const int iconWidth = size.height <= height && size.width + 24 <= actualWidth ? size.width : 0;
        int itemId = -1;
        if (icon.artifactId >= 0)
        {
            itemId = 1000 + static_cast<int>(dlg.artifacts.size());
            dlg.artifacts.push_back(icon.artifactId);
        }
        if (iconWidth)
            if (auto *item = dlg.CreateDef(cellX, y + (height - size.height) / 2, itemId, icon.def, icon.frame))
                if (itemId < 0) item->DeActivate();
        const int textX = cellX + (iconWidth ? iconWidth + 6 : 0);
        const int textWidth = actualWidth - (textX - cellX) - 4;
        if (itemId >= 0)
            dlg.CreateText(textX, y, textWidth, height, icon.label.c_str(), NH3Dlg::Text::SMALL,
                           eTextColor::REGULAR, itemId, eTextAlignment::MIDDLE_LEFT);
        else
            AddCardCell(dlg, icon.label, textX, y, textWidth, height, CardTextHeight(font, icon.label, textWidth - 18));
    }
}
void AddArtifactCard(ObjectCardDialog &dlg, const CardIcon &icon, int x, int y)
{
    const auto size = CardIconSize(icon);
    const int portraitWidth = std::max(size.width, kArtifactSlotSize);
    const int portraitHeight = std::max(size.height, kArtifactSlotSize);
    const int cellWidth = std::max(96, portraitWidth + 12);
    if (auto *background = dlg.CreatePcx(x + (cellWidth - kArtifactSlotSize) / 2,
                                         y + (portraitHeight - kArtifactSlotSize) / 2, -1, kArtifactSlotPcx))
        background->DeActivate();
    const int id = 1000 + static_cast<int>(dlg.artifacts.size());
    dlg.artifacts.push_back(icon.artifactId);
    if (size.width && size.height)
        dlg.CreateDef(x + (cellWidth - size.width) / 2, y + (portraitHeight - size.height) / 2,
                      id, icon.def, icon.frame);
    if (auto *caption = CreateObjectCaption(x, y + portraitHeight + 2, cellWidth - 4, kCatalogueCaptionHeight, id))
    {
        dlg.AddItem(caption);
        dlg.standaloneCaptions.push_back(caption);
        SetObjectCaptionText(dlg, caption, icon.label.c_str());
    }
}
bool AddComponentGrid(ObjectCardDialog &dlg, const CardRow &row, int x, int y, int width, int height)
{
    H3DefLoader def(NH3Dlg::Assets::ARTIFACT_DEF);
    if (!def.Get() || def->widthDEF <= 0 || def->heightDEF <= 0) return false;
    const int portraitWidth = std::max(def->widthDEF, kArtifactSlotSize);
    const int portraitHeight = std::max(def->heightDEF, kArtifactSlotSize);
    const int cellWidth = std::max(96, portraitWidth + 12);
    const int cellHeight = portraitHeight + kCatalogueCaptionHeight + 6;
    const int columns = std::max(1, (width - 18) / cellWidth);
    const int rows = std::max(1, height / cellHeight);
    dlg.components = row.icons;
    dlg.componentColumns = columns;
    dlg.componentRows = rows;
    dlg.componentBase = static_cast<int>(dlg.artifacts.size());
    for (int slot = 0; slot < rows * columns; ++slot)
    {
        const int left = x + (slot % columns) * cellWidth;
        const int top = y + (slot / columns) * cellHeight;
        auto *background = dlg.CreatePcx(left + (cellWidth - kArtifactSlotSize) / 2,
                                         top + (portraitHeight - kArtifactSlotSize) / 2, -1, kArtifactSlotPcx);
        if (background) background->DeActivate();
        dlg.componentBackgrounds.push_back(background);
        const int id = 1000 + static_cast<int>(dlg.artifacts.size());
        dlg.artifacts.push_back(-1);
        auto *sprite = dlg.CreateDef(left + (cellWidth - def->widthDEF) / 2,
                                     top + (portraitHeight - def->heightDEF) / 2, id, NH3Dlg::Assets::ARTIFACT_DEF, 0);
        if (!sprite) return false;
        auto *caption = CreateObjectCaption(left, top + portraitHeight + 2, cellWidth - 4, kCatalogueCaptionHeight, id);
        if (!caption) return false;
        dlg.AddItem(caption);
        dlg.componentSprites.push_back(sprite);
        dlg.captions.push_back(caption);
    }
    const int totalRows = (static_cast<int>(row.icons.size()) + columns - 1) / columns;
    if (totalRows > rows)
    {
        dlg.componentScroll = H3DlgScrollbar::Create(x + width - 16, y, 16, rows * cellHeight, 999,
                                                      totalRows - rows + 1, ObjectCardDialog::ComponentScrollProc,
                                                      false, 1, true);
        if (!dlg.componentScroll) return false;
        dlg.AddItem(dlg.componentScroll);
    }
    dlg.RefreshComponents(0);
    return true;
}
} // namespace
bool ShowCompactObjectCard(const CatalogueEntry &entry, bool popup)
{
    constexpr int padding = 4;
    const std::string title = ObjectTitle(entry.name, entry.id);
    const int width = std::min(620, H3GameWidth::Get() - 20);
    const int maxHeight = std::min(560, H3GameHeight::Get() - 20);
    const int left = 18, contentWidth = width - 36;
    // At the standard 620-pixel width, reserve 5 x 96 pixels plus the scrollbar.
    const int labelWidth = entry.page == main::eHelpPage::ARTIFACTS ? 78 : 107;
    const int valueWidth = contentWidth - labelWidth;
    const int footer = popup ? 18 : 58;
    H3BinaryLoader<H3Font> font(NH3Dlg::Text::SMALL);
    H3BinaryLoader<H3Font> titleFont(NH3Dlg::Text::MEDIUM);
    H3BinaryLoader<H3LoadedPcx> pcx(!*entry.pcx ? nullptr : H3LoadedPcx::Load(entry.pcx));
    H3BinaryLoader<H3LoadedDef> def(entry.def ? H3LoadedDef::Load(entry.def) : nullptr);
    const bool hasPcx =
        pcx.Get() && pcx->width > 0 && pcx->height > 0 && pcx->width <= contentWidth && pcx->height <= maxHeight - 260;
    const bool hasDef = !hasPcx && def.Get() && def->groups && def->groupsCount > 0 && def->groups[0] &&
                        entry.frame >= 0 && entry.frame < def->groups[0]->count && def->widthDEF > 0 &&
                        def->heightDEF > 0 && def->widthDEF <= contentWidth && def->heightDEF <= maxHeight - 260;
    const bool artifactBackground = hasDef && entry.page == main::eHelpPage::ARTIFACTS;
    const int pictureHeight = hasPcx   ? pcx->height
                              : hasDef ? std::max(def->heightDEF, artifactBackground ? kArtifactSlotSize : 0)
                                       : 0;
    const int titleY = 16 + (pictureHeight ? pictureHeight + 6 : 0);
    const int titleHeight = std::max(24, CardTextHeight(titleFont.Get(), title, contentWidth));
    const int summaryHeight = entry.summary.empty() ? 0 : CardTextHeight(font.Get(), entry.summary, contentWidth) + 4;
    const int statsY = titleY + titleHeight + summaryHeight + 8;
    const int statsCount = static_cast<int>(entry.stats.size());
    const int statWidth = statsCount ? contentWidth / statsCount : 0;
    const int statsLabelHeight =
        statsCount && *entry.statsLabel ? CardTextHeight(font.Get(), entry.statsLabel, contentWidth) + 4 : 0;
    int statsHeaderHeight = 22, statsValueHeight = 22;
    for (const auto &stat : entry.stats)
    {
        statsHeaderHeight = std::max(statsHeaderHeight, CardTextHeight(font.Get(), stat.label, statWidth - 8) + 8);
        statsValueHeight = std::max(statsValueHeight, CardTextHeight(font.Get(), stat.value, statWidth - 8) + 8);
    }
    const int statsHeight = statsCount ? statsLabelHeight + statsHeaderHeight + statsValueHeight + 8 : 0;
    const int tableY = statsY + statsHeight;
    auto rows = VisibleCardRows(entry.cardRows, popup);
    const CardRow notes{Text("help.ui.notes"), entry.notes};
    if (*entry.notes)
        rows.push_back(&notes);
    std::vector<int> naturalHeights, valueHeights, minimumHeights;
    for (const auto *rowPtr : rows)
    {
        const auto &row = *rowPtr;
        int textHeight = CardTextHeight(font.Get(), row.value, valueWidth - 2 * padding - 18);
        int minimumHeight = 40;
        const bool componentGrid = row.componentGrid && !row.icons.empty();
        if (row.artifactCard && !row.icons.empty())
        {
            const auto size = CardIconSize(row.icons[0]);
            textHeight = std::max(size.height, kArtifactSlotSize) + kCatalogueCaptionHeight + 6;
            minimumHeight = textHeight + 2 * padding;
        }
        else if (componentGrid)
        {
            const auto size = CardIconSize(row.icons[0]);
            const int cellWidth = std::max(96, std::max(size.width, kArtifactSlotSize) + 12);
            const int columns = std::max(1, (valueWidth - 2 * padding - 18) / cellWidth);
            const int cellHeight = std::max(size.height, kArtifactSlotSize) + kCatalogueCaptionHeight + 6;
            const int gridRows = (static_cast<int>(row.icons.size()) + columns - 1) / columns;
            textHeight = gridRows * cellHeight;
            minimumHeight = cellHeight + 2 * padding;
        }
        else if (!row.icons.empty())
        {
            textHeight = 0;
            const int cellWidth = (valueWidth - 2 * padding) / static_cast<int>(row.icons.size());
            for (const auto &icon : row.icons)
            {
                const auto size = CardIconSize(icon);
                const int labelHeight = CardTextHeight(font.Get(), icon.label, cellWidth - size.width - 6 - 4 - 18);
                textHeight = std::max(textHeight, std::max(size.height, labelHeight));
                minimumHeight = std::max(minimumHeight, size.height + 2 * padding);
            }
        }
        valueHeights.push_back(textHeight);
        minimumHeights.push_back(minimumHeight);
        naturalHeights.push_back(std::max(
            28, std::max(textHeight, CardTextHeight(font.Get(), row.label, labelWidth - 2 * padding)) + 2 * padding));
    }
    const int available = std::max(0, maxHeight - tableY - footer);
    std::vector<int> reserved(rows.size(), 0), flexibleNatural = naturalHeights, flexibleMinimum = minimumHeights;
    int reservedTotal = 0;
    for (size_t index = 0; index < rows.size(); ++index)
        if ((rows[index]->componentGrid || rows[index]->artifactCard) && !rows[index]->icons.empty())
        {
            reserved[index] = minimumHeights[index];
            reservedTotal += reserved[index];
            flexibleNatural[index] = std::max(0, naturalHeights[index] - reserved[index]);
            flexibleMinimum[index] = 0;
        }
    // If even one complete card per row cannot fit, use scrollable text
    // instead of drawing controls outside the dialog.
    const bool cardsFit = reservedTotal <= available;
    auto heights = cardsFit ? FitCardRows(flexibleNatural, flexibleMinimum, available - reservedTotal)
                            : FitCardRows(naturalHeights, minimumHeights, available);
    if (cardsFit)
        for (size_t index = 0; index < heights.size(); ++index)
            heights[index] += reserved[index];
    int tableHeight = 0;
    for (int height : heights)
        tableHeight += height;
    ObjectCardDialog dlg(width, tableY + tableHeight + footer);
    if (!AddSafeBackground(dlg))
        return false;
    if (hasPcx)
        dlg.CreatePcx((width - pcx->width) / 2, 16, 10, entry.pcx);
    else if (hasDef)
    {
        if (artifactBackground)
            if (auto *background = dlg.CreatePcx((width - kArtifactSlotSize) / 2,
                                                 16 + (pictureHeight - kArtifactSlotSize) / 2, -1, kArtifactSlotPcx))
                background->DeActivate();
        dlg.CreateDef((width - def->widthDEF) / 2, 16 + (pictureHeight - def->heightDEF) / 2, 10, entry.def, entry.frame);
    }
    dlg.CreateText(left, titleY, contentWidth, titleHeight, title.c_str(), NH3Dlg::Text::MEDIUM, eTextColor::GOLD, -1);
    if (!entry.summary.empty())
        dlg.CreateText(left, titleY + titleHeight, contentWidth, summaryHeight, entry.summary.c_str(),
                       NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1);
    const H3RGB565 border(H3RGB888(116, 100, 70));
    if (statsCount)
    {
        if (statsLabelHeight)
            dlg.CreateText(left, statsY, contentWidth, statsLabelHeight, entry.statsLabel, NH3Dlg::Text::SMALL,
                           eTextColor::GOLD, -1, eTextAlignment::MIDDLE_LEFT);
        const int gridY = statsY + statsLabelHeight;
        const int gridHeight = statsHeaderHeight + statsValueHeight;
        dlg.CreateFrame(left, gridY, contentWidth, gridHeight, -1, border);
        dlg.CreateFrame(left, gridY + statsHeaderHeight, contentWidth, 1, -1, border);
        for (int index = 0; index < statsCount; ++index)
        {
            const int cellX = left + index * statWidth;
            const int cellWidth = index == statsCount - 1 ? contentWidth - index * statWidth : statWidth;
            if (index)
                dlg.CreateFrame(cellX, gridY, 1, gridHeight, -1, border);
            dlg.CreateText(cellX + 4, gridY + 4, cellWidth - 8, statsHeaderHeight - 8, entry.stats[index].label,
                           NH3Dlg::Text::SMALL, eTextColor::GOLD, -1);
            dlg.CreateText(cellX + 4, gridY + statsHeaderHeight + 4, cellWidth - 8, statsValueHeight - 8,
                           entry.stats[index].value.c_str(), NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1);
        }
    }
    if (!rows.empty())
    {
        dlg.CreateFrame(left, tableY, contentWidth, tableHeight, -1, border);
        dlg.CreateFrame(left + labelWidth, tableY, 1, tableHeight, -1, border);
    }
    int rowY = tableY;
    for (size_t index = 0; index < rows.size(); ++index)
    {
        const auto &row = *rows[index];
        if (index)
            dlg.CreateFrame(left, rowY, contentWidth, 1, -1, border);
        dlg.CreateText(left + padding, rowY + padding, labelWidth - 2 * padding, heights[index] - 2 * padding,
                       row.label, NH3Dlg::Text::SMALL, eTextColor::GOLD, -1, eTextAlignment::TOP_LEFT);
        if (row.icons.empty())
            AddCardCell(dlg, row.value, left + labelWidth + padding, rowY + padding, valueWidth - 2 * padding,
                        heights[index] - 2 * padding, valueHeights[index]);
        else if (!cardsFit && (row.artifactCard || row.componentGrid))
        {
            std::string names;
            for (const auto &icon : row.icons)
                names += (names.empty() ? "" : "\n") + icon.label;
            AddCardCell(dlg, names, left + labelWidth + padding, rowY + padding, valueWidth - 2 * padding,
                        heights[index] - 2 * padding, CardTextHeight(font.Get(), names, valueWidth - 2 * padding - 18));
        }
        else if (row.artifactCard)
            AddArtifactCard(dlg, row.icons[0], left + labelWidth + padding, rowY + padding);
        else if (row.componentGrid)
        {
            if (!AddComponentGrid(dlg, row, left + labelWidth + padding, rowY + padding, valueWidth - 2 * padding,
                                  heights[index] - 2 * padding))
                return false;
        }
        else
            AddIconRow(dlg, font.Get(), row, left + labelWidth + padding, rowY + padding, valueWidth - 2 * padding,
                       heights[index] - 2 * padding);
        rowY += heights[index];
    }
    ShowDetailsDialog(dlg, popup);
    return true;
}
} // namespace helpdlg
