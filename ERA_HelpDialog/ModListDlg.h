#pragma once

#include "HelpDialogDependencies.h"
#include "ModInformation.h"
#include "HelpLogic.h"

namespace list
{
class ModListDlg : public H3Dlg
{
    const std::vector<ModInformation *> &mods;
    std::vector<H3DlgCaptionButton *> buttons;
    H3DlgScrollbar *scrollBar = nullptr;
    ModInformation *resultMod = nullptr;
    int firstIndex = 0;
    int selectedIndex = 0;
    int visibleCount = 1;
    static ModListDlg *instance;

  public:
    ModListDlg(const helpdlg::ModDropdownLayout &layout, const std::vector<ModInformation *> &mods,
               const ModInformation *selectedMod);
    virtual ~ModListDlg();

  protected:
    virtual BOOL DialogProc(H3Msg &msg) override;

  private:
    void CreateDlgItems();
    void RedrawItems(int firstIndex);
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);

  public:
    ModInformation *ResultMod() const noexcept;
};

} // namespace list
