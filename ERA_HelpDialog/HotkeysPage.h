#pragma once
#include "DlgPage.h"
#include "ModInformation.h"
#include "HelpLogic.h"

namespace main
{
class HotkeysSection final : public DlgSection, private DlgPage
{
    const std::vector<ModInformation *> &mods;
    std::vector<H3DlgCaptionButton *> contexts;
    H3DlgScrollableText *text = nullptr;
    H3String rendered;
    void Rebuild(int focusModId = -1, int focusHotkeyId = -1);

  public:
    HotkeysSection(int categoriesX, int categoriesY, int categoriesWidth, int categoriesHeight, int contentX,
                   int contentY, int contentWidth, int contentHeight, H3Dlg *dialog,
                   const std::vector<ModInformation *> &mods);
    void SetVisible(BOOL state) noexcept override;
    void SetSubtype(int subtype);
    void UpdateMousePosition(const H3Msg &msg) noexcept override;
    BOOL ProcessMessage(H3Msg &msg) override;
    void Redraw() override;
    helpdlg::HotkeyFilter Filter() const;
    void SetFilter(const helpdlg::HotkeyFilter &filter);
    void FocusHotkey(LPCSTR folder, int context, int hotkeyId);
};
} // namespace main

namespace helpdlg
{
bool ShowHotkeyPreview(const HotKey &key, LPCSTR modName);
}
