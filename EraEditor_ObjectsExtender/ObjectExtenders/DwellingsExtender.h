#pragma once
#include "../framework.h"

namespace editorObjects
{
class DwellingsExtender
{
    struct DwellingRecord
    {
        int enabled = 1;
        LPCSTR name = nullptr;
    };
    static_assert(sizeof(DwellingRecord) == 8, "Editor dwelling record layout changed");
    std::vector<DwellingRecord> single, multiple;
    std::vector<LPCSTR> singleBaseNames, multipleBaseNames;
    DwellingRecord *nativeSingle = nullptr;
    DwellingRecord *nativeMultiple = nullptr;
    size_t nativeSingleCount = 0;
    size_t nativeMultipleCount = 0;
    bool namesLoaded = false;

    static DwellingsExtender &Get();
    void LoadNames();
    void ReloadNames();
    static void __stdcall LoadNativeNames(HiHook *h);
    static void __stdcall OnAfterReloadLanguageData(Eramap::TEvent *);

  public:
    static void Init();
};
} // namespace editorObjects

