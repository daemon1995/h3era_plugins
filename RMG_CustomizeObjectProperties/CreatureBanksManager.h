#pragma once


#include <array>


class CreatureBanksManager : public extender::ObjectsExtender
{

public:


	UINT16 defaultBanksNumber = 0;
	UINT16 addedBanksNumber = 0;
	BOOL bankCombatCheck;

	struct CreatureBank
	{
		UINT m_size;

		std::vector<int> monsterAwards;
		std::vector<std::array<int, 5>> monsterGuards;
		std::vector<H3CreatureBankSetup> setups;
		std::vector<int> isNotBank;


	public:
		void CopyDefaultData(const size_t defaultSize);
		const UINT Size() const noexcept;

	} creatureBanks;

	struct LoopSoundManager
	{
		BOOL enterSoundChanged = false;
		BOOL loopSoundChanged = false;
		H3WavFile* defaultWav = nullptr;


		std::vector<H3WavFile*> loopSounds;

		std::vector<LPCSTR> loopSoundNames;

	public:
		static int __stdcall AdvMgr_MapItem_Select_Sound(HiHook* h, H3AdventureManager* adv, const int x, const int y, const int z);

		static void __stdcall OnGameLeave(Era::TEvent* event);


	} soundManager;

	void Resize(UINT16 m_size) noexcept;
	void Reserve(UINT16 m_size) noexcept;
	void ShrinkToFit() noexcept;


private:

	CreatureBanksManager();
	virtual ~CreatureBanksManager();


private:

	virtual void CreatePatches() override;
	virtual void AferLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
	//virtual void GetObjectPreperties() noexcept override  final;

	const int GetBanksSetupFromJson(const INT16 maxSubtype);
	//	void GetBanksFromJson();

	void SetRmgObjectGenData(const int objectSubtype)  noexcept;

private:

	//static _LHF_(RMG__CreateObjectGenerators);
	static _LHF_(CrBank_BeforeCombatStart);
	static signed int __stdcall CrBank_CombatStart(HiHook* h, UINT AdvMan, UINT PisMixed, UINT attHero, UINT attArmy, int PlayerIndex, UINT defTown, UINT defHero, UINT defArmy, int seed, signed int a10, int isBank);

	//	static int __stdcall CretureBankSetups__Ctor(HiHook*h);

public:
	static const int GetCreatureBankId(const int objType, const int objSubtype) noexcept;
	static const int GetCreatureBankObjectType(const int cbId) noexcept;

	static CreatureBanksManager& Get();

};

