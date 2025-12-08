#pragma once
struct ObjectLimitsInfo;
#include <array>

struct _RMGObjGenScroll_ : public H3RmgObjectGenerator
{
    static constexpr UINT MAP_CONTROL_SPELL_LEVEL = 6;
    static constexpr UINT BANNED_SPELL_LEVEL = 7;
    static constexpr UINT MAP_CONTROL_SPELL_VALUE = 20000;
    int spellLevel;
};
struct _RMGObjGenPrison_ : public H3RmgObjectGenerator
{
    int Exp;
};
struct _RMGObjGenPandoraMon_ : public H3RmgObjectGenerator
{
    int montype;
    int objectValue;
};

struct _RMGObjGenPandoraMagic_ : public H3RmgObjectGenerator
{
    int minLevel;
    int maxLevel;
    int schoolBit;
};
struct _RMGObjGenPandoraGold_ : public H3RmgObjectGenerator
{
    int gold;
};
struct _RMGObjGenPandoraExp_ : public H3RmgObjectGenerator
{
    int exp;
};
struct RMGObjectInfo
{

    constexpr static int SIZE = 5;
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
        INT32 data[5] = {};
    };
    // BOOL fromTxt = false;

  public:
    static constexpr LPCSTR PROPERTY_NAMES[] = {"enabled", "map", "zone", "value", "density"};
    static constexpr LPCSTR INI_FILE_PATH = "Runtime/RMG_CustomizeObjectsProperties.ini";
    static constexpr LPCSTR OBJECT_INFO_INI_FORMAT = "%d_%d_%d";

    static constexpr LPCSTR OBJECT_TYPE_PROPERTY_JSON_KEY_FORMAT = "RMG.objectGeneration.%d.%s";
    static constexpr LPCSTR OBJECT_SUBTYPE_PROPERTY_JSON_KEY_FORMAT = "RMG.objectGeneration.%d.%d.%s";
    static constexpr LPCSTR OBJECT_SUBTYPE_NAME_JSON_KEY_FORMAT = "RMG.objectGeneration.%d.%d.name";
    static constexpr LPCSTR OBJECT_SUBTYPE_DESC_JSON_KEY_FORMAT = "RMG.objectGeneration.%d.%d.description";

  public:
    static std::vector<RMGObjectInfo> currentRMGObjectsInfoByType[h3::limits::OBJECTS];
    static std::vector<RMGObjectInfo> defaultRMGObjectsInfoByType[h3::limits::OBJECTS];

  public:
    RMGObjectInfo(const INT32 type, const INT32 subtype);
    RMGObjectInfo();

  public:
    BOOL SetEnabled(const BOOL state) noexcept;
    BOOL Clamp() noexcept;
    void RestoreDefault() noexcept;
    void SetRandom() noexcept;
    void MakeReal() const noexcept;
    inline LPCSTR GetRmgTypeDescription() const noexcept;
    inline LPCSTR GetRmgSubtypeDescription() const noexcept;
    LPCSTR GetName() const noexcept;
    LPCSTR GetDescription() const noexcept;

    BOOL WriteToINI() const noexcept;
    inline void ReadFromINI() noexcept;

  public:
    static const RMGObjectInfo &DefaultObjectInfo(const int objType, const int subtype) noexcept;
    static const RMGObjectInfo &CurrentObjectInfo(const int objType, const int subtype) noexcept;
    static const std::vector<RMGObjectInfo> (&CurrentObjectInfo())[limits::OBJECTS];

    static void InitFromRmgObjectGenerator(const H3RmgObjectGenerator &);
    static void InitDefaultProperties(const ObjectLimitsInfo &limitInfo, const INT16 *maxSubtypes);
    static void LoadUserProperties(const INT16 *maxSubtypes);
    static LPCSTR GetObjectName(const INT32 type, const INT32 subtype);
    static LPCSTR GetObjectName(const H3MapItem *mapItem);
    static LPCSTR GetObjectDescription(const INT32 type, const INT32 subtype);
    static LPCSTR GetObjectDescription(const H3MapItem *mapItem);
};

struct GeneratedInfo
{
  private:
    BOOL isInited = false;
    int maxObjectSubtype = NULL;

    union {
        struct
        {
            int *mapGeneratedBySubtype;
            int *eachZoneGeneratedBySubtype;
            int *zoneLimitsBySubtype;
            int *mapLimitsBySubtype;
        };
        int *arrays[4];
    };

  public:
    const _RMGObjGenScroll_ *lastGeneratedSpellScroll = nullptr;

  public:
    void IncreaseObjectsCounters(const H3RmgObjectProperties *prop, const int zoneId);
    void Assign(const H3RmgRandomMapGenerator *rmg,
                const std::vector<RMGObjectInfo> (&userRmgInfoSet)[h3::limits::OBJECTS]);
    void Clear(const H3RmgRandomMapGenerator *rmgStruct);
    BOOL Inited() const noexcept;

  public:
    BOOL ObjectCantBeGenerated(const H3RmgObjectGenerator *rmgObjGen, const int zoneId) const;
};
struct ObjectLimitsInfo
{

    INT32 mapTypesLimit[H3_MAX_OBJECTS]{};
    INT32 zoneTypeLimits[H3_MAX_OBJECTS]{};

    ADDRESS RMG_ObjectTypeZoneLimit = 0x69D244;
    ADDRESS RMG_ObjectTypeMapLimit = 0x69CE9C;
};

namespace editor
{

struct PseudoH3RmgRandomMapGenerator
{
    h3func *vTable{};      /**< @brief [00]*/
    UINT32 randomSeed{};   /**< @brief [04]*/
    INT32 gameVersion = 3; /**< @brief [08]*/
    H3RmgMap map;          /**< @brief [0C]*/
    char _f_024[0x10];
    H3Vector<H3ObjectAttributes> objectPrototypes[232];
    char _f_0EB4[0x10];                                /**< @brief [EB4]*/
    char _f_0EC4[0x10];                                /**< @brief [EB4]*/
    h3unk32 progress;                                  /**< @brief [ED4]*/
    BOOL8 isHuman[8];                                  /**< @brief [ED8]*/
    INT32 playerOwner[8];                              /**< @brief [EE0]*/
    h3unk _f_f00[36];                                  /**< @brief [F00]*/
    INT32 playerTown[8];                               /**< @brief [F24]*/
    INT32 monsterOrObjectCount;                        /**< @brief [F44]*/
    INT32 humanCount;                                  /**< @brief [F48]*/
    INT32 humanTeams;                                  /**< @brief [F4C]*/
    INT32 computerCount;                               /**< @brief [F50]*/
    INT32 computerTeams;                               /**< @brief [F54]*/
    h3unk _f_f58[8];                                   /**< @brief [F58]*/
    INT32 townsCount;                                  /**< @brief [F60]*/
    h3unk _f_f64[4];                                   /**< @brief [F64]*/
    h3unk _f_f68[32];                                  /**< @brief [F68]*/
    BOOL8 bannedHeroes[156];                           /**< @brief [F88]*/
    BOOL8 bannedArtifacts[144];                        /**< @brief [1024]*/
    h3unk _f_10B4[4];                                  /**< @brief [10B4]*/
    INT32 waterAmount;                                 /**< @brief [10B8]*/
    INT32 monsterStrength;                             /**< @brief [10BC]*/
    H3String templateName;                             /**< @brief [10C0]*/
    char randomTemplates[0x10];                        /**< @brief [10D0]*/
    char zoneGenerators[0x10];                         /**< @brief [10E0]*/
    H3Vector<H3RmgObjectGenerator *> objectGenerators; /**< @brief [10F0]*/
    H3Vector<DWORD> keyMasters;                        /**< @brief [1100]*/
};

namespace fixes
{
struct RMGFixes
{
  private:
    static DWORD correctObjectPrototypeRef;

  protected:
    static _LHF_(RMG__AtSubterranianGatesPrototypeGet);
    static _LHF_(RMG__AtSecondSubterranianGatesPositioning);
    static _LHF_(RMG__AtSecondSubterranianGatesPlacement);

  public:
    static void CreatePatches(PatcherInstance *_pi);
};
} // namespace fixes
class RMGObjectsEditor : public IGamePatch
{
    static RMGObjectsEditor *instance;
    static GeneratedInfo generatedInfo;

  private:
    // used to store default generated data
    PseudoH3RmgRandomMapGenerator pseudoH3RmgRandomMapGenerator;
    BOOL isPseudoGeneration = false;

    ObjectLimitsInfo limitsInfo; // = nullptr;
    std::array<INT, limits::SPELLS> spellLvls = {};
    // std::vector<RMGObjectInfo> currentRMGObjectsInfoByType[h3::limits::OBJECTS];
    //  std::vector<RMGObjectInfo> defaultRMGObjectsInfoByType[h3::limits::OBJECTS];
    //  std::array<std::vector<RMGObjectInfo>, h3::limits::OBJECTS> defaultRMGObjectsInfoByType;

    H3Vector<H3RmgObjectGenerator *> *originalRMGObjectGenerators = nullptr;
    H3Vector<H3RmgObjectGenerator *> editedRMGObjectGenerators;

  private:
    RMGObjectsEditor();
    virtual ~RMGObjectsEditor();

  private:
    virtual void CreatePatches() override;

  private:
    void InitDefaultProperties(const INT16 *maxSubtypes);

    void BeforeMapGeneration(const H3RmgRandomMapGenerator *rmgStruct);
    void AfterMapGeneration(H3RmgRandomMapGenerator *rmgStruct) noexcept;
    //		void CreateGeneratedInfo(const H3RmgRandomMapGenerator* rmg);
    void SetMapControlSpellLevels(const BOOL state, const BOOL blockWaterSpells = true) noexcept;

  private:
    static _LHF_(RMG_OnBeforeMapGeneration);
    static _LHF_(RMG__ZoneGeneration__AfterObjectTypeZoneLimitCheck);
    static _LHF_(RMG__RMGObject_AtPlacement);
    static H3RmgObject *__stdcall RMG__RMGObjGenScroll__CreateObject(HiHook *h, _RMGObjGenScroll_ *scrollGen,
                                                                     H3RmgObjectPropsRef *ref,
                                                                     H3RmgRandomMapGenerator *rmg,
                                                                     H3RmgZoneGenerator *zoneGen) noexcept;

    static void __stdcall RMG__InitGenZones(HiHook *h, const H3RmgRandomMapGenerator *rmg,
                                            const H3RmgTemplate *RmgTemplate);
    static void __stdcall RMG__AfterMapGenerated(HiHook *h, H3RmgRandomMapGenerator *rmg);

    static void __stdcall RMG__CreateObjectGenerators(HiHook *h, H3RmgRandomMapGenerator *rmgStruct);

  public:
    const H3Vector<H3RmgObjectGenerator *> *GetObjectGeneratorsList() const noexcept;

    // Get vector of the information for all subtypes of that type

    int MaxMapTypeLimit(const UINT objType) const noexcept;

  public:
    static void Init(const INT16 *maxSubtypes);
    static RMGObjectsEditor &Get() noexcept;
};

} // namespace editor
