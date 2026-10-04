#include "MasteryPopup.h"
#include "CardLayout.h"

namespace helpdlg
{
namespace
{
constexpr int kPadding = 6;
constexpr int kHeaderHeight = 24;
} // namespace

bool ShowMasteryDialog(const CatalogueEntry &entry, bool popup)
{
    const int width = std::min(620, H3GameWidth::Get() - 20);
    const int maxHeight = std::min(560, H3GameHeight::Get() - 20);
    const int tableX = 18, tableWidth = width - 36;
    const bool spell = entry.page == main::eHelpPage::SPELLS;
    const int levelWidth = spell ? 84 : 96, manaWidth = spell ? 48 : 0, strengthWidth = spell ? 110 : 0;
    const int strengthX = tableX + levelWidth + manaWidth;
    const int effectX = strengthX + strengthWidth;
    const int effectWidth = tableWidth - levelWidth - manaWidth - strengthWidth;
    H3BinaryLoader<H3Font> font(NH3Dlg::Text::SMALL);
    H3BinaryLoader<H3Font> titleFont(NH3Dlg::Text::MEDIUM);
    H3DefLoader portrait(entry.def);
    const bool hasPortrait = portrait.Get() && portrait->groups && portrait->groupsCount > 0 && portrait->groups[0] &&
                             entry.frame >= 0 && entry.frame < portrait->groups[0]->count && portrait->widthDEF > 0 &&
                             portrait->heightDEF > 0 && portrait->widthDEF <= tableWidth &&
                             portrait->heightDEF <= maxHeight - 190;
    const int titleY = 16 + (hasPortrait ? portrait->heightDEF + 6 : 0);
    const int titleHeight = std::max(24, CardTextHeight(titleFont.Get(), entry.name, tableWidth));
    const int summaryHeight = entry.summary.empty() ? 0 : CardTextHeight(font.Get(), entry.summary, tableWidth) + 4;
    const int tableY = titleY + titleHeight + summaryHeight + 8;
    std::vector<int> effectHeights, naturalHeights;
    for (const auto &row : entry.masteryRows)
    {
        // Reserve scrollbar width when measuring so exceptional mod text can
        // switch to a scrollable cell without needing another layout pass.
        const int effectHeight = CardTextHeight(font.Get(), row.effect, effectWidth - 2 * kPadding - 18);
        effectHeights.push_back(effectHeight);
        const int strengthHeight = spell ? CardTextHeight(font.Get(), row.strength, strengthWidth - 2 * kPadding) : 0;
        naturalHeights.push_back(std::max(28, std::max(effectHeight, strengthHeight) + 2 * kPadding));
    }
    const int notesTextHeight = entry.notes.empty() ? 0 : CardTextHeight(font.Get(), entry.notes, tableWidth - 18);
    const int notesHeight = entry.notes.empty() ? 0 : std::min(notesTextHeight, 72) + 8;
    const int footer = popup ? 18 : 58;
    const int available = maxHeight - tableY - kHeaderHeight - notesHeight - footer;
    const auto heights = FitMasteryRows(naturalHeights, available, 44);
    int tableHeight = kHeaderHeight;
    for (int height : heights)
        tableHeight += height;
    const int height = tableY + tableHeight + notesHeight + footer;
    H3Dlg dlg(width, height, -1, -1, false, false);
    if (!AddSafeBackground(dlg))
        return false;
    if (hasPortrait)
    {
        const int imageX = (width - portrait->widthDEF) / 2;
        if (auto *underlay =
                CreateSpellSchoolUnderlay(imageX, 16, portrait->widthDEF, portrait->heightDEF, entry.spellSchool))
            dlg.AddItem(underlay);
        dlg.CreateDef(imageX, 16, 10, entry.def, entry.frame);
    }
    dlg.CreateText(tableX, titleY, tableWidth, titleHeight, entry.name.c_str(), NH3Dlg::Text::MEDIUM, eTextColor::GOLD,
                   -1);
    if (!entry.summary.empty())
        dlg.CreateText(tableX, titleY + titleHeight, tableWidth, summaryHeight, entry.summary.c_str(),
                       NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1);
    const H3RGB565 border(H3RGB888(116, 100, 70));
    dlg.CreateFrame(tableX, tableY, tableWidth, tableHeight, -1, border);
    dlg.CreateFrame(tableX + levelWidth, tableY, 1, tableHeight, -1, border);
    if (spell)
    {
        dlg.CreateFrame(strengthX, tableY, 1, tableHeight, -1, border);
        dlg.CreateFrame(effectX, tableY, 1, tableHeight, -1, border);
    }
    dlg.CreateText(tableX + kPadding, tableY + 3, levelWidth - 2 * kPadding, kHeaderHeight - 6,
                   Text("help.ui.mastery", "Mastery"), NH3Dlg::Text::SMALL, eTextColor::GOLD, -1,
                   eTextAlignment::MIDDLE_LEFT);
    if (spell)
    {
        dlg.CreateText(tableX + levelWidth + 2, tableY + 3, manaWidth - 4, kHeaderHeight - 6,
                       Text("help.ui.mana", "Mana"), NH3Dlg::Text::SMALL, eTextColor::GOLD, -1);
        dlg.CreateText(strengthX + kPadding, tableY + 3, strengthWidth - 2 * kPadding, kHeaderHeight - 6,
                       Text("help.ui.strength", "Strength"), NH3Dlg::Text::SMALL, eTextColor::GOLD, -1);
    }
    dlg.CreateText(effectX + kPadding, tableY + 3, effectWidth - 2 * kPadding, kHeaderHeight - 6,
                   Text("help.ui.effect", "Effect"), NH3Dlg::Text::SMALL, eTextColor::GOLD, -1,
                   eTextAlignment::MIDDLE_LEFT);
    int rowY = tableY + kHeaderHeight;
    for (size_t index = 0; index < heights.size(); ++index)
    {
        const auto &row = entry.masteryRows[index];
        const int rowHeight = heights[index];
        dlg.CreateFrame(tableX, rowY, tableWidth, 1, -1, border);
        dlg.CreateText(tableX + kPadding, rowY + kPadding, levelWidth - 2 * kPadding, rowHeight - 2 * kPadding,
                       row.level.c_str(), NH3Dlg::Text::SMALL, eTextColor::GOLD, -1, eTextAlignment::TOP_LEFT);
        if (spell)
        {
            dlg.CreateText(tableX + levelWidth + 2, rowY + kPadding, manaWidth - 4, rowHeight - 2 * kPadding,
                           row.mana.c_str(), NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1, eTextAlignment::TOP_MIDDLE);
            dlg.CreateText(strengthX + kPadding, rowY + kPadding, strengthWidth - 2 * kPadding,
                           rowHeight - 2 * kPadding, row.strength.c_str(), NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1,
                           eTextAlignment::TOP_MIDDLE);
        }
        AddCardCell(dlg, row.effect.empty() ? "-" : row.effect, effectX + kPadding, rowY + kPadding,
                    effectWidth - 2 * kPadding, rowHeight - 2 * kPadding, effectHeights[index]);
        rowY += rowHeight;
    }
    if (!entry.notes.empty())
        AddCardCell(dlg, entry.notes, tableX, rowY + 8, tableWidth, notesHeight - 8, notesTextHeight);
    ShowDetailsDialog(dlg, popup);
    return true;
}
} // namespace helpdlg
