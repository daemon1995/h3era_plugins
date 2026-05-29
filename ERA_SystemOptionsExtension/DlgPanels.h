#pragma once
#include "framework.h"

struct SettingsInfo
{
    LPCSTR uuid;
    tagPOINT position;
    int firstItemId;
    LPCSTR displayedName;
    DWORD valuePtr;
    INT32 defaultValue;
    const DWORD *hintsPointer = nullptr;
};

struct ISetting
{
    static constexpr int WIDTH = 195;
    std::string uuid;

    tagPOINT position;
    struct Value
    {
        DWORD valuePtr = 0;
        INT32 dlgStart = 0;
        INT32 current = 0;
        const INT32 byDefault = 0;

    } value;
    LPCSTR displayedName = nullptr;
    virtual void SetVisible(const BOOL visible) noexcept
    {
    }

  public:
    ISetting(const tagPOINT &position, const Value &value) : position(position), value(value)
    {

        if (value.valuePtr)
        {
            const DWORD currentValue = IntAt(value.valuePtr);
            this->value.dlgStart = currentValue;
            this->value.current = currentValue;
        }

        ClampValue();
    }
    virtual ~ISetting()
    {
    }

  public:
    virtual void ClampValue() noexcept
    {
    }

    void ResetToDefault() noexcept
    {
        value.current = value.byDefault;
    }
    static ISetting *Create(const tagPOINT position, const Value &value, LPCSTR displayedText,
                            H3Vector<H3DlgItem *> &itemsVec) noexcept;
};

struct CaptionButtonSetting : public ISetting
{
    H3DlgCaptionButton *captionButton{};
    H3DlgText *checkBoxText{};
    void(__stdcall *callback)(void) = nullptr;
    void Toggle() noexcept
    {
        if (callback)
            callback();
    }
    CaptionButtonSetting(const SettingsInfo &info)
        : ISetting(info.position, {0, 0, 0, 0}), callback(reinterpret_cast<void(__stdcall *)(void)>(info.valuePtr))
    {
    }
    virtual ~CaptionButtonSetting()
    {
    }

  public:
    static CaptionButtonSetting *Create(const SettingsInfo &info, LPCSTR defName,
                                        H3Vector<H3DlgItem *> &itemsVec) noexcept;
};

struct CheckBoxSetting : public ISetting
{
    static constexpr int TEXT_WIDGET_OFFSET = 36;
    H3DlgDef *checkBoxItem{};
    H3DlgText *checkBoxText{};
    void Toggle() noexcept
    {
        value.current ^= value.current;
        ClampValue();
    }
    CheckBoxSetting(const SettingsInfo &info) : ISetting(info.position, {info.valuePtr, 0, 0, info.defaultValue})
    {
    }
    virtual ~CheckBoxSetting()
    {
    }

  public:
    virtual void ClampValue() noexcept override
    {
        value.current = Clamp(0, value.current, 1);
    }

  public:
    static CheckBoxSetting *Create(const SettingsInfo &info, H3Vector<H3DlgItem *> &itemsVec) noexcept;
};

struct RadioButtonSetting : public ISetting
{
    static constexpr int TEXT_WIDGET_OFFSET = 24;
    H3DlgDef *radioButtonItem{};
    H3DlgText *nameItem{};
    void SetValue(const INT32 newValue) noexcept
    {
        value.current = newValue;
    }
    RadioButtonSetting(const int x, const int y, const Value &value, LPCSTR displayedText) : ISetting({x, y}, value)
    {
    }
    virtual ~RadioButtonSetting()
    {
    }

  public:
    static RadioButtonSetting *Create(const tagPOINT position, const Value &value, LPCSTR displayedText,
                                      H3BaseDlg *dlg) noexcept;
};

struct SwitchPanelInfo
{
    tagPOINT position;
    const int firstItemId;
    LPCSTR displayedName;
    const DWORD valuePtr;
    const int valuesOffset = 0;
    const int size;
    const LPCSTR *defNamesPtr = nullptr;
    const DWORD *hintsPointer = nullptr;
};

struct SwitchPanel : public ISetting
{
    static constexpr int HEIGHT = 50;

    H3DlgText *headerText{};
    H3Vector<H3DlgDefButton *> switchButtons;
    int valueOffset = 0;
    // const SwitchPanelInfo info;
    SwitchPanel(const SwitchPanelInfo &info)
        : ISetting(info.position, {info.valuePtr, 0, 0, 0}), valueOffset(info.valuesOffset)
    {
    }
    virtual ~SwitchPanel()
    {
    }

    virtual void ClampValue() noexcept override
    {

        if (const auto size = switchButtons.Size())
        {
            value.current = Clamp(0, value.current, size - 1);
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
        switchButtons[value.current + valueOffset]->SendCommand(5, 4096);
    }

  public:
    static SwitchPanel *Create(const SwitchPanelInfo &info, H3Vector<H3DlgItem *> &itemsVec) noexcept;
};
struct Switch10XPanel : public ISetting
{

    static constexpr int BUTTONS_COUNT = 10;
    static constexpr int HEIGHT = 60;
    static constexpr LPCSTR bgPcxPath = "BattleSpeed.pcx";
    H3DlgText *headerText{};
    H3DlgPcx *backgroundPcx{};
    H3DlgDef *switchButtons[BUTTONS_COUNT]{};
    const SettingsInfo &info;

  public:
    Switch10XPanel(const SettingsInfo &info) : ISetting(info.position, {info.valuePtr, 0, 0, 0}), info(info)
    {
    }
    virtual ~Switch10XPanel()
    {
    }

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

  public:
    static Switch10XPanel *Create(const SettingsInfo &info, H3Vector<H3DlgItem *> &itemsVec) noexcept;
};
