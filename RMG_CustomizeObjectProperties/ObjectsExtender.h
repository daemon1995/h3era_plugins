#pragma once
#include "RMGObjectsEditor.h"
namespace extender
{
constexpr int ERA_OBJECT_TYPE = 141;
constexpr int HOTA_OBJECT_TYPE = 144;
constexpr int HOTA_PICKUPABLE_OBJECT_TYPE = 145;
constexpr int HOTA_UNREACHABLE_YT_OBJECT_TYPE = 146;

// namespace limits

class ObjectsExtender : public IGamePatch
{


    ObjectsExtender(PatcherInstance *pi);
    virtual ~ObjectsExtender();

    // virtual void GetObjectPreperties() noexcept = 0;
  public:
    // required override for some complex structures like creature banks
    virtual void AfterLoadingObjectTxtProc(const INT16 *maxSubtypes);
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &info) const noexcept = 0;
    virtual BOOL InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept;
    virtual BOOL InitNewWeekMapItemSetup(H3MapItem *mapItem) const noexcept;
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept;
    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiResWeight, int *moveDistance, const H3Position pos) const noexcept;
    virtual BOOL RMGDlg_ShowCustomObjectHint(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                             const H3String &defaltText) noexcept;

    //	virtual int AiMapItemWeightFunction(HookContext* c, const H3MapItem* mapItem, H3Player* player);
    // returns if object was visited by some of derived classes
    // virtual BOOL HeroMapItemVisitFunction(HookContext* c, const H3Hero* hero, const H3MapItem* mapItem, const BOOL
    // isPlayer, const BOOL skipMapMessage);

  protected:
    static H3RmgObjectGenerator *CreateDefaultH3RmgObjectGenerator(const RMGObjectInfo &info) noexcept;
};

} // namespace extender
