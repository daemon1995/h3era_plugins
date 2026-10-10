#include "CombatCreatureHealthBar.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <initializer_list>

void __stdcall ShowHealthBarDlg()
{
    cmbhints::CombatHints::ShowHealthBarDlg();
}
BOOL *__stdcall HealthBarIsEnabledAddress()
{
    return &cmbhints::CombatHints::Get().settings.isEnabled;
}

namespace cmbhints
{
constexpr float HP_LABEL_HEIGHT = 4.0f;
constexpr float HP_LABEL_FILL = 0.4f;
constexpr float HP_LABEL_LOSS = 0.975f;
constexpr float HP_LABEL_SATURATION = 0.8f; // idk.mb set 1
constexpr int HP_LABEL_MAX_OFFSET = 18;
namespace
{
constexpr int COMBAT_SIDES = 2;
constexpr int COMBAT_STACKS_PER_SIDE = 21;
constexpr int COMBAT_STACK_COUNT = COMBAT_SIDES * COMBAT_STACKS_PER_SIDE;

struct StackHealthSnapshot
{
    INT32 numberAlive;
    INT32 healthLost;
    INT32 maxHp;
    std::int64_t totalHp;
};

struct PendingDamageSnapshot
{
    bool active;
    StackHealthSnapshot current;
};

struct DamageAnimationContext
{
    H3CombatManager *manager;
    bool ownsPending;
    PendingDamageSnapshot snapshots[COMBAT_STACK_COUNT];
    DamageAnimationContext *previous;
};

H3CombatManager *pendingDamageManager = nullptr;
PendingDamageSnapshot pendingDamage[COMBAT_STACK_COUNT] = {};
DamageAnimationContext *activeDamageAnimation = nullptr;

std::int64_t TotalHitPoints(INT32 numberAlive, INT32 healthLost, INT32 maxHp) noexcept
{
    return static_cast<std::int64_t>(numberAlive) * maxHp - healthLost;
}

StackHealthSnapshot CaptureStackHealth(const H3CombatCreature *stack) noexcept
{
    const INT32 maxHp = stack->MaxHitPoints();
    return {stack->numberAlive, stack->healthLost, maxHp,
            TotalHitPoints(stack->numberAlive, stack->healthLost, maxHp)};
}

int FindStackIndex(const H3CombatManager *manager, const H3CombatCreature *stack) noexcept
{
    if (!manager || !stack)
        return -1;

    for (int side = 0; side < COMBAT_SIDES; ++side)
    {
        for (int index = 0; index < COMBAT_STACKS_PER_SIDE; ++index)
        {
            if (&manager->stacks[side][index] == stack)
                return side * COMBAT_STACKS_PER_SIDE + index;
        }
    }
    return -1;
}

const StackHealthSnapshot *FindDamageAnimationSnapshot(const H3CombatManager *manager,
                                                       const H3CombatCreature *stack) noexcept
{
    const auto context = activeDamageAnimation;
    if (!context || context->manager != manager || !stack)
        return nullptr;

    const int stackIndex = FindStackIndex(manager, stack);
    if (stackIndex < 0 || !context->snapshots[stackIndex].active)
        return nullptr;
    return &context->snapshots[stackIndex].current;
}

int __stdcall BattleStack_DoPhysicalDamage(HiHook *hook, H3CombatCreature *stack, int damage)
{
    auto &settings = CombatHints::Get().settings;
    H3CombatManager *manager = H3CombatManager::Get();
    const int stackIndex = FindStackIndex(manager, stack);
    const bool shouldTrack = settings.isEnabled && damage > 0 && stackIndex >= 0;
    bool startedPending = false;
    StackHealthSnapshot before = {};

    if (shouldTrack)
    {
        before = CaptureStackHealth(stack);
        if (pendingDamageManager != manager)
        {
            std::memset(pendingDamage, 0, sizeof(pendingDamage));
            pendingDamageManager = manager;
        }

        if (!pendingDamage[stackIndex].active)
        {
            pendingDamage[stackIndex].current = before;
            pendingDamage[stackIndex].active = true;
            startedPending = true;
        }
    }

    const int result = THISCALL_2(int, hook->GetDefaultFunc(), stack, damage);

    if (shouldTrack)
    {
        const auto after = CaptureStackHealth(stack);
        if (after.totalHp >= before.totalHp)
        {
            if (startedPending)
                pendingDamage[stackIndex].active = false;
        }
        else
        {
            // DoPhysicalDamage is the hit event. Use the post-hit state for the
            // redraw that follows it, including later hits in the same animation.
            pendingDamage[stackIndex].current = after;
            if (activeDamageAnimation && activeDamageAnimation->manager == manager &&
                activeDamageAnimation->snapshots[stackIndex].active)
            {
                activeDamageAnimation->snapshots[stackIndex].current = after;
            }
        }
    }
    return result;
}

char __stdcall BattleMgr_DrawAction_Play(HiHook *hook, H3CombatManager *manager, int animationId,
                                         int showCreatureDamaged)
{
    const bool isEnabled = CombatHints::Get().settings.isEnabled;
    if (!manager || pendingDamageManager != manager)
        return THISCALL_3(char, hook->GetDefaultFunc(), manager, animationId, showCreatureDamaged);
    if (!isEnabled)
    {
        std::memset(pendingDamage, 0, sizeof(pendingDamage));
        pendingDamageManager = nullptr;
        return THISCALL_3(char, hook->GetDefaultFunc(), manager, animationId, showCreatureDamaged);
    }

    DamageAnimationContext context{};
    context.manager = manager;
    context.previous = activeDamageAnimation;
    context.ownsPending = !context.previous || context.previous->manager != manager;
    for (int index = 0; index < COMBAT_STACK_COUNT; ++index)
    {
        if (context.previous && context.previous->manager == manager && context.previous->snapshots[index].active)
            context.snapshots[index] = context.previous->snapshots[index];
        else
            context.snapshots[index] = pendingDamage[index];
    }

    activeDamageAnimation = &context;
    const char result = THISCALL_3(char, hook->GetDefaultFunc(), manager, animationId, showCreatureDamaged);
    activeDamageAnimation = context.previous;

    if (context.ownsPending)
    {
        for (int index = 0; index < COMBAT_STACK_COUNT; ++index)
        {
            if (!context.snapshots[index].active)
                continue;

            pendingDamage[index].active = false;
        }
    }
    return result;
}
} // namespace
CombatHints *CombatHints::instance = nullptr;
CombatHints::CombatHints() : IGamePatch(globalPatcher->CreateInstance("EraPlugin.CombatHints.daemon_n"))
{
    CreatePatches();

    settings.reset();
}

CombatHints &CombatHints::Get()
{
    if (instance == nullptr)
        instance = new CombatHints();
    return *instance;
}

void CombatHints::CreatePatches() noexcept
{
    if (!m_isInited)
    {

        this->_pi->WriteLoHook(0x43E38B, BeforeBattleStackHintDraw);
        this->_pi->WriteHiHook(0x443DB0, THISCALL_, BattleStack_DoPhysicalDamage);
        this->_pi->WriteHiHook(0x468570, THISCALL_, BattleMgr_DrawAction_Play);
        //_PI->WriteLoHook(0x4682C0, BattleOptionsDlg);
        // blocked for new options dlg
        // this->_pi->WriteHiHook(0x4682C0, THISCALL_, BattleOptionsDlg_Show);

        // Maybe isn't needed
        //	_PI->WriteLoHook(0x4746BD, BattleMgr_ProcessActionL);

        m_isInited = true;
    }
}

_LHF_(CombatHints::BattleMgr_ProcessActionL)
{
    H3Msg *msg = reinterpret_cast<H3Msg *>(c->eax);
    auto &settings = Get().settings;
    if (msg && settings.isHeld)
    {

        auto *combatHints = &Get();

        if (msg->GetKey() == settings.scanCode && (msg->IsKeyDown() || msg->IsKeyHeld()) &&
            combatHints->needRedraw > -1)
        {
            combatHints->needRedraw = true;
        }

        if (msg->IsKeyPress() && msg->GetKey() == settings.scanCode)
        {
            // combatHints->needRedraw = true;
            THISCALL_1(void, 0x477C00, H3CombatManager::Get());

            combatHints->needRedraw = 0;
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(CombatHints::BeforeBattleStackHintDraw)
{
    H3CombatCreature *stack = reinterpret_cast<H3CombatCreature *>(c->ebx);
    auto &settings = Get().settings;
    const H3CombatManager *cmbMgr = H3CombatManager::Get();
    const StackHealthSnapshot *actionSnapshot = FindDamageAnimationSnapshot(cmbMgr, stack);
    const bool deferDamage = actionSnapshot && actionSnapshot->numberAlive > 0 && actionSnapshot->maxHp > 0 &&
                             TotalHitPoints(stack->numberAlive, stack->healthLost, stack->MaxHitPoints()) <
                                 actionSnapshot->totalHp;

    if (stack && (stack->numberAlive || deferDamage) && settings.isEnabled)
    {

        // GetKeyState Call
        if (settings.isHeld && !(STDCALL_1(SHORT, PtrAt(0x63A294), settings.vKey) & 0x8000))
        // if key is required and isnt' pressed
        {
            return EXEC_DEFAULT; // return
        }

        // if position is out of the original label - when 0 then pos is same
        // also reverse value from the settings cause yPos is ascending
        if (const int labelYOffset = static_cast<int>(settings.height) * -1)
        {

            const int nativeLabelX = c->edi;
            const int nativeLabelY = c->esi;

            const int newLabelX = nativeLabelX;

            const int newLabelY = nativeLabelY + labelYOffset; // add border

            const BOOL labelDrawn =
                THISCALL_4(BOOL, 0x495460, cmbMgr, cmbMgr->cmNumWinPcxLoaded, newLabelX, newLabelY); // draw label pcx
            const int maxHp = deferDamage ? actionSnapshot->maxHp : stack->MaxHitPoints();

            // if origial hint was drawn this one will appear too
            if (labelDrawn && maxHp)
            {
                const int labelHeight = cmbMgr->cmNumWinPcxLoaded->height;

                // difference between
                const int protrusionSize =
                    std::abs(labelYOffset); // std::abs(nativeLabelY + labelYOffset - nativeLabelY);

                // const int newLabelHeight = Clamp(0, protrusionSize, labelHeight);
                const int drawHeightMax = labelHeight - 2;
                const int labelBorderHeight = protrusionSize >= labelHeight ? 2 : 1;

                int drawHeight = Clamp(0, protrusionSize - labelBorderHeight, drawHeightMax);

                // if draw area displayed at all
                if (drawHeight > 0)
                {

                    const int dlgX = cmbMgr->dlg->GetX();
                    const int dlgY = cmbMgr->dlg->GetY();
                    // count if it makes sense to draw health
                    const int healthLost = deferDamage ? actionSnapshot->healthLost : stack->healthLost;
                    float fillPartF = static_cast<float>((maxHp - healthLost)) * 28 / maxHp * 10;
                    bool hpRemainder = fillPartF > 4;

                    int drawWidth = static_cast<int>(fillPartF / 10 + hpRemainder);

                    const int hpBarLimit = CharAt(0x43E45B + 1);
                    drawWidth = Clamp(0, drawWidth, hpBarLimit);

                    // init real draw postion cause of an hd mod offsets
                    const int drawX = newLabelX + dlgX + 1; // add 1px offsets for the borders

                    // fix for the drawing bar under the amount
                    const BOOL isBottomDrawing = nativeLabelY < newLabelY && protrusionSize <= drawHeightMax;

                    const int drawY = dlgY + (isBottomDrawing ? nativeLabelY + labelHeight : newLabelY + 1);

                    // set draw limits for bottom panel top border
                    if (const auto bottomPanel = P_CombatManager->dlg->bottomPanel)
                    {
                        const int panelAbsY = bottomPanel->GetY() + dlgY;
                        if (panelAbsY < drawY - 1 + drawHeight)
                        {
                            drawHeight = panelAbsY - drawY;
                        }
                    }
                    // assert(P_CombatManager->updateRect.bottom < 800);
                    WindMgr_DrawColoredRect(drawX, drawY, drawWidth, drawHeight, &settings);

                    if (drawWidth < hpBarLimit)
                    {
                        // draw lost hp part
                        WindMgr_DrawColoredRect(drawX + drawWidth, drawY, hpBarLimit - drawWidth, drawHeight, &settings,
                                                true);
                    }

                    auto *combatHints = &Get();

                    // redraw dlg
                    if (settings.isHeld && combatHints->needRedraw > 0)
                    {
                        combatHints->needRedraw = -1;
                        THISCALL_1(void, 0x477C00, cmbMgr);
                    }
                }
            }
        }
    }

    return EXEC_DEFAULT;
}

void CombatHints::ShowHealthBarDlg() noexcept
{
    DlgText dlgText = DlgText{EraJS::read("gem_plugin.combat_hints.text.rmc_hint"),
                              EraJS::read("gem_plugin.combat_hints.text.handlers"),
                              EraJS::read("gem_plugin.combat_hints.text.input"),
                              EraJS::read("gem_plugin.combat_hints.text.color_health"),
                              EraJS::read("gem_plugin.combat_hints.text.color_loss"),
                              EraJS::read("gem_plugin.combat_hints.text.height"),
                              EraJS::read("gem_plugin.combat_hints.text.saturation"),
                              EraJS::read("gem_plugin.combat_hints.text.default"),
                              EraJS::read("gem_plugin.combat_hints.text.hotkey"),
                              EraJS::read("gem_plugin.combat_hints.text.wrong_hotkey"),
                              EraJS::read("gem_plugin.combat_hints.text.enable"),
                              EraJS::read("gem_plugin.combat_hints.text.set_hotkey"),
                              EraJS::read("gem_plugin.combat_hints.text.press_any"),
                              EraJS::read("gem_plugin.combat_hints.text.held")};
    auto &settings = Get().settings;

    const Settings before = settings;
    SettingsDlg dlg(450, 450, &settings, &dlgText);
    dlg.Start();
    AdditionalConfig::Get().showCreatureHealthBar.SetValue(settings.isEnabled,
                                                           AdditionalConfig::EOptionChangeSource::Dialog);
    if (before.isEnabled != settings.isEnabled || before.isHeld != settings.isHeld || before.vKey != settings.vKey ||
        before.fcolorFill != settings.fcolorFill || before.fcolorLoss != settings.fcolorLoss ||
        before.fsaturation != settings.fsaturation || before.height != settings.height)
        AdditionalConfig::MarkDirty();
    AdditionalConfig::SaveIfDirty(TRUE);
}
int __fastcall CombatHints::CombatOptionsCallback(H3Msg *msg) noexcept
{

    if (msg->IsLeftClick())
    {
        //	using namespace Era;

        CombatHints::ShowHealthBarDlg();

        msg->GetDlg()->Redraw();
    }

    return false;
}

//	_LHF_(CombatHints::BattleOptionsDlg)
void __stdcall CombatHints::BattleOptionsDlg_Show(HiHook *h, H3BaseDlg *dlg)
{
    //	H3BaseDlg* dlg = reinterpret_cast<H3BaseDlg*>(c->ecx);
    if (dlg)
    {
        int x, y;

        if (IntAt(0x46DF20 + 1) == dlg->GetHeight())
        {
            auto def = dlg->GetDefButton(30722);
            x = def->GetX() + (def->GetWidth() >> 1) - 20;
            y = def->GetY() - 60;
        }
        else
        {
            auto def = dlg->GetDefButton(201);
            x = def->GetX() - 100;
            y = def->GetY();
        }
        x += 3;
        y += 5;

        H3DlgCustomButton *bttn = H3DlgCustomButton::Create(x, y + 5, "HpSttngs.DEF", CombatOptionsCallback, 0, 1);
        bttn->AddHotkey(eVKey::H3VK_H);
        dlg->CreatePcx(bttn->GetX() - 1, bttn->GetY() - 1, 15, NH3Dlg::Assets::BOX_64_32_PCX);

        // dlg->CreateFrame(bttn, color,0,1);
        //	H3DlgScrollbar * scrollBar = H3DlgScrollbar
        dlg->AddItem(bttn);
        H3DlgText *text =
            H3DlgText::Create(x, y - 33, bttn->GetWidth(), 40, EraJS::read("gem_plugin.combat_hints.text.button_name"),
                              NH3Dlg::Text::MEDIUM);
        dlg->AddItem(text);
    }
    THISCALL_1(void, h->GetDefaultFunc(), dlg);

    //	return EXEC_DEFAULT;
}

void CombatHints::WindMgr_DrawColoredRect(const int x, const int y, const int width, const int height,
                                          const Settings *stg, const BOOL lost) noexcept
{
    if (width <= 0 || height <= 0)
        return;
    auto *buffer = P_WindowManager->GetDrawBuffer();
    // 0x44E610 clips only the right/bottom; protect the left/top edges as well.
    const int left = Clamp(0, x, buffer->width);
    const int top = Clamp(0, y, buffer->height);
    const int right = Clamp(left, x + width, buffer->width);
    const int bottom = Clamp(top, y + height, buffer->height);
    if (right > left && bottom > top)
        buffer->AdjustHueSaturation(left, top, right - left, bottom - top, lost ? stg->fcolorLoss : stg->fcolorFill,
                                    stg->fsaturation);
}

void SettingsDlg::HitPointsBarDraw() noexcept
{
    if (!originalLabel || !labelForHp)
        return;
    // draw label colors

    const int labelYOffset = static_cast<int>(settings->height) * -1;
    const int nativeLabelY = originalLabel->GetY();

    // const int newLabelX = labelForHp->GetX();
    const int newLabelY = nativeLabelY + labelYOffset; // add border
    // labelForHp->SetY(newLabelY);
    const int labelHeight = labelForHp->GetHeight();
    const int protrusionSize = std::abs(labelYOffset);
    const int drawHeightMax = labelHeight - 2;
    const int labelBorderHeight = protrusionSize >= labelHeight ? 2 : 1;
    int drawHeight = Clamp(0, protrusionSize - labelBorderHeight, drawHeightMax);

    // const int drawY = originalLabel->GetAbsoluteY() + originalLabel->GetHeight() + 1;
    // if draw area displayed at all
    if (drawHeight > 0)
    {
        //	const int newLabelHeight = Clamp(0, std::abs(newLabelY - nativeLabelY), originalLabel->GetHeight());
        const BOOL isBottomDrawing = nativeLabelY < newLabelY && protrusionSize <= drawHeightMax;

        const int drawY = this->yDlg + (isBottomDrawing ? nativeLabelY + labelHeight : newLabelY + 1);

        constexpr int filledHp = 16;
        const int drawX = labelForHp->GetAbsoluteX() + 1; // add 1px offsets for the borders

        // draw hp
        CombatHints::WindMgr_DrawColoredRect(drawX, drawY, filledHp, drawHeight, settings);
        CombatHints::WindMgr_DrawColoredRect(drawX + filledHp, drawY, labelForHp->GetWidth() - filledHp - 2, drawHeight,
                                             settings, true);
        labelForHp->Refresh();
    }
}

void SettingsDlg::RedrawPreview() noexcept
{
    if (!creatureDef)
        return;
    // Restore the whole dialog before applying the label colors.
    Redraw();
    HitPointsBarDraw();
    needRedraw = false;
}

void __fastcall SettingsDlg::ScrollBarGeneralProc(SettingsDlg *dlg, INT32 scrollBarId, float &valuePtr)
{
    // auto txt = dlg->GetText(txtId);
    auto scroll = dlg->GetScrollbar(scrollBarId);
    if (scroll) // txt && )
    {
        scroll->Draw();
        valuePtr = sysopts::TickToNormalized(scroll->GetTick(), scroll->GetTicksCount());
        scroll->Refresh();
        dlg->RedrawPreview();
    }
}
void __fastcall SettingsDlg::ColorFillScrollBarProc(INT32 value, H3BaseDlg *dlg)
{
    auto sDlg = dynamic_cast<SettingsDlg *>(dlg);
    ScrollBarGeneralProc(sDlg, 100, sDlg->settings->fcolorFill);
}

void __fastcall SettingsDlg::ColorLossScrollBarProc(INT32 value, H3BaseDlg *dlg)
{
    auto sDlg = dynamic_cast<SettingsDlg *>(dlg);
    ScrollBarGeneralProc(sDlg, 101, sDlg->settings->fcolorLoss);
}

void __fastcall SettingsDlg::SaturationScrollBarProc(INT32 value, H3BaseDlg *dlg)
{
    auto sDlg = dynamic_cast<SettingsDlg *>(dlg);
    ScrollBarGeneralProc(sDlg, 102, sDlg->settings->fsaturation);
}

void __fastcall SettingsDlg::HeightScrollBarProc(INT32 value, H3BaseDlg *dlg)
{
    auto sDlg = dynamic_cast<SettingsDlg *>(dlg);

    float newVal = static_cast<float>(value - HP_LABEL_MAX_OFFSET);
    sDlg->settings->height = newVal;

    sDlg->labelForHp->SetY(sDlg->originalLabel->GetY() - static_cast<int>(newVal));
    auto scroll = dlg->GetScrollbar(103);
    scroll->Draw();
    scroll->Refresh();
    sDlg->RedrawPreview();
}

BOOL SettingsDlg::DialogProc(H3Msg &msg)
{
    // External API changes also reach an already open advanced dialog.
    auto enabled = GetDefButton(7);
    if (enabled && enabled->GetFrame() != settings->isEnabled)
    {
        enabled->SetFrame(settings->isEnabled);
        enabled->SetClickFrame(settings->isEnabled);
        enabled->Draw();
        enabled->Refresh();
    }
    if (creatureDef)
    {
        const DWORD currentTime = GetTime();

        if (static_cast<INT32>(currentTime - nextAnimationAt) >= 0)
        {
            // рисуем следующий кадр анимации
            THISCALL_1(void, 0x04EB140, creatureDef);

            const DWORD elapsed = currentTime - nextAnimationAt;
            nextAnimationAt += elapsed < 100 ? 100 : elapsed;
            Redraw();
            needRedraw = true;
        }
    }

    if (needRedraw)
    {
        needRedraw = false;
        HitPointsBarDraw();
    }

    if (hk.bttn && !hk.bttn->IsVisible() && msg.IsKeyDown())
    {
        for (auto it : dlgItems)
            it->Activate();

        if (msg.IsKeyDown())
        {

            eVKey scanCode = msg.GetKey();

            if (settings->validateScanCode(scanCode))
            {
                settings->vKey = MapVirtualKeyA(scanCode, MAPVK_VSC_TO_VK);
                settings->scanCode = scanCode;
                //	str = H3String::Format("\'{%s}\'", getVirtualKeyName(settings.vKey).String());
                hk.name->SetText(getVirtualKeyName(settings->vKey));
            }
            else
            {
                H3Messagebox::RMB(this->text->wrong);
                // hk.text->SetText(text->hotkey);
            }
            hk.text->SetText(text->setHk);
        }

        hk.bttn->Show();
        Redraw();
        needRedraw = true;
    }

    // click on the check boxes
    if (msg.subtype == eMsgSubtype::LBUTTON_CLICK)
    {
        int itemId = msg.itemId;

        auto bttn = GetDefButton(itemId);

        if (bttn)
        {

            if (itemId == 7 || itemId == 8)
            {
                BOOL *optionPtr = itemId == 7 ? &settings->isEnabled : &settings->isHeld;
                *optionPtr ^= 1;
                if (itemId == 7)
                    AdditionalConfig::Get().showCreatureHealthBar.SetValue(
                        settings->isEnabled, AdditionalConfig::EOptionChangeSource::Dialog);
                bttn->SetFrame(*optionPtr);
                bttn->SetClickFrame(*optionPtr);
                bttn->Draw();
                bttn->Refresh();
            }
            else if (itemId == 9)
            {

                // reset settings values
                settings->reset();
                AdditionalConfig::Get().showCreatureHealthBar.SetValue(settings->isEnabled,
                                                                       AdditionalConfig::EOptionChangeSource::Dialog);
                constexpr UINT16 SIZE = 4;
                float values[SIZE] = {settings->fcolorFill, settings->fcolorLoss, settings->fsaturation,
                                      settings->height + HP_LABEL_MAX_OFFSET};

                // set correct positions
                for (UINT16 i = 0; i < SIZE; i++)
                {
                    auto scrollBar = GetScrollbar(100 + i);
                    if (i == 3)
                    {
                        scrollBar->SetTick(static_cast<int>(values[i]));
                    }
                    else
                    {
                        scrollBar->SetTick(sysopts::NormalizedToTick(values[i], scrollBar->GetTicksCount()));
                    }

                    scrollBar->SetButtonPosition();
                    scrollBar->ParentRedraw();
                }
                labelForHp->SetY(originalLabel->GetY() - static_cast<int>(settings->height));
                bttn = GetDefButton(7);
                bttn->SetFrame(settings->isEnabled);
                bttn->SetClickFrame(settings->isEnabled);

                // bttn->Draw();
                // bttn->Refresh();

                bttn = GetDefButton(8);
                bttn->SetFrame(settings->isHeld);
                bttn->SetClickFrame(settings->isHeld);

                hk.name->SetText(getVirtualKeyName(settings->vKey));
                Redraw();
                needRedraw = true;
            }
        }
    }

    if (hintBar)
        hintBar->ShowHint(&msg);

    return 0;
}

void SinkItem(H3LoadedPcx16 *pcx, H3DlgItem *it)
{
    pcx->SinkArea(it->GetX() - 5, it->GetY(), it->GetWidth() + 10, it->GetHeight());
}

SettingsDlg::SettingsDlg(int width, int height, Settings *incomingSettings, DlgText *text)
    : H3Dlg(width, height, -1, -1, true), text(text), needRedraw(true), nextAnimationAt(GetTime())
{
    settings = incomingSettings;
    auto okBttn = CreateOK32Button(widthDlg - 100, heightDlg - 80);
    okBttn->AddHotkey(eVKey::H3VK_ESCAPE);
    // okBttn->AddHotkey(eVKey::H3VK_H);

    //  auto cmb = H3CombatManager::Get();
    LPCSTR barPcxName = ValueAt<LPCSTR>(0x0462F75 + 1); // cmb->cmNumWinPcxLoaded->GetName();

    // H3DlgDef* creatureDef = H3DlgDef::Create(1, 25, 100, 130, 12, P_CreatureInformation[12].defName, 0, 2);

    H3DlgDef *d = H3ObjectAllocator<H3DlgDef>().allocate(1);
    creatureDef = THISCALL_12(H3DlgDef *, 0x4EA800, d, 30, 12, 100, 130, 225,
                              P_CreatureInformation[eCreature::HORNED_DEMON].defName, 0, 2, 0, 0, 0x12);

    // THISCALL_16(H3DlgDef *,0x4EA800, d,)
    AddItem(creatureDef);
    // SinkItem(background, creatureDef);
    background->SinkArea(creatureDef->GetX() - 5, creatureDef->GetY() + 11, creatureDef->GetWidth() + 25,
                         creatureDef->GetHeight() - 10);
    eTextColor textColor = eTextColor::WHITE;
    originalLabel = H3DlgPcx16::Create(creatureDef->GetX() + creatureDef->GetWidth() - 25,
                                       creatureDef->GetY() + creatureDef->GetHeight() - 30, 5, nullptr);
    if (!originalLabel)
        return;

    H3PcxLoader barPcx(barPcxName);
    const int lWidth = barPcx->width;
    const int lHeight = barPcx->height;
    originalLabel->SetWidth(lWidth);
    originalLabel->SetHeight(lHeight);
    H3LoadedPcx16 *barPcx16 = H3LoadedPcx16::Create(lWidth, lHeight);

    barPcx->DrawToPcx16(barPcx16, 0, 0, 1);
    barPcx16->AdjustHueSaturation(1, 1, lWidth - 2, lHeight - 2, FloatAt(0x43E3AC + 1), FloatAt(0x43E3A7 + 1));

    // draw text over the color
    barPcx16->TextDraw(P_TinyFont->Get(), "505", 0, 0, lWidth, lHeight);
    originalLabel->SetPcx(barPcx16);

    labelForHp = H3DlgPcx::Create(originalLabel->GetX(), originalLabel->GetY() - static_cast<int>(settings->height), 6,
                                  barPcxName);
    if (labelForHp)
        AddItem(labelForHp);
    // add after to draw inbefore
    AddItem(originalLabel);

    // glob = new Settings{};

    textColor = eTextColor::REGULAR;
    H3DlgDefButton *enabledChebox = H3DlgDefButton::Create(160, 30, 7, NH3Dlg::Assets::ON_OFF_CHECKBOX,
                                                           settings->isEnabled, settings->isEnabled, false, -1);
    AddItem(enabledChebox);

    H3DlgText *enabledText =
        CreateText(enabledChebox->GetX() + enabledChebox->GetWidth() + 12, enabledChebox->GetY() - 7, 200, 40,
                   text->enable, NH3Dlg::Text::MEDIUM, textColor, 0, eTextAlignment::MIDDLE_LEFT);
    SinkItem(background, enabledText);

    H3DlgDefButton *onlyHeldCheckBox =
        CreateButton(enabledChebox->GetX(), enabledChebox->GetY() + enabledText->GetHeight(), 8,
                     NH3Dlg::Assets::ON_OFF_CHECKBOX, settings->isHeld, settings->isHeld, false, -1);

    H3DlgText *onlyHeldText =
        CreateText(enabledText->GetX(), enabledText->GetY() + enabledText->GetHeight(), enabledText->GetWidth(), 40,
                   text->held, NH3Dlg::Text::MEDIUM, textColor, 0, eTextAlignment::MIDDLE_LEFT);
    SinkItem(background, onlyHeldText);

    // getVirtualKeyName(settings.vKey).

    auto fnt = H3MediumFont::Get();
    int textWidth = 0;
    for (const int key : {VK_SHIFT, VK_CONTROL, VK_MENU, VK_CAPITAL, VK_SPACE, VK_OEM_3, VK_BACK})
        textWidth = (std::max)(textWidth, fnt->GetMaxLineWidth(getVirtualKeyName(key).String()));
    int textX = widthDlg - textWidth - 16;

    hk.bttn = CreateCustomButton(onlyHeldCheckBox->GetX(), onlyHeldCheckBox->GetY() + enabledText->GetHeight() - 3, 15,
                                 "iam009.DEF", SettingsHotkeyCallback, 0, 1);
    hk.bttn->ColorDefToPlayer(IntAt(0x69CCF4));
    int textY = 85;

    hk.text =
        CreateText(onlyHeldText->GetX(), onlyHeldText->GetY() + onlyHeldText->GetHeight(), enabledText->GetWidth(), 40,
                   text->setHk, NH3Dlg::Text::MEDIUM, textColor, 0, eTextAlignment::MIDDLE_LEFT);
    SinkItem(background, hk.text);

    // SinkItem(background, hk.name);

    CreateBlackBox(onlyHeldText->GetX(), hk.text->GetY() + hk.text->GetHeight(), enabledText->GetWidth(),
                   enabledText->GetHeight());
    hk.name = CreateText(onlyHeldText->GetX(), hk.text->GetY() + hk.text->GetHeight(), enabledText->GetWidth(),
                         enabledText->GetHeight(), getVirtualKeyName(settings->vKey).String(), fnt->GetName(),
                         eTextColor::WHITE, 17);

    // create default bttn
    H3DlgDefButton *bttn = H3DlgDefButton::Create(100 - okBttn->GetWidth(), okBttn->GetY(), 9, "wogbttn.def", 12, 13,
                                                  false, eVKey::H3VK_D);
    if (bttn)
    {
        bttn->SetHint(text->dfltName);
        CreatePcx(bttn->GetX() - 1, bttn->GetY() - 1, 15, NH3Dlg::Assets::BOX_64_30_PCX);
        AddItem(bttn);
    }

    constexpr int SIZE = 4;
    int ticks[SIZE] = {101, 101, 11, HP_LABEL_MAX_OFFSET * 2 + 1};
    float values[SIZE] = {settings->fcolorFill, settings->fcolorLoss, settings->fsaturation,
                          settings->height + HP_LABEL_MAX_OFFSET};

    LPCSTR hints[SIZE] = {text->health, text->loss, text->density, text->height};
    H3DlgScrollbar_proc procs[SIZE] = {ColorFillScrollBarProc, ColorLossScrollBarProc, SaturationScrollBarProc,
                                       HeightScrollBarProc};
    H3DlgText *description;

    // background->DrawShadow(160, originalLabel->GetY() + 20, widthDlg - 60, heightDlg / 3);

    H3DlgText *dlgText = CreateText(45, hk.text->GetY() + hk.text->GetHeight() + 45, widthDlg - 90, 24, text->handlers,
                                    NH3Dlg::Text::BIG, 7, 0);

    for (size_t i = 0; i < SIZE; i++)
    {
        H3DlgScrollbar *scrollBar = H3DlgScrollbar::Create(20, 34 * i + dlgText->GetY() + 27, 148, 16, i + 100,
                                                           ticks[i], procs[i], false, 2, true);

        // disable catch keys
        if (scrollBar)
        {
            IntAt(reinterpret_cast<int>(scrollBar) + 0x5C) = NULL;
            AddItem(scrollBar);
            scrollBar->SetTick(sysopts::NormalizedToTick(values[i], ticks[i]));

            if (i == 3)
            {
                scrollBar->SetTick(static_cast<int>(values[i]));
            }
            scrollBar->SetButtonPosition();
        }

        description =
            H3DlgText::Create(scrollBar->GetX() + scrollBar->GetWidth() + 12, scrollBar->GetY() - 2, 224, 20, hints[i],
                              NH3Dlg::Text::MEDIUM, textColor, i + 100, eTextAlignment::MIDDLE_LEFT);
        if (description)
        {
            background->SinkArea(description->GetX() - 5, description->GetY(), description->GetWidth() + 10,
                                 description->GetHeight());
            AddItem(description);
        }
    }
}

BOOL Settings::validateScanCode(eVKey scanCode) const noexcept
{
    return scanCode >= eVKey::H3VK_1 && scanCode <= eVKey::H3VK_BACKSPACE ||
           scanCode >= eVKey::H3VK_Q && scanCode <= eVKey::H3VK_RIGHT_BRACKET ||
           scanCode >= eVKey::H3VK_CTRL && scanCode <= eVKey::H3VK_ALT;
}

H3String SettingsDlg::getVirtualKeyName(int vKey) const noexcept
{
    LPCSTR key = nullptr;
    switch (vKey)
    {
    case VK_SHIFT:
        key = "gem_plugin.combat_hints.text.key_names.shift";
        break;
    case VK_CONTROL:
        key = "gem_plugin.combat_hints.text.key_names.ctrl";
        break;
    case VK_MENU:
        key = "gem_plugin.combat_hints.text.key_names.alt";
        break;
    case VK_CAPITAL:
        key = "gem_plugin.combat_hints.text.key_names.caps_lock";
        break;
    case VK_SPACE:
        key = "gem_plugin.combat_hints.text.key_names.space";
        break;
    case VK_OEM_3:
        key = "gem_plugin.combat_hints.text.key_names.tilde";
        break;
    case VK_BACK:
        key = "gem_plugin.combat_hints.text.key_names.backspace";
        break;
    default: {
        const char character[] = {static_cast<char>(MapVirtualKeyA(vKey, MAPVK_VK_TO_CHAR)), 0};
        return H3String(Era::tr("gem_plugin.combat_hints.text.key_names.character", {"key", character}).c_str());
    }
    }
    return H3String(EraJS::read(key));
}

int __fastcall SettingsDlg::SettingsHotkeyCallback(H3Msg *msg) noexcept
{
    if (msg->subtype == eMsgSubtype::LBUTTON_CLICK)
    {
        auto bttn = msg->GetDlg()->GetCustomButton(15);
        if (bttn)
        {
            auto dlg = dynamic_cast<SettingsDlg *>(msg->GetDlg());

            dlg->hk.text->SetText(dlg->text->press);

            for (auto it : dlg->dlgItems)
                it->DeActivate();

            bttn->Hide();

            dlg->Redraw();
            dlg->needRedraw = true;
        }
    }

    return false;
}
SettingsDlg::~SettingsDlg()
{
    if (!originalLabel)
        return;
    auto pcx16 = originalLabel->GetPcx();
    if (pcx16)
    {
        pcx16->Destroy();
        originalLabel->SetPcx(nullptr);
    }
}

void Settings::reset()
{
    isEnabled = true;
    isHeld = false;
    vKey = VK_CONTROL;
    scanCode = eVKey::H3VK_CTRL;
    height = HP_LABEL_HEIGHT;
    fcolorFill = HP_LABEL_FILL;
    fcolorLoss = HP_LABEL_LOSS;
    fsaturation = HP_LABEL_SATURATION;
}

} // namespace cmbhints
