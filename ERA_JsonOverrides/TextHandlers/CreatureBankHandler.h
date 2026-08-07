#pragma once

#include "HandlersList.h"

class CreatureBankHandler
{
    template <typename T> static void ReadStateInt(LPCSTR format, const int bank, const int stateId, T &target)
    {
        bool readSuccess = false;
        libc::sprintf(h3_TextBuffer, format, bank, stateId);
        const int value = EraJS::readInt(h3_TextBuffer, readSuccess);
        if (readSuccess)
            target = static_cast<T>(value);
    }

  public:
    struct formats
    {
        static constexpr LPCSTR REWARD_TYPE = "era.creatureBanks.%d.states.%d.creatureRewardType";
        static constexpr LPCSTR REWARD_COUNT = "era.creatureBanks.%d.states.%d.creatureRewardCount";
        static constexpr LPCSTR CHANCE = "era.creatureBanks.%d.states.%d.chance";
        static constexpr LPCSTR UPGRADE = "era.creatureBanks.%d.states.%d.upgrade";
        static constexpr LPCSTR ARTIFACT_COUNT = "era.creatureBanks.%d.states.%d.artifactTypeCounts.%d";
    };

    static void Init()
    {
        for (int bank = 0; bank < 11; bank++)
        {
            auto &setup = P_CreatureBankSetup[bank];
            for (int stateId = 0; stateId < 4; stateId++)
            {
                auto &state = setup.states[stateId];

                ReadStateInt(formats::REWARD_TYPE, bank, stateId, state.creatureRewardType);
                ReadStateInt(formats::REWARD_COUNT, bank, stateId, state.creatureRewardCount);
                ReadStateInt(formats::CHANCE, bank, stateId, state.chance);
                ReadStateInt(formats::UPGRADE, bank, stateId, state.upgrade);

                for (int artifactType = 0; artifactType < 4; artifactType++)
                {
                    libc::sprintf(h3_TextBuffer, formats::ARTIFACT_COUNT, bank, stateId, artifactType);
                    bool readSuccess = false;
                    const int value = EraJS::readInt(h3_TextBuffer, readSuccess);
                    if (readSuccess)
                        state.artifactTypeCounts[artifactType] = static_cast<INT8>(value);
                }
            }
        }
    }
};
