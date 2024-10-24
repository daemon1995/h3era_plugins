#pragma once

struct RMGObjectInfo
{

	//	zoneType 0..3 human-computer-treasure-junction*/

	INT type = eObject::NO_OBJ;
	INT subtype = eObject::NO_OBJ;
	union
	{
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
	BOOL fromTxt = false;


	RMGObjectInfo(const INT32 type, const INT32 subtype);
	RMGObjectInfo();
public:
	BOOL SetEnabled(const BOOL state) noexcept;
	BOOL Clamp() noexcept;
	void RestoreDefault() noexcept;
	void SetRandom() noexcept;
	void MakeReal() const noexcept;
	LPCSTR GetName() const noexcept;

public:
	static LPCSTR GetObjectName(const INT32 type, const INT32 subtype);
	static LPCSTR GetObjectName(const H3MapItem* mapItem);
};

struct GeneratedInfo
{
private:
	BOOL isInited = false;
	int maxObjectSubtype = NULL;

	union
	{
		struct
		{
			int* mapGeneratedBySubtype;
			int* eachZoneGeneratedBySubtype;
			int* zoneLimitsBySubtype;
			int* mapLimitsBySubtype;
		};
		int* arrays[4];
	};

public:
	void IncreaseObjectsCounters(const H3RmgObjectProperties* prop, const int zoneId);
	void Assign(const H3RmgRandomMapGenerator* rmg, const std::vector<std::vector<RMGObjectInfo>>& userRmgInfoSet);
	void Clear(const H3RmgRandomMapGenerator* rmgStruct);
	BOOL Inited() const noexcept;
public:
	BOOL ObjectCantBeGenerated(const H3RmgObjectGenerator* rmgObjGen, const int zoneId) const;

};
struct ObjectLimitsInfo
{

	INT32 mapTypesLimit[H3_MAX_OBJECTS]{};
	INT32 zoneTypeLimits[H3_MAX_OBJECTS]{};


	ADDRESS RMG_ObjectTypeZoneLimit = 0x69D244;
	ADDRESS RMG_ObjectTypeMapLimit = 0x69CE9C;

};

struct PseudoH3RmgRandomMapGenerator
{
	h3func* vTable{};                 /**< @brief [00]*/
	UINT32                         randomSeed{};             /**< @brief [04]*/
	INT32                          gameVersion = 3;            /**< @brief [08]*/
	H3RmgMap                        map;                    /**< @brief [0C]*/
	char _f_024[0x10];
	H3Vector<H3ObjectAttributes> objectPrototypes[232];
	char _f_0EB4[0x10];                /**< @brief [EB4]*/
	char _f_0EC4[0x10];                /**< @brief [EB4]*/
	h3unk32                        progress;               /**< @brief [ED4]*/
	BOOL8                          isHuman[8];             /**< @brief [ED8]*/
	INT32                          playerOwner[8];         /**< @brief [EE0]*/
	h3unk                          _f_f00[36];             /**< @brief [F00]*/
	INT32                          playerTown[8];          /**< @brief [F24]*/
	INT32                          monsterOrObjectCount;   /**< @brief [F44]*/
	INT32                          humanCount;             /**< @brief [F48]*/
	INT32                          humanTeams;             /**< @brief [F4C]*/
	INT32                          computerCount;          /**< @brief [F50]*/
	INT32                          computerTeams;          /**< @brief [F54]*/
	h3unk                          _f_f58[8];              /**< @brief [F58]*/
	INT32                          townsCount;             /**< @brief [F60]*/
	h3unk                          _f_f64[4];              /**< @brief [F64]*/
	h3unk                          _f_f68[32];             /**< @brief [F68]*/
	BOOL8                          bannedHeroes[156];      /**< @brief [F88]*/
	BOOL8                          bannedArtifacts[144];   /**< @brief [1024]*/
	h3unk                          _f_10B4[4];             /**< @brief [10B4]*/
	INT32                          waterAmount;            /**< @brief [10B8]*/
	INT32                          monsterStrength;        /**< @brief [10BC]*/
	H3String                       templateName;           /**< @brief [10C0]*/
	char        randomTemplates[0x10];        /**< @brief [10D0]*/
	char   zoneGenerators[0x10];         /**< @brief [10E0]*/
	H3Vector<H3RmgObjectGenerator*> objectGenerators;       /**< @brief [10F0]*/
	H3Vector<DWORD>               keyMasters;                /**< @brief [1100]*/

};

namespace editor
{
	class RMGObjectsEditor : public IGamePatch
	{

	private:
		// used to store default generated data
		PseudoH3RmgRandomMapGenerator pseudoH3RmgRandomMapGenerator;
		BOOL isPseudoGeneration = false;

		ObjectLimitsInfo limitsInfo;// = nullptr;

		std::vector<std::vector<RMGObjectInfo>> currentRMGObjectsInfoByType;
		std::vector<std::vector<RMGObjectInfo>> defaultRMGObjectsInfoByType;
		//PseudoH3RmgRandomMapGenerator randomMapGenerator;
		H3Vector<H3RmgObjectGenerator*>* defaultObjectGenerators;
		H3Vector<H3RmgObjectGenerator*> editedObjectGenerators;

	private:
		RMGObjectsEditor();
		virtual ~RMGObjectsEditor();

	private:
		virtual void CreatePatches() override;

	private:
		void InitDefaults(const INT16* maxSubtypes);
		void InitLoading(const INT16* maxSubtypes);
		void CreateGeneratedInfo(const H3RmgRandomMapGenerator* rmg);

	private:
		static _LHF_(RMG_OnBeforeMapGeneration);
		static _LHF_(RMG__ZoneGeneration__AfterObjectTypeZoneLimitCheck);
		static _LHF_(RMG__RMGObject_AtPlacement);

		static void __stdcall RMG__InitGenZones(HiHook* h, const H3RmgRandomMapGenerator* rmg, const H3RmgTemplate* RmgTemplate);
		static void __stdcall RMG__AfterMapGenerated(HiHook* h, H3RmgRandomMapGenerator* rmg);


		static void __stdcall RMG__CreateObjectGenerators(HiHook* h, H3RmgRandomMapGenerator* rmgStruct);

	public:

		const H3Vector<H3RmgObjectGenerator*>& GetObjectGeneratorsList() const noexcept;

		// Get vector of the information for all subtypes of that type
		const RMGObjectInfo& DefaultObjectInfo(const int objType, const int subtype) const noexcept;
		const RMGObjectInfo& CurrentObjectInfo(const int objType, const int subtype) const noexcept;
		int MaxMapTypeLimit(const UINT objType) const noexcept;
		void SetObjectInfoAsCurrent(const RMGObjectInfo& info) noexcept;

	public:

		static void Init(const INT16* maxSubtypes);
		static RMGObjectsEditor& Get() noexcept;

	};


}



