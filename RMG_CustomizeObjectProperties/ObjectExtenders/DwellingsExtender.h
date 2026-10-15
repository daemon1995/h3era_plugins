#pragma once
#include "../pch.h"
#include "DwellingSetup.h"

namespace dwellings
{
class DwellingsExtender final : public extender::ObjectExtender
{
    struct Table
    {
        int type = 0;
        int slotCount = 0;
        int originalCount = 0;
        int frameCount = 0;
        std::vector<int> creatures;
        std::vector<LPCSTR> names;
        std::vector<LPCSTR> originalNames;
        std::vector<LPCSTR> publishedNames;
        std::vector<int> frames;
    };
    Table single, multiple;
    int originalSpecialFrame = 0;
    int pendingSubtype = -1;
    std::vector<SavedDwelling> loadedDwellings;
    bool restoreLoadedDwellings = false;

    DwellingsExtender();
    Table *FindTable(int type) noexcept;
    const Table *FindTable(int type) const noexcept;
    const int *FindCreatures(int type, int subtype) const noexcept;
    void LoadTable(Table &table, int maxSubtype);
    void ReloadNames();
    static int SubtypeAtPosition(const H3Generator &dwelling) noexcept;

    void CreatePatches() override;
    void AfterLoadingObjectsTxtProc(const INT16 *maxSubtypes) override;
    H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectProperties &info,
                                           BOOL isPseudoGeneration) const noexcept override;
    BOOL RMGDlg_ShowCustomObjectHint(const H3ObjectAttributes &attributes,
                                    H3String &defaultText) noexcept override;

    static int __stdcall Rmg_GetDwellingValue(HiHook *h, const H3RmgObjectGenerator *generator,
                                              const H3RmgZoneGenerator *zone,
                                              const H3RmgRandomMapGenerator *rmg);

    static _LHF_(Map_ReadDwellingSubtype);
    static _LHF_(Rmg_ReadDwellingSubtype);
    static _LHF_(Erm_ReadDwellingSubtype);
    static _LHF_(Dwelling_SelectCreatures);
    static _LHF_(Adventure_GetDwellingName);
    static _LHF_(Adventure_GetDwellingClickName);
    static _LHF_(Kingdom_GetDwellingFrame);
    static _LHF_(Kingdom_GetDwellingHint);
    static _LHF_(Kingdom_CompareSpecialFrame);
    static void __stdcall OnAfterReloadLanguageData(Era::TEvent *);
    static void __stdcall OnSavegameWrite(Era::TEvent *);
    static void __stdcall OnSavegameRead(Era::TEvent *);
    static void __stdcall OnAfterLoadGame(Era::TEvent *);
    static void __stdcall OnGameLeave(Era::TEvent *);

  public:
    static DwellingsExtender &Get();
    void InitDefaultRmgValues() const noexcept;
    LPCSTR GetObjectName(int type, int subtype) const noexcept;
};
} // namespace dwellings
