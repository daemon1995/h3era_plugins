#pragma once
#include "HandlersList.h"

struct MonsterHandler
{
    inline static EraJS::TextPointerBackup single, plural, descriptions;
    inline static H3CreatureInformation *information = nullptr;
    inline static size_t informationCount = 0;
    static LPCSTR *Singles() { return *reinterpret_cast<LPCSTR **>(0x47B12C + 1); }
    static LPCSTR *Plurals() { return *reinterpret_cast<LPCSTR **>(0x47B10C + 1); }
    static LPCSTR *Descriptions() { return *reinterpret_cast<LPCSTR **>(0x47B0EC + 1); }
    static size_t Count() { return static_cast<size_t>((std::max)(0, IntAt(0x4A1657))); }
    static void ForgetInformation()
    {
        if (information) EraJS::ForgetTextRange(information, informationCount * sizeof(*information));
        information = nullptr;
        informationCount = 0;
    }
    static void Apply()
    {
        auto *singles = Singles(), *plurals = Plurals(), *descs = Descriptions();
        const size_t count = Count();
        information = H3CreatureInformation::Get();
        informationCount = count;
        for (size_t i = 0; i < count; ++i)
        {
            // Capture native values in both tables before replacing either pointer.
            auto &info = information[i];
            info.nameSingular = singles[i];
            info.namePlural = plurals[i];
            info.description = descs[i];
            EraJS::ReadField(singles[i], "era.monsters.%d.name.singular", static_cast<int>(i));
            EraJS::ReadField(plurals[i], "era.monsters.%d.name.plural", static_cast<int>(i));
            EraJS::ReadCreatureDescription(descs[i], static_cast<int>(i));
            EraJS::ReadField(info.nameSingular, "era.monsters.%d.name.singular", static_cast<int>(i));
            EraJS::ReadField(info.namePlural, "era.monsters.%d.name.plural", static_cast<int>(i));
            EraJS::ReadCreatureDescription(info.description, static_cast<int>(i));
        }
    }
    static bool __stdcall LoadTraits(HiHook *h)
    {
        // The native loader may release/reassign strings on another read.
        ForgetInformation();
        single.Restore(Singles(), Count());
        plural.Restore(Plurals(), Count());
        descriptions.Restore(Descriptions(), Count());
        const bool loaded = CDECL_0(bool, h->GetDefaultFunc());
        if (loaded)
        {
            // Snapshot the strings actually allocated by this read.
            single.Capture(Singles(), Count());
            plural.Capture(Plurals(), Count());
            descriptions.Capture(Descriptions(), Count());
            Apply();
        }
        return loaded;
    }
    static void __stdcall DestroySingles(HiHook *h)
    {
        ForgetInformation();
        single.Restore(Singles(), Count(), true);
        CDECL_0(void, h->GetDefaultFunc());
    }
    static void __stdcall DestroyPlurals(HiHook *h)
    {
        ForgetInformation();
        plural.Restore(Plurals(), Count(), true);
        CDECL_0(void, h->GetDefaultFunc());
    }
    static void __stdcall DestroyDescriptions(HiHook *h)
    {
        ForgetInformation();
        descriptions.Restore(Descriptions(), Count(), true);
        CDECL_0(void, h->GetDefaultFunc());
    }
    static void Init()
    {
        _PI->WriteHiHook(0x4EDE90, CDECL_, LoadTraits);
        _PI->WriteHiHook(0x47B120, CDECL_, DestroySingles);
        _PI->WriteHiHook(0x47B100, CDECL_, DestroyPlurals);
        _PI->WriteHiHook(0x47B0E0, CDECL_, DestroyDescriptions);
    }
};
