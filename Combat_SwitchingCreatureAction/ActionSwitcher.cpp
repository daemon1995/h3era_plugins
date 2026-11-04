#include "ActionSwitcher.h"
#include <array>

#pragma comment(linker, "/EXPORT:SetOnBattleStackActionDetermination=_SetOnBattleStackActionDetermination@4")

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

const char *ActionText(eAction action, bool rightClick)
{
    struct Text { const char *key, *hint, *description; };
    static const Text texts[] = {
        {"none", "Switch creature action (ALT).", "{Alternate actions}\n\nPress ALT or click this button to cycle through the actions available to the active stack. The button shows the next action."},
        {"melee", "Switch to melee attack (ALT).", "{Melee attack}\n\nSwitch to a melee attack instead of shooting or casting a creature spell."},
        {"shoot", "Switch to ranged attack (ALT).", "{Ranged attack}\n\nSwitch to an ordinary shot at an enemy stack. Ammo and native shooting restrictions apply."},
        {"cast", "Switch to creature spell (ALT).", "{Creature spell}\n\nSwitch to the creature's spell or ability. Spells on living targets allow ordinary movement. Reachable corpse spells use the separate Move mode to walk without resurrecting."},
        {"move", "Move without resurrecting (ALT).", "{Move}\n\nMove onto an allied corpse instead of resurrecting it. Available only when this stack can both resurrect and reach such a hex."},
        {"return", "Switch to attack and return (ALT).", "{Attack and return}\n\nReturn to the original hex after attacking."},
        {"no_return", "Switch to attack without returning (ALT).", "{Attack and stay}\n\nRemain near the target after attacking."},
        {"area_shot", "Choose area shot target (ALT / G).", "{Area shot}\n\nPress ALT, G or click this button to open target selection. ESC or right-click cancels and returns to ordinary shooting. Area targeting is not remembered for the next turn. The shot hits the center and adjacent hexes, including allies. Splash immunity depends on the configured shot type. Uses one shot."}
    };
    const INT index = static_cast<INT>(action);
    const Text &text = texts[index >= 0 && index < 8 ? index : 0];
    char key[96];
    sprintf(key, "combat_switching_actions.%s.%s", rightClick ? "rmc" : "hint", text.key);
    bool found = false;
    const char *translation = EraJS::read(key, found);
    if (found && translation && translation[0])
        return translation;
    return rightClick ? text.description : text.hint;
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
    if (def && def->groupsCount && def->groups[0] && def->groups[0]->count > frame + 2)
    {
        button->SetFrame(frame);
        button->SetClickFrame(frame + 1);
        // Native drawing also uses disabled and selected frames. Its defaults
        // are 2 and 3; frame 3 would incorrectly show melee on a neutral button.
        IntAt(reinterpret_cast<INT>(button) + 0x3C) = frame + 2;
        IntAt(reinterpret_cast<INT>(button) + 0x40) = frame;
    }
}
} // namespace

ActionSwitcher *ActionSwitcher::instance = nullptr;
SwitcherDlgPanel *SwitcherDlgPanel::instance = nullptr;

DllExport bool SetState(const bool enabled)
{
    return ActionSwitcher::Get().SetPluginState(enabled);
}

DllExport BOOL __stdcall SetOnBattleStackActionDetermination(const INT functionId)
{
    return ActionSwitcher::Get().SetActionDeterminationCallback(functionId);
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
    actionSwitchButton->AddHotkey(static_cast<INT>(eVKey::H3VK_ALT));
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
    aoe::ResetTargeting(P_CombatManager->Get());
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
    if (actionDepth || checkingAvailability)
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
    actionOptions = StackActionOptions{};
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
        actionOptions.melee = !warMachine;
        actionOptions.shoot = stack->type != eCreature::CATAPULT && ActionSwitcher::CanShootNormally(stack);
        actionOptions.areaShot = aoe::IsAvailable(stack);
        actionOptions.attackReturn = !warMachine && StackCanAttackAndReturn(stack);
        if (stack->info.spellCharges > 0)
        {
            for (int hex = 0; hex < 187; ++hex)
            {
                H3CombatCreature *target = manager->squares[hex].GetCreature();
                if (!target || target->numberAlive <= 0)
                {
                    // Only reachable empty/corpse spell targets compete with walking.
                    if (!actionOptions.castHex &&
                        (THISCALL_2(BOOL8, 0x4470F0, stack, hex) || THISCALL_2(BOOL8, 0x4473E0, stack, hex)) &&
                        CanMoveToHex(manager, stack, hex))
                        actionOptions.castHex = TRUE;
                    continue;
                }
                BOOL &canCast = target->GetSide() == stack->GetSide() ? actionOptions.castAlly : actionOptions.castEnemy;
                if (!canCast && THISCALL_2(BOOL8, 0x4473E0, stack, hex))
                    canCast = TRUE;
            }
        }
    }
    ActionSwitcher::Get().DetermineStackActions(stack, actionOptions);
    // Build UI modes from the permitted capabilities. Return/stay replaces the
    // duplicate ordinary melee mode; all permitted spell targets share CAST.
    if (actionOptions.melee)
    {
        if (actionOptions.attackReturn)
        {
            creatureActions.push_back(eAction::RETURN);
            creatureActions.push_back(eAction::NO_RETURN);
        }
        else
            creatureActions.push_back(eAction::MELEE);
    }
    if (actionOptions.shoot)
        creatureActions.push_back(eAction::SHOOT);
    if (actionOptions.castEnemy || actionOptions.castAlly || actionOptions.castHex)
        creatureActions.push_back(eAction::CAST);
    if (actionOptions.castHex && !IsWarMachine(stack))
        creatureActions.push_back(eAction::MOVE);
    if (actionOptions.areaShot)
        creatureActions.push_back(eAction::AREA_SHOT);
    checkingAvailability = FALSE;
    const auto remembered = rememberedActions.find(stack);
    selectedAction = remembered != rememberedActions.end() ? remembered->second : eAction::NONE;
    if (selectedAction == eAction::AREA_SHOT)
        selectedAction = eAction::SHOOT;
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

BOOL SwitcherDlgPanel::CanMoveToHex(H3CombatManager *manager, H3CombatCreature *stack, const INT hex)
{
    if (hex < 0 || hex >= 187 || hex % 17 == 0 || hex % 17 == 16 ||
        stack->info.cannotMove || stack->GetStackSpeed() <= 0 || !stack->dendroidBinder.IsEmpty())
        return FALSE;

    // Ask the native cursor/pathfinder with creature casting suppressed. This
    // respects obstacles, double width, flight and movement range. Cursor queries
    // write target fields, so they must not replace the player's actual target.
    const INT targetSide = stack->_f_10;
    const INT targetIndex = stack->_f_14;
    const INT destination = stack->battlefieldDestination;
    checkingMovement = TRUE;
    const INT cursor = THISCALL_2(INT, 0x475DC0, manager, hex);
    checkingMovement = FALSE;
    stack->_f_10 = targetSide;
    stack->_f_14 = targetIndex;
    stack->battlefieldDestination = destination;
    return cursor == 1 || cursor == 2;
}

eAction SwitcherDlgPanel::NextAvailableAction() const
{
    if (creatureActions.size() < 2)
        return eAction::NONE;
    auto current = std::find(creatureActions.begin(), creatureActions.end(), selectedAction);
    if (current == creatureActions.end())
    {
        // NONE preserves native behaviour. Use the original selector's default
        // priority to identify which action ALT will switch away from.
        const eAction defaults[] = {eAction::CAST, eAction::SHOOT, eAction::RETURN, eAction::MELEE};
        for (const eAction action : defaults)
        {
            current = std::find(creatureActions.begin(), creatureActions.end(), action);
            if (current != creatureActions.end())
                break;
        }
        if (current == creatureActions.end())
            current = creatureActions.begin();
    }
    if (++current == creatureActions.end())
        current = creatureActions.begin();
    return *current;
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
    H3CombatManager *manager = P_CombatManager->Get();
    const BOOL eligible = IsEligibleForCurrentPlayer(manager);
    UpdateLayout();
    if (background)
    {
        if (eligible && visible && creatureActions.size() > 1)
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
                item.button->SetHints(ActionText(item.action, false), ActionText(item.action, true), FALSE);
            }
            if (eligible && visible && item.action != eAction::NONE)
                item.button->ShowActivate();
            else
                item.button->HideDeactivate();
        }
        if (item.number)
        {
            if (eligible && visible && item.action != eAction::NONE)
                item.number->Show();
            else
                item.number->HideDeactivate();
        }
    }
    if (actionSwitchButton)
    {
        const eAction next = eligible ? NextAvailableAction() : eAction::NONE;
        actionSwitchButton->SetHints(ActionText(next, false), ActionText(next, true), FALSE);
        SetActionFrames(actionSwitchButton, next);
        if (!eligible)
        {
            actionSwitchButton->Show();
            actionSwitchButton->DeActivate();
            actionSwitchButton->RemoveState(eControlState::PRESSED | eControlState::SELECTED);
            actionSwitchButton->Shade();
        }
        else if (creatureActions.size() > 1)
        {
            actionSwitchButton->UnShade();
            actionSwitchButton->ShowActivate();
        }
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
    if (action == eAction::AREA_SHOT)
    {
        // G, ALT and menu clicks share this single transient target picker.
        // AoE is selected only while this dialog is open. The saved preference
        // remains ordinary shooting; the button displays the action after AoE.
        selectedAction = eAction::AREA_SHOT;
        const eAction fallback = actionOptions.shoot ? eAction::SHOOT : eAction::NONE;
        rememberedActions[currentStack] = fallback;
        pendingButton = -1;
        SetVisible(FALSE);
        UpdateButtonStates();
        buttonNeedsRedraw = TRUE;
        Refresh();
        aoe::OpenForActiveStack();
        const BOOL advance = pendingButton == BUTTON_ID;
        const eAction next = advance ? NextAvailableAction() : fallback;
        pendingButton = -1;
        if (advance)
        {
            // Leave the target picker before changing modes; never nest dialogs.
            SelectAction(next, manager);
            return;
        }
        selectedAction = fallback;
        manager->mouseCoord = -1;
        manager->moveType = -99;
        UpdateButtonStates();
        buttonNeedsRedraw = TRUE;
        battlefieldNeedsRedraw |= manager->action == eCombatAction::CANCEL;
        Refresh();
        THISCALL_1(void, 0x472730, manager->dlg);
        THISCALL_1(INT, 0x4ECCD0, *reinterpret_cast<H3InputManager **>(0x699530));
        return;
    }
    const bool changed = selectedAction != action;
    if (changed)
        aoe::ResetTargeting(manager);
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
    // Match the original selector: regenerate shading, redraw the battlefield,
    // and enqueue a mouse move to update the cursor even on the same hex.
    THISCALL_3(INT, 0x4934B0, manager, 0, 1);
    battlefieldNeedsRedraw = TRUE;
    Refresh();
    // This H3API header points P_InputManager at the pathfinder (0x6992D4).
    // The native input queue used by the original selector is at 0x699530.
    auto inputManager = *reinterpret_cast<H3InputManager **>(0x699530);
    THISCALL_1(INT, 0x4ECCD0, inputManager);
}

BOOL SwitcherDlgPanel::HandleAreaShotSwitch(H3Msg *msg)
{
    H3CombatManager *manager = P_CombatManager->Get();
    if (!msg || selectedAction != eAction::AREA_SHOT || !IsEligibleForCurrentPlayer(manager) ||
        !actionSwitchButton || !actionSwitchButton->IsVisible() || creatureActions.size() < 2)
        return FALSE;
    if (!msg->IsKeyDown() && !msg->IsKeyPress() && !msg->IsKeyHeld() &&
        static_cast<INT>(msg->command) != 8 && static_cast<INT>(msg->command) != 16 && !msg->IsLeftClick())
        return FALSE;

    // The target picker deactivates the parent dialog. Dispatch only our DEF
    // button, including its native ALT hotkey, without enabling other controls.
    H3Msg buttonMessage = *msg;
    // H3API does not expose the nesting counter or the message virtual publicly.
    const INT buttonAddress = reinterpret_cast<INT>(actionSwitchButton);
    INT &deactivationCount = IntAt(buttonAddress + 0x2C);
    const INT deactivations = deactivationCount;
    deactivationCount = 0;
    const INT processMessage = IntAt(IntAt(buttonAddress) + 0x08);
    THISCALL_2(INT, processMessage, actionSwitchButton, &buttonMessage);
    deactivationCount = deactivations;
    if (buttonMessage.IsLeftClick() && buttonMessage.itemId == BUTTON_ID)
    {
        pendingButton = BUTTON_ID;
        return TRUE;
    }
    return FALSE;
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
        aoe::ResetTargeting(manager);
        panel.SetVisible(FALSE);
        if (!panel.IsEligibleForCurrentPlayer(manager))
            panel.UpdateButtonStates();
        panel.pendingButton = -1;
        panel.heldNumberKeys = 0;
        panel.consumedNumberKeys = 0;
        panel.actionsNeedRefresh = TRUE;
        return THISCALL_2(INT, h->GetDefaultFunc(), manager, msg);
    }
    if (panel.actionsNeedRefresh || panel.currentStack != manager->activeStack ||
        panel.currentTurn != manager->turn)
        panel.InitNewStackAttackType();
    if ((msg->IsKeyDown() || msg->IsKeyPress() || msg->IsKeyHeld()) &&
        msg->GetKey() == eVKey::H3VK_G &&
        std::find(panel.creatureActions.begin(), panel.creatureActions.end(), eAction::AREA_SHOT) != panel.creatureActions.end())
    {
        if (msg->IsKeyDown() && aoe::IsAvailable(manager->activeStack))
        {
            panel.pendingButton = -1;
            panel.SelectAction(eAction::AREA_SHOT, manager);
        }
        return TRUE;
    }
    if (msg->IsRightClick())
    {
        H3DlgCustomButton *button = msg->itemId == BUTTON_ID ? panel.actionSwitchButton : nullptr;
        if (!button && panel.visible)
            for (const auto &item : panel.switchItems)
                if (item.button && item.button->GetID() == msg->itemId)
                    button = item.button;
        if (button && button->IsVisible() && button->GetRightClickHint())
        {
            panel.pendingButton = -1;
            H3Messagebox::RMB(button->GetRightClickHint());
            manager->mouseCoord = -1;
            THISCALL_1(INT, 0x4ECCD0, *reinterpret_cast<H3InputManager **>(0x699530));
            return TRUE;
        }
    }
    // Custom callbacks queue clicks during native dialog dispatch. Also handle the
    // original click directly when it reaches the combat procedure first.
    const int buttonId = panel.pendingButton >= 0 ? panel.pendingButton :
                         (msg->IsLeftClick() ? msg->itemId : -1);
    panel.pendingButton = -1;
    if (buttonId == BUTTON_ID && panel.creatureActions.size() > 1)
    {
        panel.SelectAction(panel.NextAvailableAction(), manager);
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
    aoe::ResetTargeting(manager);
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
        instance->panelShownByAlt = FALSE;
        instance->SetVisible(FALSE);
        aoe::ResetTargeting(P_CombatManager->Get());
        instance->pendingButton = -1;
        instance->heldNumberKeys = instance->consumedNumberKeys = 0;
        // Native control transfer draws the complete bottom panel once afterwards.
        // The default ALT interface has no open menu to restore above it.
        instance->buttonNeedsRedraw = FALSE;
        instance->battlefieldNeedsRedraw = instance->visible;
    }
    return EXEC_DEFAULT;
}

ActionSwitcher::ActionSwitcher() : IGamePatch(_PI)
{
    CreatePatches();
}

BOOL ActionSwitcher::SetActionDeterminationCallback(const INT functionId)
{
    if (functionId < 0)
        return FALSE;
    actionDeterminationCallback = functionId;
    if (switcherDlgPanel)
        switcherDlgPanel->actionsNeedRefresh = TRUE;
    return TRUE;
}

void ActionSwitcher::DetermineStackActions(H3CombatCreature *stack, StackActionOptions &options)
{
    const INT callback = actionDeterminationCallback;
    if (!callback || !stack || !Era::GetArgXVars || !Era::GetRetXVars || !Era::FireErmEvent)
        return;

    // x1 = stack ID, x2 = current owner hero. x3..x9 are boolean capabilities,
    // not action IDs/counts: melee, shoot, area, enemy/ally/hex spell, return.
    const StackActionOptions physical = options;
    auto arguments = *Era::GetArgXVars();
    auto results = *Era::GetRetXVars();
    std::array<INT, 16> savedArguments;
    std::array<INT, 16> savedResults;
    std::copy(arguments, arguments + 16, savedArguments.begin());
    std::copy(results, results + 16, savedResults.begin());
    std::fill(arguments, arguments + 16, 0);
    arguments[0] = stack->Index();
    const H3Hero *hero = stack->GetOwner();
    arguments[1] = hero ? hero->id : -1;
    arguments[2] = options.melee;
    arguments[3] = options.shoot;
    arguments[4] = options.areaShot;
    arguments[5] = options.castEnemy;
    arguments[6] = options.castAlly;
    arguments[7] = options.castHex;
    arguments[8] = options.attackReturn;
    Era::FireErmEvent(callback);

    std::array<INT, 16> returned;
    std::copy(results, results + 16, returned.begin());
    std::copy(savedArguments.begin(), savedArguments.end(), arguments);
    std::copy(savedResults.begin(), savedResults.end(), results);

    // A flag can permit a native capability, not create a spell/shooting ability.
    options.melee = physical.melee && returned[2] != 0;
    options.shoot = physical.shoot && returned[3] != 0;
    options.areaShot = physical.areaShot && returned[4] != 0;
    options.castEnemy = physical.castEnemy && returned[5] != 0;
    options.castAlly = physical.castAlly && returned[6] != 0;
    options.castHex = physical.castHex && returned[7] != 0;
    options.attackReturn = physical.attackReturn && returned[8] != 0;
}

const StackActionOptions *ActionSwitcher::GetActiveOptions(const H3CombatCreature *stack) const
{
    H3CombatManager *manager = P_CombatManager->Get();
    if (!state || !switcherDlgPanel || !manager || manager->dlg != switcherDlgPanel->dialog ||
        manager->activeStack != stack || switcherDlgPanel->currentStack != stack ||
        switcherDlgPanel->checkingAvailability || switcherDlgPanel->checkingMovement ||
        !switcherDlgPanel->IsEligibleForCurrentPlayer(manager))
        return nullptr;
    return &switcherDlgPanel->actionOptions;
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

_LHF_(ActionSwitcher::Y_ExecuteMeleeCommand)
{
    H3CombatManager *manager = P_CombatManager->Get();
    const auto stack = reinterpret_cast<H3CombatCreature *>(c->esi);
    if (instance && instance->IsEnabled() && manager &&
        manager->currentMonSide >= 0 && manager->currentMonSide < 2 &&
        manager->currentMonIndex >= 0 && manager->currentMonIndex < 21 &&
        &manager->stacks[manager->currentMonSide][manager->currentMonIndex] == stack &&
        manager->action == eCombatAction::WALK_ATTACK)
    {
        // The command is sent by the game to both peers. Its melee path must not
        // fall back to shooting after movement, regardless of the local UI mode.
        // Preserve the shooter flag: doubleAttack still applies only to shots.
        // MakeAttack clears activeStack before this branch; use its stable IDs.
        c->return_address = 0x4458E2;
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
    if (actionDepth)
        return THISCALL_2(BOOL8, h->GetDefaultFunc(), stack, target);
    const eAction action = instance ? instance->GetActiveAction(stack) : eAction::NONE;
    const StackActionOptions *options = instance ? instance->GetActiveOptions(stack) : nullptr;
    if (options && !(action == eAction::AREA_SHOT ? options->areaShot : options->shoot))
        return FALSE;
    if (action == eAction::MELEE || action == eAction::MOVE || action == eAction::CAST ||
        action == eAction::RETURN || action == eAction::NO_RETURN)
        return FALSE;
    return THISCALL_2(BOOL8, h->GetDefaultFunc(), stack, target);
}

BOOL8 __stdcall ActionSwitcher::Y_CanCast(HiHook *h, H3CombatCreature *stack, INT hex)
{
    if (actionDepth)
        return THISCALL_2(BOOL8, h->GetDefaultFunc(), stack, hex);
    const eAction action = instance ? instance->GetActiveAction(stack) : eAction::NONE;
    if (action != eAction::NONE && action != eAction::CAST)
        return FALSE;
    const StackActionOptions *options = instance ? instance->GetActiveOptions(stack) : nullptr;
    if (options && hex >= 0 && hex < 187)
    {
        H3CombatCreature *target = P_CombatManager->squares[hex].GetCreature();
        const BOOL allowed = !target || target->numberAlive <= 0 ? options->castHex :
            target->GetSide() == stack->GetSide() ? options->castAlly : options->castEnemy;
        if (!allowed)
            return FALSE;
    }
    return THISCALL_2(BOOL8, h->GetDefaultFunc(), stack, hex);
}

INT __stdcall ActionSwitcher::Y_GetCursorAction(HiHook *h, H3CombatManager *manager, INT hex)
{
    const int cursor = THISCALL_2(INT, h->GetDefaultFunc(), manager, hex);
    const eAction action = instance ? instance->GetActiveAction(manager->activeStack) : eAction::NONE;
    const StackActionOptions *options = instance ? instance->GetActiveOptions(manager->activeStack) : nullptr;
    if (options && cursor == 7 && !options->melee)
        return 0;
    // Native cursor values: 1 walk, 2 fly, 3/15 shoot, 7 melee, 20 creature spell.
    // Preserve native reachability, direction, target and command parameters.
    if (action == eAction::MOVE && cursor != 1 && cursor != 2 && cursor != 22)
        return 0;
    if (action == eAction::SHOOT && cursor != 3 && cursor != 15)
        return 0;
    if (action == eAction::CAST && cursor != 20)
    {
        // Casting on living creatures does not compete with walking on an empty
        // hex. Keep the explicit CAST/MOVE choice only for reachable corpse casts.
        const bool movement = cursor == 1 || cursor == 2 || cursor == 22;
        if (!movement || instance->switcherDlgPanel->actionOptions.castHex)
            return 0;
    }
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
    _PI->WriteLoHook(0x445829, Y_ExecuteMeleeCommand);
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
