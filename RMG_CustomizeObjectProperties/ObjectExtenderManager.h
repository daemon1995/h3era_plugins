#pragma once
#include "ObjectExtender.h"
#include "RMGObjectsEditor.h"

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
enum ObjectExtenderMethods
{

    AFTER_LOADING_OBJECTS_TXT = 0,
    CREATE_RMG_OBJECT_GEN,
    NEW_GAME_OBJECT_ITERATION,
    NEW_WEEK_OBJECT_ITERATION,
    HERO_MAP_ITEM_VISIT,
    SET_HINT_IN_H3_TEXT_BUFFER,
    AI_MAP_ITEM_WEIGHT,
    RMG_DLG_SHOW_CUSTOM_OBJECT_HINT,
    METHODS_COUNT
};
class ObjectExtenderManager : public IGamePatch
{
    BOOL skipMapMessageByHdMod = false;
    BOOL allowRegistration = true;
    // contains all the extenders for objects addded by this and other plugins
    std::vector<ObjectExtender *> objectExtenders;
    // std::map<DWORD, ObjectExtender *> extendersMap;
    std::vector<RMGObjectInfo> additionalRmgObjects;

    std::vector<ObjectExtender *> objectExtendersTypeRelated[232];

    std::array<std::vector<ObjectExtender *>, ObjectExtenderMethods::METHODS_COUNT> overridenMethods[232];

    // std::vector< ObjectExtender*> objectExtendersTypeRelated[232];
    //  contains all the additional properties to add/replace in objects.txt
    AdditionalProperties additionalProperties;
    struct ExtenderLookup;
    std::unordered_map<int, ExtenderLookup> extendersByType;

    struct ExtenderLookup
    {
        ObjectExtender *forAllSubtypes = nullptr;            // если экстендер для всех подтипов
        std::unordered_map<int, ObjectExtender *> bySubtype; // если экстендеры для отдельных подтипов
    };
    static ObjectExtenderManager *instance;

  private:
    ObjectExtenderManager();
    void CreatePatches() override;

  private:
  private:
    ObjectExtender *findExtender(int type, int subtype)
    {
        auto it = extendersByType.find(type);
        if (it == extendersByType.end())
            return nullptr;
        auto &lookup = it->second;
        auto jt = lookup.bySubtype.find(subtype);
        if (jt != lookup.bySubtype.end())
            return jt->second;
        return lookup.forAllSubtypes;
    }

    // hooks used before game loading
    // static methods to use them as General Objects Extending hooks
    static void __stdcall H3GameMainSetup__LoadObjects(HiHook *h, const H3MainSetup *setup);
    // used to increase properties in objects.txt (game main setup list)
    static _LHF_(LoadObjectsTxt);
    int ShowObjectHint(LoHook *h, HookContext *c, const BOOL isRightClick);
    //  ObjectExtender *GetExtender(H3MapItem *mapItem);
    //    ObjectExtender *GetExtender(const INT16 mapItemType, const INT16 mapItemSubtype);

  private:
    // hooks used during the game
    static _LHF_(Game__NewGameObjectIteration);
    static _LHF_(Game__NewWeekObjectIteration);
    static _LHF_(H3AdventureManager__ObjectVisit);
    static _LHF_(H3AdventureManager__GetDefaultObjectHoverHint);
    static _LHF_(H3AdventureManager__GetDefaultObjectClickHint);
    static _LHF_(AIHero_GetObjectPosWeight);

  public:
    static H3RmgObjectGenerator *CreateDefaultH3RmgObjectGenerator(const RMGObjectInfo &info) noexcept;
    void AddObjectsToObjectGenList(H3Vector<H3RmgObjectGenerator *> *rmgObjecsList);
    static BOOL ShowObjectExtendedInfo(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                       H3String &resultString) noexcept;
    BOOL AddExtender(ObjectExtender *ext);
    static ObjectExtenderManager *Get();
};
DllExport BOOL __stdcall RegisterObjectExtender(ObjectExtender *extender) noexcept;

// Get the singleton instance

} // namespace extender
