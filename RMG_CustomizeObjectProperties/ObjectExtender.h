#pragma once
#include "RMGObjectsEditor.h"
namespace extender
{
constexpr int ERA_OBJECT_TYPE = eObject::BLANK3;
constexpr int HOTA_OBJECT_TYPE = eObject::BLANK5;
constexpr int HOTA_PICKUPABLE_OBJECT_TYPE = eObject::BLANK6;
constexpr int HOTA_UNREACHABLE_OBJECT_TYPE = eObject::BLANK7;

// namespace limits
struct RMGObjectProperties
{
    BOOL enabled = TRUE;
    INT32 mapLimit = 0;
    INT32 zoneLimit = 0;
    INT32 value = 0;
    INT32 density = 0;
};
class ObjectExtender
{
  protected:
    int objectType = eObject::NO_OBJ;
    int objectSubtype = eObject::NO_OBJ;
    BOOL m_isInited = FALSE;
    PatcherInstance *_pi = nullptr;

  public:
    ObjectExtender(PatcherInstance *_pi) : _pi(_pi)
    {
    }

    ObjectExtender();
    virtual ~ObjectExtender();

  protected:
    void CreatePatches();
    // virtual void GetObjectPreperties() noexcept = 0;
  public:
    // required override for some complex structures like creature banks
    virtual void AfterLoadingObjectTxtProc(const INT16 *maxSubtypes);
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &info) const noexcept;

    virtual BOOL InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept;
    virtual BOOL InitNewWeekMapItemSetup(H3MapItem *mapItem) const noexcept;
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept;
    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiResWeight, int *moveDistance, const H3Position pos) const noexcept;
    virtual BOOL RMGDlg_ShowCustomObjectHint(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                             const H3String &defaultText) noexcept;

    //	virtual int AiMapItemWeightFunction(HookContext* c, const H3MapItem* mapItem, H3Player* player);
    // returns if object was visited by some of derived classes
    // virtual BOOL HeroMapItemVisitFunction(HookContext* c, const H3Hero* hero, const H3MapItem* mapItem, const BOOL
    // isPlayer, const BOOL skipMapMessage);

  protected:
  public:
    static inline int RegisterExtender(ObjectExtender *extender) noexcept
    {

        constexpr const char *RMGPluginName = "RMG_CustomizeObjectProperties.era";
        typedef int(__stdcall * RegisterObjectExtender_t)(ObjectExtender *);
        HMODULE pl = LoadLibraryA(RMGPluginName);
        if (pl)
        {
            RegisterObjectExtender_t f = RegisterObjectExtender_t(GetProcAddress(pl, "_RegisterObjectExtender@4"));
            if (f)
                return f(extender);
        }
        return NULL;
    }
};

} // namespace extender
