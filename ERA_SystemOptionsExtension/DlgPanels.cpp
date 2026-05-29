#include "DlgPanels.h"

CaptionButtonSetting *CaptionButtonSetting::Create(const SettingsInfo &info, LPCSTR defName,
                                                   H3Vector<H3DlgItem *> &itemsVec) noexcept

{

    CaptionButtonSetting *setting = new CaptionButtonSetting(info);
    if (!setting)
        return setting;

    int x = info.position.x;
    int y = info.position.y;

    H3LoadedDef *def = H3LoadedDef::Load(defName);

    x = x + ((WIDTH - def->widthDEF) >> 1);

    auto bttn = H3DlgCaptionButton::Create(x, y, info.firstItemId, def->GetName(), info.displayedName,
                                           NH3Dlg::Text::MEDIUM, 0, 0, false, 0, eTextColor::REGULAR);

    bttn->SetClickFrame(1);
    H3RGB565 color(H3RGB888::Highlight());

    H3DlgFrame *frame = H3DlgFrame::Create(bttn, color, -1, 1);

    if (info.hintsPointer && *info.hintsPointer)
    {
        bttn->SetHint(ValueAt<LPCSTR>(*info.hintsPointer));
    }
    itemsVec.Add(frame);
    itemsVec.Add(bttn);
    setting->captionButton = bttn;
    def->Dereference();
    return setting;
}

CheckBoxSetting *CheckBoxSetting::Create(const SettingsInfo &info, H3Vector<H3DlgItem *> &itemsVec) noexcept
{

    CheckBoxSetting *setting = new CheckBoxSetting(info);
    if (!setting)
        return setting;

    int x = info.position.x;
    int y = info.position.y;

    // setting->checkBoxItem = H3DlgDef::Create(x, y, info.displayedName);
    const int frameId = setting->value.current;
    setting->checkBoxItem = H3DlgDef::Create(x + WIDTH - TEXT_WIDGET_OFFSET, y, info.firstItemId,
                                             NH3Dlg::Assets::ON_OFF_CHECKBOX, frameId, frameId);
    if (info.hintsPointer && *info.hintsPointer)
    {
        setting->checkBoxItem->SetHint(ValueAt<LPCSTR>(*info.hintsPointer));
    }

    itemsVec.Add(setting->checkBoxItem);
    // x += TEXT_WIDGET_OFFSET;
    setting->checkBoxText =
        H3DlgText::Create(x, y, WIDTH - TEXT_WIDGET_OFFSET, 24, info.displayedName, NH3Dlg::Text::MEDIUM,
                          eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
    itemsVec.Add(setting->checkBoxText);
    return setting;
}

RadioButtonSetting *RadioButtonSetting::Create(const RadioButtonInfo &info, H3Vector<H3DlgItem *> &itemsVec) noexcept
{
    RadioButtonSetting *setting = new RadioButtonSetting(info);
    if (!setting)
        return setting;

    int itemX = info.position.x;
    int itemY = info.position.y;

    if (auto &text = info.displayedName)
    {
        setting->titleItem = ISetting::CreateTitle(itemX, itemY, text, itemsVec);
    }

    const int frameId = setting->value.current;

    for (size_t i = 0; i < info.size; i++)
    {

		//auto& checkBox = setting->radioButtons[i];
        // setting->checkBoxItem = H3DlgDef::Create(x, y, info.displayedName);
        //auto& checkBox = H3DlgDef::Create(itemX + WIDTH - TEXT_WIDGET_OFFSET, y, info.firstItemId,
        //    NH3Dlg::Assets::ON_OFF_CHECKBOX, frameId, frameId);
        //if (info.hintsPointer && *info.hintsPointer)
        //{
        //    setting->checkBoxItem->SetHint(ValueAt<LPCSTR>(*info.hintsPointer));
        //}

        //itemsVec.Add(setting->checkBoxItem);
        //// x += TEXT_WIDGET_OFFSET;
        //setting->checkBoxText =
        //    H3DlgText::Create(x, y, WIDTH - TEXT_WIDGET_OFFSET, 24, info.displayedName, NH3Dlg::Text::MEDIUM,
        //        eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
        //itemsVec.Add(setting->checkBoxText);
    }


    return setting;
}

SwitchPanel *SwitchPanel::Create(const SwitchPanelInfo &info, H3Vector<H3DlgItem *> &itemsVec) noexcept
{
    SwitchPanel *setting = new SwitchPanel(info);

    if (!setting)
        return setting;

    setting->position = info.position;

    int itemX = info.position.x;
    int itemY = info.position.y;

    if (auto &text = info.displayedName)
    {
        setting->titleItem = ISetting::CreateTitle(itemX, itemY, text, itemsVec);
    }
    // create text field with name of the setting
    const auto size = info.size;

    for (size_t i = 0; i < size; i++)
    {
        auto def = H3DlgDefButton::Create(itemX, itemY, info.firstItemId + i, info.defNamesPtr[i], 0, 1, FALSE, NULL);
        def->SendCommand(6, 4096);
        setting->switchButtons += def;
        itemsVec += def;
        itemX += def->GetWidth();
    }
    setting->switchButtons[setting->value.current + setting->valueOffset]->SendCommand(5, 4096);

    if (!info.hintsPointer)
        return setting;

    for (size_t i = 0; i < size; i++)
    {
        if (auto hint = info.hintsPointer[i])
            setting->switchButtons[i]->SetHint(ValueAt<LPCSTR>(hint));
    }

    return setting;
}

Switch10XPanel *Switch10XPanel::Create(const SettingsInfo &info, H3Vector<H3DlgItem *> &itemsVec) noexcept
{

    Switch10XPanel *setting = new Switch10XPanel(info);

    if (!setting)
        return setting;

    setting->position = info.position;

    int itemX = info.position.x;
    int itemY = info.position.y;

    // create text field with name of the setting
    constexpr int textFieldWidth = BUTTONS_COUNT * 19;

    setting->titleItem = H3DlgText::Create(itemX, itemY, textFieldWidth, 24, info.displayedName, NH3Dlg::Text::MEDIUM,
                                            eTextColor::HIGHLIGHT, -1);

    itemsVec += setting->titleItem;

    itemY += 20;
    // create background pcx
    setting->backgroundPcx = H3DlgPcx::Create(itemX, itemY, Switch10XPanel::bgPcxPath);
    itemsVec += setting->backgroundPcx;
    itemY += 3;
    itemX += 4;

    for (size_t i = 0; i < BUTTONS_COUNT; i++)
    {
        auto &def = setting->switchButtons[i];
        def = H3DlgDef::Create(itemX, itemY, info.firstItemId + i, NH3Dlg::Assets::SYSLB_DEF, i);

        def->SendCommand(6, 4);
        itemsVec += def;
        itemX += 19;
    }
    // display current selection
    setting->switchButtons[setting->value.current]->SendCommand(5, 4);

    if (!info.hintsPointer)
        return setting;

    for (size_t i = 0; i < BUTTONS_COUNT; i++)
    {
        if (auto hint = info.hintsPointer[i])
            setting->switchButtons[i]->SetHint(ValueAt<LPCSTR>(hint));
    }

    return setting;
}
