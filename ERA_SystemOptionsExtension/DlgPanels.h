#pragma once
#include <functional>

#include "framework.h"
void DrawThickFrameOverItem(H3LoadedPcx16 *back, H3DlgItem *item);
H3DlgFrame *CreateThickFrameOverItem(H3DlgItem *item);

struct SettingsInfo
{
    LPCSTR uuid = nullptr;
    tagPOINT position = {};
    int firstItemId = 0;
    int *const valuePtr = 0;
    LPCSTR displayedName = nullptr;
    union {
        LPCSTR rmcHint;
        LPCSTR *rmcHints = nullptr;
    };
    BOOL isBlocked = FALSE;
};
struct RadioButtonInfo
{
    LPCSTR uuid = nullptr;
    tagPOINT position;
    const int firstItemId;
    int *const valuePtr;
    LPCSTR displayedName = nullptr;
    const UINT size;
    LPCSTR *textPtrs = nullptr;
    LPCSTR *rmcHints = nullptr;
    const BOOL requiresSelection = FALSE;
};
struct SwitchPanelInfo
{
    tagPOINT position = {};
    const int firstItemId = 0;
    int *const valuePtr = 0;
    LPCSTR displayedName = nullptr;
    const UINT defsNum = 0;
    LPCSTR *defNamesPtrs = nullptr;
    LPCSTR *rmcHints = nullptr;
    const int valuesOffset = 0;
};

struct ISetting
{
    struct Value
    {
        int *const valuePtr = nullptr;
        INT32 dlgStart = 0;
        INT32 current = 0;
        const BOOL isBlocked = FALSE;
    };
    using OnChangeCallback = std::function<void(ISetting *sender)>;
    static constexpr int WIDTH = 195;
    static constexpr int TITLE_HEIGHT = 24 - 4;
    static constexpr int BASE_SETTINGS_Y_OFFSET = TITLE_HEIGHT + 6 + 4;
    std::string uuid;
    tagPOINT position;

    struct Value value;
    LPCSTR displayedName = nullptr;
    H3DlgItem *titleItem = nullptr;
    int firstClickableItemId = -1;
    int lastClickableItemId = -1;

  protected:
    OnChangeCallback m_onChange = nullptr;

  public:
    ISetting(const tagPOINT &position, const Value &value) : position(position), value(value)
    {
        if (value.valuePtr)
        {
            const int currentValue = *value.valuePtr;
            this->value.dlgStart = currentValue;
            this->value.current = currentValue;
        }
    }
    virtual ~ISetting() {};

  public:
    virtual void ClampValue() noexcept {};
    virtual void SetVisible(const BOOL visible) noexcept {};
    virtual BOOL ProcessMessage(H3Msg &msg) noexcept
    {
        return 0;
    }

  public:
    void SetOnChange(OnChangeCallback cb) noexcept
    {
        m_onChange = cb;
    }

    void TriggerChange() noexcept
    {
        if (m_onChange)
            m_onChange(this);
    }

  public:
    static H3DlgItem *CreateTitle(int x, int &y, LPCSTR displayedText, H3Vector<H3DlgItem *> &itemsVec) noexcept
    {
        auto titleItem = H3DlgText::Create(x, y, WIDTH, TITLE_HEIGHT, EraJS::read(displayedText), NH3Dlg::Text::MEDIUM,
                                           eTextColor::HIGHLIGHT, -1);

        H3LoadedPcx16 *titleBackPcx16 = H3LoadedPcx16::Create(WIDTH, TITLE_HEIGHT);
        H3PcxLoader pcxBack(NH3Dlg::Assets::DIBOXBACK);

        pcxBack->DrawToPcx16(titleBackPcx16, 0, 0, 1);
        H3DlgPcx16 *titleBack = H3DlgPcx16::Create(x, y, nullptr);
        titleBack->SetWidth(WIDTH);
        titleBack->SetHeight(TITLE_HEIGHT);
        const auto color = H3RGB888(0x7A, 0x65, 0x48);

        titleBackPcx16->DarkenArea(0, 0, WIDTH, TITLE_HEIGHT, 25);

        titleBack->SetPcx(titleBackPcx16);
        itemsVec += titleBack;
        itemsVec += CreateThickFrameOverItem(titleItem);
        itemsVec += titleItem;

        y += TITLE_HEIGHT; // +4; // +6;

        return titleItem;
    }
};

struct CaptionButtonSetting : public ISetting
{
    static constexpr LPCSTR SINGLE_BUTTON = "GSPsys0.def";
    H3DlgCaptionButton *captionButton{};

    CaptionButtonSetting(const SettingsInfo &info) : ISetting(info.position, {})
    {
        this->firstClickableItemId = info.firstItemId;
        this->lastClickableItemId = info.firstItemId;
    }
    virtual ~CaptionButtonSetting() {};

  public:
    virtual BOOL ProcessMessage(H3Msg &msg) noexcept override
    {
        if (msg.IsLeftClick() && msg.itemId == firstClickableItemId)
        {
            TriggerChange();
            return TRUE;
        }
        return FALSE;
    }

  public:
    static CaptionButtonSetting *Create(const SettingsInfo &info, H3Vector<H3DlgItem *> &itemsVec,
                                        H3LoadedPcx16 *background) noexcept;
};

struct CheckBoxSetting : public ISetting
{
    static constexpr int CHECKBOX_WIDTH = 32;
    static constexpr int CHECKBOX_HEIGHT = 24;
    H3DlgDef *checkBoxItem{};

  public:
    CheckBoxSetting(const SettingsInfo &info) : ISetting(info.position, {info.valuePtr, 0, 0, info.isBlocked})
    {
        firstClickableItemId = info.firstItemId;
        lastClickableItemId = info.firstItemId;
        ClampValue();
    }
    virtual ~CheckBoxSetting()
    {
        if (!value.isBlocked && value.valuePtr && *value.valuePtr != value.dlgStart)
        {
            TriggerChange();
        }
    }

  public:
    virtual void ClampValue() noexcept override
    {
        value.current = Clamp(0, value.current, 1);
    }
    virtual BOOL ProcessMessage(H3Msg &msg) noexcept override
    {
        if (msg.IsLeftDown() && msg.itemId == firstClickableItemId)
        {
            const int newValue = value.current ^= 1; // value.current;
            ClampValue();
            *value.valuePtr = newValue;
            P_SoundManager->ClickSound();
            SetCheckBoxValue(checkBoxItem, newValue);
            return TRUE;
        }
        return FALSE;
    }

  public:
    static void SetCheckBoxValue(H3DlgDef *checkBox, const BOOL newValue) noexcept
    {
        checkBox->SetFrame(newValue);
        checkBox->Draw();
        checkBox->Refresh();
        // TriggerChange();
    }
    static CheckBoxSetting *Create(const SettingsInfo &info, H3Vector<H3DlgItem *> &itemsVec,
                                   H3LoadedPcx16 *background) noexcept;
};

struct RadioButtonSetting : public ISetting
{
    static constexpr int CHECKBOX_WIDTH = 32;
    static constexpr int CHECKBOX_HEIGHT = 24;
    BOOL requiresSelection = FALSE;

    H3Vector<H3DlgDef *> checkBoxes;
    H3Vector<H3DlgText *> checkBoxTexts;

  public:
    RadioButtonSetting(const RadioButtonInfo &info)
        : ISetting(info.position, {info.valuePtr, 0, 0, 0}), requiresSelection(info.requiresSelection)
    {
        firstClickableItemId = info.firstItemId;
        lastClickableItemId = info.firstItemId + info.size - 1;
    }
    virtual ~RadioButtonSetting() {};

  public:
    virtual void ClampValue() noexcept override
    {
        value.current = Clamp(0, value.current, checkBoxes.Size() - requiresSelection);
    }
    virtual BOOL ProcessMessage(H3Msg &msg) noexcept override
    {

        if (msg.IsLeftDown() && msg.itemId >= firstClickableItemId && msg.itemId <= lastClickableItemId)
        {
            const int valueIndex = msg.itemId - firstClickableItemId;

            if (requiresSelection && valueIndex == value.current)
            {
                return TRUE;
            }

            value.current = valueIndex; // value.current;
            *value.valuePtr = value.current;

            ClampValue();
            const auto size = checkBoxes.Size();
            P_SoundManager->ClickSound();

            for (size_t i = 0; i < size; i++)
            {
                CheckBoxSetting::SetCheckBoxValue(checkBoxes[i], i == valueIndex);
            }
            //  checkBoxes[valueIndex]->SetFrame(1);
            return TRUE;
        }
        return FALSE;
    }

  public:
    static RadioButtonSetting *Create(const RadioButtonInfo &info, H3Vector<H3DlgItem *> &itemsVec,
                                      H3LoadedPcx16 *background) noexcept;
};

struct SwitchPanel : public ISetting
{
    static constexpr int HEIGHT = 50;

    H3Vector<H3DlgDefButton *> switchButtons;
    int valueOffset = 0;

  public:
    SwitchPanel(const SwitchPanelInfo &info)
        : ISetting(info.position, {info.valuePtr, 0, 0, 0}), valueOffset(info.valuesOffset)
    {
        firstClickableItemId = info.firstItemId;
        lastClickableItemId = info.firstItemId + info.defsNum - 1;
    }
    virtual ~SwitchPanel() {};

  public:
    virtual void ClampValue() noexcept override
    {
        if (const auto size = switchButtons.Size())
        {
            // value.current = Clamp(0 - valueOffset, value.current, size - 1 - valueOffset);
        }
    }
    virtual void SetVisible(const BOOL visible) noexcept override
    {
        if (!visible)
            return;
        const auto size = switchButtons.Size();
        for (size_t i = 0; i < size; i++)
        {
            switchButtons[i]->SendCommand(6, 4096);
        }
        switchButtons[value.current - valueOffset]->SendCommand(5, 4096);
    }
    virtual BOOL ProcessMessage(H3Msg &msg) noexcept override
    {
        if (msg.IsLeftClick() && msg.itemId >= firstClickableItemId && msg.itemId <= lastClickableItemId)
        {
            const int buttonIndex = msg.itemId - firstClickableItemId;
            if (value.current - valueOffset == buttonIndex)
            {
                return TRUE;
            }

            auto &oldButton = switchButtons[value.current - valueOffset];
            oldButton->SendCommand(6, 4096);
            oldButton->Draw();
            oldButton->Refresh();
            //  ClampValue();
            value.current = buttonIndex + valueOffset;
            TriggerChange();
            auto &newButton = switchButtons[buttonIndex];
            newButton->SendCommand(5, 4096);
            newButton->Draw();
            newButton->Refresh();

            return TRUE;
        }
        return 0;
    }

  public:
    static SwitchPanel *Create(const SwitchPanelInfo &info, H3Vector<H3DlgItem *> &itemsVec,
                               H3LoadedPcx16 *background) noexcept;
};
struct Switch10XPanel : public ISetting
{
    static constexpr int BUTTONS_COUNT = 10;
    static constexpr int HEIGHT = 60;
    static constexpr LPCSTR bgPcxPath = "GSP10pnl.pcx";
    H3DlgPcx *backgroundPcx{};
    H3DlgDef *switchButtons[BUTTONS_COUNT]{};

  public:
    Switch10XPanel(const SettingsInfo &info) : ISetting(info.position, {info.valuePtr, 0, 0, info.isBlocked})
    {
        firstClickableItemId = info.firstItemId;
        lastClickableItemId = info.firstItemId + BUTTONS_COUNT - 1;
    };
    virtual ~Switch10XPanel() {};

  public:
    virtual void ClampValue() noexcept override
    {
        value.current = Clamp(0, value.current, BUTTONS_COUNT - 1);
    }
    virtual void SetVisible(const BOOL visible) noexcept override
    {
        if (!visible)
            return;

        for (size_t i = 0; i < BUTTONS_COUNT; i++)
        {
            switchButtons[i]->SendCommand(6, 4);
        }
        switchButtons[value.current]->SendCommand(5, 4);
    }
    virtual BOOL ProcessMessage(H3Msg &msg) noexcept override
    {
        if (msg.IsLeftDown() && msg.itemId >= firstClickableItemId && msg.itemId <= lastClickableItemId)
        {
            const int buttonIndex = msg.itemId - firstClickableItemId;
            if (value.current == buttonIndex)
            {
                return TRUE;
            }
            ClampValue();
            value.current = buttonIndex;

            backgroundPcx->Draw();
            backgroundPcx->Refresh();
            switchButtons[value.current]->Draw();
            switchButtons[value.current]->Refresh();
            TriggerChange();
            P_SoundManager->ClickSound();
            return TRUE;
        }
        return FALSE;
    }

  public:
    static Switch10XPanel *Create(const SettingsInfo &info, H3Vector<H3DlgItem *> &itemsVec,
                                  H3LoadedPcx16 *background) noexcept;
};
