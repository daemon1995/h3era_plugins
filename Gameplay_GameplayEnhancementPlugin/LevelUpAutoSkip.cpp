#include "LevelUpAutoSkip.h"
#include "ModuleSupport.h"

namespace features
{
namespace
{
constexpr int SKIP_DIALOGS_ADDRESS = 0x698450;
constexpr int LEVEL_UP_TIMEOUT_ADDRESS = 0x6977D4;
constexpr int LEVEL_UP_OK_ID = 30722;
constexpr int AUTO_SKIP_ID = 2020;
constexpr int HIDDEN_SKIP_ID = 2021;
constexpr int AI_CHOICE_RESULT = 9999;

enum class SkipMode
{
    Manual,
    ShowDialogs,
    HideDialogs
};

struct LevelUpSeries
{
    H3Hero *hero;
    H3BaseDlg *dialog = nullptr;
    bool eligible = false;
    SkipMode mode = SkipMode::Manual;
    bool autoSkipSoundPlayed = false;
};

LevelUpSeries *currentSeries = nullptr;

void __stdcall Hero_CheckLevelUps(HiHook *h, H3Hero *hero)
{
    LevelUpSeries series{hero};
    gem::ScopedValue<LevelUpSeries *> seriesScope(currentSeries, &series);
    gem::ScopedValue<int> skipDialogsScope(IntAt(SKIP_DIALOGS_ADDRESS));
    THISCALL_1(void, h->GetDefaultFunc(), hero);
}

_LHF_(Hero_LevelUpTargetCalculated)
{
    // 0x4DAA3A: EBX is the hero, EDX is the final level calculated by the game.
    // This is before the first increment, for both the table and formula paths.
    if (currentSeries && currentSeries->hero == c->Ebx<H3Hero *>())
        currentSeries->eligible = static_cast<int>(c->edx) - currentSeries->hero->level > 3;
    return EXEC_DEFAULT;
}

_LHF_(Hero_LevelUpSound)
{
    if (currentSeries && currentSeries->hero == c->Ebx<H3Hero *>() && currentSeries->mode == SkipMode::ShowDialogs)
    {
        if (currentSeries->autoSkipSoundPlayed)
        {
            // Skip the sound block before its arguments are pushed at 0x4DAC91.
            // Resume after LoadWAVplayAsync, retaining the network/dialog logic.
            c->return_address = 0x4DACA0;
            return NO_EXEC_DEFAULT;
        }
        currentSeries->autoSkipSoundPlayed = true;
    }
    return EXEC_DEFAULT;
}

H3BaseDlg *__stdcall Hero_LevelUpDialogCtor(HiHook *h, H3BaseDlg *dlg, H3Hero *hero, int primarySkill, int firstSkill,
                                            int secondSkill)
{
    H3BaseDlg *result = THISCALL_5(H3BaseDlg *, h->GetDefaultFunc(), dlg, hero, primarySkill, firstSkill, secondSkill);
    if (!result || !currentSeries || currentSeries->hero != hero || !currentSeries->eligible)
        return result;

    auto *okay = result->GetDefButton(LEVEL_UP_OK_ID);
    if (!okay)
        return result;

    constexpr int BUTTON_WIDTH = 64;
    constexpr int BUTTON_HEIGHT = 30;
    constexpr int BUTTON_GAP = 8;
    const int autoX = okay->GetX() - BUTTON_WIDTH - BUTTON_GAP;
    const int hiddenX = autoX - BUTTON_WIDTH - BUTTON_GAP;
    if (hiddenX < 0)
        return result;

    result->CreatePcx(autoX - 1, okay->GetY() - 1, -1, NH3Dlg::Assets::BOX_64_30_PCX);
    result->CreateButton(autoX, okay->GetY(), BUTTON_WIDTH, BUTTON_HEIGHT, AUTO_SKIP_ID, NH3Dlg::Assets::OKAY_DEF, 0, 1,
                         FALSE);

    result->CreatePcx(hiddenX - 1, okay->GetY() - 1, -1, NH3Dlg::Assets::BOX_64_30_PCX);
    result->CreateButton(hiddenX, okay->GetY(), BUTTON_WIDTH, BUTTON_HEIGHT, HIDDEN_SKIP_ID, NH3Dlg::Assets::CANCEL_DEF,
                         0, 1, FALSE);
    currentSeries->dialog = result;
    return result;
}

LPCSTR SkipButtonHint(int itemId)
{
    const bool hidden = itemId == HIDDEN_SKIP_ID;
    bool found = false;
    return EraJS::read(hidden ? "gem_plugin.level_up_dlg.skip_hidden_hint" : "gem_plugin.level_up_dlg.skip_hint");
}

int CloseLevelUpWithAI(H3Msg *msg)
{
    // Mirror the native timeout path at 0x4F9E67, including its timer reset.
    // With two offers, CheckLevelUps consumes 9999 at 0x4DAEAC and calls the AI.
    // With one or zero offers, it already applies the skill or primary stat itself.
    P_WindowManager->SetClickedItemId(AI_CHOICE_RESULT);
    IntAt(LEVEL_UP_TIMEOUT_ADDRESS) = 0;
    msg->command = eMsgCommand::MOUSE_BUTTON;
    msg->subtype = eMsgSubtype::END_DIALOG;
    msg->itemId = 10;
    return 2;
}

int __stdcall Hero_LevelUpDialogProc(HiHook *h, H3BaseDlg *dlg, H3Msg *msg)
{
    if (currentSeries && currentSeries->eligible && currentSeries->dialog == dlg)
    {
        // ShowAndRun has already drawn the next window before invoking its proc.
        if (currentSeries->mode != SkipMode::Manual)
            return CloseLevelUpWithAI(msg);

        if (msg->itemId == AUTO_SKIP_ID || msg->itemId == HIDDEN_SKIP_ID)
        {
            if (msg->IsRightClick())
            {
                H3Messagebox::RMB(SkipButtonHint(msg->itemId));
                return msg->StopProcessing();
            }
            if (msg->IsLeftClick())
            {
                currentSeries->mode = msg->itemId == HIDDEN_SKIP_ID ? SkipMode::HideDialogs : SkipMode::ShowDialogs;
                if (currentSeries->mode == SkipMode::HideDialogs)
                    IntAt(SKIP_DIALOGS_ADDRESS) = 1;
                return CloseLevelUpWithAI(msg);
            }
        }
    }
    return THISCALL_2(int, h->GetDefaultFunc(), dlg, msg);
}

int __stdcall Hero_LevelUpDialogDtor(HiHook *h, H3BaseDlg *dlg)
{
    if (currentSeries && currentSeries->dialog == dlg)
        currentSeries->dialog = nullptr;
    return THISCALL_1(int, h->GetDefaultFunc(), dlg);
}
} // namespace

void InstallLevelUpAutoSkip(PatcherInstance *pi)
{
    pi->WriteHiHook(0x4DA990, THISCALL_, Hero_CheckLevelUps);
    pi->WriteLoHook(0x4DAA3A, Hero_LevelUpTargetCalculated);
    pi->WriteLoHook(0x4DAC91, Hero_LevelUpSound);
    pi->WriteHiHook(0x4F8F10, THISCALL_, Hero_LevelUpDialogCtor);
    pi->WriteHiHook(0x4F9E10, THISCALL_, Hero_LevelUpDialogProc);
    pi->WriteHiHook(0x4F9DC0, THISCALL_, Hero_LevelUpDialogDtor);
}
} // namespace features
