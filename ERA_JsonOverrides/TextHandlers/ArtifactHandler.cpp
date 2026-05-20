#include "HandlersList.h"

#define READ_ART_FIELD(obj, field, idx)                                                                                \
    ArtifactHandler::ReadField<decltype((obj).field)>((obj).field, ArtifactHandler::formats::field, idx)

bool __stdcall LoadArtTraitsTxt(HiHook *h)
{
    bool result = CDECL_0(bool, h->GetDefaultFunc());
    // if (result)
    {
        h->Undo();

        const int artsNum = ArtifactHandler::GetArtifactsNumber();
        bool readSuccess = false;
        LPCSTR readResult = nullptr;
        LPCSTR *eventTable = ArtifactHandler::GetEventTable();
        const int lastEventIndex = H3AdveventText::Get()->end() - H3AdveventText::Get()->begin() >> 2;
        for (size_t i = 0; i < artsNum; i++)
        {
            auto &artInfo = P_ArtifactSetup->Get()[i];

            READ_ART_FIELD(artInfo, name, i);
            READ_ART_FIELD(artInfo, cost, i);
            READ_ART_FIELD(artInfo, position, i);
            READ_ART_FIELD(artInfo, type, i);
            READ_ART_FIELD(artInfo, description, i);
            READ_ART_FIELD(artInfo, comboID, i);
            READ_ART_FIELD(artInfo, combinationArtifactId, i);
            READ_ART_FIELD(artInfo, disabled, i);
            READ_ART_FIELD(artInfo, newSpell, i);

            if (!ArtifactHandler::ReadField<LPCSTR>(eventTable[i], ArtifactHandler::formats::event, i) &&
                i > lastEventIndex)
            {
                eventTable[i] = h3_NullString; // if not read, set to nullptr
            }
        }
    }

    return result;
}

inline LPCSTR *ArtifactHandler::GetEventTable() noexcept
{
    return *reinterpret_cast<LPCSTR **>(0x49F51B + 3);
}
inline int ArtifactHandler::GetArtifactsNumber() noexcept
{
    return IntAt(0x717020);
}

void ArtifactHandler::Init()
{
    // Warning: this hook is after all txt read
    _PI->WriteHiHook(0x04EE036, CDECL_, LoadArtTraitsTxt);
}
template <typename T>
inline constexpr bool is_artifact_related_v = std::is_same_v<T, int> || std::is_same_v<T, eCombinationArtifacts> ||
                                              std::is_same_v<T, eArtifactSlots> || std::is_same_v<T, eArtifactType>;

template <class T> static BOOL ArtifactHandler::ReadField(T &target, LPCSTR format, const int idx) noexcept
{
    bool readSuccess = false;
    libc::sprintf(h3_TextBuffer, format, idx);
    T readResult;
    if constexpr (std::is_same_v<T, LPCSTR>)
    {
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
    }
    else if constexpr (is_artifact_related_v<T>)
    {
        readResult = static_cast<T>(EraJS::readInt(h3_TextBuffer, readSuccess));
    }
    else
    {
        return false; // unsupported type
    }
    if (readSuccess)
    {
        if constexpr (std::is_same_v<T, eCombinationArtifacts>)
        {
            readResult = static_cast<T>(Clamp(-1, static_cast<int>(readResult), 123));
        }
        target = readResult;
    }
    return readSuccess;
}
