#pragma once
#include "unordered_set"

#include "DlgPanels.h"
namespace sound
{
class SoundSettings : public IGamePatch
{
    Patch *blockLoopSounds = nullptr;

    H3WavFile *originalButtonClickSound = nullptr;
    H3WavFile *newButtonClickSounds[2] = {nullptr, nullptr};
    Patch *buttonClickSoundPatches[2] = {nullptr, nullptr};
    std::unordered_set<DWORD> buttonsPressed;

    SoundSettings() noexcept : IGamePatch(_PI)
    {
    }

  protected:
    static void __stdcall AdvMgr__StartLoopSound(HiHook *h, H3AdventureManager *_this, int x, int y, int z,
                                                 INT32 volume, int a5)
    {
    }
    static void PlaySecondClickSound()
    {
        auto snd = P_SoundManager->Get();

        auto secondClickSound = Get().newButtonClickSounds[1];
        BOOL32 backup = snd->clickSoundVar;
        snd->clickSoundVar = 1;
        secondClickSound->spinCount = 64; // volume
        secondClickSound->debugInfo = PRTL_CRITICAL_SECTION_DEBUG(1);
        secondClickSound->lockSemaphore = HANDLE(HANDLE_FLAG_PROTECT_FROM_CLOSE | HANDLE_FLAG_INHERIT);

        THISCALL_2(VOID, 0x59A510, snd, secondClickSound);
        snd->clickSoundVar = backup;
    }
    static DWORD __stdcall Dlg_BattleResults_Dtor(HiHook *hook, H3Msg *msg)
    {

        auto result = THISCALL_1(DWORD, hook->GetDefaultFunc(), msg);
        PlaySecondClickSound();

        return result;
    }
    static DWORD __stdcall DefButtonOnDraw(HiHook *hook, H3DlgDefButton *button)
    {

        // PlaySecondClickSound();
        if (button->IsPressed() && button->GetParent() == P_WindowManager->lastDlg)
        {
            Get().buttonsPressed.insert(DWORD(button));
        }
        else if (button->IsActive() && Get().buttonsPressed.erase(DWORD(button)))
        {
            PlaySecondClickSound();
        }
        return THISCALL_1(DWORD, hook->GetDefaultFunc(), button);
    }
    static DWORD __stdcall DefButtonDtor(HiHook *hook, H3DlgDefButton *button)
    {

        Get().buttonsPressed.erase(DWORD(button));
        return THISCALL_1(DWORD, hook->GetDefaultFunc(), button);
    }
    void CreatePatches() noexcept
    {
        if (m_isInited)
            return;

        m_isInited = true;
        // block background sounds
        blockLoopSounds = _pi->CreateHiHook(0x0418B70, SPLICE_, EXTENDED_, THISCALL_, AdvMgr__StartLoopSound);

        // enable second click sound
        buttonClickSoundPatches[0] = _pi->CreateHiHook(0x0456620, SPLICE_, EXTENDED_, THISCALL_, DefButtonOnDraw);
        buttonClickSoundPatches[1] = _pi->CreateHiHook(0x0455DD0, SPLICE_, EXTENDED_, THISCALL_, DefButtonDtor);
        newButtonClickSounds[0] = H3WavFile::Load("BUTTON0.WAV");
        newButtonClickSounds[1] = H3WavFile::Load("BUTTON1.WAV");
        originalButtonClickSound = (H3WavFile *)DwordAt(0x694DF4);
        // _pi->WriteHiHook(0x04772FE, THISCALL_, Dlg_BattleResults_Dtor);
        // _pi->WriteHiHook(0x047724F, THISCALL_, Dlg_BattleResults_Dtor);
    }

  public:
    static void SetBackgroundSoundsState(const BOOL enabled)
    {
        enabled ? Get().blockLoopSounds->Undo() : Get().blockLoopSounds->Apply();
    }
    static void SetAlternativButtonClickState(const BOOL enabled)
    {
        if (enabled)
        {
            DwordAt(0x694DF4) = (DWORD)Get().newButtonClickSounds[0];
            for (size_t i = 0; i < 2; i++)
                Get().buttonClickSoundPatches[i]->Apply();
        }
        else
        {
            DwordAt(0x694DF4) = (DWORD)(Get().originalButtonClickSound);
            for (size_t i = 0; i < 2; i++)
                Get().buttonClickSoundPatches[i]->Undo();
        }
    }
    static void StopBackgroundSounds(ISetting *sender)
    {
        const BOOL enableLoopSounds = sender->value.current;

        auto advMan = P_AdventureManager->Get();
        if (!advMan || !advMan->dlg)
        {
            SetBackgroundSoundsState(enableLoopSounds);
            return;
        }
        if (enableLoopSounds)
        {
            Get().blockLoopSounds->Undo();
            const int currentTown = P_Game->GetPlayer()->currentTown;

            H3Position pos;

            if (currentTown != -1)
            {
                auto &town = P_Game->towns[currentTown];
                pos = H3Position(town.x, town.y, town.z);
            }
            else
            {
                auto hero = P_Game->GetPlayer()->GetActiveHero();
                pos = H3Position(hero->x, hero->y, hero->z);
            }

            THISCALL_3(void, 0x0418330, advMan, pos, FALSE); // start play new sounds
        }
        else
        {
            H3Position pos(1023, 0, 0);                     // position that blocks sound play
            THISCALL_3(void, 0x0418330, advMan, pos, TRUE); // stop current sound and dont play new;
            Get().blockLoopSounds->Apply();
        }
    }

  public:
    static SoundSettings &Get() noexcept
    {
        static SoundSettings instance;
        instance.CreatePatches();
        return instance;
    }

    static inline void AdjustSoundVolume(ISetting *sender, const DWORD addres) noexcept
    {
        auto &value = sender->value;
        auto snd = P_SoundManager->Get();

        if (*value.valuePtr || snd->driver)
        {
            *value.valuePtr = value.current;
            BOOL32 backup = snd->clickSoundVar;
            snd->clickSoundVar = 1;
            THISCALL_1(VOID, 0x059A4B0, snd);
            snd->clickSoundVar = backup;
        }
        else
        {
            H3Messagebox(P_GeneralText->GetText(152));
        }
    }
    static void OnMusicVolumeChanged(ISetting *sender)
    {
        AdjustSoundVolume(sender, 0x059A4B0);
    }
    static void OnSoundVolumeChanged(ISetting *sender)
    {
        AdjustSoundVolume(sender, 0x059A3C0);
    }
};

} // namespace sound
