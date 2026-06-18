#include "DlgPanels.h"

const H3RGB565 frameColor(0x7A, 0x65, 0x48);

H3DlgFrame *CreateThickFrameOverItem(H3DlgItem *item)
{

    H3DlgFrame *frame = H3DlgFrame::Create(item, frameColor, -1, 1);
    frame->DeActivate();
    return frame;
}
H3DlgFrame *CreateThickFrameAtPosition(const int x, const int y, const int w, const int h)
{
    //    const H3RGB565 frameColor(0x7A, 0x65, 0x48);

    H3DlgFrame *frame = H3DlgFrame::Create(x, y, w, h, -1, frameColor);
    frame->DeActivate();
    return frame;
}
CaptionButtonSetting *CaptionButtonSetting::Create(const SettingsInfo &info, H3Vector<H3DlgItem *> &itemsVec,
                                                   H3LoadedPcx16 *background) noexcept
{

    CaptionButtonSetting *setting = new CaptionButtonSetting(info);
    if (!setting)
        return setting;

    int x = info.position.x;
    int y = info.position.y;

    H3LoadedDef *def = H3LoadedDef::Load(SINGLE_BUTTON);

    x = x + ((WIDTH - def->widthDEF) >> 1);

    auto bttn = H3DlgCaptionButton::Create(x, y, info.firstItemId, def->GetName(), EraJS::read(info.displayedName),
                                           NH3Dlg::Text::MEDIUM, 0, 0, false, 0, eTextColor::REGULAR);
    bttn->SetClickFrame(1);

    if (info.rmcHint)
        bttn->SetRightClickHint(EraJS::read(info.rmcHint));

    itemsVec += CreateThickFrameOverItem(bttn);
    itemsVec += bttn;
    setting->captionButton = bttn;
    def->Dereference();
    return setting;
}

CheckBoxSetting *CheckBoxSetting::Create(const SettingsInfo &info, H3Vector<H3DlgItem *> &itemsVec,
                                         H3LoadedPcx16 *background) noexcept
{

    CheckBoxSetting *setting = new CheckBoxSetting(info);
    if (!setting)
        return setting;

    const int itemX = info.position.x;
    const int itemY = info.position.y;

    // setting->checkBoxItem = H3DlgDef::Create(x, y, info.displayedName);
    const int frameId = setting->value.current;

    auto checkBox = H3DlgDef::Create(itemX + WIDTH - CHECKBOX_WIDTH, itemY, info.firstItemId,
                                     NH3Dlg::Assets::ON_OFF_CHECKBOX, frameId, frameId);
    if (info.rmcHint)
        checkBox->SetRightClickHint(EraJS::read(info.rmcHint));

    if (info.isBlocked)
    {
        checkBox->SendCommand(5, 4096);
        checkBox->SendCommand(6, 2);
    }

    itemsVec += checkBox;
    setting->checkBoxItem = checkBox;
    setting->titleItem =
        H3DlgText::Create(itemX, itemY, WIDTH - CHECKBOX_WIDTH, CHECKBOX_HEIGHT, EraJS::read(info.displayedName),
                          NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
    itemsVec += setting->titleItem;
    return setting;
}

RadioButtonSetting *RadioButtonSetting::Create(const RadioButtonInfo &info, H3Vector<H3DlgItem *> &itemsVec,
                                               H3LoadedPcx16 *background) noexcept
{
    RadioButtonSetting *setting = new RadioButtonSetting(info);
    if (!setting)
        return setting;

    int itemX = info.position.x;
    int itemY = info.position.y;

    if (auto &text = info.displayedName)
        setting->titleItem = ISetting::CreateTitle(itemX, itemY, text, itemsVec);

    // create code for checkboxes
    const int frameId = setting->value.current;

    int itemId = info.firstItemId;
    const UINT size = info.size;
    const int selectedBoxId = setting->value.current - (info.requiresSelection ? 1 : 0);
    for (size_t i = 0; i < size; i++)
    {

        const BOOL8 frameId = i == selectedBoxId;

        auto checkBox = H3DlgDef::Create(itemX + WIDTH - CHECKBOX_WIDTH, itemY, itemId++,
                                         NH3Dlg::Assets::ON_OFF_CHECKBOX, frameId, frameId);
        setting->checkBoxes += checkBox;
        auto text =
            H3DlgText::Create(itemX, itemY, WIDTH - CHECKBOX_WIDTH, CHECKBOX_HEIGHT, EraJS::read(info.textPtrs[i]),
                              NH3Dlg::Text::MEDIUM, eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
        setting->checkBoxTexts += text;

        itemsVec += text;
        itemsVec += checkBox;
        itemY += BASE_SETTINGS_Y_OFFSET;
    }

    if (!info.rmcHints)
        return setting;

    for (size_t i = 0; i < size; i++)
        if (auto hint = info.rmcHints[i])
            setting->checkBoxes[i]->SetRightClickHint(EraJS::read(hint));

    return setting;
}

SwitchPanel *SwitchPanel::Create(const SwitchPanelInfo &info, H3Vector<H3DlgItem *> &itemsVec,
                                 H3LoadedPcx16 *background) noexcept
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
        // itemsVec += CreateThickFrameOverItem(setting->titleItem);

        if (background)
        {
            //  DrawThickFrameOverItem(background, setting->titleItem);
        }
    }

    // create text field with name of the setting
    const auto size = info.defsNum;
    if (!size)
        return setting;
    itemY += 1;
    auto def = H3LoadedDef::Load(info.defNamesPtrs[0]);
    itemsVec += CreateThickFrameAtPosition(itemX - 1, itemY - 1, WIDTH + 2, def->heightDEF + 2);

    const int padding = (WIDTH - size * def->widthDEF) / (size + 1);
    itemX += padding;
    for (size_t i = 0; i < size; i++)
    {
        auto def = H3DlgDefButton::Create(itemX, itemY, info.firstItemId + i, info.defNamesPtrs[i], 0, 1, FALSE, NULL);
        setting->switchButtons += def;
        itemsVec += def;
        itemX += def->GetWidth() + padding;
    }
    def->Dereference();

    setting->switchButtons[setting->value.current - setting->valueOffset]->SendCommand(5, 4096);

    if (!info.rmcHints)
        return setting;

    for (size_t i = 0; i < size; i++)
        if (auto hint = info.rmcHints[i])
            setting->switchButtons[i]->SetRightClickHint(EraJS::read(hint));

    return setting;
}

Switch10XPanel *Switch10XPanel::Create(const SettingsInfo &info, H3Vector<H3DlgItem *> &itemsVec,
                                       H3LoadedPcx16 *background) noexcept
{

    Switch10XPanel *setting = new Switch10XPanel(info);

    if (!setting)
        return setting;

    int itemX = info.position.x;
    int itemY = info.position.y;

    // create text field with name of the setting
    // setting->titleItem = H3DlgText::Create(itemX, itemY, WIDTH, TITLE_HEIGHT, EraJS::read(info.displayedName),
    //                                       NH3Dlg::Text::MEDIUM, eTextColor::HIGHLIGHT, -1);

    // auto frame = CreateThickFrameOverItem(setting->titleItem);
    // itemsVec.Add(frame);

    setting->titleItem = CreateTitle(itemX, itemY, info.displayedName, itemsVec);

    if (background)
    {
        // DrawThickFrameOverItem(background, setting->titleItem);
    }
    // itemsVec += setting->titleItem;

    //  itemY -= 4;
    // create background pcx
    setting->backgroundPcx = H3DlgPcx::Create(itemX, itemY, Switch10XPanel::bgPcxPath);
    if (background)
    {
        // DrawThickFrameOverItem(background, setting->backgroundPcx);
    }
    itemsVec += CreateThickFrameOverItem(setting->backgroundPcx);
    //    CreateThickFrameAtPosition(itemX - 1, itemY - 1, textFieldWidth + 2, setting->backgroundPcx->GetHeight() + 2);
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

    if (!info.rmcHints)
        return setting;

    for (size_t i = 0; i < BUTTONS_COUNT; i++)
        if (auto hint = info.rmcHints[i])
            setting->switchButtons[i]->SetRightClickHint(EraJS::read(hint));

    return setting;
}
