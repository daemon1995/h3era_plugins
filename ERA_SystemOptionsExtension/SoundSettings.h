#pragma once
#include "DlgPanels.h"
namespace sound
{
class SoundSettings : public IGamePatch
{
    static SoundSettings *SoundSettings::instance;
    Patch *blockLoopSounds = nullptr;
    SoundSettings() noexcept : IGamePatch(_PI)
    {
    }

  protected:
    static void __stdcall AdvMgr__StartLoopSound(HiHook *h, H3AdventureManager *_this, int x, int y, int z,
                                                 INT32 volume, int a5)
    {
    }
    void CreatePatches() noexcept
    {
        if (m_isInited)
            return;

        m_isInited = true;
        // block backfround sounds
        blockLoopSounds = _pi->CreateHiHook(0x0418B70, SPLICE_, EXTENDED_, THISCALL_, AdvMgr__StartLoopSound);
    }

  public:
    static void SetBackgroundSoundsState(const BOOL enabled)
    {
        enabled ? Get().blockLoopSounds->Undo() : Get().blockLoopSounds->Apply();
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
