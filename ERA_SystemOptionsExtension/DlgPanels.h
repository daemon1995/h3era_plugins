#pragma once
#include "framework.h"

struct ISetting
{
    static constexpr int WIDTH = 195;
    tagPOINT position;
    struct Value
    {
        const DWORD valuePtr;
        INT32 dlgStart;
        INT32 current;
        const INT32 byDefault;

    } value;
    virtual void SetVisible(const BOOL visible) noexcept
    {
    }

  public:
    ISetting(const tagPOINT position, const Value &value) : position(position), value(value)
    {
        const DWORD currentValue = IntAt(value.valuePtr);
        this->value.dlgStart = currentValue;
        this->value.current = currentValue;
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
    static ISetting *Create(const tagPOINT position, const Value &value, LPCSTR displayedText, H3BaseDlg *dlg) noexcept;
};

struct CheckBoxSetting : public ISetting
{
    static constexpr int TEXT_WIDGET_OFFSET = 24;
    H3DlgDef *checkBoxItem{};
    H3DlgText *nameItem{};
    void Toggle() noexcept
    {
        value.current ^= value.current;
    }
    CheckBoxSetting(const int x, const int y, LPCSTR displayedText) : ISetting({x, y}, {0, 0, 0, 0})
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
    static CheckBoxSetting *Create(const tagPOINT position, const INT32 defaultValue, LPCSTR displayedText,
                                   H3BaseDlg *dlg) noexcept;
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
    const tagPOINT position;
    const DWORD generalStringIndex;
    const DWORD firstItemId;
    const DWORD valuePtr;
    const DWORD *hintsPointer = nullptr;
};

struct SwitchPanel : public ISetting
{
    static constexpr LPCSTR bgPcxPath = "BattleSpeed.pcx";
    H3DlgText *switchText{};
    H3DlgPcx *backgroundPcx{};
    H3DlgDef *switchButtons[10]{};
    SwitchPanelInfo &info;
    virtual ~SwitchPanel()
    {
    }

  public:
    static SwitchPanel *Create(const SwitchPanelInfo &info, H3BaseDlg *dlg) noexcept;
};
struct Switch10XPanel : public ISetting
{

    static constexpr int BUTTONS_COUNT = 10;
    static constexpr int HEIGHT = 60;
    static constexpr LPCSTR bgPcxPath = "BattleSpeed.pcx";
    H3DlgText *switchText{};
    H3DlgPcx *backgroundPcx{};
    H3DlgDef *switchButtons[BUTTONS_COUNT]{};
    const SwitchPanelInfo &info;

  public:
    Switch10XPanel(const SwitchPanelInfo &info) : ISetting(info.position, {info.valuePtr, 0, 0, 0}), info(info)
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
    static Switch10XPanel *Create(const SwitchPanelInfo &info, H3Vector<H3DlgItem *> &itemsVec) noexcept;
};
