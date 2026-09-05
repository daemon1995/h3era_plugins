#include "HeaderPage.h"

#include <algorithm>

namespace main
{

HeaderPage::HeaderPage(const int x, const int y, const int width, const int height, H3Dlg *dialog) : HelpPage(dialog)
{
    constexpr LPCSTR staticNames[6] = {"Creatures", "Artifacts", "Heroes", "Secondary Skills", "Spells", "Towns"};
    constexpr int staticIds[6] = {buttons::CREATURES, buttons::ARTIFACTS, buttons::HEROES,
                                  buttons::SECONDARY_SKILLS, buttons::SPELLS, buttons::TOWNS};
    constexpr LPCSTR utilityNames[4] = {"Mods", "All Hotkeys", "Resize", "Help for Help?"};
    constexpr int utilityIds[4] = {buttons::MODLIST, buttons::HOTKEYS, buttons::RESIZE_DLG, buttons::HELP};
    constexpr LPCSTR utilityDefs[4] = {defName, defName, defName, "RMG_help.def"};

    H3DefLoader def(defName);
    AddFrame(x, y, width, height);
    if (!def.Get())
        return;

    // The utility controls remain independent of the catalogue grid. Keep the
    // two left controls narrow so the six static buttons start earlier.
    const int modsWidth = std::min(136, std::max(112, width / 6));
    const int hotkeysWidth = std::min(112, std::max(96, width / 7));
    const int resizeWidth = 58;
    const int helpWidth = 118;
    const int buttonHeight = std::max(24, height - 8);
    const int positions[4] = {x + 4, x + 4 + modsWidth + 7,
                              x + width - helpWidth - resizeWidth - 12, x + width - helpWidth - 4};
    const int widths[4] = {modsWidth, hotkeysWidth, resizeWidth, helpWidth};
    for (int index = 0; index < 4; ++index)
    {
        utilityButtons[index] = H3DlgCaptionButton::Create(
            positions[index], y + 4, widths[index], buttonHeight, utilityIds[index], utilityDefs[index],
            utilityNames[index], NH3Dlg::Text::SMALL, 0, 0, false, static_cast<eVKey>(0), eTextColor::HIGHLIGHT);
        if (utilityButtons[index])
        {
            utilityButtons[index]->SetClickFrame(1);
            AddItem(utilityButtons[index]);
        }
    }

    const int gridX = positions[1] + widths[1] + 8;
    const int gridRight = positions[2] - 7;
    const int gridWidth = std::max(1, gridRight - gridX);
    const int gridCellWidth = std::max(1, gridWidth / 3);
    const int gridCellHeight = std::max(1, (height - 10) / 2);
    for (int index = 0; index < 6; ++index)
    {
        const int column = index % 3;
        const int row = index / 3;
        buttons[index] = H3DlgCaptionButton::Create(
            gridX + column * gridCellWidth, y + 3 + row * gridCellHeight, gridCellWidth - 3, gridCellHeight - 3,
            staticIds[index], defName, staticNames[index], NH3Dlg::Text::SMALL, 0, 0, false, static_cast<eVKey>(0),
            eTextColor::HIGHLIGHT);
        if (buttons[index])
        {
            buttons[index]->SetClickFrame(1);
            AddItem(buttons[index]);
        }
    }
}

void HeaderPage::SetActiveButton(const int buttonId) noexcept
{
    activeButton = nullptr;
    for (auto *button : buttons)
    {
        if (!button)
            continue;
        const bool active = button->GetID() == buttonId;
        button->SetFrame(active ? 1 : 0);
        if (active)
            activeButton = button;
    }
    for (auto *button : utilityButtons)
    {
        if (button)
            button->SetFrame(button->GetID() == buttonId ? 1 : 0);
    }
    RedrawDialog();
}

} // namespace main
