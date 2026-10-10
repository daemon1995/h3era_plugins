#pragma once
#include "pch.h"

#include <vector>
#include <unordered_set>

namespace advMapHints
{
constexpr LPCSTR KEY_ALT_HINT_ENOUGH_POINTS = "gem_plugin.adventure_map_dlg.pathfinder_hints.enough";
constexpr LPCSTR KEY_ALT_HINT_NOT_ENOUGH_POINTS = "gem_plugin.adventure_map_dlg.pathfinder_hints.not_enough";
constexpr LPCSTR KEY_ALT_HINT_CANT_REACH = "gem_plugin.adventure_map_dlg.pathfinder_hints.cannot_reach";
constexpr LPCSTR MOUSEOVER_HINT_MOVEPTS = "gem_plugin.adventure_map_dlg.pathfinder_hints.movepoints_left";
struct AdventureHintsSettings : public ISettings
{
    BOOL isHeld;
    BOOL drawOverFogOfWar = true;
    struct
    {
        bool defaultValue = false;
        bool userValue = false;
        INT16 yOffset = NULL;
    } drawObjectHint[232];

    LPCSTR creatureHintFormat = nullptr;
    LPCSTR visitedHintFormat = nullptr;
    LPCSTR nonVisitedHintFormat = nullptr;

  public:
    AdventureHintsSettings(const char *filePath, const char *sectionName);

  public:
    virtual void reset() override;
    virtual BOOL load() override;
    virtual BOOL save() override;
};
class AdventureMapHints : public IGamePatch
{
    struct DrawnHintInfo
    {
        RECT rect;
        int mapX;
    };

    static constexpr LPCSTR ERM_VARIABLE_FORMAT = "gem_adventure_map_object_hints_option_%d";

    static RECT m_mapView;
    static AdventureMapHints *instance;
    static constexpr LPCSTR vipPluginInstanceName = "EraPlugin.AdventureMapHints.daemon_n";

    Patch *blockAdventureHintDraw = nullptr;
    Patch *blockIgnoreHintBarFocus = nullptr;
    AdventureHintsSettings settings;
    H3LoadedPcx16 *tempHintBuffer = nullptr;
    std::vector<DrawnHintInfo> drawnHintRects;

    BOOL altIsPressed = FALSE;
    BOOL isCustomHintCreation = FALSE;

  public:
    BOOL needDrawHints = false;
    INT playerID = -1;
    std::unordered_set<UINT16> drawnOjectIndexes;

    AdventureMapHints(PatcherInstance *pi);
    bool NeedDrawMapItem(const H3MapItem *mIt) const noexcept;

  private:
    LPCSTR GetHintText(const H3AdventureManager *adv, const H3MapItem *mapItem, const int mapX, const int mapY,
                       const int mapZ) noexcept;

  public:
    static AdventureMapHints &Get();

    void CreatePatches() noexcept override;
    virtual ~AdventureMapHints();

  protected:
    static void __stdcall AdvMgr_TileObjectDraw(HiHook *h, H3AdventureManager *advMan, int mapX, int mapY, int mapZ,
                                                int screenX, int screenY);

    static int __stdcall H3AdventureManager_ProcMapScreen(HiHook *h, H3AdventureManager *advMgr, H3Msg *msg);
    static void __stdcall H3AdventureManager_SetHint(HiHook *h, H3AdventureManager *advMgr, H3MapItem *cell, int x,
                                                     int y);
    static _LHF_(H3AdventureManager_SetHeroObjectHint);

    static _LHF_(AdvMgr_BeforeObjectsDraw);
    static void __stdcall ReloadLanguage(Era::TEvent *event);
};

} // namespace advMapHints
