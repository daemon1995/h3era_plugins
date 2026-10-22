#pragma once
#include "pch.h"
#include "DwellingSetup.h"
#include "DwellingPortraits.h"
#include "../headers/Era/JsonObjectExport.hpp"
#include "../headers/EraPluginsAPI/ConfigExportAPI.h"

namespace dwellings
{
class DwellingsExtender final
{
    PatcherInstance *_pi = nullptr;
    bool m_isInited = false;
    bool installed = false;
    struct Table
    {
        int type = 0;
        int slotCount = 0;
        int originalCount = 0;
        int frameCount = 0;
        int nativeFrameCount = 0;
        std::vector<int> creatures;
        std::vector<LPCSTR> names;
        std::vector<LPCSTR> originalNames;
        std::vector<LPCSTR> publishedNames;
        std::vector<int> frames;
        std::vector<int> overviewFrames;
        std::vector<PortraitSource> portraits;
    };
    Table single, multiple;
    int originalSpecialFrame = 0;
    std::array<int, 5> specialFrames{};
    DwellingPortraits portraits;
    int pendingSubtype = -1;
    std::vector<SavedDwelling> loadedDwellings;
    bool restoreLoadedDwellings = false;

    DwellingsExtender();
    Table *FindTable(int type) noexcept;
    const Table *FindTable(int type) const noexcept;
    const int *FindCreatures(int type, int subtype) const noexcept;
    void LoadTable(Table &table, int maxSubtype);
    void ReloadNames();
    void PreparePortraits(H3LoadedDef *resource);
    static int SubtypeAtPosition(const H3Generator &dwelling) noexcept;

    void CreatePatches();
    static void __stdcall AfterLoadObjects(HiHook *h, const H3MainSetup *setup);
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
    static H3LoadedDef *__stdcall Kingdom_LoadDef(HiHook *h, LPCSTR name);
    static void __stdcall OnAfterReloadLanguageData(Era::TEvent *);
    static void __stdcall OnSavegameWrite(Era::TEvent *);
    static void __stdcall OnSavegameRead(Era::TEvent *);
    static void __stdcall OnAfterLoadGame(Era::TEvent *);
    static void __stdcall OnGameLeave(Era::TEvent *);

  public:
    static DwellingsExtender &Get();
    void Install();
    static int __stdcall CollectConfig(void *, const era_config::Sink *);
    void Initialize(const INT16 *maxSubtypes);
    BOOL IsActive() const noexcept { return m_isInited; }
    LPCSTR GetObjectName(int type, int subtype) const noexcept;
    BOOL GetExportInfo(int type, int subtype, EraJS::DwellingExportInfo *info) const noexcept;
};
} // namespace dwellings
