#pragma once
#include <set>

namespace advMapHints
{

struct AdventureHintsSettings : public ISettings
{
    bool isHeld;
    int vKey;
    H3String fontName;
    UINT borderSize;
    bool m_objectsToDraw[232];

  public:
    AdventureHintsSettings(const char *filePath, const char *sectionName);

  public:
    virtual void reset() override;
    virtual BOOL load() override;
    virtual BOOL save() override;
};
class AdventureMapHints : public IGamePatch
{

    static RECT m_mapView;
    // bool m_objectsToDraw[232];

    Patch *blockAdventureHintDraw = nullptr;
    AdventureHintsSettings *settings = nullptr;

    bool IsNeedDraw(const H3MapItem *mIt) const noexcept;

    std::set<UINT16> m_drawnOjects;
    struct AccessableH3GeneralText : public H3GeneralText
    {
        // void
        LPCSTR *GetStringAdddres(const int row);
        // ADDRESS
    };

    AdventureMapHints(PatcherInstance *pi);

  private:
    static LPCSTR GetHintText(const H3AdventureManager *adv, const H3MapItem *mapItem, const int mapX, const int mapY,
                              const int mapZ) noexcept;

  public:
    static AdventureMapHints *instance;

    H3LoadedPcx *glBackPtr = nullptr;

    static void Init(PatcherInstance *pi);

    void CreatePatches() noexcept override;
    // bool * ObjectsToDraw()  noexcept;
    virtual ~AdventureMapHints();

    static void __stdcall AdvMgr_ObjectDraw(HiHook *h, H3AdventureManager *advMan, int mapX, int mapY, int mapZ,
                                            int screenX, int screenY);
};

} // namespace advMapHints
