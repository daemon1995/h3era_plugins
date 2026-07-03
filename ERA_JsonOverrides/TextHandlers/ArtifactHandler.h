#pragma once
#include "HandlersList.h"

#define ART_FIELDS(X)                                                                                                  \
    X(name)                                                                                                            \
    X(cost)                                                                                                            \
    X(position)                                                                                                        \
    X(type)                                                                                                            \
    X(description)                                                                                                     \
    X(comboArtifactId)                                                                                                 \
    X(partOfComboArtifactId)                                                                                           \
    X(disabled)                                                                                                        \
    X(hasSpell)

class ArtifactHandler
{
  public:
    struct formats
    {
        // artraits.txt fields
#define X(field) GENERATE_FORMAT_STR(H3ArtifactSetup, artifacts, field)
        ART_FIELDS(X)
#undef X
        // artevent.txt fields
        DECLARE_JSON_FIELD(artifacts, event)
    };

  protected:
    // if txt file has less lines than patched lines to read we fix it
    static H3TextTable *__stdcall LoadArtTraitsFile(HiHook *h, LPCSTR fileName)
    {
        H3TextTable *artraitsTxt = THISCALL_1(H3TextTable *, h->GetDefaultFunc(), fileName);
        if (artraitsTxt)
        {
            const DWORD rowCount4 = artraitsTxt->CountRows() << 2;
            if (IntAt(0x44CCA8) > rowCount4)
                _PI->WriteDword(0x44CCA8, rowCount4);

            if (IntAt(0x44CACA) > rowCount4)
                _PI->WriteDword(0x44CACA, rowCount4);
        }
        return artraitsTxt;
    }

    static bool __stdcall AfterReadAllTxtFiles(HiHook *h)
    {
        bool result = CDECL_0(bool, h->GetDefaultFunc());
        // if (result)
        {
            h->Undo();

            const int artsNum = ArtifactHandler::GetArtifactsNumber();
            bool readSuccess = false;
            LPCSTR readResult = nullptr;
            LPCSTR *eventTable = ArtifactHandler::GetEventTable();
            const int lastEventIndex = H3AdveventText::Get()->Size();
            for (size_t i = 0; i < artsNum; i++)
            {
                auto &setup = H3ArtifactSetup::Get()[i];

#define X(field) GENERATE_PARSER_BLOCK(setup, field)
                ART_FIELDS(X)
#undef X

                if (!EraJS::ReadField(eventTable[i], ArtifactHandler::formats::event, i) && i > lastEventIndex)
                {
                    eventTable[i] = h3_NullString; // if not read, set to nullptr
                }
            }
        }

        return result;
    }

  public:
    static void Init()
    {
        _PI->WriteHiHook(0x04EE036, CDECL_, AfterReadAllTxtFiles);
        _PI->WriteHiHook(0x044CA43, THISCALL_, LoadArtTraitsFile);
    }

    static LPCSTR *GetEventTable() noexcept
    {
        return *reinterpret_cast<LPCSTR **>(0x49F51B + 3);
    }
    static int GetArtifactsNumber() noexcept
    {
        return IntAt(0x717020);
    }
};
#undef ART_FIELDS
