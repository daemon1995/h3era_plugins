#pragma once
#include "RMGObjectsEditor.h"
#include "ObjectsExtender.h"

#include <map>
#include <unordered_map>
#include <vector>
namespace extender
{
namespace limits
{
constexpr int EXTENDED = 1024;
constexpr int COMMON = 255;
} // namespace limits

struct RMGObjectSetable
{
    int type;
    int subtype;
    struct HashFunction
    {
        size_t operator()(const RMGObjectSetable &obj) const noexcept;
    };
    bool operator==(const RMGObjectSetable &other) const noexcept;
};

class ExtenderManager : public IGamePatch

{

    // contains all the extenders for objects addded by this and other plugins
    std::vector<ObjectsExtender *> objectExtenders;
    // std::map<DWORD, ObjectsExtender *> extendersMap;
    std::vector<RMGObjectInfo> additionalRmgObjects;

    // contains all the additional properties to add/replace in objects.txt
    AdditionalProperties additionalProperties;

  private:
    ExtenderManager();
    void CreatePatches() override;

  private:
    BOOL AddProperty(std::string &other) noexcept;

  private:
    static ExtenderManager *instance;

  private:
    // hooks used before game loading
    // static methods to use them as General Objects Extending hooks
    static void __stdcall H3GameMainSetup__LoadObjects(HiHook *h, const H3MainSetup *setup);
    // used to increase properties in objects.txt (game main setup list)
    static _LHF_(LoadObjectsTxt);
    int ShowObjectHint(LoHook *h, HookContext *c, const BOOL isRightClick);
    //  ObjectsExtender *GetExtender(H3MapItem *mapItem);
    //    ObjectsExtender *GetExtender(const INT16 mapItemType, const INT16 mapItemSubtype);

  private:
    // hooks used during the game
    static _LHF_(Game__NewGameObjectIteration);
    static _LHF_(Game__NewWeekObjectIteration);
    static _LHF_(H3AdventureManager__ObjectVisit);
    static _LHF_(H3AdventureManager__GetDefaultObjectHoverHint);
    static _LHF_(H3AdventureManager__GetDefaultObjectClickHint);
    static _LHF_(AIHero_GetObjectPosWeight);

  public:
    // static ObjectsExtender* Create();
    void AddObjectsToObjectGenList(H3Vector<H3RmgObjectGenerator *> *rmgObjecsList);
    static BOOL ShowObjectExtendedInfo(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                       H3String &resultString) noexcept;
    static BOOL AddExtender(ObjectsExtender*);
    static ExtenderManager *Get();
};
} // namespace extender
