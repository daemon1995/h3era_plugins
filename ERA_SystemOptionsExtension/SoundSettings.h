#pragma once
#include <unordered_set>

#include "DlgPanels.h"
namespace sound
{
class SoundSettings : public IGamePatch
{
    Patch *blockLoopSounds = nullptr;

    H3WavFile *originalButtonClickSound = nullptr;
    H3WavFile *newButtonClickSounds[2] = {nullptr, nullptr};
    Patch *buttonClickSoundPatches[2] = {nullptr, nullptr};
    std::unordered_set<H3DlgDefButton *> buttonsPressed;

    SoundSettings() noexcept : IGamePatch(_PI) {};

  protected:
    void CreatePatches() noexcept override
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
        originalButtonClickSound = ValueAt<H3WavFile *>(0x694DF4);
        // _pi->WriteHiHook(0x04772FE, THISCALL_, Dlg_BattleResults_Dtor);
        // _pi->WriteHiHook(0x047724F, THISCALL_, Dlg_BattleResults_Dtor);
    }

  protected:
    // is used to block looping sound at all
    static void __stdcall AdvMgr__StartLoopSound(HiHook *h, H3AdventureManager *_this, int x, int y, int z,
                                                 INT32 volume, int a5) {};

    // different click sound
    static void PlaySecondClickSound()
    {
        auto snd = P_SoundManager->Get();

        auto secondClickSound = Get().newButtonClickSounds[1];
        if (!snd || !secondClickSound)
            return;
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
    static void __stdcall DefButtonOnDraw(HiHook *hook, H3DlgDefButton *button)
    {

        // PlaySecondClickSound();
        if (button->IsPressed() && button->GetParent() == P_WindowManager->lastDlg)
        {
            Get().buttonsPressed.insert(button);
        }
        else if (Get().buttonsPressed.erase(button) && !button->IsPressed() && button->IsActive() &&
                 button->GetParent() == P_WindowManager->lastDlg)
        {
            PlaySecondClickSound();
        }
        THISCALL_1(void, hook->GetDefaultFunc(), button);
    }
    static void __stdcall DefButtonDtor(HiHook *hook, H3DlgDefButton *button)
    {

        Get().buttonsPressed.erase(button);
        THISCALL_1(void, hook->GetDefaultFunc(), button);
    }

  public:
    static void SetBackgroundSoundsState(const BOOL enabled)
    {
        enabled ? Get().blockLoopSounds->Undo() : Get().blockLoopSounds->Apply();
    }
    static void SetAlternativButtonClickState(const BOOL enabled)
    {
        auto &settings = Get();
        // Leave the original click available if either optional WAV is missing.
        settings.buttonsPressed.clear();
        if (enabled && settings.newButtonClickSounds[0] && settings.newButtonClickSounds[1])
        {
            DwordAt(0x694DF4) = (DWORD)settings.newButtonClickSounds[0];
            for (size_t i = 0; i < 2; i++)
                settings.buttonClickSoundPatches[i]->Apply();
        }
        else
        {
            DwordAt(0x694DF4) = (DWORD)settings.originalButtonClickSound;
            for (size_t i = 0; i < 2; i++)
                settings.buttonClickSoundPatches[i]->Undo();
        }
    }
    static void ApplyBackgroundSoundsState(const BOOL enabled)
    {
        auto advMan = P_AdventureManager ? P_AdventureManager->Get() : nullptr;
        if (!advMan || !advMan->dlg)
        {
            SetBackgroundSoundsState(enabled);
            return;
        }
        if (enabled)
        {
            Get().blockLoopSounds->Undo();
            const int currentTown = P_Game->GetPlayer()->currentTown;

            H3Position pos;

            if (currentTown != -1)
            {
                auto &town = P_Game->towns[currentTown];
                pos = H3Position(town.x, town.y, static_cast<INT8>(town.z));
            }
            else
            {
                auto hero = P_Game->GetPlayer()->GetActiveHero();
                if (!hero)
                    return;
                pos = H3Position(hero->x, hero->y, static_cast<INT8>(hero->z));
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

    static inline void AdjustSoundVolume(ISetting *sender, const DWORD address) noexcept
    {
        auto &value = sender->value;
        auto snd = P_SoundManager->Get();

        if (*value.valuePtr || snd->driver)
        {
            *value.valuePtr = value.current;
            BOOL32 backup = snd->clickSoundVar;
            snd->clickSoundVar = 1;
            THISCALL_1(VOID, address, snd);
            snd->clickSoundVar = backup;
        }
        else
        {
            H3Messagebox(P_GeneralText->GetText(152));
        }
    }
    static void OnMusicVolumeChanged(ISetting *sender)
    {
        OriginalConfig::Get().lastMusicVolume = sender->value.current;
        AdjustSoundVolume(sender, 0x059A4B0);
    }
    static void OnSoundVolumeChanged(ISetting *sender)
    {
        const int currentVolume = sender->value.current;
        OriginalConfig::Get().lastEffectsVolume = currentVolume;
        auto advMan = P_AdventureManager->Get();
        if (!currentVolume && advMan && advMan->dlg)
        {
            H3Position pos(1023, 0, 0);                     // position that blocks sound play
            THISCALL_3(void, 0x0418330, advMan, pos, TRUE); // stop current sound and dont play new;
        }
        AdjustSoundVolume(sender, 0x059A3C0);
    }
};

} // namespace sound
