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
        if (i < 0 || i >= h3::limits::TOTAL_SPELLS) return;
        auto &setup = H3Spell::Get()[i];
        EraJS::ReadNumberInRange(setup.type, formats::type, -1, 1, i);
        EraJS::ReadResourceField(setup.soundName, formats::soundName, i);
        EraJS::ReadField(setup.animationIndex, formats::animationIndex, i);
        EraJS::ReadField(setup.flags, formats::flags, i);
        EraJS::ReadField(setup.name, formats::name, i);
        EraJS::ReadField(setup.shortName, formats::shortName, i);
        EraJS::ReadNumberInRange(setup.level, formats::level, 0, 5, i);
        EraJS::ReadNumberInRange(setup.school, formats::school, 0, 15, i);
        EraJS::ReadField(setup.spEffect, formats::spEffect, i);
        for (int expertise = 0; expertise < 4; ++expertise)
            EraJS::ReadNumberInRange(setup.manaCost[expertise], formats::manaCost, 0, INT_MAX, i, expertise);
        EraJS::ReadArrayField(setup.baseValue, formats::baseValue, i);
        for (int town = 0; town < 9; ++town)
            EraJS::ReadNumberInRange(setup.chanceToGet[town], formats::chanceToGet, 0, 100, i, town);
        EraJS::ReadArrayField(setup.aiValue, formats::aiValue, i);
        EraJS::ReadArrayField(setup.description, formats::description, i);
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
