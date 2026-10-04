#include "ObjectCard.h"
#include "CardLayout.h"

namespace helpdlg
{
namespace
{
struct IconSize
{
    int width = 0, height = 0;
};
IconSize CardIconSize(const CardIcon &icon)
{
    H3DefLoader def(icon.def.c_str());
    if (!def.Get() || !def->groups || def->groupsCount <= 0 || !def->groups[0] || icon.frame < 0 ||
        icon.frame >= def->groups[0]->count || def->widthDEF <= 0 || def->heightDEF <= 0 || def->widthDEF > 64 ||
        def->heightDEF > 64)
        return {};
    return {def->widthDEF, def->heightDEF};
}
void AddIconRow(H3Dlg &dlg, H3Font *font, const CardRow &row, int x, int y, int width, int height)
{
    const int columns = static_cast<int>(row.icons.size()), cellWidth = width / columns;
    for (int index = 0; index < columns; ++index)
    {
        const auto &icon = row.icons[index];
        const auto size = CardIconSize(icon);
        const int cellX = x + index * cellWidth;
        const int actualWidth = index == columns - 1 ? width - index * cellWidth : cellWidth;
        const int iconWidth = size.height <= height && size.width + 24 <= actualWidth ? size.width : 0;
        if (iconWidth)
            if (auto *item = dlg.CreateDef(cellX, y + (height - size.height) / 2, -1, icon.def.c_str(), icon.frame))
                item->DeActivate();
        const int textX = cellX + (iconWidth ? iconWidth + 6 : 0);
        const int textWidth = actualWidth - (textX - cellX) - 4;
        AddCardCell(dlg, icon.label, textX, y, textWidth, height, CardTextHeight(font, icon.label, textWidth - 18));
    }
}
} // namespace
bool ShowCompactObjectCard(const CatalogueEntry &entry, bool popup)
{
    constexpr int padding = 4;
    const int width = std::min(620, H3GameWidth::Get() - 20);
    const int maxHeight = std::min(560, H3GameHeight::Get() - 20);
    const int left = 18, contentWidth = width - 36, labelWidth = 112;
    const int valueWidth = contentWidth - labelWidth;
    const int footer = popup ? 18 : 58;
    H3BinaryLoader<H3Font> font(NH3Dlg::Text::SMALL);
    H3BinaryLoader<H3Font> titleFont(NH3Dlg::Text::MEDIUM);
    H3BinaryLoader<H3LoadedPcx> pcx(entry.pcx.empty() ? nullptr : H3LoadedPcx::Load(entry.pcx.c_str()));
    H3BinaryLoader<H3LoadedDef> def(entry.def ? H3LoadedDef::Load(entry.def) : nullptr);
    const bool hasPcx =
        pcx.Get() && pcx->width > 0 && pcx->height > 0 && pcx->width <= contentWidth && pcx->height <= maxHeight - 260;
    const bool hasDef = !hasPcx && def.Get() && def->groups && def->groupsCount > 0 && def->groups[0] &&
                        entry.frame >= 0 && entry.frame < def->groups[0]->count && def->widthDEF > 0 &&
                        def->heightDEF > 0 && def->widthDEF <= contentWidth && def->heightDEF <= maxHeight - 260;
    const int pictureHeight = hasPcx ? pcx->height : hasDef ? def->heightDEF : 0;
    const int titleY = 16 + (pictureHeight ? pictureHeight + 6 : 0);
    const int titleHeight = std::max(24, CardTextHeight(titleFont.Get(), entry.name, contentWidth));
    const int summaryHeight = entry.summary.empty() ? 0 : CardTextHeight(font.Get(), entry.summary, contentWidth) + 4;
    const int statsY = titleY + titleHeight + summaryHeight + 8;
    const int statsCount = static_cast<int>(entry.stats.size());
    const int statWidth = statsCount ? contentWidth / statsCount : 0;
    const int statsLabelHeight =
        statsCount && !entry.statsLabel.empty() ? CardTextHeight(font.Get(), entry.statsLabel, contentWidth) + 4 : 0;
    int statsHeaderHeight = 22, statsValueHeight = 22;
    for (const auto &stat : entry.stats)
    {
        statsHeaderHeight = std::max(statsHeaderHeight, CardTextHeight(font.Get(), stat.label, statWidth - 8) + 8);
        statsValueHeight = std::max(statsValueHeight, CardTextHeight(font.Get(), stat.value, statWidth - 8) + 8);
    }
    const int statsHeight = statsCount ? statsLabelHeight + statsHeaderHeight + statsValueHeight + 8 : 0;
    const int tableY = statsY + statsHeight;
    auto rows = VisibleCardRows(entry.cardRows, popup);
    if (!entry.notes.empty())
        rows.push_back({"Notes", entry.notes});
    std::vector<int> naturalHeights, valueHeights, minimumHeights;
    for (const auto &row : rows)
    {
        int textHeight = CardTextHeight(font.Get(), row.value, valueWidth - 2 * padding - 18);
        int minimumHeight = 40;
        if (!row.icons.empty())
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
    const auto heights = FitCardRows(naturalHeights, minimumHeights, maxHeight - tableY - footer);
    int tableHeight = 0;
    for (int height : heights)
        tableHeight += height;
    H3Dlg dlg(width, tableY + tableHeight + footer, -1, -1, false, false);
    if (!AddSafeBackground(dlg))
        return false;
    if (hasPcx)
        dlg.CreatePcx((width - pcx->width) / 2, 16, 10, entry.pcx.c_str());
    else if (hasDef)
        dlg.CreateDef((width - def->widthDEF) / 2, 16, 10, entry.def, entry.frame);
    dlg.CreateText(left, titleY, contentWidth, titleHeight, entry.name.c_str(), NH3Dlg::Text::MEDIUM, eTextColor::GOLD,
                   -1);
    if (!entry.summary.empty())
        dlg.CreateText(left, titleY + titleHeight, contentWidth, summaryHeight, entry.summary.c_str(),
                       NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1);
    const H3RGB565 border(H3RGB888(116, 100, 70));
    if (statsCount)
    {
        if (statsLabelHeight)
            dlg.CreateText(left, statsY, contentWidth, statsLabelHeight, entry.statsLabel.c_str(), NH3Dlg::Text::SMALL,
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
            dlg.CreateText(cellX + 4, gridY + 4, cellWidth - 8, statsHeaderHeight - 8, entry.stats[index].label.c_str(),
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
        const auto &row = rows[index];
        if (index)
            dlg.CreateFrame(left, rowY, contentWidth, 1, -1, border);
        dlg.CreateText(left + padding, rowY + padding, labelWidth - 2 * padding, heights[index] - 2 * padding,
                       row.label.c_str(), NH3Dlg::Text::SMALL, eTextColor::GOLD, -1, eTextAlignment::TOP_LEFT);
        if (row.icons.empty())
            AddCardCell(dlg, row.value, left + labelWidth + padding, rowY + padding, valueWidth - 2 * padding,
                        heights[index] - 2 * padding, valueHeights[index]);
        else
            AddIconRow(dlg, font.Get(), row, left + labelWidth + padding, rowY + padding, valueWidth - 2 * padding,
                       heights[index] - 2 * padding);
        rowY += heights[index];
    }
    ShowDetailsDialog(dlg, popup);
    return true;
}
} // namespace helpdlg
