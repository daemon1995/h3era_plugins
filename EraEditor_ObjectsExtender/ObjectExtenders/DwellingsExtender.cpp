#include "DwellingsExtender.h"
#include "../ObjectProperties.h"
#include "../ObjectDefinitions.h"
#include "../../headers/Era/ObjectText.hpp"

namespace editorObjects
{
namespace
{
LPCSTR ReadName(int type, int subtype, LPCSTR base)
{
    LPCSTR name = EraJS::ReadObjectName(type, subtype);
    if (name)
        return name;
    return base;
}
} // namespace

DwellingsExtender &DwellingsExtender::Get()
{
    static auto *instance = new DwellingsExtender;
    return *instance;
}

void DwellingsExtender::LoadNames()
{
    // Read the live operands: GrayFace/Unleashed may already have relocated/expanded these records.
    const DWORD begin1 = DwordAt(0x43DC7B);
    const DWORD end1 = DwordAt(0x43DCB1);
    const DWORD begin4 = DwordAt(0x43DDA2);
    const DWORD end4 = DwordAt(0x43DDD8);
    if (end1 < begin1 || end4 < begin4 || (end1 - begin1) % sizeof(DwellingRecord) ||
        (end4 - begin4) % sizeof(DwellingRecord))
        return;
    nativeSingleCount = (end1 - begin1) / sizeof(DwellingRecord);
    nativeMultipleCount = (end4 - begin4) / sizeof(DwellingRecord);
    if (nativeSingleCount > dwellings::MAX_CAPACITY || nativeMultipleCount > dwellings::MAX_CAPACITY)
        return;
    // The loader operands point to Name, while the published pointers point to the record's start.
    nativeSingle = reinterpret_cast<DwellingRecord *>(begin1 - sizeof(LPCSTR));
    nativeMultiple = reinterpret_cast<DwellingRecord *>(begin4 - sizeof(LPCSTR));
    singleBaseNames.resize(nativeSingleCount);
    multipleBaseNames.resize(nativeMultipleCount);
    for (size_t i = 0; i < nativeSingleCount; ++i)
        singleBaseNames[i] = nativeSingle[i].name;
    for (size_t i = 0; i < nativeMultipleCount; ++i)
        multipleBaseNames[i] = nativeMultiple[i].name;
    namesLoaded = true;
    ReloadNames();

    // Restore full subtype indexing where the WoG editor replaced SIB operands with NOPs.
    // Do this after native TXT initialization so other editor plugins finish their startup patches first.
    if (ByteAt(0x43DF4C) == 0x8D)
        _PI->WriteHexPatch(0x43DF4C, "8D 04 C1");       // LEA EAX,[ECX+EAX*8]
    if (ByteAt(0x48CC50) == 0x8B)
        _PI->WriteHexPatch(0x48CC50, "8B 74 F0 04");    // MOV ESI,[EAX+ESI*8+4]
    if (ByteAt(0x41D274) == 0x8B)
        _PI->WriteHexPatch(0x41D274, "8B 54 CA 04");    // MOV EDX,[EDX+ECX*8+4]
    if (ByteAt(0x41D29A) == 0x8B)
        _PI->WriteHexPatch(0x41D29A, "8B 54 CA 04");
}

void DwellingsExtender::ReloadNames()
{
    if (!namesLoaded)
        return;
    size_t singleCount = (std::max)(size_t(dwellings::MIN_CAPACITY), nativeSingleCount);
    size_t multipleCount = (std::max)(size_t(dwellings::MIN_CAPACITY), nativeMultipleCount);
    ObjectProperty property;
    for (const auto &line : ObjectDefinitions::Properties())
        if (ParseObjectProperty(line, property))
        {
            switch (property.type)
            {
            case dwellings::SINGLE_TYPE:
                singleCount = (std::max)(singleCount, size_t(property.subtype + 1));
                break;
            case dwellings::MULTIPLE_TYPE:
                multipleCount = (std::max)(multipleCount, size_t(property.subtype + 1));
                break;
            }
        }
    single.assign(singleCount, DwellingRecord{1, "Creature dwelling"});
    multiple.assign(multipleCount, DwellingRecord{1, "Creature dwelling"});
    for (const int type : {dwellings::SINGLE_TYPE, dwellings::MULTIPLE_TYPE})
    {
        std::vector<DwellingRecord> *records = nullptr;
        const std::vector<LPCSTR> *baseNames = nullptr;
        DwellingRecord *native = nullptr;
        switch (type)
        {
        case dwellings::SINGLE_TYPE:
            records = &single;
            baseNames = &singleBaseNames;
            native = nativeSingle;
            break;
        case dwellings::MULTIPLE_TYPE:
            records = &multiple;
            baseNames = &multipleBaseNames;
            native = nativeMultiple;
            break;
        }
        for (size_t i = 0; i < records->size(); ++i)
        {
            LPCSTR name = (*records)[i].name;
            if (i < baseNames->size() && (*baseNames)[i])
                name = (*baseNames)[i];
            if (i < baseNames->size())
                (*records)[i].enabled = native[i].enabled;
            (*records)[i].name = ReadName(type, static_cast<int>(i), name);
            // Keep existing records visible to GrayFace's own dwelling list synchronized too.
            if (i < baseNames->size())
                native[i].name = (*records)[i].name;
        }
    }
    _PI->WriteDword(0x585450, reinterpret_cast<DWORD>(single.data()));
    _PI->WriteDword(0x585454, reinterpret_cast<DWORD>(multiple.data()));
}

void __stdcall DwellingsExtender::LoadNativeNames(HiHook *h)
{
    CDECL_0(void, h->GetDefaultFunc());
    Get().LoadNames();
}
void __stdcall DwellingsExtender::OnAfterReloadLanguageData(Eramap::TEvent *)
{
    Get().ReloadNames();
}
void DwellingsExtender::Init()
{
    _PI->WriteHiHook(0x43DB8C, CDECL_, LoadNativeNames);
    Eramap::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");
}
} // namespace editorObjects

