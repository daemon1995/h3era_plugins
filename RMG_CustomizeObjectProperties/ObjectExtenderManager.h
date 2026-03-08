#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "AdditionalProperties.h"
#include "ObjectExtender.h"

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
// enum ObjectExtenderMethods
//{
//
//     AFTER_LOADING_OBJECTS_TXT = 0,
//     CREATE_RMG_OBJECT_GEN,
//     NEW_GAME_OBJECT_ITERATION,
//     NEW_WEEK_OBJECT_ITERATION,
//     HERO_MAP_ITEM_VISIT,
//     SET_HINT_IN_H3_TEXT_BUFFER,
//     AI_MAP_ITEM_WEIGHT,
//     RMG_DLG_SHOW_CUSTOM_OBJECT_HINT,
//     METHODS_COUNT
// };

class ObjectExtenderManager : public IGamePatch
{
    struct ErrorText
    {
        static constexpr LPCSTR TITLE_ERROR = "Error";
        static constexpr LPCSTR TITLE_WARNING = "Warning";
        static constexpr LPCSTR ERR_EMPTY_EXTENDER = PROJECT_NAME ": Attempt to register an empty extender.";
        static constexpr LPCSTR ERR_REGISTRATION_CLOSED =
            PROJECT_NAME ": ObjectExtender registration is closed after loading objects.txt.";
        static constexpr LPCSTR ERR_EXTENDER_ALREADY_REGISTERED =
            PROJECT_NAME ": Attempt to register an already registered extender.";
    };

    BOOL skipMapMessageByHdMod = false;
    BOOL allowRegistration = true;
    // contains all the extenders for objects addded by this and other plugins
    std::vector<ObjectExtender *> objectExtenders;
    // std::map<DWORD, ObjectExtender *> extendersMap;
    std::vector<RMGObjectInfo> additionalRmgObjects;
    INT16 maximumObjectSubtypes[h3::limits::OBJECTS] = {};

    ObjectExtender *objectExtendersTypeRelated[h3::limits::OBJECTS]{};
    ObjectExtender **objectExtendersSubTypeRelated[h3::limits::OBJECTS]{{}};

    std::array<std::vector<ObjectExtender *>, ObjectExtenderMethods::METHODS_COUNT> overridenMethods[232];

    // std::vector< ObjectExtender*> objectExtendersSubTypeRelated[232];
    //  contains all the additional properties to add/replace in objects.txt
    AdditionalProperties additionalProperties;
    struct ExtenderLookup
    {
        ObjectExtender *forAllSubtypes = nullptr;            // если экстендер для всех подтипов
        std::unordered_map<int, ObjectExtender *> bySubtype; // если экстендеры для отдельных подтипов
    };
    std::unordered_map<int, ExtenderLookup> extendersByType;
    std::unordered_set<ObjectExtender *> registeredExtenders;

  private:
    static ObjectExtenderManager *instance;

  private:
    ObjectExtenderManager();
    void CreatePatches() override;

  private:
    void InitializeExtendersTypes(const BOOL asArray = false)
    {

        if (asArray || 1)
        {

            // for each extender assign it to the type/subtype array
            for (auto &ext : objectExtenders)
            {

                const int type = ext->GetObjectType();

                if (type >= 0 && type < h3::limits::OBJECTS)
                {
                    const int subtype = ext->GetObjectSubtype();

                    // allocate subtype array if not yet
                    if (!objectExtendersSubTypeRelated[type])
                    {
                        const int maxSubtype = maximumObjectSubtypes[type] + 1;
                        objectExtendersSubTypeRelated[type] = new ObjectExtender *[maxSubtype]();
                    }

                    // if subtype is specified, assign to subtype array
                    if (subtype != eObject::NO_OBJ)
                    {
                        objectExtendersSubTypeRelated[type][subtype] = ext;
                    }
                    // else assign to type array
                    else
                    {
                        objectExtendersTypeRelated[type] = ext;
                    }
                }
            }
            // after assigning all extenders, we can fill in the gaps in the subtype arrays
            for (int type = 0; type < h3::limits::OBJECTS; type++)
            {
                // allocate empty extender pointers for subtype related objects w/o extender
                if (objectExtendersSubTypeRelated[type] == nullptr)
                {
                    const int maxSubtype = maximumObjectSubtypes[type] + 1;
                    objectExtendersSubTypeRelated[type] = new ObjectExtender *[maxSubtype]();
                }

                // if assigned type extender exists and subtype array allocated
                else if (objectExtendersTypeRelated[type])
                {
                    const int maxSubtype = maximumObjectSubtypes[type] + 1;
                    // iterate through subtype array and fill in gaps
                    for (int subtype = 0; subtype < maxSubtype; subtype++)
                    {
                        if (!objectExtendersSubTypeRelated[type][subtype])
                        {
                            objectExtendersSubTypeRelated[type][subtype] = objectExtendersTypeRelated[type];
                        }
                    }
                }
            }
        }
        else
        {
        }
    }

    ObjectExtender *findExtender(const int type, const int subtype)
    {
        if (auto result = objectExtendersSubTypeRelated[type][subtype])
            return result;
        return objectExtendersTypeRelated[type];

        auto it = extendersByType.find(type);
        if (it == extendersByType.end())
            return nullptr;
        auto &lookup = it->second;
        auto jt = lookup.bySubtype.find(subtype);
        if (jt != lookup.bySubtype.end())
            return jt->second;
        return lookup.forAllSubtypes;
    }
    static ObjectExtender *FindExtender(const int type, const int subtype) noexcept
    {
        return instance->findExtender(type, subtype);
    }
    static ObjectExtender *FindExtender(const H3MapItem *mapItem) noexcept
    {
        return instance->findExtender(mapItem->objectType, mapItem->objectSubtype);
    }

  private:
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
    void AddObjectsToObjectGenList(H3Vector<H3RmgObjectGenerator *> *rmgObjecsList);
    BOOL AddExtender(ObjectExtender *ext);

  public:
    static H3RmgObjectGenerator *CreateDefaultH3RmgObjectGenerator(const RMGObjectInfo &info) noexcept;
    static BOOL ShowObjectExtendedInfo(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                       H3String &resultString) noexcept;

    static ObjectExtenderManager *Get();
};
DllExport BOOL __stdcall RegisterObjectExtenderOld(ObjectExtender *extender) noexcept;
DllExport ObjectExtender *__stdcall CreateObjectExtender(ObjectExtender *_this) noexcept;
DllExport ObjectExtender *__stdcall CreateObjectExtenderByType(const int objectType, const int objectSubtype) noexcept;

// Get the singleton instance

} // namespace extender
