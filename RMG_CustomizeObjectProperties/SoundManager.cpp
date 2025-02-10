#include "pch.h"

namespace sound
{
SoundManager SoundManager::soundManager;

void SoundManager::CreateNewLoopSoundsTable(const std::vector<ObjectSound> &additionalLoopSounds)
{
    if (additionalLoopSounds.size())
    {

        std::unordered_map<std::string, UINT> loopWavNamesMap;

        LPSTR *originalWavTable = reinterpret_cast<LPSTR *>(0x065F794);
        loopSoundNamePointers.resize(LOOP_SOUNDS_AMOUNT);
        volatile size_t wavFileIndex = 0;
        for (; wavFileIndex < LOOP_SOUNDS_AMOUNT; wavFileIndex++)
        {
            loopSoundNamePointers[wavFileIndex] = originalWavTable[wavFileIndex];
            std::string wavName = originalWavTable[wavFileIndex];
            std::transform(wavName.begin(), wavName.end(), wavName.begin(), ::tolower);

            loopWavNamesMap.insert(std::make_pair(wavName, wavFileIndex));
        }

        for (const auto &loopSound : additionalLoopSounds)
        {
            std::string addedLoopSoundName = loopSound.wavName;

            // empty string means no loop sound (is need to block original loop sound)
            int objectLoopSoundIndex = -1;

            if (!addedLoopSoundName.empty())
            {
                std::transform(addedLoopSoundName.begin(), addedLoopSoundName.end(), addedLoopSoundName.begin(),
                               ::tolower);

                auto findResult = loopWavNamesMap.find(addedLoopSoundName);
                // if name isn't in the table add it and allocate memory
                if (findResult == loopWavNamesMap.end())
                {
                    // add new wav entry into cehck map and into the table
                    loopWavNamesMap.insert(std::make_pair(addedLoopSoundName.c_str(), wavFileIndex));
                    objectLoopSoundIndex = wavFileIndex++;
                    LPSTR namePointer = new char[addedLoopSoundName.length() + 1];
                    strcpy(namePointer, addedLoopSoundName.c_str());
                    loopSoundNamePointers.emplace_back(namePointer);
                }
                else
                {
                    objectLoopSoundIndex = findResult->second;
                }
            }
            // store in the map to use in hook
            loopSoundObjectIndexes.insert(std::make_pair(loopSound.objectType, objectLoopSoundIndex));
        }

        // if we added new wav files into the table we should replace the original wav table
        // and patch Adventure manager to use extended wav table
        const size_t wavTableSize = loopWavNamesMap.size();
        // return;
        if (wavTableSize > LOOP_SOUNDS_AMOUNT)
        {

            loopSoundNamePointers.shrink_to_fit();
            loopSoundsWavTable.resize(wavTableSize, nullptr);
            soundsStates.resize(wavTableSize, 0);

            ReplaceAdventureManagerLoadedWavsArray();
        }

        _PI->WriteLoHook(0x4AA757, H3AdventureManager__ObjectVisit_SoundPlay);
        _PI->WriteLoHook(0x04185DD, H3AdventureManager_AtGettingLoopSoundId);
    }
}
void SoundManager::ReplaceAdventureManagerLoadedWavsArray()
{

    if (!patchesSet && loopSoundsWavTable.size())
    {
        // check if somebody pathed native wav table
        if (WordAt(0x041836E) != 0x8C8B || WordAt(0x041850F) != 0x948B || WordAt(0x041854D) != 0x848B)
        {
            return;
        }
        // globalPatcher->UndoAllAt(0x041836E);

        // patch AdventureManager::loopSounds[70] with our wavTable
        const DWORD soundNamesTable = DWORD(loopSoundNamePointers.data());
        _PI->WriteDword(0x0418C70 + 3, soundNamesTable);

        // patch all the AdventureManager::loopSounds[70] entries with own wavTable
        const DWORD wavTable = DWORD(loopSoundsWavTable.data());

        _PI->WriteDword(0x041836E + 3, wavTable);
        _PI->WriteWord(0x041836E + 1, 0x850C); // mov edx, dword ptr ds:[eax*4+wavTable]

        _PI->WriteDword(0x041850F + 3, wavTable);
        _PI->WriteWord(0x041850F + 1, 0x8514); // mov edx, dword ptr ds:[eax*4+wavTable]

        _PI->WriteDword(0x041854D + 3, wavTable);
        _PI->WriteWord(0x041854D + 1, 0x8D04); // mov eax, dword ptr ds:[ecx*4+wavTable]

        _PI->WriteDword(0x0418C2B + 3, wavTable);
        _PI->WriteWord(0x0418C2B + 1, 0x8514); // mov edx, dword ptr ds:[eax*4+ wavTable]

        _PI->WriteDword(0x0418C5C + 3, wavTable);
        _PI->WriteWord(0x0418C5C + 1, 0xBD04); // mov eax, dword ptr ds:[edi*4+wavTable]

        _PI->WriteDword(0x0418C7C + 3, wavTable);
        _PI->WriteWord(0x0418C7C + 1, 0xBD04); // mov dword ptr ds : [edi * 4 + wavTable] , eax

        _PI->WriteDword(0x0418C86 + 3, wavTable);
        _PI->WriteWord(0x0418C86 + 1, 0xBD14); // mov edx, dword ptr ds:[edi*4+wavTable]

        _PI->WriteDword(0x0418C97 + 3, wavTable);
        _PI->WriteWord(0x0418C97 + 1, 0xBD14); // mov edx, dword ptr ds:[edi*4+wavTable]

        _PI->WriteDword(0x0418CA5 + 3, wavTable);
        _PI->WriteWord(0x0418CA5 + 1, 0xBD04); // mov eax, dword ptr ds:[edi*4+wavTable]

        _PI->WriteDword(0x0418CB3 + 3, wavTable);
        _PI->WriteWord(0x0418CB3 + 1, 0xBD0C); // mov ecx, dword ptr ds:[edi*4+wavTable]

        // patch loop table size used to load sounds
        _PI->WriteLoHook(0x0418C57, H3AdventureManager__AtLoadingWav);

        _PI->WriteLoHook(0x041A17F, H3AdventureManager__AtTrimSound);
        _PI->WriteLoHook(0x04073CB, H3AdventureManager__AtShow);
        _PI->WriteLoHook(0x04078C2, H3AdventureManager__AtHide);
        patchesSet = true;
    }
}
_LHF_(SoundManager::H3AdventureManager__AtLoadingWav)
{
    const int loopSoundId = c->edi;
    if (loopSoundId < soundManager.loopSoundsWavTable.size())
    {
        c->return_address = 0x0418C5C;
    }
    else
    {
        c->return_address = 0x0418C83;
    }

    return NO_EXEC_DEFAULT;
}
_LHF_(SoundManager::H3AdventureManager__AtShow)
{
    for (auto &wav : soundManager.loopSoundsWavTable)
    {
        wav = nullptr;
    }
    // soundManager;
    return EXEC_DEFAULT;
}
_LHF_(SoundManager::H3AdventureManager__AtHide)
{
    for (auto &wav : soundManager.loopSoundsWavTable)
    {
        if (wav)
        {
            wav->Dereference();
            wav = nullptr;
        }
    }

    // soundManager;
    return EXEC_DEFAULT;
}

_LHF_(SoundManager::H3AdventureManager__AtTrimSound)
{
    const int soundsToPlay = c->ebx;
    const size_t loopSoundsAmount = soundManager.loopSoundsWavTable.size();
    if (soundsToPlay < loopSoundsAmount)
    {
        if (const H3AdventureManager *advManager = *reinterpret_cast<H3AdventureManager **>(c->ebp - 0x4))
        {
            volatile int usedSoundsConuter = 0;
            auto &soundStates = soundManager.soundsStates;
            memset(soundStates.data(), 0, std::size(soundStates));
            for (const auto &currentSound : advManager->currentSounds)
            {
                const int soundId = currentSound.loopSound;
                if (soundId > eLoopSooundId::NONE && soundId < loopSoundsAmount)
                {
                    soundStates[soundId] = true;
                    usedSoundsConuter++;
                }
            }

            if (usedSoundsConuter < soundsToPlay)
            {
                for (size_t i = 0; i < loopSoundsAmount; i++)
                {
                    if (!soundStates[i])
                    {
                        if (soundManager.loopSoundsWavTable[i])
                        {
                            ++usedSoundsConuter;
                            soundStates[i] = true;
                            if (usedSoundsConuter >= soundsToPlay)
                            {
                                break;
                            }
                        }
                    }
                }

                for (size_t i = 0; i < loopSoundsAmount; i++)
                {
                    if (auto &wav = soundManager.loopSoundsWavTable[i])
                    {
                        if (!soundStates[i])
                        {
                            wav->Dereference();
                            wav = nullptr;
                        }
                    }
                }
            }
        }
    }

    c->return_address = 0x041A22A;
    return NO_EXEC_DEFAULT;
    // soundManager;
}

SoundManager::eLoopSooundId SoundManager::GetLoopSoundId(const H3MapItem *mapItem) const noexcept
{

    const DWORD key = (mapItem->objectType << 16) | mapItem->objectSubtype;
    auto findResult = loopSoundObjectIndexes.find(key);
    if (findResult != loopSoundObjectIndexes.end())
    {
        return eLoopSooundId(findResult->second);
    }
    return eLoopSooundId::UNDEFINED;
}

void __stdcall SoundManager::H3AdventureManager__TrimSound(HiHook *h, const H3AdventureManager *advManager,
                                                           const int type)
{
}

void SoundManager::Init(const std::vector<ObjectSound> &additionalLoopSounds)
{
    soundManager.CreateNewLoopSoundsTable(additionalLoopSounds);
}

_LHF_(SoundManager::H3AdventureManager_AtGettingLoopSoundId)
{

    if (const auto mapItem = reinterpret_cast<H3MapItem *>(c->ecx))
    {

        const int soundId = soundManager.GetLoopSoundId(mapItem);
        if (soundId != SoundManager::eLoopSooundId::UNDEFINED)
        {

            c->eax = soundId;
            c->return_address = 0x041860B;
            return NO_EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(SoundManager::H3AdventureManager__ObjectVisit_SoundPlay) // (HiHook* h, const int objType, const int objSetup)
{
    // before player visits object
    // get object
    if (auto mapItem = reinterpret_cast<H3MapItem *>(c->ebx))
    {
        bool readSuccess = false;

        // try to read sound file name from ERA js
        H3String soundFileName = EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.sound.enter", mapItem->objectType, mapItem->objectSubtype)
                .String(),
            readSuccess);
        // if there is entry
        if (readSuccess)
        {
            //  and filename isn't empty
            if (!soundFileName.Empty())
                // play sound
                P_SoundManager->PlaySoundAsync(soundFileName.String());
            // remove pusheed argumnets before hook
            c->esp += 8;
            // set new return address
            c->return_address = 0x4AA75C;
            // jump after native function
            return NO_EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;
}

} // namespace sound
