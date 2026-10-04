#pragma once

#include "pch.h"
#include <unordered_map>
#include <vector>

namespace switcher
{
namespace aoe
{
bool IsAvailable(const H3CombatCreature *stack);
void OpenForActiveStack();
void Install(PatcherInstance *patcher);
void SetEnabled(bool enabled);
} // namespace aoe

enum class eAction : INT
{
    NONE = 0,
    MELEE,
    SHOOT,
    CAST,
    MOVE,
    RETURN,
    NO_RETURN,
    AREA_SHOT
};

class ActionSwitcher;

class SwitcherDlgPanel
{
    friend class ActionSwitcher;

  public:
    static constexpr int BUTTON_ID = 2030;
    static constexpr int MAX_ACTIONS = 7;
    static SwitcherDlgPanel *instance;

  private:
    struct PanelSwitchItem
    {
        eAction action = eAction::NONE;
        H3DlgCustomButton *button = nullptr;
        H3DlgText *number = nullptr;
    };

    H3CombatBottomPanel *bottomPanel = nullptr;
    H3CombatDlg *dialog = nullptr;
    H3DlgPcx16 *background = nullptr;
    H3DlgCustomButton *actionSwitchButton = nullptr;
    std::vector<PanelSwitchItem> switchItems;
    std::vector<eAction> creatureActions;
    // The panel lives for one battle; choices belong to individual stack objects.
    std::unordered_map<const H3CombatCreature *, eAction> rememberedActions;
    H3CombatCreature *currentStack = nullptr;
    INT currentTurn = -1;
    eAction selectedAction = eAction::NONE;
    BOOL visible = false;
    BOOL panelShownByAlt = false;
    BOOL checkingAvailability = false;
    BOOL actionsNeedRefresh = TRUE;
    BOOL buttonNeedsRedraw = FALSE;
    BOOL battlefieldNeedsRedraw = FALSE;
    UINT consumedNumberKeys = 0;
    UINT heldNumberKeys = 0;
    INT pendingButton = -1;
    INT menuWidth = 0;

  public:
    explicit SwitcherDlgPanel(H3CombatBottomPanel *combatBottomPanel,
                              const int firstItemId = BUTTON_ID);
    ~SwitcherDlgPanel();

    eAction GetSelectedAction() const { return checkingAvailability ? eAction::NONE : selectedAction; }

  private:
    void Draw();
    void Refresh();
    BOOL SetVisible(BOOL show);
    void InitNewStackAttackType();
    void UpdateLayout();
    void SelectAction(size_t index, H3CombatManager *manager);
    void SelectAction(eAction action, H3CombatManager *manager);
    BOOL IsEligibleForCurrentPlayer(H3CombatManager *manager) const;
    BOOL CanInteract(H3CombatManager *manager) const;
    void UpdateButtonStates();

    static INT __fastcall SwitchPanelButtonProcedure(H3Msg *msg);
    static void __stdcall H3CombatDlg_DrawBattlefield(HiHook *h, H3CombatDlg *dlg, BOOL redraw);
    static INT __stdcall H3CombatDlg_Procedure(HiHook *h, H3CombatManager *manager, H3Msg *msg);
    static INT __stdcall H3CombatManager_ProcessAction(HiHook *h, H3CombatManager *manager,
                                                      H3Msg *msg, INT argument);
    static _LHF_(H3CombatManager_GetControl);
};

class ActionSwitcher : public IGamePatch
{
    static ActionSwitcher *instance;

  private:
    BOOL state = true;
    SwitcherDlgPanel *switcherDlgPanel = nullptr;
    std::vector<Patch *> interfaceChangePatches;
    HiHook *canShootHook = nullptr;

    ActionSwitcher();
    void CreatePatches() noexcept final;

    static _LHF_(Y_BattleMgr_WOG_HarpyReturn);
    static BOOL8 __stdcall Y_CanShoot(HiHook *h, H3CombatCreature *stack, INT target);
    static BOOL8 __stdcall Y_CanCast(HiHook *h, H3CombatCreature *stack, INT hex);
    static INT __stdcall Y_GetCursorAction(HiHook *h, H3CombatManager *manager, INT hex);
    static H3CombatBottomPanel *__stdcall Y_CreateBottomPanel(HiHook *h, H3CombatBottomPanel *panel,
                                                             H3CombatDlg *dlg, INT argument);
    static void __stdcall Y_DestroyBottomPanel(HiHook *h, H3CombatBottomPanel *panel);
    eAction GetActiveAction(const H3CombatCreature *stack) const;

  public:
    BOOL SetPluginState(bool state) noexcept;
    BOOL IsEnabled() const { return state; }
    static BOOL CanShootNormally(const H3CombatCreature *stack);
    static ActionSwitcher &Get();
};
} // namespace switcher
