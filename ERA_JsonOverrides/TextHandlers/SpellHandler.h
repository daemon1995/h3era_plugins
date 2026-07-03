#pragma once
#include "HandlersList.h"

#define SPELL_FIELDS(X)                                                                                                \
    X(type)                                                                                                            \
    X(soundName)                                                                                                       \
    X(animationIndex)                                                                                                  \
    X(flags)                                                                                                           \
    X(name)                                                                                                            \
    X(shortName)                                                                                                       \
    X(level)                                                                                                           \
    X(school)                                                                                                          \
    X(spEffect)                                                                                                        \
    X(manaCost)                                                                                                        \
    X(baseValue)                                                                                                       \
    X(chanceToGet)                                                                                                     \
    X(aiValue)                                                                                                         \
    X(description)
class SpellHandler
{

  public:
    struct formats
    {
#define X(field) GENERATE_FORMAT_STR(H3Spell, spells, field)
        SPELL_FIELDS(X)
#undef X
    };

  public:
    static void ParseSingleSpell(const int i)
    {
        auto &setup = H3Spell::Get()[i];
#define X(field) GENERATE_PARSER_BLOCK(setup, field)
        SPELL_FIELDS(X)
#undef X
    }

    static bool __stdcall LoadSpTraits(HiHook *h)

    {
        bool result = CDECL_0(bool, h->GetDefaultFunc());
        h->Undo();
        for (size_t i = 0; i < h3::limits::TOTAL_SPELLS; i++)
        {
            ParseSingleSpell(i);
        }

        return result;
    }
    static void __cdecl Wog_ParseSpell(HiHook *h, DWORD stringIndex, int spellId, int txtLine)
    {
        CDECL_3(void, h->GetDefaultFunc(), stringIndex, spellId, txtLine);
        ParseSingleSpell(spellId);
    }
    static void Init()
    {
        // Warning: this hook is after all txt read
        // _PI->WriteHiHook(0x04EDEAF, CDECL_, LoadSpTraits);
        _PI->WriteHiHook(0x0775887, CDECL_, Wog_ParseSpell);
        _PI->WriteHiHook(0x07758B8, CDECL_, Wog_ParseSpell);
        _PI->WriteHiHook(0x07758E9, CDECL_, Wog_ParseSpell);
        // _PI->WriteHiHook(0x044CA43, THISCALL_, LoadArtTraitsFile);
    }
    static int GetSpellsNumber() noexcept;
};
#undef SPELL_FIELDS
