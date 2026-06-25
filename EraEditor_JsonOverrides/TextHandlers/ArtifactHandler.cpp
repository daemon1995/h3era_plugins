#include "ArtifactHandler.h"

#define READ_ART_FIELD(obj, field, idx)                                                                                \
    ArtifactHandler::ReadField<decltype((obj).field)>((obj).field, ArtifactHandler::formats::field, idx)

char ArtifactHandler::textBuffer[0x200]{};
ArtifactHandler::TxtHandler ArtifactHandler::artifactsHandler{};

// if txt file has less lines than patched lines to read we fix it
H3TextTable *__stdcall LoadArtTraitsFile(HiHook *h, LPCSTR fileName)
{
    H3TextTable *artraitsTxt = CDECL_1(H3TextTable *, h->GetDefaultFunc(), fileName);
    if (artraitsTxt)
    {

        const DWORD rowCount = artraitsTxt->CountRows();
        const DWORD expectedRowCount = IntAt(0x040365E + 1) >> 2;
        auto &artifactsHandler = ArtifactHandler::Handler();

        artifactsHandler.rowCount = rowCount;
        artifactsHandler.objectsCount = rowCount - 2;
        artifactsHandler.expectedRowCount = expectedRowCount;
        artifactsHandler.expectedObjectsCount = expectedRowCount - 2;

        const DWORD newSizeToIterate = rowCount << 2;

        // if txt file has less lines than patched lines to read we fix it
        if (IntAt(0x040365E + 1) > newSizeToIterate)
        {
            _PI->WriteDword(0x040365E + 1, newSizeToIterate);
        }
        if (IntAt(0x040369D + 2) > newSizeToIterate)
        {

            _PI->WriteDword(0x040369D + 2, newSizeToIterate);
        }

        if (IntAt(0x040382E + 3) > newSizeToIterate)
        {
            _PI->WriteDword(0x040382E + 3, newSizeToIterate);
        }
    }
    return artraitsTxt;
}

bool __stdcall LoadAllArtifactTxtFiles(HiHook *h)
{
    bool result = CDECL_0(bool, h->GetDefaultFunc());
    // if (result)
    {
        h->Undo();

        const auto &artifactsHandler = ArtifactHandler::Handler();

        const DWORD artsNum = artifactsHandler.objectsCount;
        const DWORD expectedObjectsCount = artifactsHandler.expectedObjectsCount;

        H3ArtifactSetup *artSetupTable = *reinterpret_cast<H3ArtifactSetup **>(0x04036EC + 1);

        for (size_t i = 0; i < expectedObjectsCount; i++)
        {
            auto &artInfo = artSetupTable[i];

            READ_ART_FIELD(artInfo, name, i);
            READ_ART_FIELD(artInfo, cost, i);
            READ_ART_FIELD(artInfo, position, i);
            READ_ART_FIELD(artInfo, type, i);
            READ_ART_FIELD(artInfo, description, i);
            READ_ART_FIELD(artInfo, comboArtifactId, i);
            READ_ART_FIELD(artInfo, partOfComboArtifactId, i);
            READ_ART_FIELD(artInfo, disabled, i);
            READ_ART_FIELD(artInfo, hasSpell, i);
        }
    }

    return result;
}

inline int ArtifactHandler::GetArtifactsNumber() noexcept
{
    return artifactsHandler.objectsCount;
}

void ArtifactHandler::Init()
{
    // Warning: this hook is after all txt read
    _PI->WriteHiHook(0x045C72C, CDECL_, LoadAllArtifactTxtFiles);
    _PI->WriteHiHook(0x040362E, CDECL_, LoadArtTraitsFile);
}
template <typename T>
inline constexpr bool is_artifact_related_v = std::is_same_v<T, int> || std::is_same_v<T, eCombinationArtifacts> ||
                                              std::is_same_v<T, eArtifactPositions> || std::is_same_v<T, eArtifactType>;

template <class T> static BOOL ArtifactHandler::ReadField(T &target, LPCSTR format, const int idx) noexcept
{
    bool readSuccess = false;
    sprintf(textBuffer, format, idx);
    T readResult;
    if constexpr (std::is_same_v<T, LPCSTR>)
        readResult = EraJS::read(textBuffer, readSuccess);
    else if constexpr (is_artifact_related_v<T>)
        readResult = static_cast<T>(EraJS::readInt(textBuffer, readSuccess));
    else
        return false; // unsupported type
    if (readSuccess)
        target = readResult;
    return readSuccess;
}
