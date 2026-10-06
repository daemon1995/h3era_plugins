#pragma once

#include "pch.h"
#include "PluginSettings.h"

class PluginSettingsDlg : public H3Dlg
{
    creatureInfo::PluginSettings draft;

    void AddCheckbox(int id, int y, LPCSTR labelKey, LPCSTR hintKey, bool checked);
    bool IsCheckboxChecked(int id, bool fallback);

  protected:
    BOOL OnLeftClick(INT itemId, H3Msg &msg) override;
    VOID OnOK() override;

  public:
    PluginSettingsDlg();
};
