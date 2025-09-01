#pragma once
#include "RMGObjectsEditor.h"
namespace extender
{
constexpr int ERA_OBJECT_TYPE = 141;
constexpr int HOTA_OBJECT_TYPE = 144;
constexpr int HOTA_PICKUPABLE_OBJECT_TYPE = 145;
constexpr int HOTA_UNREACHABLE_YT_OBJECT_TYPE = 146;

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

  public:
    ObjectExtender();
    virtual ~ObjectExtender();

    // virtual void GetObjectPreperties() noexcept = 0;
  public:
    // required override for some complex structures like creature banks
    virtual void __thiscall AfterLoadingObjectTxtProc(const INT16 *maxSubtypes);
    virtual H3RmgObjectGenerator *__thiscall CreateRMGObjectGen(const RMGObjectInfo &info) const noexcept = 0;

    virtual BOOL __thiscall InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept;
    virtual BOOL __thiscall InitNewWeekMapItemSetup(H3MapItem *mapItem) const noexcept;
    virtual BOOL __thiscall VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                                         const BOOL isHuman) const noexcept;
    virtual BOOL __thiscall SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero,
                                                  const H3Player *activePlayer, const BOOL isRightClick) const noexcept;
    virtual BOOL __thiscall SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                               int &aiResWeight, int *moveDistance,
                                               const H3Position pos) const noexcept;
    virtual BOOL __thiscall RMGDlg_ShowCustomObjectHint(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
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
        typedef int(__stdcall * RegisterExtender_t)(ObjectExtender *);
        HMODULE pl = LoadLibraryA(RMGPluginName);
        if (pl)
        {
            RegisterExtender_t f = RegisterExtender_t(GetProcAddress(pl, "_RegisterExtender@4"));
            if (f)
                return f(extender);
        }
        return NULL;
    }
};

} // namespace extender
