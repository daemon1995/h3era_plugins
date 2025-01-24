#pragma once
#include <unordered_map>

namespace sound
{

struct SoundManager
{
    static constexpr int LOOP_SOUNDS_AMOUNT = sizeof(P_AdventureManager->Get()->loopSounds) / sizeof(H3WavFile *);

    enum eLoopSooundId : int
    {
        UNDEFINED = -2,
        NONE = -1
    };
    struct ObjectSound
    {
        LPCSTR wavName;
        DWORD objectType;
    };

    BOOL enterSoundChanged = false;
    BOOL loopSoundChanged = false;
    BOOL patchesSet = false;
    H3WavFile *defaultWav = nullptr;

    std::vector<LPSTR> loopSoundNamePointers;
    std::vector<H3WavFile *> loopSoundsWavTable;
    std::vector<int> soundsStates;

    std::unordered_map<DWORD, int> loopSoundObjectIndexes;

    static SoundManager soundManager;

  private:
    void CreateNewLoopSoundsTable(const std::vector<ObjectSound> &additionalLoopSounds);
    void ReplaceAdventureManagerLoadedWavsArray();
    eLoopSooundId GetLoopSoundId(const H3MapItem *mapItem) const noexcept;

  private:
    static _LHF_(H3AdventureManager__ObjectVisit_SoundPlay); // taken from json
    static _LHF_(H3AdventureManager_AtGettingLoopSoundId);
    static _LHF_(H3AdventureManager__AtShow);
    static _LHF_(H3AdventureManager__AtHide);
    static _LHF_(H3AdventureManager__AtLoadingWav);
    static _LHF_(H3AdventureManager__AtTrimSound);
    static void __stdcall H3AdventureManager__TrimSound(HiHook *h, const H3AdventureManager *advManager,
                                                        const int type);

  public:
    static void Init(const std::vector<ObjectSound> &additionalLoopSounds);
};

} // namespace sound
