#pragma once
#include <functional>

#include "framework.h"

struct SettingsInfo
{
    LPCSTR uuid;
    tagPOINT position;
    int firstItemId;
    int *const valuePtr;
    LPCSTR displayedName = nullptr;
    const DWORD *hintsPointer = nullptr;
    BOOL isBlocked = FALSE;
};

struct ISetting
{
    static constexpr int WIDTH = 195;
    static constexpr int TITLE_HEIGHT = 24;
    std::string uuid;
    tagPOINT position;
    struct Value
    {
        int *const valuePtr = nullptr;
        INT32 dlgStart = 0;
        INT32 current = 0;
        const INT32 isBlocked = 0;

    } value;
    LPCSTR displayedName = nullptr;
    H3DlgText *titleItem = nullptr;
    int firstClickableItemId = -1;
    int lastClickableItemId = -1;

  protected:
    using OnChangeCallback = std::function<void(ISetting *sender)>;
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
    static H3DlgText *CreateTitle(int x, int &y, LPCSTR displayedText, H3Vector<H3DlgItem *> &itemsVec) noexcept
    {
        auto titleItem = H3DlgText::Create(x, y, WIDTH, TITLE_HEIGHT, EraJS::read(displayedText), NH3Dlg::Text::MEDIUM,
                                           eTextColor::HIGHLIGHT, -1);
        itemsVec += titleItem;
        y += TITLE_HEIGHT;
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
    virtual BOOL ProcessMessage(H3Msg &msg) noexcept
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
    H3DlgText *checkBoxText{};

  public:
    CheckBoxSetting(const SettingsInfo &info) : ISetting(info.position, {info.valuePtr, 0, 0, info.isBlocked})
    {
        firstClickableItemId = info.firstItemId;
        lastClickableItemId = info.firstItemId;
        ClampValue();
    }
    virtual ~CheckBoxSetting() {};

  public:
    virtual void ClampValue() noexcept override
    {
        value.current = Clamp(0, value.current, 1);
    }
    virtual void SetVisible(const BOOL visible) noexcept override
    {
        if (visible && value.isBlocked)
        {
            auto &it = checkBoxItem;
            it->SendCommand(5, 4096);
            it->SendCommand(6, 2);
        }
    }
    virtual BOOL ProcessMessage(H3Msg &msg) noexcept override
    {
        if (msg.IsLeftDown() && msg.itemId == checkBoxItem->GetID() && !value.isBlocked)
        {
            Toggle();
            return TRUE;
        }
        return FALSE;
    }

  public:
    void Toggle() noexcept
    {
        value.current ^= 1; // value.current;
        ClampValue();
        checkBoxItem->SetFrame(value.current);
        *value.valuePtr = value.current;
        P_SoundManager->ClickSound();
        checkBoxItem->Draw();
        checkBoxItem->Refresh();
    }
    static CheckBoxSetting *Create(const SettingsInfo &info, H3Vector<H3DlgItem *> &itemsVec,
                                   H3LoadedPcx16 *background) noexcept;
};
struct RadioButtonInfo
{
    tagPOINT position;
    const int firstItemId;
    LPCSTR displayedName = nullptr;
    int *const valuePtr;
    const UINT size;
    const LPCSTR *textPtrs = nullptr;
    const DWORD *hintsPointer = nullptr;
    const BOOL canBeDisabled = FALSE;
};
struct RadioButtonSetting : public ISetting
{
    static constexpr int CHECKBOX_WIDTH = 32;
    static constexpr int CHECKBOX_HEIGHT = 24;

    H3Vector<H3DlgDef *> checkBoxes;
    H3DlgText *checkBoxText{};

  public:
    RadioButtonSetting(const RadioButtonInfo &info) : ISetting(info.position, {info.valuePtr, 0, 0, 0})
    {
        firstClickableItemId = info.firstItemId;
        lastClickableItemId = info.firstItemId + info.size - 1;
    }
    virtual ~RadioButtonSetting() {};

  public:
    virtual BOOL ProcessMessage(H3Msg &msg) noexcept override
    {
        return 0;
    }
    static RadioButtonSetting *Create(const RadioButtonInfo &info, H3Vector<H3DlgItem *> &itemsVec,
                                      H3LoadedPcx16 *background) noexcept;
};

struct SwitchPanelInfo
{
    tagPOINT position;
    const int firstItemId;
    LPCSTR displayedName;
    int *const valuePtr;
    const int valuesOffset = 0;
    const UINT size;
    const LPCSTR *defNamesPtr = nullptr;
    const DWORD *hintsPointer = nullptr;
};

struct SwitchPanel : public ISetting
{
    static constexpr int HEIGHT = 50;

    H3Vector<H3DlgDefButton *> switchButtons;
    int valueOffset = 0;

  public:
    // const SwitchPanelInfo info;
    SwitchPanel(const SwitchPanelInfo &info)
        : ISetting(info.position, {info.valuePtr, 0, 0, 0}), valueOffset(info.valuesOffset)
    {
        value.dlgStart += valueOffset;
        value.current += valueOffset;
        firstClickableItemId = info.firstItemId;
        lastClickableItemId = info.firstItemId + info.size - 1;
    }
    virtual ~SwitchPanel() {};

  public:
    virtual void ClampValue() noexcept override
    {
        if (const auto size = switchButtons.Size())
        {
            value.current = Clamp(0 - valueOffset, value.current, size - 1 - valueOffset);
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
        switchButtons[value.current]->SendCommand(5, 4096);
    }
    virtual BOOL ProcessMessage(H3Msg &msg) noexcept override
    {
        if (msg.IsLeftClick() && msg.itemId >= firstClickableItemId && msg.itemId <= lastClickableItemId)
        {
            const int buttonIndex = msg.itemId - firstClickableItemId;
            if (value.current + valueOffset == buttonIndex)
            {
                return TRUE;
            }

            auto &oldButton = switchButtons[value.current];
            oldButton->SendCommand(6, 4096);
            oldButton->Draw();
            oldButton->Refresh();
            //  ClampValue();
            value.current = buttonIndex - valueOffset;

            auto &newButton = switchButtons[value.current];
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
    static constexpr LPCSTR bgPcxPath = "BattleSpeed.pcx";
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
