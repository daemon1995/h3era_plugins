#pragma once
#include "pch.h"
#include <set>

namespace rs
{
    struct ObjectFloatingHint
    {
        H3Position m_pos;

        int m_type;
        int m_subType;
        int m_drawOffset;
        const RECT drawBorders{};
        ObjectFloatingHint(UINT pos, H3MapItem* mapItem);
        ~ObjectFloatingHint();
    };
    struct ResourceFloatingHint : public ObjectFloatingHint
    {
        int m_value;
        int m_drawTime;
        int m_lastDrawnType;

        int m_drawCounter;
        ResourceFloatingHint(UINT pos, H3MapItem* mapItem);

        static ResourceFloatingHint* Create(UINT pos, int value, int drawTime);
        static ResourceFloatingHint* Create(UINT8 x, UINT8 y, UINT8 z, int value, int drawTime);
    
        virtual ~ResourceFloatingHint();
    };
    struct CustomObjectFloatingHint :public ObjectFloatingHint
    {


       // static H3LoadedPcx16* CreateAsset(const H3String& srcName,int )
        virtual ~CustomObjectFloatingHint();
        static std::set<H3LoadedPcx16Resized*> assets;

    };


};

struct AdventureHintsSettings : public ISettings
{

    AdventureHintsSettings(const char* filePath, const char* sectionName);
    bool isHeld;
    int vKey;
    H3String fontName;
    UINT borderSize;
    bool m_objectsToDraw[232];

    void reset() override;
    BOOL load()override;
    BOOL save()override;
};
class AdventureMapHints :
    public IGamePatch
{

    static AdventureMapHints* instance;
    static RECT m_mapView;
    bool m_objectsToDraw[232];



    std::vector<rs::ResourceFloatingHint*> m_pickupResources;
    AdventureHintsSettings* settings;
    bool IsNeedDraw(H3MapItem* mIt);

    std::set<UINT16> m_drawnOjects;


    AdventureMapHints(PatcherInstance *pi);

private:
    static LPCSTR GetHintText(H3MapItem* mapItem)  noexcept;

public:


    static void Init(PatcherInstance * pi);
    

    void  CreatePatches() noexcept override;
    bool * ObjectsToDraw()  noexcept;
    virtual ~AdventureMapHints();
    static _LHF_(Hero_AddPickupResource);
    static _LHF_(AdvMgr_DrawFogOfWar);
    static _LHF_(AdvMgr_AfterObjectDraw);
    static void __stdcall Enter2Object(HiHook* h, H3AdventureManager* advMan, H3Hero* hero, H3MapItem* mapItem, __int64 pos);
    static void __stdcall AdvMgr_ObjectDraw(HiHook* h, H3AdventureManager* advMan, int mapX, int mapY, int mapZ, int screenX, int screenY);
    static void __stdcall AdvMgr_BeforeDrawPath(HiHook* h, H3AdventureManager* advMan, int mapX, int mapY, int mapZ, int screenX, int screenY);
};

