#pragma once
using namespace h3;
namespace extender
{
constexpr const char *RMGPluginName = "RMG_CustomizeObjectProperties.era";
constexpr const char *RMGPluginPath = "EraPlugins/RMG_CustomizeObjectProperties.era";
class ObjectExtender;
typedef int(__stdcall *TRegisterObjectExtender)(ObjectExtender *);
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
struct RMGObjectProperties
{
    ;
    constexpr static int DATA_SIZE = 5;
    constexpr static int UNDEFINED = -1;

    //	zoneType 0..3 human-computer-treasure-junction*/
    INT type = eObject::NO_OBJ;
    INT subtype = eObject::NO_OBJ;
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
    inline RMGObjectProperties(INT type = eObject::NO_OBJ, INT subtype = eObject::NO_OBJ, BOOL enabled = FALSE,
                               INT32 mapLimit = UNDEFINED, INT32 zoneLimit = UNDEFINED, INT32 value = UNDEFINED,
                               INT32 density = UNDEFINED) noexcept
        : type(type), subtype(subtype), enabled(enabled), mapLimit(mapLimit), zoneLimit(zoneLimit), value(value),
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
    if (HMODULE pl = GetRMGPluginModule())
    {
        static TGetObjectName f = TGetObjectName(GetProcAddress(pl, "GetObjectName"));
        if (f)
            return f(type, subtype);
    }
    return NULL;
}
inline const char *GetObjectName(const H3MapItem *mapItem)
{
    return GetObjectName(mapItem->objectType, mapItem->objectSubtype);
}
class ObjectExtender
{
  protected:
    int objectType = eObject::NO_OBJ;
    H3Vector<UINT> objectSubtypes;
    BOOL m_isInited = FALSE;
    PatcherInstance *_pi = nullptr;

  public:
    ObjectExtender(PatcherInstance *_pi) : _pi(_pi) {};
    ObjectExtender();
    virtual ~ObjectExtender() {};

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
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectProperties &info) const noexcept
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
    inline int GetObjectType() const noexcept
    {
        return objectType;
    }
    inline const H3Vector<UINT> &GetObjectSubtypes() const noexcept
    {
        return objectSubtypes;
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
            objGen =
                THISCALL_5(H3RmgObjectGenerator *, 0x534640, objGen, info.type, info.subtype, info.value, info.density);
        }
        return objGen;
    }

  public:
    static inline int RegisterExtender(ObjectExtender *extender) noexcept
    {

        if (HMODULE pl = GetRMGPluginModule())
        {
            static TRegisterObjectExtender f = TRegisterObjectExtender(GetProcAddress(pl, "RegisterObjectExtender"));
            if (f)
                return f(extender);
        }
        return NULL;
    }
};

} // namespace extender
