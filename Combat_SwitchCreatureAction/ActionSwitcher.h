#pragma once
#include "pch.h"
#include <array>

#include <unordered_set>
namespace switcher
{
class ActionSwitcher;

constexpr int COUNT_TYPES = 10;

enum eAction : INT
{
    FIRST,
    NONE = FIRST,
    MELEE,
    SHOOT,
    CAST,
    MOVE,
    RETURN,
    NO_RETURN,
    LAST = NO_RETURN
};
namespace states
{
constexpr int FIRST = 0;
constexpr int STATE_SECOND = 1;
constexpr int STATE_THIRD = 2;
} // namespace states

class SwitcherText : public IPluginText
{

  public:
    struct
    {

    } settings;
    eVKey hotKey;

    char *ButtonHints[COUNT_TYPES];
    char *ButtonRmc[COUNT_TYPES];
    char *AutoBattleQuestion;
    char *ButtonName;

  protected:
    virtual void Load() noexcept override;

  public:
    SwitcherText();
};
class SwitcherDlgPanel
{
    friend class ActionSwitcher;

  private:
    static constexpr LPCSTR BACK_PANEL_NAME = "swpnlbg.pcx";

  public:
    static constexpr int BUTTON_ID = 2030;
    static SwitcherDlgPanel *instance;

  private:
    BOOL inited = false;
    BOOL visible = false;
    BOOL activateByHover = true;
    BOOL activateByKey = true;
    DWORD flags = 0;

    BOOL panelShownByHover = false;
    BOOL panelShownByKey = false;
    struct PanelSwitchItem
    {
        BOOL32 state = false;
        BOOL32 toggled = false;

        H3DlgDefButton *button = nullptr;

      public:
        BOOL SetState(BOOL32 _state);
    };

    H3DlgPcx16 *backDlgPcx16 = nullptr;
    H3DlgCustomButton *actionSwitchButton = nullptr;
    std::vector<PanelSwitchItem> switchItems;
    std::unordered_set<eAction> creeatureActions = {eAction::NONE};

  public:
    SwitcherDlgPanel(H3Vector<H3DlgItem *> *combatBottomPanelItems, const SwitcherText &text,
                     const int firstItemId = BUTTON_ID);
    ~SwitcherDlgPanel();

  private:
    void Draw();
    void Refresh();
    BOOL SetVisible(const BOOL visible = true);
    void InitNewStackAttackType(const BOOL redraw = false);
    void UpdateStackAttackType(const eAction selectedAction = eAction::NONE);
    void Init(H3Vector<H3DlgItem *> *combatBottomPanelItems, const int firstItemId = BUTTON_ID);

  private:
    static INT __fastcall SwitchPanelButtonProcedure(H3Msg *msg);
    static char __stdcall H3CombatDlg_BeforeBaseDlgRedraw(HiHook *h);
    static INT __stdcall H3CombatDlg_Procedure(HiHook *h, H3CombatManager *cmbMgr, H3Msg *msg);
    static INT __stdcall H3CombatDlg_BottoomPanel_HoverProcedure(HiHook *h, H3WindowManager *wndMgr, H3Msg *msg);
    static _LHF_(H3CombatManager_GetControl);
    static _LHF_(H3CombatManager_AfterHeroCastSpell);
};

class ActionSwitcher : public IGamePatch
{

  private:
    BOOL state = true;
    SwitcherDlgPanel *switcherDlgPanel;
    std::vector<Patch *> interfaceChangePatches;
    std::vector<Patch *> behaviorChangePatches;

  private:
    ActionSwitcher();

  protected:
    virtual void CreatePatches() noexcept final;

  private:
    SwitcherText text;

  private:
    static _LHF_(Y_BattleMgr_WOG_HarpyReturn);
    static void __stdcall Y_AddChooseAttackButton(HiHook *h, H3Vector<H3DlgItem *> *combatBottomPanelItems,
                                                  H3DlgItem **end, const int count, H3DlgItem *lastItem);

  public:
    BOOL SetPluginState(const bool state) noexcept;
    static ActionSwitcher &Get(); // (PatcherInstance* _PI);
};
} // namespace switcher
