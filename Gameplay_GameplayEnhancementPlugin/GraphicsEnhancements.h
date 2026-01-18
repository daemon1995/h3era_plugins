#pragma once
#include "pch.h"
#include <array>

namespace graphics
{

class GraphicsEnhancements : public IGamePatch
{
  public:
    static constexpr UINT MAX_UNIQUE_HEROES = 1024;
    static constexpr UINT MAX_UNIQUE_CLASSES = 128;
    static constexpr LPCSTR BUILD_BUTTON_NAME_FORMAT_MAP = "gem_map_town_building_%d";
    static constexpr LPCSTR BUILD_BUTTON_NAME_FORMAT_TOWN = "gem_town_town_building_%d";

  private:
    static GraphicsEnhancements *instance;

    // heroes graphics defs
    std::array<H3LoadedDef *, MAX_UNIQUE_HEROES> uniqueHeroDefs;
    std::array<H3LoadedDef *, MAX_UNIQUE_CLASSES> heroClassDefs[2]; // 0 - male, 1 - female

    // towns is build graphics pointers
    int maxTownsDisplayableBuiltIcons = 5;
	int maxTownsBuildings = h3::limits::BUILDINGS;
    struct BuiltDefButtons
    {
        std::array<H3DlgDefButton *, 7> advMapDlg;
        std::array<H3DlgDefButton *, 4> townDlg;
    } builtDefButtons;

  protected:
    GraphicsEnhancements();
    virtual void CreatePatches() noexcept override;

  public:
    static GraphicsEnhancements *Get() noexcept;
    static H3LoadedDef *Hero_GetMapItemDef(const H3Hero *hero) noexcept;

	static int GetMaxTownBuildingCount() noexcept;
    H3LoadedDef *InitHeroData(const UINT heroId) noexcept;
    void InitHeroClassData(const UINT classId) noexcept;
    void InitAdventureMapTownBuiltDefs() noexcept;
    void DrawAdventureMapTownBuiltStatus(H3AdventureMgrDlg *dlg, const BOOL draw = true,
                                         const BOOL updateScreen = true) noexcept;

    void InitTownDlgDefButtons(H3TownDialog *dlg) noexcept;
    void DrawTownDlgBuiltStatus(H3TownDialog *dlg) noexcept;

    void CleanUpData() noexcept;
};

} // namespace graphics
