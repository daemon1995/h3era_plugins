#include "DlgPanels.h"
#include "CombatCreatureHealthBar.h"

#include <algorithm>
#include <array>
#include <cstdlib>

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
constexpr auto COMBAT_STACK_COUNT = sizeof(H3CombatManager::stacks) / sizeof(H3CombatCreature);
std::array<INT32, COMBAT_STACK_COUNT> visibleHealthLost{};
std::array<Patch *, 4> battleHooks{};
H3CombatDlg *healthBarDlg = nullptr;

H3CombatManager *VisibleCombatManager() noexcept
{
    auto *manager = H3CombatManager::Get();
    return manager && healthBarDlg && manager->dlg == healthBarDlg && !P_AutoSolo && !manager->IsHiddenBattle()
               ? manager
               : nullptr;
}

void SetBattleHooksEnabled(const bool enabled) noexcept
{
    for (auto *hook : battleHooks)
    {
        if (hook)
            enabled ? hook->Apply() : hook->Undo();
    }
    visibleHealthLost.fill(-1);
}

H3CombatDlg *__stdcall CombatDlg_Ctor(HiHook *hook, H3CombatDlg *dlg, INT32 tactics)
{
    // A new attempt must never inherit the previous battle's visible HP.
    SetBattleHooksEnabled(false);
    healthBarDlg = nullptr;
    auto *result = THISCALL_2(H3CombatDlg *, hook->GetDefaultFunc(), dlg, tactics);
    auto *manager = H3CombatManager::Get();
    if (result && manager && !P_AutoSolo && !manager->IsHiddenBattle())
    {
        healthBarDlg = result;
        SetBattleHooksEnabled(true);
    }
    return result;
}

void __stdcall CombatDlg_Dtor(HiHook *hook, H3CombatDlg *dlg)
{
    if (dlg == healthBarDlg)
    {
        SetBattleHooksEnabled(false);
        healthBarDlg = nullptr;
    }
    THISCALL_1(void, hook->GetDefaultFunc(), dlg);
}

_LHF_(AfterBattleAction)
{
    visibleHealthLost.fill(-1);
    return EXEC_DEFAULT;
}

int __stdcall BattleStack_DoPhysicalDamage(HiHook *hook, H3CombatCreature *stack, int damage)
{
    auto *visibleLoss = damage > 0 && CombatHints::Get().settings.isEnabled && VisibleCombatManager()
                            ? &visibleHealthLost[stack->Index()]
                            : nullptr;
    const INT32 before = stack->healthLost;
    if (visibleLoss && !stack->hasLosses)
        *visibleLoss = -1;

    const int killed = THISCALL_2(int, hook->GetDefaultFunc(), stack, damage);
    if (visibleLoss && stack->healthLost != before)
        *visibleLoss = before;
    return killed;
}

_LHF_(BeforeBattleStackSpriteDraw)
{
    if (!VisibleCombatManager())
        return EXEC_DEFAULT;
    const auto *stack = reinterpret_cast<const H3CombatCreature *>(c->ebx);
    using Groups = eSpriteCreatureGroups;
    // This path draws the sprite after the engine has selected its animation group.
    if (!stack->hasLosses || stack->animation == Groups::GETTING_HIT ||
        stack->animation == Groups::DEFEND || stack->animation == Groups::DEATH)
    {
        visibleHealthLost[stack->Index()] = -1;
    }
    return EXEC_DEFAULT;
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

        battleHooks[0] = _pi->CreateLoHook(0x43E38B, BeforeBattleStackHintDraw);
        battleHooks[1] = _pi->CreateHiHook(0x443DB0, SPLICE_, EXTENDED_, THISCALL_, BattleStack_DoPhysicalDamage);
        battleHooks[2] = _pi->CreateLoHook(0x43DF52, BeforeBattleStackSpriteDraw);
        // ERA replaces 0x75D306..0x75D316 and resumes here after firing BG1.
        battleHooks[3] = _pi->CreateLoHook(0x75D317, AfterBattleAction);
        _pi->WriteHiHook(0x471D20, SPLICE_, EXTENDED_, THISCALL_, CombatDlg_Ctor);
        _pi->WriteHiHook(0x472440, SPLICE_, EXTENDED_, THISCALL_, CombatDlg_Dtor);

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
    const H3CombatManager *cmbMgr = VisibleCombatManager();
    if (!cmbMgr)
        return EXEC_DEFAULT;
    H3CombatCreature *stack = reinterpret_cast<H3CombatCreature *>(c->ebx);
    auto &settings = Get().settings;

    const int visibleCount = stack->previousNumber == -1 ? stack->numberAlive : stack->previousNumber;

    if (visibleCount > 0 && settings.isEnabled)
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
            const int maxHp = stack->MaxHitPoints();

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
                    const int visibleLoss = visibleHealthLost[stack->Index()];
                    const int healthLost = visibleLoss >= 0 ? visibleLoss : stack->healthLost;
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
                    WindMgr_DrawColoredRect(drawX, drawY, drawWidth, drawHeight, &settings, false,
                                            static_cast<float>(maxHp - healthLost) / maxHp);

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
                              EraJS::read("gem_plugin.combat_hints.text.held"),
                              EraJS::read("gem_plugin.combat_hints.text.progressive"),
                              EraJS::read("gem_plugin.combat_hints.text.progressive_hint")};
    auto &settings = Get().settings;

    const Settings before = settings;
    SettingsDlg dlg(450, 470, &settings, &dlgText);
    dlg.Start();
    AdditionalConfig::Get().showCreatureHealthBar.SetValue(settings.isEnabled,
                                                           AdditionalConfig::EOptionChangeSource::Dialog);
    if (before.isEnabled != settings.isEnabled || before.isHeld != settings.isHeld ||
        before.progressiveColor != settings.progressiveColor || before.vKey != settings.vKey ||
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
                                          const Settings *stg, const BOOL lost, const float healthFraction) noexcept
{
    if (width <= 0 || height <= 0)
        return;
    auto *buffer = P_WindowManager->GetDrawBuffer();
    // 0x44E610 clips only the right/bottom; protect the left/top edges as well.
    const int left = Clamp(0, x, buffer->width);
    const int top = Clamp(0, y, buffer->height);
    const int right = Clamp(left, x + width, buffer->width);
    const int bottom = Clamp(top, y + height, buffer->height);
    if (right <= left || bottom <= top)
        return;
    if (stg->progressiveColor)
    {
        // HD_HOTA: green at full HP, yellow above 25%, red at 25% or less.
        const UINT16 light = lost ? 0x2104 : healthFraction >= 1.0f ? 0x2527
                                               : healthFraction > 0.25f ? 0xFDC0 : 0xD9E7;
        const UINT16 dark = lost ? 0x1082 : healthFraction >= 1.0f ? 0x1345
                                              : healthFraction > 0.25f ? 0xCCA0 : 0x8144;
        const H3RGB888 first(H3RGB565(light).GetRGB888());
        const H3RGB888 last(H3RGB565(dark).GetRGB888());
        const int steps = (std::max)(1, height - 1);
        for (int row = top; row < bottom; ++row)
        {
            const int offset = row - y;
            buffer->FillRectangle(left, row, right - left, 1,
                                  first.r + (last.r - first.r) * offset / steps,
                                  first.g + (last.g - first.g) * offset / steps,
                                  first.b + (last.b - first.b) * offset / steps);
        }
    }
    else
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

        const int filledHp = settings->progressiveColor ? previewHp : 16;
        const int drawX = labelForHp->GetAbsoluteX() + 1; // add 1px offsets for the borders

        // draw hp
        CombatHints::WindMgr_DrawColoredRect(drawX, drawY, filledHp, drawHeight, settings, false, filledHp / 28.0f);
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
    if (dlg->settings->progressiveColor)
        return;
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

void SettingsDlg::SyncControls() noexcept
{
    const BOOL states[] = {settings->isEnabled, settings->isHeld, settings->progressiveColor};
    const int ids[] = {7, 8, 10};
    for (int i = 0; i < 3; ++i)
        if (auto *checkBox = GetDef(ids[i]))
            checkBox->SetFrame(!!states[i]);

    for (int i = 0; i < 3; ++i)
    {
        auto *slider = GetScrollbar(100 + i);
        auto *label = GetText(200 + i);
        if (settings->progressiveColor)
        {
            slider->DeActivate();
            slider->Shade();
            label->Shade();
        }
        else
        {
            slider->Activate();
            slider->UnShade();
            label->UnShade();
        }
    }
}

BOOL SettingsDlg::DialogProc(H3Msg &msg)
{
    if (auto *enabled = GetDef(7))
        if (enabled->GetFrame() != !!settings->isEnabled)
        {
            enabled->SetFrame(!!settings->isEnabled);
            enabled->Draw();
            enabled->Refresh();
        }

    if (hk.bttn && !hk.bttn->IsVisible() && msg.IsKeyDown())
    {
        for (auto *item : dlgItems)
            item->Activate();
        const eVKey scanCode = msg.GetKey();
        if (settings->validateScanCode(scanCode))
        {
            settings->vKey = MapVirtualKeyA(scanCode, MAPVK_VSC_TO_VK);
            settings->scanCode = scanCode;
            hk.name->SetText(getVirtualKeyName(settings->vKey));
        }
        else
            H3Messagebox::RMB(text->wrong);
        hk.text->SetText(text->setHk);
        hk.bttn->Show();
        SyncControls(); // Restore slider locks after hotkey capture.
        RedrawPreview();
        return TRUE;
    }

    if (msg.IsLeftDown() && (msg.itemId == 7 || msg.itemId == 8 || msg.itemId == 10))
    {
        BOOL *option = msg.itemId == 7 ? &settings->isEnabled
                       : msg.itemId == 8 ? &settings->isHeld : &settings->progressiveColor;
        *option = !*option;
        if (msg.itemId == 7)
            AdditionalConfig::Get().showCreatureHealthBar.SetValue(
                settings->isEnabled, AdditionalConfig::EOptionChangeSource::Dialog);
        SyncControls();
        RedrawPreview();
        P_SoundManager->ClickSound();
        return TRUE;
    }

    if (msg.IsLeftClick() && msg.itemId == 9)
    {
        settings->reset();
        AdditionalConfig::Get().showCreatureHealthBar.SetValue(settings->isEnabled,
                                                               AdditionalConfig::EOptionChangeSource::Dialog);
        for (int i = 0; i < 4; ++i)
        {
            auto *slider = GetScrollbar(100 + i);
            slider->SetTick(i == 3 ? static_cast<int>(settings->height) + HP_LABEL_MAX_OFFSET
                                  : sysopts::NormalizedToTick(settings->values[i], slider->GetTicksCount()));
            slider->SetButtonPosition();
        }
        if (labelForHp && originalLabel)
            labelForHp->SetY(originalLabel->GetY() - static_cast<int>(settings->height));
        hk.name->SetText(getVirtualKeyName(settings->vKey));
        SyncControls();
        RedrawPreview();
    }

    if (creatureDef)
    {
        const DWORD now = GetTime();
        if (static_cast<INT32>(now - nextAnimationAt) >= 0)
        {
            THISCALL_1(void, 0x4EB140, creatureDef);
            const DWORD elapsed = now - nextAnimationAt;
            nextAnimationAt += elapsed < 100 ? 100 : elapsed;
            if (settings->progressiveColor)
                previewHp = previewHp > 1 ? previewHp - 1 : 28;
            needRedraw = true;
        }
    }
    if (needRedraw)
        RedrawPreview();
    if (hintBar)
        hintBar->ShowHint(&msg);
    return FALSE;
}

SettingsDlg::SettingsDlg(int width, int height, Settings *incomingSettings, DlgText *text)
    : H3Dlg(width, height, -1, -1, false, true, IntAt(0x69CCF4)), text(text), needRedraw(true),
      nextAnimationAt(GetTime()), settings(incomingSettings)
{
    auto *ok = CreateOK32Button(width - 100, height - 60);
    ok->AddHotkey(eVKey::H3VK_ESCAPE);

    const LPCSTR barPcxName = ValueAt<LPCSTR>(0x462F75 + 1);
    auto *def = H3ObjectAllocator<H3DlgDef>().allocate(1);
    creatureDef = THISCALL_12(H3DlgDef *, 0x4EA800, def, 30, 20, 100, 130, 225,
                              P_CreatureInformation[eCreature::HORNED_DEMON].defName, 0, 2, 0, 0, 0x12);
    AddItem(creatureDef);
    background->SinkArea(25, 31, 125, 120);
    originalLabel = H3DlgPcx16::Create(creatureDef->GetX() + creatureDef->GetWidth() - 25,
                                       creatureDef->GetY() + creatureDef->GetHeight() - 30, 5, nullptr);
    if (!originalLabel)
        return;
    H3PcxLoader barPcx(barPcxName);
    const int labelWidth = barPcx->width;
    const int labelHeight = barPcx->height;
    originalLabel->SetWidth(labelWidth);
    originalLabel->SetHeight(labelHeight);
    auto *label = H3LoadedPcx16::Create(labelWidth, labelHeight);
    barPcx->DrawToPcx16(label, 0, 0, 1);
    label->AdjustHueSaturation(1, 1, labelWidth - 2, labelHeight - 2,
                               FloatAt(0x43E3AC + 1), FloatAt(0x43E3A7 + 1));
    label->TextDraw(P_TinyFont->Get(), "505", 0, 0, labelWidth, labelHeight);
    originalLabel->SetPcx(label);
    labelForHp = H3DlgPcx::Create(originalLabel->GetX(), originalLabel->GetY() - static_cast<int>(settings->height),
                                  6, barPcxName);
    if (labelForHp)
        AddItem(labelForHp);
    AddItem(originalLabel);

    const LPCSTR names[] = {text->enable, text->held, text->progressive};
    const int ids[] = {7, 8, 10};
    for (int i = 0; i < 3; ++i)
    {
        const int y = 28 + i * 38;
        auto *checkBox = H3DlgDef::Create(width - 56, y, ids[i], NH3Dlg::Assets::ON_OFF_CHECKBOX, 0, 0);
        if (i == 2)
            checkBox->SetRightClickHint(text->progressiveHint);
        AddItem(CreateThickFrameOverItem(checkBox));
        AddItem(checkBox);
        CreateText(170, y - 5, width - 232, 34, names[i], NH3Dlg::Text::MEDIUM,
                   eTextColor::REGULAR, 20 + i, eTextAlignment::MIDDLE_LEFT);
    }

    const H3RGB888 frameLight(0x7A, 0x65, 0x48);
    const H3RGB888 frameDark(0x31, 0x21, 0x10);
    background->DrawThickFrame(158, 24, 1, 122, 1, frameLight);
    background->DrawThickFrame(159, 24, 1, 122, 1, frameDark);
    background->DrawThickFrame(16, 164, width - 32, 52, 1, frameLight);
    background->DrawThickFrame(17, 165, width - 34, 50, 1, frameDark);

    hk.bttn = CreateCustomButton(24, 176, 15, "iam009.DEF", SettingsHotkeyCallback, 0, 1);
    hk.bttn->ColorDefToPlayer(IntAt(0x69CCF4));
    hk.text = CreateText(66, 175, width - 232, 34, text->setHk, NH3Dlg::Text::MEDIUM,
                          eTextColor::REGULAR, 23, eTextAlignment::MIDDLE_LEFT);
    CreateBlackBox(300, 179, width - 324, 26);
    hk.name = CreateText(300, 179, width - 324, 26, getVirtualKeyName(settings->vKey).String(),
                          NH3Dlg::Text::MEDIUM, eTextColor::WHITE, 17);
    AddItem(CreateThickFrameOverItem(hk.name));

    background->DrawThickFrame(24, 222, width - 48, 1, 1, frameLight);
    background->DrawThickFrame(24, 223, width - 48, 1, 1, frameDark);

    CreateText(24, 230, width - 48, 24, text->handlers, NH3Dlg::Text::MEDIUM, eTextColor::HIGHLIGHT, 24);
    const int ticks[] = {101, 101, 11, HP_LABEL_MAX_OFFSET * 2 + 1};
    const LPCSTR hints[] = {text->health, text->loss, text->density, text->height};
    H3DlgScrollbar_proc procs[] = {ColorFillScrollBarProc, ColorLossScrollBarProc,
                                    SaturationScrollBarProc, HeightScrollBarProc};
    for (int i = 0; i < 4; ++i)
    {
        const int y = 264 + i * 34;
        auto *slider = H3DlgScrollbar::Create(24, y, 160, 16, 100 + i, ticks[i], procs[i], false, 2, true);
        IntAt(reinterpret_cast<int>(slider) + 0x5C) = 0; // No keyboard capture.
        AddItem(slider);
        slider->SetTick(i == 3 ? static_cast<int>(settings->height) + HP_LABEL_MAX_OFFSET
                              : sysopts::NormalizedToTick(settings->values[i], ticks[i]));
        slider->SetButtonPosition();
        CreateText(196, y - 3, width - 220, 24, hints[i], NH3Dlg::Text::MEDIUM,
                   eTextColor::REGULAR, 200 + i, eTextAlignment::MIDDLE_LEFT);
    }
    auto *reset = H3DlgDefButton::Create(24, height - 60, 9, "wogbttn.def", 12, 13, false, eVKey::H3VK_D);
    reset->SetHint(text->dfltName);
    CreatePcx(23, height - 61, 25, NH3Dlg::Assets::BOX_64_30_PCX);
    AddItem(reset);
    SyncControls();
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
    progressiveColor = false;
    vKey = VK_CONTROL;
    scanCode = eVKey::H3VK_CTRL;
    height = HP_LABEL_HEIGHT;
    fcolorFill = HP_LABEL_FILL;
    fcolorLoss = HP_LABEL_LOSS;
    fsaturation = HP_LABEL_SATURATION;
}

} // namespace cmbhints
