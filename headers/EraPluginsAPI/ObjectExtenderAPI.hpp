#pragma once
#include <type_traits>
#include "RuntimeServices.h"
#include "../Objects/ObjectLimits.h"

using namespace h3;
namespace extender
{
constexpr const char *RMGPluginName = "RMG_CustomizeObjectProperties.era";
constexpr const char *RMGPluginPath = "EraPlugins/RMG_CustomizeObjectProperties.era";
class ObjectExtender;
typedef const char *(__stdcall *TGetObjectName)(const int, const int);

constexpr int WOG_OBJECT_TYPE = eObject::PYRAMID;
constexpr int DECORATIVE_OBJECT_TYPE = eObject::BLANK1;        // obstackle that is hidden on puzzle_map
constexpr int DECORATIVE_PUZZLE_OBJECT_TYPE = eObject::BLANK2; // obstackle that isn't hidden on puzzle_map
constexpr int ERA_OBJECT_TYPE = eObject::BLANK3;
constexpr int WAREHOUSE_OBJECT_TYPE = eObject::BLANK4;
constexpr int HOTA_OBJECT_TYPE = eObject::BLANK5;
constexpr int HOTA_PICKUPABLE_OBJECT_TYPE = eObject::BLANK6;
constexpr int HOTA_UNREACHABLE_OBJECT_TYPE = eObject::BLANK7;

// namespace limits
struct UniqueObjectType
{
    INT16 type = eObject::NO_OBJ;
    INT16 subtype = eObject::NO_OBJ;
};
enum eRmgDlgObjectPage
{
    ePageUnknown = -1,
    ePageCommon,
    ePageLearning,
    ePageIncome,
    ePageCreatureBank,
    ePageDwelling,
    ePagePrison,
    ePageQuest = ePagePrison,
    ePageWogObject,
    ePageTreasure,
    ePandoraBox
};
struct UniqueObjectInfo
{
    struct FormatKey
    {
        static constexpr LPCSTR info = "RMG.objectGeneration.%d.%d.text.info";
        static constexpr LPCSTR hint = "RMG.objectGeneration.%d.%d.text.hint";
        static constexpr LPCSTR visit = "RMG.objectGeneration.%d.%d.text.visit";
        static constexpr LPCSTR visited = "RMG.objectGeneration.%d.%d.text.visited";
        static constexpr LPCSTR cannotVisit = "RMG.objectGeneration.%d.%d.text.cannotVisit";
    };

    UniqueObjectType uniqueObjectType;
    // size_t size = sizeof(UniqueObjectInfo);
    INT aiScoutingWeight = -1;
    // The dialog page can be assigned later, after the object has been registered.
    eRmgDlgObjectPage page = ePageUnknown;

  public:
    // H3String GetStringMessage(LPCSTR key) const
    //{
    //     H3String message = H3String::Format("{%s}", GetObjectName(uniqueObjectType.type, uniqueObjectType.subtype));
    //     message.Append(EraJS::read(H3String::Format(key, uniqueObjectType.type, uniqueObjectType.subtype).String()));
    //     return message;
    // }

    // H3String GetVisitingMessage() const
    //{
    //     return GetStringMessage(FormatKey::visit);
    // }

    // H3String GetVisitedMessage() const
    //{
    //     return GetStringMessage(FormatKey::visited);
    // }

    // H3String GetCannotVisitMessage() const
    //{
    //     return GetStringMessage(FormatKey::cannotVisit);
    // }
};
static_assert(std::is_standard_layout<UniqueObjectInfo>::value &&
                  std::is_trivially_copyable<UniqueObjectInfo>::value,
              "UniqueObjectInfo must remain a trivially copyable append-only ABI record");
using UniqueObjectInfoPtr = UniqueObjectInfo *;

// Registration is a C ABI boundary. The host must not derive an element count
// from sizeof(UniqueObjectInfo), because an extender may have been built with an
// older or newer version of this header.
enum eObjectInfoStorageFlags : UINT32
{
    OBJECT_INFO_CONTIGUOUS = 0,
    OBJECT_INFO_POINTERS = 1
};

#pragma pack(push, 4)
struct ObjectExtenderObjectInfoView
{
    UINT32 structSize = sizeof(ObjectExtenderObjectInfoView);
    const void *objectInfos = nullptr;
    UINT32 objectInfoCount = 0;
    UINT32 objectInfoSize = sizeof(UniqueObjectInfo);
    UINT32 objectInfoStride = sizeof(UniqueObjectInfoPtr);
    UINT32 flags = OBJECT_INFO_POINTERS;
};

typedef int(__stdcall *TGetObjectExtenderObjectInfos)(void *, ObjectExtenderObjectInfoView *);

struct ObjectExtenderRegistration
{
    static constexpr UINT32 API_VERSION = 2;

    UINT32 structSize = sizeof(ObjectExtenderRegistration);
    UINT32 apiVersion = API_VERSION;
    ObjectExtender *extender = nullptr;
    void *objectInfosContext = nullptr;
    TGetObjectExtenderObjectInfos getObjectInfos = nullptr;
};
#pragma pack(pop)

typedef int(__stdcall *TRegisterObjectExtenderEx)(const ObjectExtenderRegistration *);
using TObjectVisitObserver = void(__stdcall *)(H3Hero *, H3MapItem *, H3Position, BOOL);
using TRegisterObjectVisitObserver = BOOL(__stdcall *)(TObjectVisitObserver);
constexpr const char *DISPATCH_SERVICE = "ERA.Services.ObjectExtenders.v2";
struct DispatchApi
{
    unsigned structSize, version;
    TRegisterObjectExtenderEx registerExtender;
    TGetObjectName getName;
    TRegisterObjectVisitObserver registerObserver;
};
inline const DispatchApi *GetDispatchApi();

struct RMGObjectProperties
{
    constexpr static int DATA_SIZE = 5;
    constexpr static int UNDEFINED = -1;

    //	zoneType 0..3 human-computer-treasure-junction*/
    union {
        UniqueObjectType objectType;
        struct
        {
            INT16 type;
            INT16 subtype;
        };
    };
    union {
        struct
        {
            BOOL enabled;
            INT32 mapLimit;
            INT32 zoneLimit;
            INT32 value;
            INT32 density;
        };
        INT32 data[DATA_SIZE] = {};
    };

  public:
    inline RMGObjectProperties(INT16 type = eObject::NO_OBJ, INT16 subtype = eObject::NO_OBJ, BOOL enabled = FALSE,
                               INT32 mapLimit = UNDEFINED, INT32 zoneLimit = UNDEFINED, INT32 value = UNDEFINED,
                               INT32 density = UNDEFINED) noexcept
        : objectType{type, subtype}, enabled(enabled), mapLimit(mapLimit), zoneLimit(zoneLimit), value(value),
          density(density)
    {
    }

  public:
};
template <class T> inline static T *GetFromMapItem(H3MapItem *mapItem)
{
    return reinterpret_cast<T *>(&mapItem->setup);
}
// template <class T> inline static T &GetFromMapItem(H3MapItem *mapItem)
//{
//     return mapItem->setup;
// }
static HMODULE GetRMGPluginModule()
{
    static HMODULE pl = GetModuleHandleA(RMGPluginName);
    if (!pl)
        pl = LoadLibraryA(RMGPluginPath);

    return pl;
}
inline const char *GetObjectName(const int type, const int subtype)
{
    const auto *api = GetDispatchApi();
    return api && api->getName ? api->getName(type, subtype) : nullptr;
}

inline const char *GetObjectName(const H3MapItem *mapItem)
{
    return GetObjectName(mapItem->objectType, mapItem->objectSubtype);
}

inline const DispatchApi *GetDispatchApi()
{
    if (const auto *api = era_services::Find<DispatchApi>(DISPATCH_SERVICE, 2)) return api;
    // Preserve the existing explicit dependency bootstrap for extenders loaded
    // before RMG. Discovery of all functions still uses the shared table.
    GetRMGPluginModule();
    return era_services::Find<DispatchApi>(DISPATCH_SERVICE, 2);
}
inline BOOL RegisterObjectVisitObserver(TObjectVisitObserver observer)
{
    const auto *api = GetDispatchApi();
    return api && api->registerObserver && api->registerObserver(observer);
}

class ObjectExtender
{
  protected:
    // Keep the records outside of H3Vector. This keeps their addresses stable when
    // the vector grows and allows UniqueObjectInfo to be extended without changing
    // the layout of the vector itself.
    H3Vector<UniqueObjectInfoPtr> objectSubtypesInfo;
    BOOL m_isInited = FALSE;
    PatcherInstance *_pi = nullptr;

  protected:
    void ReleaseObjectSubtypesInfo() noexcept
    {
        H3ObjectAllocator<UniqueObjectInfo> allocator;
        for (auto *info : objectSubtypesInfo)
        {
            if (info)
            {
                allocator.destroy(info);
                allocator.deallocate(info);
            }
        }
        objectSubtypesInfo.RemoveAll();
    }

  private:
    static int __stdcall ProvideObjectInfos(void *context, ObjectExtenderObjectInfoView *view) noexcept
    {
        if (!context || !view)
            return FALSE;

        ObjectExtender *extender = static_cast<ObjectExtender *>(context);
        const auto &objects = extender->GetObjectSubtypesInfo();
        *view = ObjectExtenderObjectInfoView{};
        view->objectInfos = objects.begin();
        view->objectInfoCount = objects.Size();
        return TRUE;
    }

  public:
    ObjectExtender(PatcherInstance *_pi) : _pi(_pi) {};
    ObjectExtender();
    ObjectExtender(int objectType, int objectSubtype, PatcherInstance *_pi = nullptr)
    {
        if (objectType >= 0 && objectType < 232)
        {
            //  objectSubtypesInfo += UniqueObjectInfo{(INT16)objectType, (INT16)objectSubtype, -1};
        }
        _pi = this->_pi;
    }
    ObjectExtender(UniqueObjectInfo &info, PatcherInstance *_pi = nullptr);
    virtual ~ObjectExtender()
    {
        ReleaseObjectSubtypesInfo();
    };

  protected:
    virtual void CreatePatches()
    {
        if (!m_isInited)
            m_isInited = true;
    };
    // virtual void GetObjectPreperties() noexcept = 0;

  public:
    // required override for some complex structures like creature banks
    virtual void AfterLoadingObjectsTxtProc(const INT16 *maxSubtypes)
    {
    }
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectProperties &info,
                                                     const BOOL isPseudoGeneration) const noexcept
    {
        return CreateDefaultH3RmgObjectGenerator(info);
    }

    virtual BOOL InitNewGameMapItemSetup(H3MapItem *mapItem, int typeCounter, int subtypeByTypeCounter) const noexcept
    {
        return false;
    }
    virtual BOOL InitNewWeekMapItemSetup(H3MapItem *mapItem) const noexcept
    {
        return false;
    }
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept
    {
        return false;
    }

    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const int interactPlayerId,
                                       const BOOL isRightClick) const noexcept
    {
        return false;
    }
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiResWeight, int *moveDistance, const H3Position pos) const noexcept
    {
        return false;
    }
    virtual BOOL RMGDlg_ShowCustomObjectHint(const H3ObjectAttributes &attributes, H3String &defaultText) noexcept
    {
        return false;
    }

  public:
    inline H3Vector<UniqueObjectInfoPtr> &GetObjectSubtypesInfo() noexcept
    {
        return objectSubtypesInfo;
    }

    inline const H3Vector<UniqueObjectInfoPtr> &GetObjectSubtypesInfo() const noexcept
    {
        return objectSubtypesInfo;
    }

  public:
    void AddUniqueObjectInfo(const int type, const int subtype = -1, const int aiScouting = -1,
                             const eRmgDlgObjectPage page = ePageUnknown) noexcept
    {
        if (!object_config::IsObjectType(type) || (subtype != -1 && !object_config::IsObjectSubtype(subtype)))
            return;
        UniqueObjectInfo info{(INT16)type, (INT16)subtype, aiScouting};
        info.page = page;
        AddUniqueObjectInfo(info);
    }
    void AddUniqueObjectInfo(const UniqueObjectInfo &info) noexcept
    {
        if (object_config::IsObjectType(info.uniqueObjectType.type) && info.uniqueObjectType.subtype >= -1)
        {
            H3ObjectAllocator<UniqueObjectInfo> allocator;
            UniqueObjectInfo *storedInfo = allocator.allocate(1);
            if (!storedInfo)
                return;

            allocator.construct(storedInfo, info);

            if (!objectSubtypesInfo.Add(storedInfo))
            {
                allocator.destroy(storedInfo);
                allocator.deallocate(storedInfo);
            }
        }
    }

    BOOL Register() noexcept
    {
        return RegisterExtender(this) != NULL;
    }
    //	virtual int AiMapItemWeightFunction(HookContext* c, const H3MapItem* mapItem, H3Player* player);
    // returns if object was visited by some of derived classes
    // virtual BOOL HeroMapItemVisitFunction(HookContext* c, const H3Hero* hero, const H3MapItem* mapItem, const BOOL
    // isPlayer, const BOOL skipMapMessage);

  public:
    static ObjectExtender *CreateExtenderInstance(int objectType, PatcherInstance *pi) noexcept
    {

        return nullptr;
    }

    static H3RmgObjectGenerator *CreateDefaultH3RmgObjectGenerator(const RMGObjectProperties &info) noexcept
    {
        H3RmgObjectGenerator *objGen = nullptr;
        if (objGen = H3ObjectAllocator<H3RmgObjectGenerator>().allocate(1))
        {
            objGen = THISCALL_5(H3RmgObjectGenerator *, 0x534640, objGen, info.objectType.type, info.objectType.subtype,
                                info.value, info.density);
        }
        return objGen;
    }

  public:
    static inline int RegisterExtender(ObjectExtender *extender) noexcept
    {

        if (const auto *api = GetDispatchApi())
        {
            if (!api->registerExtender) return FALSE;
            ObjectExtenderRegistration registration;
            registration.extender = extender;
            registration.objectInfosContext = extender;
            registration.getObjectInfos = ProvideObjectInfos;
            return api->registerExtender(&registration);
        }
        return NULL;
    }
};

} // namespace extender
