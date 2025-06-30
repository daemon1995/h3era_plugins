#include "ActionSwitcher.h"
namespace switcher
{

constexpr int DEFAULT_TRIGGER_ID = 200000;
int trggerId = DEFAULT_TRIGGER_ID;

DllExport bool SetState(const bool state)
{

    return ActionSwitcher::Get().SetPluginState(state);
}
SwitcherDlgPanel *SwitcherDlgPanel::instance = nullptr;
_LHF_(CombatStackAccpetsTurn)
{
    // this is init button hook

    // here we collect base data and assign to xVars

    // get xVars ptr
    Era::TXVars *xVarsBeforeCall = Era::GetArgXVars();
    // assign data to pass
    /*           (*xVarsBeforeCall)[0] = stackId;
               (*xVarsBeforeCall)[1] = creautreId;
               (*xVarsBeforeCall)[2] = data1;
               (*xVarsBeforeCall)[3] = data2;
               (*xVarsBeforeCall)[4] = data3;*/

    // call erm function
    Era::FireErmEvent(trggerId);

    // get chnged args after handling by all external erm/plugins
    Era::TXVars *xVars = Era::GetRetXVars();

    // data1 = (*xVars)[2];
    // data2 = (*xVars)[3];
    // data3 = (*xVars)[4];

    // setting button/data stuff next

    return EXEC_DEFAULT;
}

void HooksInit()
{
    _PI->WriteLoHook(0x423423, CombatStackAccpetsTurn);
    Era::AllocErmFunc("ErmTriggerFromPlugin", trggerId);
}

void __stdcall ErmTriggerFromPlugin(Era::TEvent *e)
{
}
constexpr int NETMSG_ID = 1989;
#define NONE 0

void SendNetData_AttackType(int type)
{

    // if Network combat
    if (IntAt(0x69959C) && type != NONE)
    {
        H3CombatManager *bm = P_CombatManager->Get();
        H3Game *game = P_Game->Get();
        if (bm->isHuman[0] >= 0 && bm->isHuman[1] >= 0)
        {
            //  if (o_GameMgr->IsPlayerHuman(cmbMgr->playerID[1])) {
            //     if (o_GameMgr->IsPlayerHuman(cmbMgr->playerID[0])) {
            int activeID = game->GetPlayerID();
            int meID = bm->heroOwner[bm->currentActiveSide];

            if (activeID == meID)
            {
                _dword_ targetPlayerID = DwordAt(0x697790 - 4 * bm->currentActiveSide);

                INT32 netData[6];
                netData[0] = -1;              // recipient_id
                netData[1] = 0;               // field_04
                netData[2] = NETMSG_ID;       // msg_id
                netData[3] = sizeof(netData); // size
                netData[4] = 0;               // field_10
                netData[5] = type;            // shortData

                // SendData()
                FASTCALL_4(INT32, 0x5549E0, (_dword_)&netData, targetPlayerID, 0, 1);
                //       }
                //   }
            }
        }
    }
}

ActionSwitcher::ActionSwitcher() : IGamePatch(_PI)
{
    CreatePatches();
}

_LHF_(ActionSwitcher::Y_BattleMgr_WOG_HarpyReturn)
{
    // if (currentType == NO_RETURN) {
    //     arrayTypesClear();
    c->return_address = 0x75E0B4;
    return NO_EXEC_DEFAULT;
    //  }

    return EXEC_DEFAULT;
}

char __stdcall SwitcherDlgPanel::H3CombatDlg_BeforeBaseDlgRedraw(HiHook *h)
{
    if (instance)
    {
        if (auto it = P_CombatManager->dlg->GetH3DlgItem(BUTTON_ID))
        {
            instance->Draw();
        }
    }

    return CDECL_0(char, h->GetDefaultFunc());
}

inline BOOL IsMouseOverDlgItem(H3Msg *msg, H3DlgItem *item)
{
    const int x = msg->GetX() - item->GetAbsoluteX();
    const int y = msg->GetY() - item->GetAbsoluteY();

    return x >= 0 && x < item->GetWidth() && y >= 0 && y < item->GetHeight();
}

inline BOOL IsKeyHeld(const int key)
{

    return GetActiveWindow() == H3Hwnd::Get() && (STDCALL_1(SHORT, PtrAt(0x63A294), key) & 0x800);
}
INT __stdcall SwitcherDlgPanel::H3CombatDlg_Procedure(HiHook *h, H3CombatManager *cmbMgr, H3Msg *msg)
{
    const BOOL isAltPressed = msg->IsAltPressed();
    BOOL needRedraw = false;
    BOOL assumedPanelVisability = false;
    INT result = THISCALL_2(INT, h->GetDefaultFunc(), cmbMgr, msg);

    //  if (IsKeyHeld(VK_LMENU))
    {
        //    assumedPanelVisability = true;
        // Era::ExecErmCmd("IF:L^ %y1 %y2^");
    }
    // if (isAltPressed)
    {
        //   msg->flags = eMsgFlag::SHIFT;
    }

    if (instance->activateByKey && msg->GetKey() == eVKey::H3VK_CTRL)
    {
        switch (msg->command)
        {

        case eMsgCommand::KEY_DOWN:
            instance->panelShownByKey = true;

            if (instance->SetVisible(1))
            {
                //    instance->Draw();
                //    instance->Refresh();
                Era::y[1] = assumedPanelVisability;
                //  Era::ExecErmCmd("IF:L^ %y1 %y2^");

                cmbMgr->Refresh();
            }
            break;
        case eMsgCommand::KEY_UP:

            instance->panelShownByKey = false;

            if (!instance->panelShownByHover && instance->SetVisible(0))
            {
                //  Era::ExecErmCmd("IF:L^ %y1 %y2^");
                cmbMgr->Refresh();
            }
            break;
        case eMsgCommand::KEY_HELD:
            instance->panelShownByKey = true;

            if (1)
            {
            }
            // cmbMgr->Refresh();

            //  Era::ExecErmCmd("IF:L^ %y1 %y2^");

            break;

        case eMsgCommand::MOUSE_OVER:

            // don't hadnle buttions when already shown
            if (instance->visible)
            {
                break;
            }
            break;

            if (auto bttn = instance->actionSwitchButton)
            {

                const BOOL isOverButton = IsMouseOverDlgItem(msg, bttn);
                assumedPanelVisability = isOverButton;

                if (!isOverButton)
                {
                    if (auto pcx = instance->backDlgPcx16)
                    {

                        const BOOL isOverPcx = IsMouseOverDlgItem(msg, pcx);
                        if (isOverPcx)
                        {
                            assumedPanelVisability = true;
                            break;
                        }
                    }
                }
                if (instance->SetVisible(assumedPanelVisability))
                {
                    //    instance->Draw();
                    //    instance->Refresh();
                    Era::y[1] = assumedPanelVisability;
                    //  Era::ExecErmCmd("IF:L^ %y1 %y2^");

                    cmbMgr->Refresh();
                }
            }
            break;
        default:
            break;
        }
    }
    if (!instance->panelShownByKey)
    {
        if (instance->activateByHover && msg->command == eMsgCommand::MOUSE_OVER)
        {

            if (auto bttn = instance->actionSwitchButton)
            {
                // check if mouse is over button
                const BOOL isOverButton = IsMouseOverDlgItem(msg, bttn);
                assumedPanelVisability = isOverButton;

                if (!isOverButton)
                {
                    if (auto pcx = instance->backDlgPcx16)
                    {

                        const BOOL isOverPcx = IsMouseOverDlgItem(msg, pcx);
                        if (isOverPcx)
                        {
                            assumedPanelVisability = true;
                        }
                    }
                }
                if (instance->SetVisible(assumedPanelVisability))
                {
                    //    instance->Draw();
                    //    instance->Refresh();
                    Era::y[1] = assumedPanelVisability;
                    //  Era::ExecErmCmd("IF:L^ %y1 %y2^");

                    //     cmbMgr->Refresh();
                }
            }
        }
    }

    // if (needRedraw)
    //{
    //     cmbMgr->Refresh();
    // }

    return result;
}
INT __stdcall SwitcherDlgPanel::H3CombatDlg_BottoomPanel_HoverProcedure(HiHook *h, H3WindowManager *wndMgr, H3Msg *msg)
{

    // INT result = THISCALL_2(INT, h->GetDefaultFunc(), cmbMgr, msg);
    //    auto item = THISCALL_3(H3DlgItem *, 0x5FF9A0, wndMgr->lastDlg, msg->GetX(), msg->GetY());
    if (msg->IsMouseOver() && instance->actionSwitchButton->GetID())
    {
        Era::y[1] = msg->GetX();
        Era::y[2] = msg->GetY();
        Era::ExecErmCmd("IF:L^ %y1 %y2^");
    }

    INT result = THISCALL_2(INT, h->GetDefaultFunc(), wndMgr, msg);

    return result;
}

inline BOOL8 IsStackAutoControlledByAI(const H3CombatManager *mgr, const H3CombatCreature *stack)
{

    return THISCALL_2(BOOL8, 0x474520, mgr, stack);
}

inline BOOL StackCanAttackAndReturn(const H3CombatCreature *stack)
{
    const int creatureType = stack->type;

    return creatureType == IntAt(0x75E05B)    //  harpy type
           || creatureType == IntAt(0x75E060) // harpy hag type
           || creatureType == IntAt(0x75E064) // darkness dragon type
           || CDECL_1(BOOL, 0x71E433, stack); // creature stack exp bonus check
}
void SwitcherDlgPanel::InitNewStackAttackType(const BOOL redraw)
{
    //  arrayTypesClear();
    // updateAttackTypeButton(NONE, redraw);

    // функция сетевой отсылки текущего действия
    SendNetData_AttackType(NONE);
    auto cmbMgr = P_CombatManager->Get();

    if (!cmbMgr->tacticsPhase)
    {
        creeatureActions.clear();

        if (auto stack = cmbMgr->activeStack)
        {
            const int meID = P_Game->Get()->GetPlayerID();
            if (cmbMgr->heroOwner[cmbMgr->currentMonSide] != meID)
            {
                return;
            }

            if (cmbMgr->autoCombat || P_AutoSolo || IsStackAutoControlledByAI(cmbMgr, stack))
            {
                return;
            }

            if (stack->type >= eCreature::CATAPULT || stack->type <= eCreature::ARROW_TOWER)
            {
                return;
            }
            creeatureActions.reserve(10);

            BOOL stackCanCast = false;
            for (size_t i = 0; i < 187; ++i)
            {
                if (stack->CanCastSpellAtEmptyHex(i))
                {
                    stackCanCast = true;
                    creeatureActions.insert(eAction::CAST);
                    creeatureActions.insert(eAction::MOVE);
                    break;
                }
            }

            const BOOL stackCanShoot = THISCALL_2(BOOL, 0x0442610, stack, nullptr);
            if (stackCanCast)
            {
            }

            if (stack->faerieDragonSpell && stack->info.spellCharges > 0)
            {
                // can shoot
                if (!stackCanShoot)
                {
                    creeatureActions.insert(eAction::CAST);
                }
                creeatureActions.insert(eAction::MELEE);

                //    creeatureActions.insert(eAction::MELEE);
            }
            if (stackCanShoot)
            {
                creeatureActions.insert(eAction::SHOOT);
                creeatureActions.insert(eAction::MELEE);
            }
            if (Era::IsCommanderId(stack->type))
            {

                // add NPC Handler
            }
            if (StackCanAttackAndReturn)
            {
                creeatureActions.insert(eAction::RETURN);
                creeatureActions.insert(eAction::NO_RETURN);
            }
        }
    }

    // updateAttackTypeButton(currentType, redraw);
    SendNetData_AttackType(0);
}

void SwitcherDlgPanel::UpdateStackAttackType(const eAction selectedAction)
{
}

_LHF_(SwitcherDlgPanel::H3CombatManager_GetControl)
{

    instance->InitNewStackAttackType();
    instance->UpdateStackAttackType();

    return EXEC_DEFAULT;
}
_LHF_(SwitcherDlgPanel::H3CombatManager_AfterHeroCastSpell)
{

    instance->InitNewStackAttackType();
    instance->UpdateStackAttackType();

    return EXEC_DEFAULT;
}
void SwitcherDlgPanel::Refresh()
{
    if (backDlgPcx16)
    {
        backDlgPcx16->Refresh();
        backDlgPcx16->ParentRedraw();
    }

    for (PanelSwitchItem &i : switchItems)
    {
        if (auto button = i.button)
        {
            button->Refresh();
            button->ParentRedraw();
        }
    }
}

void SwitcherDlgPanel::Draw()
{
    if (visible)
    {
        if (backDlgPcx16)
        {
            backDlgPcx16->Draw();
        }

        for (PanelSwitchItem &i : switchItems)
        {
            if (auto button = i.button)
            {
                button->Draw();
            }
        }
    }
    else
    {
        Refresh();
    }
}
BOOL SwitcherDlgPanel::SetVisible(const BOOL visible)
{

    if (creeatureActions.empty())
    {
        //  return false;
    }

    if (this->visible != visible)
    {

        if (backDlgPcx16)
        {
            visible ? backDlgPcx16->ShowActivate() : backDlgPcx16->HideDeactivate();
        }

        for (PanelSwitchItem &i : switchItems)
        {
            if (auto button = i.button)
            {
                visible ? button->ShowActivate() : button->HideDeactivate();
            }
        }
        this->visible = visible;
        return true;
    }

    return false;
}
INT __fastcall SwitcherDlgPanel::SwitchPanelButtonProcedure(H3Msg *msg)
{
    if (msg->IsMouseOver())
    {
        Era::ExecErmCmd("IF:L^%y1^");
    }

    return 0;
}
_LHF_(Y_BattleMgr_ProcessAction_LMC)
{
    auto msg = *reinterpret_cast<H3Msg **>(c->ebp + 0x8); // rein
    if (msg->IsMouseOver())
    {

        Era::y[1] = msg->itemId;
        Era::ExecErmCmd("IF:L^%y1^");
        Era::ExecErmCmd("IF:L^^");
    }

    return EXEC_DEFAULT;
}

void __stdcall ActionSwitcher::Y_AddChooseAttackButton(HiHook *h, H3Vector<H3DlgItem *> *combatBottomPanelItems,
                                                       H3DlgItem **end, const int count, H3DlgItem *lastItem)
{
    THISCALL_4(void, h->GetDefaultFunc(), combatBottomPanelItems, end, count, lastItem);

    // if (auto vec = reinterpret_cast<H3Vector<H3DlgItem *> *>(c->esi))
    {
        Get().switcherDlgPanel = new SwitcherDlgPanel(combatBottomPanelItems, Get().text, SwitcherDlgPanel::BUTTON_ID);
    }

    // H3CombatDlg *dlg = reinterpret_cast<H3CombatDlg *>(c->edi);

    //  return EXEC_DEFAULT;
}
SwitcherDlgPanel::SwitcherDlgPanel(H3Vector<H3DlgItem *> *combatBottomPanelItems, const SwitcherText &text,
                                   const int firstItemId)
{

    constexpr int buttonHeight = 36;
    constexpr int buttonX = 658;
    actionSwitchButton = H3DlgCustomButton::Create(buttonX, 4, 44, buttonHeight, firstItemId, text.ButtonName,
                                                   SwitchPanelButtonProcedure, 0, 1);
    // устанавливаем описания
    actionSwitchButton->SetHints(text.ButtonHints[NONE], text.ButtonRmc[NONE], false);
    combatBottomPanelItems->AddOne(actionSwitchButton);
    actionSwitchButton->AddHotkey(eVKey::H3VK_ALT);

    constexpr int backWidth = 44 * 4 + 5 * 3;
    constexpr int backPanelX = buttonX - 51;
    constexpr int backPanelY = -54;
    H3LoadedPcx16 *backPcx = H3LoadedPcx16::Load(BACK_PANEL_NAME);

    H3DlgPcx16 *buttonsBack =
        H3DlgPcx16::Create(backPanelX, backPanelY, backPcx->width, backPcx->height + 2, firstItemId + 1, 0);

    buttonsBack->SetPcx(backPcx);
    backDlgPcx16 = buttonsBack;
    combatBottomPanelItems->AddOne(buttonsBack);

    for (size_t i = 0; i < 4; i++)
    {
        const int frameId = (i + 1) * 3;
        H3DlgDefButton *actionButton =
            H3DlgDefButton::Create(backPanelX + 4 + i * 47, backPanelY + 2, firstItemId + 2 + i, text.ButtonName,
                                   frameId, frameId + 1, false, eVKey::H3VK_1 + i);

        // actionButton->HideDeactivate();
        //  add to dlg
        combatBottomPanelItems->AddOne(actionButton);

        // and save ptr and state
        switchItems.emplace_back(PanelSwitchItem{true, false, actionButton});
    }
    // hide items by default
    visible = true;
    SetVisible(false);
    // create native panel clickable button

    //    bttn->short_tip_text = ButtonHints[currentType];
    //  bttn->full_tip_text = ButtonRmc[currentType];

    instance = this;
}
SwitcherDlgPanel::~SwitcherDlgPanel()
{
    instance = nullptr;
}
void ActionSwitcher::CreatePatches() noexcept
{

    if (!m_isInited)
    {
        m_isInited = true;

        // _pi

        interfaceChangePatches.reserve(100);
        // id = 2003 (настройки)
        interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B3D9 + 1, 4));
        interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B3D3 + 1, 44));
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B3C9 + 1, (int)"icm003q.def"));

        // id = 2001 (сдаться)
        interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B2CF + 1, 51));
        interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B2CB + 1, 44));
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B2BF + 1, (int)"icm001q.def"));

        // id = 2002 (убежать)
        interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B354 + 1, 98));
        interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B34E + 1, 44));
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B344 + 1, (int)"icm002q.def"));

        // id = 2004 (автобитва)
        interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B45E + 1, 145));
        interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B458 + 1, 44));
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B44E + 1, (int)"icm004q.def"));

        // id = 2300 (тактика - следующее существо)
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46BC1F + 1, 192));
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B800 + 1, 188));

        // id = 30722 (тактика - начать битву)
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46BC8F + 1, 398));
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46BC88 + 1, 188));
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46BC7C + 1, (int)"icm012q.def"));

        // id = 2005 (окно лога)
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B807 + 1, 194));
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B800 + 1, 390));

        // id = 2006 (стрелка лога вверх)
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B87E + 1, 590));

        // id = 2007 (стрелка лога вниз)
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B917 + 1, 590));

        // id = 2008 (колдовство)
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B4E6 + 1, 611));
        interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B4E0 + 1, 44));
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B4D6 + 1, (int)"icm005q.def"));

        // id = 2009 (ждать)
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B56E + 1, 705));
        interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B568 + 1, 44));
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B55E + 1, (int)"icm006q.def"));

        // id = 2010 (в защите)
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B5F6 + 1, 752));
        interfaceChangePatches.emplace_back(_PI->WriteByte(0x46B5F0 + 1, 44));
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x46B5E6 + 1, (int)"icm007q.def"));

        // id = 2 (чат)
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x471ED9 + 1, 195));
        interfaceChangePatches.emplace_back(_PI->WriteDword(0x471ECF + 1, 390));
        // добавление кнопки на экран битвы
        _pi->WriteHiHook(0x46B664, THISCALL_, Y_AddChooseAttackButton);
        _pi->WriteHiHook(0x04945E2, CDECL_, SwitcherDlgPanel::H3CombatDlg_BeforeBaseDlgRedraw);
        _pi->WriteHiHook(0x04746B0, THISCALL_, SwitcherDlgPanel::H3CombatDlg_Procedure);

        //  _pi->WriteHiHook(0x0474E42, THISCALL_, SwitcherDlgPanel::H3CombatDlg_BottoomPanel_HoverProcedure);

        // при установке активного стека (обновить кнопку)
        _pi->WriteLoHook(0x477D98, SwitcherDlgPanel::H3CombatManager_GetControl);
        _pi->WriteLoHook(0x478997, SwitcherDlgPanel::H3CombatManager_AfterHeroCastSpell);

        // _pi->WriteLoHook(0x0474764, Y_BattleMgr_ProcessAction_LMC);

        interfaceChangePatches.shrink_to_fit();

        behaviorChangePatches.reserve(100);

        //// взаимодействия с кнопокой по ЛКМ и ПКМ
        //_PI->WriteHiHook(0x474714, CALL_, EXTENDED_, THISCALL_, Y_BattleMgr_MouseClickRMC_OnButton);
        //_PI->WriteLoHook(0x474764, Y_BattleMgr_ProcessAction_LMC);

        //// установка событий обработки типа действия
        //_PI->WriteLoHook(0x442635, Y_BattleMgr_CanStackShoot);
        //_PI->WriteLoHook(0x75E0E7, Y_BattleMgr_WOG_HarpyReturn);
        //_PI->WriteHiHook(0x41FA10, SPLICE_, EXTENDED_, THISCALL_, Y_BattleMgr_CanCast);

        //// проверка на возможность стрельбы уже во время исполнения атаки монстром
        //_PI->WriteCodePatch(0x445829, "%n", 12); // 12 nops
        //_PI->WriteLoHook(0x445829, Y_BattleMgr_MakeAttack_isStackCanShoot);

        //// нажатие на кнопку автоматической битвы
        //_PI->WriteLoHook(0x47478A, Y_ClickAutoBattleButton_A_id2004);

        //// при установке активного стека (обновить кнопку)
        //_PI->WriteLoHook(0x477D98, Y_BattleMgr_SetActiveStack);

        //// после каста героем (обновить кнопку)
        //_PI->WriteLoHook(0x478997, Y_LoHook_BM_AfterHeroSpellCast);

        //// восстанавливаем оригинальное действие кнопки "В защите"
        //// на которое ставит свой хук WOG
        //_PI->WriteHexPatch(0x47265C, "B8 08000000 E9 64FFFFFF");

        //// получение данных по сети
        //// send 0x4787BC
        //// get 0x473D27
        //_PI->WriteHiHook(0x473D27, CALL_, EXTENDED_, FASTCALL_, Y_HiHook_BM_GetNetData);
    }
}

BOOL ActionSwitcher::SetPluginState(const bool state) noexcept
{
    if (P_CombatManager->dlg == nullptr && state != this->state)
    {

        this->state = state;
        for (auto &patch : interfaceChangePatches)
        {
            state ? patch->Apply() : patch->Undo();
        }
        for (auto &patch : behaviorChangePatches)
        {
            state ? patch->Apply() : patch->Undo();
        }
        return true;
    }
    return false;
}

ActionSwitcher &ActionSwitcher::Get()
{
    static ActionSwitcher instance;
    return instance; // TODO: insert return statement here
}

#define NONE 0
#define MELEE 1
#define SHOOT 2
#define CAST 3
#define MOOVE 4
#define RETURN 5
#define NO_RETURN 6
void SwitcherText::Load() noexcept
{

    ButtonName = EraJS::read("choose_attack_type.name");

    ButtonHints[NONE] = EraJS::read("choose_attack_type.hint.none");
    ButtonRmc[NONE] = EraJS::read("choose_attack_type.rmc.none");

    ButtonHints[MELEE] = EraJS::read("choose_attack_type.hint.melee");
    ButtonRmc[MELEE] = EraJS::read("choose_attack_type.rmc.melee");

    ButtonHints[SHOOT] = EraJS::read("choose_attack_type.hint.shoot");
    ButtonRmc[SHOOT] = EraJS::read("choose_attack_type.rmc.shoot");

    ButtonHints[CAST] = EraJS::read("choose_attack_type.hint.cast");
    ButtonRmc[CAST] = EraJS::read("choose_attack_type.rmc.cast");

    ButtonHints[MOOVE] = EraJS::read("choose_attack_type.hint.moove");
    ButtonRmc[MOOVE] = EraJS::read("choose_attack_type.rmc.moove");

    ButtonHints[RETURN] = EraJS::read("choose_attack_type.hint.return");
    ButtonRmc[RETURN] = EraJS::read("choose_attack_type.rmc.return");

    ButtonHints[NO_RETURN] = EraJS::read("choose_attack_type.hint.no_return");
    ButtonRmc[NO_RETURN] = EraJS::read("choose_attack_type.rmc.no_return");

    AutoBattleQuestion = EraJS::read("choose_attack_type.autobattle_question");
}

SwitcherText::SwitcherText()
{
    Load();
}

BOOL SwitcherDlgPanel::PanelSwitchItem::SetState(BOOL32 _state)
{
    if (state != _state)
    {

        _state ? button->Enable() : button->Disable();

        return true;
    }

    return 0;
}
void SwitcherDlgPanel::Init(H3Vector<H3DlgItem *> *panelItems, const int firstItemId)
{
}

} // namespace switcher
