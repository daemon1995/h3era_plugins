#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "AdditionalProperties.h"
#include "RMGObjectsEditor.h"

using namespace extender;
namespace extendersManager
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
// template <class T> inline static T &GetFromMapItem(H3MapItem *mapItem)
//{
//    return static_cast<T>(mapItem->setup);
//}

struct ObjectExtenderRegistrator
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

    BOOL allowRegistration = true;
    std::unordered_set<ObjectExtender *> registeredExtenders;

  public:
    BOOL AddExtender(ObjectExtender *ext);

    static ObjectExtenderRegistrator &Get()
    {
        static ObjectExtenderRegistrator instance;
        return instance;
    }
};

struct ObjectCounter
{
    int types[h3::limits::OBJECTS] = {};
    int *subtypes[h3::limits::OBJECTS] = {};
    INT16 *maxSubtypes = nullptr;

  public:
    ObjectCounter(INT16 *maxSubtypes) : maxSubtypes(maxSubtypes)
    {
        for (int i = 0; i < h3::limits::OBJECTS; ++i)
        {
            types[i] = 0;
            if (maxSubtypes[i] > 0)
            {
                subtypes[i] = new int[maxSubtypes[i] + 1]();
            }
            else
            {
                subtypes[i] = nullptr;
            }
        }
    }
    ~ObjectCounter()
    {
        for (int i = 0; i < h3::limits::OBJECTS; ++i)
        {
            delete[] subtypes[i];
        }
    }

  public:
    inline size_t Type(const H3MapItem *mapItem) const
    {
        return types[mapItem->objectType];
    }
    inline size_t Subtype(const H3MapItem *mapItem) const
    {
        return subtypes[mapItem->objectType][mapItem->objectSubtype];
    }
    void Increment(const H3MapItem *mapItem)
    {
        int type = mapItem->objectType;
        int subtype = mapItem->objectSubtype;
        if (type >= 0 && type < h3::limits::OBJECTS)
        {
            ++types[type];
            if (subtype >= 0 && subtype <= maxSubtypes[type] && subtypes[type])
            {
                ++subtypes[type][subtype];
            }
        }
    }
};

class ObjectExtenderManager : public IGamePatch
{
  public:
    static constexpr LPCSTR DLG_HORIZONTAL_GAP = "\n\n\n";

  protected:
    BOOL skipMapMessageByHdMod = false;
    // contains all the extenders for objects addded by this and other plugins
    std::vector<ObjectExtender *> objectExtenders;
    // std::map<DWORD, ObjectExtender *> extendersMap;
    std::vector<RMGObjectProperties> additionalRmgObjects;
    INT16 lastObjectSubtypes[h3::limits::OBJECTS] = {};

    ObjectExtender *typeRelatedExtenders[h3::limits::OBJECTS]{};
    ObjectExtender **subTypeRelatedExtenders[h3::limits::OBJECTS]{{}};

    // std::array<std::vector<ObjectExtender *>, ObjectExtenderMethods::METHODS_COUNT> overridenMethods[232];

    // std::vector< ObjectExtender*> subTypeRelatedExtenders[232];
    //  contains all the additional properties to add/replace in objects.txt
    AdditionalProperties additionalProperties;
    ObjectCounter *objectCounter = nullptr;

  private:
    static ObjectExtenderManager *instance;

  private:
    ObjectExtenderManager();
    void CreatePatches() override;

  private:
    void InitializeObjectExtenders(const std::unordered_set<ObjectExtender *> &registeredExtenders);
    void AssignExtendersToObjectSubtypes();

    ObjectExtender *findExtender(const int type, const UINT subtype)
    {

        return subtype <= lastObjectSubtypes[type] ? subTypeRelatedExtenders[type][subtype] : nullptr;
        if (auto result = subTypeRelatedExtenders[type][subtype])
            return result;
        return typeRelatedExtenders[type];
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
    static void __stdcall Game__SetObjectsInitialParams(HiHook *h, H3Game *game);
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
    static BOOL ShowObjectExtendedInfo(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                       H3String &resultString) noexcept;

    static ObjectExtenderManager *Get();
    static void DebugObjectList();
    static void DebugObjectExtenderList();
};
DllExport BOOL __stdcall RegisterObjectExtenderOld(ObjectExtender *extender) noexcept;
DllExport LPCSTR __stdcall GetObjectName(const int objectType, const int objectSubtype) noexcept;

// Get the singleton instance

} // namespace extendersManager
