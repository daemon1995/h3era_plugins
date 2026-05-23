#include "DlgPanels.h"

CheckBoxSetting *CheckBoxSetting::Create(const tagPOINT position, const INT32 defaultValue, LPCSTR displayedText,
                                         H3BaseDlg *dlg) noexcept
{
    return nullptr;
}

Switch10XPanel *Switch10XPanel::Create(const SwitchPanelInfo &panelInfo, H3Vector<H3DlgItem *> &itemsVec) noexcept
{

    Switch10XPanel *panel = new Switch10XPanel(panelInfo);

    if (!panel)
        return panel;

    panel->position = panelInfo.position;

    int itemX = panelInfo.position.x;
    int itemY = panelInfo.position.y;

    // create text field with name of the panel
    constexpr int textFieldWidth = BUTTONS_COUNT * 19;

    panel->switchText =
        H3DlgText::Create(itemX, itemY, textFieldWidth, 24, P_GeneralText->GetText(panelInfo.generalStringIndex),
                          NH3Dlg::Text::MEDIUM, eTextColor::HIGHLIGHT, -1);

    itemsVec += panel->switchText;

    itemY += 20;
    // create background pcx
    panel->backgroundPcx = H3DlgPcx::Create(itemX, itemY, Switch10XPanel::bgPcxPath);
    itemsVec += panel->backgroundPcx;
    itemY += 3;
    itemX += 4;

    for (size_t i = 0; i < BUTTONS_COUNT; i++)
    {
        auto &def = panel->switchButtons[i];
        def = H3DlgDef::Create(itemX, itemY, panelInfo.firstItemId + i, NH3Dlg::Assets::SYSLB_DEF, i);

        def->SendCommand(6, 4);
        itemsVec += def;
        itemX += 19;
    }
    // display current selection
    panel->switchButtons[panel->value.current]->SendCommand(5, 4);

    if (!panelInfo.hintsPointer)
        return panel;

    for (size_t i = 0; i < BUTTONS_COUNT; i++)
    {
        if (auto hint = panelInfo.hintsPointer[i])
            panel->switchButtons[i]->SetHint(ValueAt<LPCSTR>(hint));
    }

    return panel;
}
