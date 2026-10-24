#pragma once
#include "ArtifactTablesManager.h"
#include "ArtifactImages.h"
#include "../headers/EraPluginsAPI/ConfigExportAPI.h"

namespace artifacts
{
class ArtifactsExtender
{
    ArtifactTablesManager tables;
    ArtifactImages images;
    std::array<ArtifactData, CAPACITY> definitions;
    std::array<unsigned char, CAPACITY> mapDisabled{};
    std::array<unsigned char, CAPACITY> mapUsed{};
    H3MainSetup *loadedObjectSetup = nullptr;
    bool ready = false;

    void Discover();
    void AfterTextLoaded();
    void ApplyImages();
    void AddMapObjects(H3MainSetup *setup);
    void ResetExtraState();
    bool IsDisabled(H3Main *main, int id) const;
    unsigned char &Used(H3Main *main, int id);

    static int __stdcall AfterReadAllTxtFiles(HiHook *hook);
    static H3LoadedDef *__stdcall LoadDef(HiHook *hook, LPCSTR name);
    static int __stdcall RandomArtifact(HiHook *hook, H3Main *main, int level);
    static int __stdcall ArtDisabled(HiHook *hook, int id);
    static void __stdcall SetArtDisabled(HiHook *hook, int id, int disabled);
    static int __stdcall AIValue(HiHook *hook, H3Hero *hero, int mode, int id, int subtype, int context);
    static _LHF_(MarkMapArtifactUsed);
    static _LHF_(DisableQuestArtifact);
    static int __stdcall LoadMap(HiHook *hook, H3Main *main, int file);
    static void __stdcall LoadObjects(HiHook *hook, H3MainSetup *setup);
    static _ERH_(OnBeforeLoadGame);
    static _ERH_(OnGraphicsResourcesReady);

    static _ERH_(OnSavegameWrite);
    static _ERH_(OnSavegameRead);
    static _ERH_(OnAfterReloadLanguageData);

  public:
    static ArtifactsExtender &Get();
    void Init();
    static int __stdcall CollectConfig(void *, const era_config::Sink *);
};
} // namespace artifacts
