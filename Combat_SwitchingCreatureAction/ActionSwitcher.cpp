#include "ActionSwitcher.h"

namespace switcher
{
namespace
{
constexpr int kButtonX = 658;
constexpr int kButtonY = 4;
constexpr int kButtonWidth = 44;
constexpr int kButtonHeight = 36;
constexpr int kOptionSpacing = 47;
constexpr int kMenuHeight = 54;
constexpr LPCSTR kButtonDef = "icmAltq.def";
INT actionDepth = 0;

const char *ActionHint(eAction action)
{
    switch (action)
    {
    case eAction::MELEE: return "Melee attack (ALT + number).";
    case eAction::SHOOT: return "Ranged attack (ALT + number).";
    case eAction::CAST: return "Creature spell (ALT + number).";
    case eAction::MOVE: return "Move onto an allied corpse without resurrecting (ALT + number).";
    case eAction::RETURN: return "Attack and return (ALT + number).";
    case eAction::NO_RETURN: return "Attack without returning (ALT + number).";
    case eAction::AREA_SHOT: return "Area shot: choose a battlefield hex (ALT + number).";
    default: return "Hold ALT to choose an action for the active creature.";
    }
}

bool IsWarMachine(const H3CombatCreature *stack)
{
    return stack->type >= eCreature::CATAPULT && stack->type <= eCreature::ARROW_TOWER;
}

bool StackCanAttackAndReturn(const H3CombatCreature *stack)
{
    const int type = stack->type;
    // The harpy comparisons use imm8; the darkness dragon comparison uses imm32.
    return type == static_cast<signed char>(ByteAt(0x75E05B)) ||
           type == static_cast<signed char>(ByteAt(0x75E060)) ||
           type == IntAt(0x75E064) || CDECL_1(BOOL, 0x71E433, stack);
}

bool IsAltHeld(const H3Msg *msg)
{
    const bool focused = GetActiveWindow() == H3Hwnd::Get();
    if (!focused)
        return false;
    if ((msg->IsKeyDown() || msg->IsKeyPress()) && msg->GetKey() == eVKey::H3VK_ALT)
        return msg->IsKeyDown() != FALSE;
    return msg->AltPressed() || (GetAsyncKeyState(VK_MENU) & 0x8000);
}

UINT ReadNumberKeys()
{
    UINT keys = 0;
    if (GetActiveWindow() == H3Hwnd::Get())
        for (int i = 0; i < SwitcherDlgPanel::MAX_ACTIONS; ++i)
            if ((GetAsyncKeyState('1' + i) | GetAsyncKeyState(VK_NUMPAD1 + i)) & 0x8000)
                keys |= 1u << i;
    return keys;
}

// The original resource has actions 0..6. Extend its loaded copy with independent
// copies of the shooting frames for action 7; a supplied 24-frame DEF is preserved.
void AddAreaShotFrames(H3LoadedDef *def)
{
    if (!def || !def->groupsCount || !def->groups[0] || def->groups[0]->count < 21)
        return;
    const int first = static_cast<int>(eAction::AREA_SHOT) * 3;
    auto group = def->groups[0];
    if (group->count >= first + 3)
        return;
    // AddFrameToGroup only fills preallocated slots; it does not grow the array.
    auto frames = static_cast<H3DefFrame **>(H3Realloc(group->frames, (first + 3) * sizeof(H3DefFrame *)));
    if (!frames)
        return;
    group->frames = frames;
    group->spritesSize = first + 3;
    while (def->groups[0]->count < first + 3)
    {
        const int state = def->groups[0]->count - first;
        H3DefFrame *source = def->GetGroupFrame(0, static_cast<int>(eAction::SHOOT) * 3 + state);
        H3DefFrame *copy = H3ObjectAllocator<H3DefFrame>().allocate(1);
        if (!copy)
            return;
        libc::memcpy(copy, source, sizeof(*copy));
        copy->rawData = ByteAllocator().allocate(source->rawDataSize);
        if (!copy->rawData)
        {
            H3Free(copy);
            return;
        }
        libc::memcpy(copy->rawData, source->rawData, source->rawDataSize);
        def->AddFrameToGroup(0, copy);
    }
}

void SetActionFrames(H3DlgCustomButton *button, eAction action)
{
    const int frame = static_cast<int>(action) * 3;
    H3LoadedDef *def = button->GetDef();
    if (def && def->groupsCount && def->groups[0] && def->groups[0]->count > frame + 1)
    {
        button->SetFrame(frame);
        button->SetClickFrame(frame + 1);
    }
}
} // namespace

ActionSwitcher *ActionSwitcher::instance = nullptr;
SwitcherDlgPanel *SwitcherDlgPanel::instance = nullptr;

DllExport bool SetState(const bool enabled)
{
    return ActionSwitcher::Get().SetPluginState(enabled);
}

SwitcherDlgPanel::SwitcherDlgPanel(H3CombatBottomPanel *panel, const int firstItemId)
    : bottomPanel(panel), dialog(static_cast<H3CombatDlg *>(panel->GetParent()))
{
    // AddItem registers the controls with the dialog and applies the panel offset.
    // The native panel constructor must have finished before calling it.
    actionSwitchButton = H3DlgCustomButton::Create(kButtonX, kButtonY, kButtonWidth, kButtonHeight,
        firstItemId, kButtonDef, SwitchPanelButtonProcedure, 0, 1);
    if (!actionSwitchButton)
        return;
    AddAreaShotFrames(actionSwitchButton->GetDef());
    panel->AddItem(actionSwitchButton);

    background = H3DlgPcx16::Create(0, -kMenuHeight, 1, kMenuHeight, firstItemId + 1, nullptr);
    if (background)
        panel->AddItem(background);

    switchItems.reserve(MAX_ACTIONS);
    for (int i = 0; i < MAX_ACTIONS; ++i)
    {
        auto button = H3DlgCustomButton::Create(0, 0, kButtonWidth, kButtonHeight,
            firstItemId + 2 + i, kButtonDef, SwitchPanelButtonProcedure, 0, 1);
        const char number[] = {static_cast<char>('1' + i), '\0'};
        auto label = H3DlgText::Create(0, 0, kButtonWidth, 12, number, NH3Dlg::Text::SMALL,
            eTextColor::REGULAR, firstItemId + 2 + MAX_ACTIONS + i, eTextAlignment::MIDDLE_CENTER);
        if (button)
            panel->AddItem(button);
        if (label)
            panel->AddItem(label);
        switchItems.push_back(PanelSwitchItem{eAction::NONE, button, label});
    }
    instance = this;
    UpdateButtonStates();
}

SwitcherDlgPanel::~SwitcherDlgPanel()
{
    // The combat panel owns and destroys all registered dialog items.
    if (instance == this)
        instance = nullptr;
}

BOOL SwitcherDlgPanel::IsEligibleForCurrentPlayer(H3CombatManager *manager) const
{
    if (!manager || manager->dlg != dialog || !manager->activeStack || manager->tacticsPhase ||
        manager->autoCombat || P_AutoSolo || manager->IsHiddenBattle())
        return FALSE;
    const int side = manager->currentActiveSide;
    if (side < 0 || side > 1 || !manager->isNotAI[side])
        return FALSE;
    if (THISCALL_2(BOOL8, 0x474520, manager, manager->activeStack))
        return FALSE;
    return manager->heroOwner[side] == P_Game->Get()->GetPlayerID();
}

BOOL SwitcherDlgPanel::CanInteract(H3CombatManager *manager) const
{
    return !actionDepth && manager && manager->action == eCombatAction::CANCEL &&
           IsEligibleForCurrentPlayer(manager);
}

void SwitcherDlgPanel::InitNewStackAttackType()
{
    // Availability queries must see the settled battlefield, outside animations.
    if (actionDepth)
    {
        actionsNeedRefresh = TRUE;
        return;
    }
    H3CombatManager *manager = P_CombatManager->Get();
    const auto oldActions = creatureActions;
    const eAction oldSelection = selectedAction;
    const BOOL oldVisible = visible;
    const BOOL oldButtonVisible = actionSwitchButton && actionSwitchButton->IsVisible();
    actionsNeedRefresh = FALSE;
    creatureActions.clear();
    if (!IsEligibleForCurrentPlayer(manager))
    {
        currentStack = nullptr;
        selectedAction = eAction::NONE;
        panelShownByAlt = visible = FALSE;
        buttonNeedsRedraw |= oldButtonVisible;
        battlefieldNeedsRedraw |= oldVisible;
        UpdateButtonStates();
        return;
    }

    H3CombatCreature *stack = manager->activeStack;
    currentStack = stack;
    currentTurn = manager->turn;

    // cannotMove only disables movement; it must not discard shooting or spells.
    checkingAvailability = TRUE;
    if (stack->numberAlive > 0 && !stack->activeSpellDuration[eSpell::BLIND] &&
        !stack->activeSpellDuration[eSpell::STONE] && !stack->activeSpellDuration[eSpell::PARALYZE])
    {
        const bool warMachine = IsWarMachine(stack);
        if (!warMachine)
            creatureActions.push_back(eAction::MELEE);
        if (stack->type != eCreature::CATAPULT && ActionSwitcher::CanShootNormally(stack))
            creatureActions.push_back(eAction::SHOOT);

        bool canCast = false;
        bool canResurrect = false;
        if (stack->info.spellCharges > 0)
        {
            for (int hex = 0; hex < 187; ++hex)
            {
                // This native query covers allied resurrection/demon raising,
                // including charges, target restrictions and the effective side.
                if (THISCALL_2(BOOL8, 0x4470F0, stack, hex))
                {
                    canCast = canResurrect = true;
                    break;
                }
                if (!canCast)
                    canCast = THISCALL_2(BOOL8, 0x4473E0, stack, hex) != FALSE;
            }
        }
        if (canCast)
            creatureActions.push_back(eAction::CAST);
        if (canResurrect && !warMachine && !stack->info.cannotMove &&
            stack->GetStackSpeed() > 0 && stack->dendroidBinder.IsEmpty())
            creatureActions.push_back(eAction::MOVE);
        if (!warMachine && StackCanAttackAndReturn(stack))
        {
            creatureActions.push_back(eAction::RETURN);
            creatureActions.push_back(eAction::NO_RETURN);
        }
        if (aoe::IsAvailable(stack))
            creatureActions.push_back(eAction::AREA_SHOT);
    }
    checkingAvailability = FALSE;
    const auto remembered = rememberedActions.find(stack);
    selectedAction = remembered != rememberedActions.end() ? remembered->second : eAction::NONE;
    // Keep the preference in memory even if it is unavailable on this turn.
    if (std::find(creatureActions.begin(), creatureActions.end(), selectedAction) == creatureActions.end())
        selectedAction = eAction::NONE;
    if (creatureActions.size() < 2)
        visible = FALSE;
    buttonNeedsRedraw |= oldActions != creatureActions || oldSelection != selectedAction ||
                         oldButtonVisible != (creatureActions.size() > 1);
    battlefieldNeedsRedraw |= oldVisible != visible || (visible && oldActions != creatureActions);
    UpdateButtonStates();
}

void SwitcherDlgPanel::UpdateLayout()
{
    if (!background || !actionSwitchButton || creatureActions.size() < 2)
        return;
    const int width = static_cast<int>(creatureActions.size()) * kOptionSpacing - 3 + 8;
    if (width != menuWidth)
    {
        H3LoadedPcx16 *pcx = H3LoadedPcx16::Create(width, kMenuHeight);
        if (pcx)
        {
            pcx->BackgroundRegion(0, 0, width, kMenuHeight, FALSE);
            pcx->SimpleFrameRegion(0, 0, width, kMenuHeight);
            H3LoadedPcx16 *old = background->GetPcx();
            background->SetPcx(pcx);
            if (old)
                old->Destroy();
            menuWidth = width;
            background->SetWidth(width);
        }
    }
    const int left = actionSwitchButton->GetX() + kButtonWidth - width;
    const int top = actionSwitchButton->GetY() - kButtonY - kMenuHeight;
    background->SetX(left);
    background->SetY(top);
    for (size_t i = 0; i < switchItems.size(); ++i)
    {
        auto &item = switchItems[i];
        const int x = left + 4 + static_cast<int>(i) * kOptionSpacing;
        if (item.button)
        {
            item.button->SetX(x);
            item.button->SetY(top + 4);
        }
        if (item.number)
        {
            item.number->SetX(x);
            item.number->SetY(top + 40);
        }
    }
}

void SwitcherDlgPanel::UpdateButtonStates()
{
    UpdateLayout();
    if (background)
    {
        if (visible && creatureActions.size() > 1)
            background->Show();
        else
            background->HideDeactivate();
    }
    for (size_t i = 0; i < switchItems.size(); ++i)
    {
        auto &item = switchItems[i];
        item.action = i < creatureActions.size() ? creatureActions[i] : eAction::NONE;
        if (item.button)
        {
            if (item.action != eAction::NONE)
            {
                SetActionFrames(item.button, item.action);
                item.button->SetHints(ActionHint(item.action), "", FALSE);
            }
            if (visible && item.action != eAction::NONE)
                item.button->ShowActivate();
            else
                item.button->HideDeactivate();
        }
        if (item.number)
        {
            if (visible && item.action != eAction::NONE)
                item.number->Show();
            else
                item.number->HideDeactivate();
        }
    }
    if (actionSwitchButton)
    {
        actionSwitchButton->SetHints(ActionHint(selectedAction), "", FALSE);
        SetActionFrames(actionSwitchButton, selectedAction);
        if (creatureActions.size() > 1)
            actionSwitchButton->ShowActivate();
        else
            actionSwitchButton->HideDeactivate();
    }
}

void SwitcherDlgPanel::SelectAction(size_t index, H3CombatManager *manager)
{
    if (index < creatureActions.size())
        SelectAction(creatureActions[index], manager);
}

void SwitcherDlgPanel::SelectAction(eAction action, H3CombatManager *manager)
{
    if (!CanInteract(manager) ||
        std::find(creatureActions.begin(), creatureActions.end(), action) == creatureActions.end())
        return;
    const bool changed = selectedAction != action;
    selectedAction = action;
    rememberedActions[currentStack] = action;
    // Recalculate the native cursor at the next mouse message, even on the same hex.
    manager->mouseCoord = -1;
    manager->moveType = -99;
    if (changed)
    {
        buttonNeedsRedraw = TRUE;
        battlefieldNeedsRedraw |= visible;
        UpdateButtonStates();
    }
    if (action == eAction::AREA_SHOT)
    {
        SetVisible(FALSE);
        Refresh();
        // The modal targeting callback builds the standard SHOOT command. The outer
        // combat message loop executes it after this procedure returns.
        checkingAvailability = TRUE;
        aoe::OpenForActiveStack();
        checkingAvailability = FALSE;
    }
    else
        Refresh();
}

BOOL SwitcherDlgPanel::SetVisible(const BOOL show)
{
    const BOOL value = show && creatureActions.size() > 1;
    if (visible == value)
        return FALSE;
    visible = value;
    battlefieldNeedsRedraw = TRUE;
    UpdateButtonStates();
    return TRUE;
}

void SwitcherDlgPanel::Refresh()
{
    H3CombatManager *manager = P_CombatManager->Get();
    if (CanInteract(manager))
    {
        if (battlefieldNeedsRedraw)
        {
            // Rebuild only when the menu changes, including erasing it on close.
            battlefieldNeedsRedraw = FALSE;
            manager->Refresh();
        }
        if (buttonNeedsRedraw && actionSwitchButton)
        {
            buttonNeedsRedraw = FALSE;
            if (actionSwitchButton->IsVisible())
            {
                actionSwitchButton->Draw();
                actionSwitchButton->Refresh();
            }
            else
                bottomPanel->Redraw(); // Restore the native background under a hidden button.
        }
    }
}

void SwitcherDlgPanel::Draw()
{
    // Battlefield frames never overwrite the main button on the bottom panel.
    // Only the open menu needs to be restored after a native battlefield copy.
    H3CombatManager *manager = P_CombatManager->Get();
    if (!visible || !CanInteract(manager) || currentStack != manager->activeStack ||
        currentTurn != manager->turn)
        return;
    if (background && background->GetPcx())
        background->Draw();
    for (const auto &item : switchItems)
    {
        if (item.button && item.button->IsVisible())
        {
            item.button->Draw();
            if (item.action == selectedAction)
                item.button->DrawTempFrame(1, 255, 210, 80);
        }
        if (item.number && item.number->IsVisible())
            item.number->Draw();
    }
}

void __stdcall SwitcherDlgPanel::H3CombatDlg_DrawBattlefield(HiHook *h, H3CombatDlg *dlg, BOOL redraw)
{
    if (!instance || instance->dialog != dlg || !instance->visible ||
        !ActionSwitcher::Get().IsEnabled() || !instance->CanInteract(P_CombatManager->Get()))
    {
        // Preserve the native redraw area during actions and automatic combat.
        THISCALL_2(void, h->GetDefaultFunc(), dlg, redraw);
        return;
    }
    THISCALL_2(void, h->GetDefaultFunc(), dlg, FALSE);
    // This call draws the completed battlefield, after animation/background copies.
    instance->Draw();
    if (redraw)
        P_WindowManager->H3Redraw(dlg->GetX(), dlg->GetY(), dlg->GetWidth(), instance->bottomPanel->GetY());
}

INT __fastcall SwitcherDlgPanel::SwitchPanelButtonProcedure(H3Msg *msg)
{
    if (instance && msg && msg->IsLeftClick() && ActionSwitcher::Get().IsEnabled() &&
        instance->CanInteract(P_CombatManager->Get()))
        instance->pendingButton = msg->itemId;
    return 0;
}

INT __stdcall SwitcherDlgPanel::H3CombatDlg_Procedure(HiHook *h, H3CombatManager *manager, H3Msg *msg)
{
    if (!instance || !manager || !msg || manager->dlg != instance->dialog || !ActionSwitcher::Get().IsEnabled())
        return THISCALL_2(INT, h->GetDefaultFunc(), manager, msg);

    SwitcherDlgPanel &panel = *instance;
    if (!panel.CanInteract(manager))
    {
        panel.SetVisible(FALSE);
        if (!panel.IsEligibleForCurrentPlayer(manager) && panel.actionSwitchButton)
            panel.actionSwitchButton->HideDeactivate();
        panel.pendingButton = -1;
        panel.heldNumberKeys = ReadNumberKeys();
        panel.consumedNumberKeys = 0;
        panel.actionsNeedRefresh = TRUE;
        return THISCALL_2(INT, h->GetDefaultFunc(), manager, msg);
    }
    if (panel.actionsNeedRefresh || panel.currentStack != manager->activeStack ||
        panel.currentTurn != manager->turn)
        panel.InitNewStackAttackType();
    const BOOL alt = IsAltHeld(msg);
    const UINT heldKeys = ReadNumberKeys();
    const UINT newKeys = heldKeys & ~panel.heldNumberKeys;
    panel.heldNumberKeys = heldKeys;
    panel.panelShownByAlt = alt;
    panel.SetVisible(alt);

    // Windows can reserve Alt+digits for system input. Poll their physical rising
    // edges on the combat loop's idle messages as well as accepting game events.
    if (alt && newKeys && panel.visible)
    {
        for (int i = 0; i < MAX_ACTIONS; ++i)
            if (newKeys & (1u << i))
            {
                panel.consumedNumberKeys |= 1u << i;
                panel.SelectAction(static_cast<size_t>(i), manager);
                panel.Refresh();
                return TRUE;
            }
    }

    const bool keyMessage = msg->IsKeyDown() || msg->IsKeyPress() || msg->IsKeyHeld();
    const int number = keyMessage ? static_cast<int>(msg->GetKey()) - static_cast<int>(eVKey::H3VK_1) : -1;
    if (number >= 0 && number < MAX_ACTIONS)
    {
        const UINT bit = 1u << number;
        const bool consumed = (panel.consumedNumberKeys & bit) != 0;
        if (msg->IsKeyPress())
            panel.consumedNumberKeys &= ~bit;
        if (panel.visible || consumed)
        {
            // Accept both native key transitions, including Alt+digit messages
            // arriving without a preceding Alt event. Never select twice per press.
            if (panel.visible && !consumed && !msg->IsKeyHeld())
            {
                if (msg->IsKeyDown())
                    panel.consumedNumberKeys |= bit;
                panel.SelectAction(static_cast<size_t>(number), manager);
            }
            panel.Refresh();
            return TRUE;
        }
    }

    const bool altEvent = keyMessage && msg->GetKey() == eVKey::H3VK_ALT;
    if (altEvent)
    {
        if (!alt)
            panel.consumedNumberKeys = 0;
        panel.Refresh();
        return TRUE;
    }

    // Custom callbacks queue clicks during native dialog dispatch. Also handle the
    // original click directly when it reaches the combat procedure first.
    const int buttonId = panel.pendingButton >= 0 ? panel.pendingButton :
                         (msg->IsLeftClick() ? msg->itemId : -1);
    panel.pendingButton = -1;
    if (buttonId == BUTTON_ID && panel.creatureActions.size() > 1)
    {
        const auto current = std::find(panel.creatureActions.begin(), panel.creatureActions.end(), panel.selectedAction);
        const size_t next = current == panel.creatureActions.end() ? 0 :
            (static_cast<size_t>(current - panel.creatureActions.begin()) + 1) % panel.creatureActions.size();
        panel.SelectAction(next, manager);
        return TRUE;
    }
    for (size_t i = 0; i < panel.switchItems.size(); ++i)
    {
        const auto &item = panel.switchItems[i];
        if (item.button && buttonId == item.button->GetID() && panel.visible)
        {
            panel.SelectAction(i, manager);
            return TRUE;
        }
    }
    panel.Refresh();
    return THISCALL_2(INT, h->GetDefaultFunc(), manager, msg);
}

INT __stdcall SwitcherDlgPanel::H3CombatManager_ProcessAction(HiHook *h, H3CombatManager *manager,
                                                            H3Msg *msg, INT argument)
{
    ++actionDepth;
    if (instance && instance->dialog == manager->dlg)
    {
        instance->SetVisible(FALSE);
        instance->pendingButton = -1;
        // Native action drawing erases the menu. Do not refresh it ourselves.
        instance->battlefieldNeedsRedraw = FALSE;
    }
    const INT result = THISCALL_3(INT, h->GetDefaultFunc(), manager, msg, argument);
    --actionDepth;
    if (instance && instance->dialog == manager->dlg)
        instance->actionsNeedRefresh = TRUE;
    return result;
}

_LHF_(SwitcherDlgPanel::H3CombatManager_GetControl)
{
    if (instance && ActionSwitcher::Get().IsEnabled())
    {
        instance->InitNewStackAttackType();
        if (actionDepth)
            return EXEC_DEFAULT;
        instance->panelShownByAlt = GetActiveWindow() == H3Hwnd::Get() &&
                                   (GetAsyncKeyState(VK_MENU) & 0x8000);
        instance->SetVisible(instance->panelShownByAlt);
        instance->pendingButton = -1;
        instance->heldNumberKeys = ReadNumberKeys();
        instance->consumedNumberKeys = instance->heldNumberKeys;
        // Native control transfer draws the complete bottom panel once afterwards.
        // Its screen update covers only the bottom strip; restore an open menu
        // above it on the first idle message after control has been accepted.
        instance->buttonNeedsRedraw = FALSE;
        instance->battlefieldNeedsRedraw = instance->visible;
    }
    return EXEC_DEFAULT;
}

ActionSwitcher::ActionSwitcher() : IGamePatch(_PI)
{
    CreatePatches();
}

eAction ActionSwitcher::GetActiveAction(const H3CombatCreature *stack) const
{
    H3CombatManager *manager = P_CombatManager->Get();
    if (!state || !switcherDlgPanel || !manager || !manager->dlg ||
        manager->dlg != switcherDlgPanel->dialog || manager->activeStack != stack ||
        switcherDlgPanel->currentStack != stack || !switcherDlgPanel->IsEligibleForCurrentPlayer(manager))
        return eAction::NONE;
    return switcherDlgPanel->GetSelectedAction();
}

_LHF_(ActionSwitcher::Y_BattleMgr_WOG_HarpyReturn)
{
    ActionSwitcher &plugin = Get();
    if (plugin.GetActiveAction(reinterpret_cast<H3CombatCreature *>(c->ebx)) == eAction::NO_RETURN)
    {
        c->return_address = 0x75E0B4;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

BOOL ActionSwitcher::CanShootNormally(const H3CombatCreature *stack)
{
    if (!stack)
        return FALSE;
    const int function = instance && instance->canShootHook ? instance->canShootHook->GetDefaultFunc() : 0x442610;
    return THISCALL_2(BOOL8, function, stack, 0);
}

BOOL8 __stdcall ActionSwitcher::Y_CanShoot(HiHook *h, H3CombatCreature *stack, INT target)
{
    const eAction action = instance ? instance->GetActiveAction(stack) : eAction::NONE;
    if (action == eAction::MELEE || action == eAction::MOVE || action == eAction::CAST ||
        action == eAction::RETURN || action == eAction::NO_RETURN)
        return FALSE;
    return THISCALL_2(BOOL8, h->GetDefaultFunc(), stack, target);
}

BOOL8 __stdcall ActionSwitcher::Y_CanCast(HiHook *h, H3CombatCreature *stack, INT hex)
{
    const eAction action = instance ? instance->GetActiveAction(stack) : eAction::NONE;
    if (action != eAction::NONE && action != eAction::CAST)
        return FALSE;
    return THISCALL_2(BOOL8, h->GetDefaultFunc(), stack, hex);
}

INT __stdcall ActionSwitcher::Y_GetCursorAction(HiHook *h, H3CombatManager *manager, INT hex)
{
    const int cursor = THISCALL_2(INT, h->GetDefaultFunc(), manager, hex);
    const eAction action = instance ? instance->GetActiveAction(manager->activeStack) : eAction::NONE;
    // Native cursor values: 1 walk, 2 fly, 3/15 shoot, 7 melee, 20 creature spell.
    // Preserve native reachability, direction, target and command parameters.
    if (action == eAction::MOVE && cursor != 1 && cursor != 2 && cursor != 22)
        return 0;
    if (action == eAction::SHOOT && cursor != 3 && cursor != 15)
        return 0;
    if (action == eAction::CAST && cursor != 20)
        return 0;
    return cursor;
}

H3CombatBottomPanel *__stdcall ActionSwitcher::Y_CreateBottomPanel(HiHook *h, H3CombatBottomPanel *panel,
                                                                  H3CombatDlg *dlg, INT argument)
{
    auto result = THISCALL_3(H3CombatBottomPanel *, h->GetDefaultFunc(), panel, dlg, argument);
    ActionSwitcher &plugin = Get();
    if (plugin.IsEnabled() && result)
    {
        delete plugin.switcherDlgPanel;
        plugin.switcherDlgPanel = new SwitcherDlgPanel(result);
    }
    return result;
}

void __stdcall ActionSwitcher::Y_DestroyBottomPanel(HiHook *h, H3CombatBottomPanel *panel)
{
    if (instance && instance->switcherDlgPanel && instance->switcherDlgPanel->bottomPanel == panel)
    {
        delete instance->switcherDlgPanel;
        instance->switcherDlgPanel = nullptr;
    }
    THISCALL_1(void, h->GetDefaultFunc(), panel);
}

void ActionSwitcher::CreatePatches() noexcept
{
    if (m_isInited)
        return;

    m_isInited = true;
    interfaceChangePatches.reserve(32);

    // Move and resize the stock combat controls to make room for the action selector.
    interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B3D9 + 1, 4));
    interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B3D3 + 1, 44));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B3C9 + 1, (int)"icm003q.def"));

    interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B2CF + 1, 51));
    interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B2CB + 1, 44));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B2BF + 1, (int)"icm001q.def"));

    interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B354 + 1, 98));
    interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B34E + 1, 44));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B344 + 1, (int)"icm002q.def"));

    interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B45E + 1, 145));
    interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B458 + 1, 44));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B44E + 1, (int)"icm004q.def"));

    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46BC1F + 1, 192));

    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46BC8F + 1, 398));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46BC88 + 1, 188));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46BC7C + 1, (int)"icm012q.def"));

    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B807 + 1, 194));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B800 + 1, 390));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B87E + 1, 590));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B917 + 1, 590));

    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B4E6 + 1, 611));
    interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B4E0 + 1, 44));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B4D6 + 1, (int)"icm005q.def"));

    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B56E + 1, 705));
    interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B568 + 1, 44));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B55E + 1, (int)"icm006q.def"));

    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B5F6 + 1, 752));
    interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B5F0 + 1, 44));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B5E6 + 1, (int)"icm007q.def"));

    interfaceChangePatches.emplace_back(_PI->WriteDword(0x471ED9 + 1, 195));
    interfaceChangePatches.emplace_back(_PI->WriteDword(0x471ECF + 1, 390));

    _PI->WriteHiHook(0x46B160, THISCALL_, Y_CreateBottomPanel);
    _PI->WriteHiHook(0x46B700, THISCALL_, Y_DestroyBottomPanel);
    _PI->WriteHiHook(0x472DD0, THISCALL_, SwitcherDlgPanel::H3CombatDlg_DrawBattlefield);
    _PI->WriteHiHook(0x04746B0, THISCALL_, SwitcherDlgPanel::H3CombatDlg_Procedure);
    _PI->WriteHiHook(0x4786B0, THISCALL_, SwitcherDlgPanel::H3CombatManager_ProcessAction);
    _PI->WriteLoHook(0x477D98, SwitcherDlgPanel::H3CombatManager_GetControl);
    canShootHook = _PI->WriteHiHook(0x442610, THISCALL_, Y_CanShoot);
    _PI->WriteHiHook(0x4470F0, THISCALL_, Y_CanCast);
    _PI->WriteHiHook(0x4473E0, THISCALL_, Y_CanCast);
    _PI->WriteHiHook(0x475DC0, THISCALL_, Y_GetCursorAction);
    _PI->WriteLoHook(0x75E0E7, Y_BattleMgr_WOG_HarpyReturn);

    aoe::Install(_PI);
    aoe::SetEnabled(state != FALSE);
}

BOOL ActionSwitcher::SetPluginState(const bool enabled) noexcept
{
    if (P_CombatManager->dlg != nullptr || static_cast<BOOL>(enabled) == state)
        return FALSE;

    state = enabled;
    aoe::SetEnabled(enabled != FALSE);
    for (Patch *patch : interfaceChangePatches)
    {
        if (enabled)
            patch->Apply();
        else
            patch->Undo();
    }

    return TRUE;
}

ActionSwitcher &ActionSwitcher::Get()
{
    if (!instance)
        instance = new ActionSwitcher();
    return *instance;
}
} // namespace switcher
