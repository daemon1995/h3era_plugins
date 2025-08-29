#include "HandlersList.h"

#define READ_ART_FIELD(obj, field, idx)                                                                                \
    ArtifactHandler::ReadField<decltype((obj).field)>((obj).field, ArtifactHandler::formats::field, idx)

bool __stdcall LoadArtTraitsTxt(HiHook *h)
{
    bool result = CDECL_0(bool, h->GetDefaultFunc());
    if (result)
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

            // ArtifactHandler::ReadField(artInfo.name, ArtifactHandler::formats::name, i);
            //   READ_FIELD(artInfo, description, i)
            READ_ART_FIELD(artInfo, name, i);
            READ_ART_FIELD(artInfo, description, i);

            //READ_ART_FIELD(artInfo, cost, i);
            //     READ_ART_FIELD(artInfo, comboID, i)
            // READ_ART_FIELD(artInfo, event, i)
            // READ_ART_FIELD(artInfo, cost, i)

            sprintf(h3_TextBuffer, ArtifactHandler::formats::event, i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
            {
                eventTable[i] = readResult;
            }
            else if (i > lastEventIndex)
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
    _PI->WriteHiHook(0x04EDEA2, CDECL_, LoadArtTraitsTxt);
}

// template <> static void ArtifactHandler::ReadField<LPCSTR>(LPCSTR &target, const char *format, int idx) noexcept
//{
//     bool readSuccess = false;
//     libc::sprintf(h3_TextBuffer, format, idx);
//     LPCSTR readResult = EraJS::read(h3_TextBuffer, readSuccess);
//     if (readSuccess)
//     {
//         target = readResult;
//     }
// }
//  template <>
//  static void ArtifactHandler::ReadField<INT>(INT &target, const char *format, int idx) noexcept
//{
//      bool readSuccess = false;
//      libc::sprintf(h3_TextBuffer, format, idx);
//      INT readResult = EraJS::readInt(h3_TextBuffer, readSuccess);
//      if (readSuccess)
//      {
//          target = readResult;
//      }
//  }

template <class T> static void ArtifactHandler::ReadField(T &target, const char *format, int idx) noexcept
{
    bool readSuccess = false;
    libc::sprintf(h3_TextBuffer, format, idx);
    T readResult;
    if constexpr (std::is_same_v<T, const char *>)
    {
        readResult = EraJS::read(h3_TextBuffer, readSuccess);
    }
    else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, eCombinationArtifacts>)
    {
        readResult = EraJS::readInt(h3_TextBuffer, readSuccess);
    }
    else
    {

        return; // unsupported type
    }
    if (readSuccess)
    {
        if constexpr (std::is_same_v<T, eCombinationArtifacts>)
        {
            readResult = Clamp(-1, readResult, 123);
        }
        target = readResult;
    }
}
