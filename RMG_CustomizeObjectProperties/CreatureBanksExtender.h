#pragma once
#include "pch.h"
struct GemH3TextFile : public H3TextFile
{
	void AddLine(LPCSTR txt);
};


class CreatureBanksExtender :  public ObjectsExtender
{
private:

	CreatureBanksExtender(PatcherInstance* pi);



public:

	
	int defaultBanksNumber = 0;
	int addedBanksNumber = 0;
	int totalBanksNumber;
	struct CreatureBank
	{
		std::vector<int> monsterAwards;
		std::vector<std::array<int, 5>> monsterGuards;
		std::vector<H3CreatureBankSetup> setups;
		std::vector<int> values;
		std::vector<int> densities;
		std::vector<int> cbId;
		std::vector<int> isNotBank;
		std::multimap<int, std::string> properties;


		std::vector<int> types;
		std::vector<int> subtypes;


		int size;


		void CopyDefaultData(int defaultSize);

	} banks;

	bool bankCombatCheck;

	void Resize(int size) noexcept;
	void Reserve(int size) noexcept;
	void ShrinkToFit() noexcept;
	struct LoopSoundManager
	{

		std::vector<H3WavFile*> loopSounds;
		std::vector<LPCSTR> loopSoundNames;
		std::vector<LPCSTR> enterSoundNames;

		static _LHF_(MapItemSound_BeforeCrBankTypeSelect);
		static _LHF_(MapItemSound_BeforeVisit);
		static void __stdcall OnGameLeave(Era::TEvent* event);
		H3WavFile* defaultWav = nullptr;
		bool enterSoundChanged = false;
		bool loopSoundChanged = false;

	} soundManager;


private:

	int GetBanksSetupFromJson();
//	void GetBanksFromJson();
	void CreatePatches() override;
	static _LHF_(CrBanksTxt_BeforeLoad);
	static _LHF_(CrBanksTxt_AfterLoad);
	static _LHF_(LoadObjectsTxt);
	static _LHF_(CreatureBanksListCreation);

	static _LHF_(CrBank_BeforeCombatStart);
	static signed int __stdcall  CrBank_CombatStart(HiHook* h, UINT AdvMan, UINT PisMixed, UINT attHero, UINT attArmy, int PlayerIndex, UINT defTown, UINT defHero, UINT defArmy, int seed, signed int a10, int isBank);
public:
	static CreatureBanksExtender* instance;
	static int GetCreatureBankId(int objType, int objSubtype);

    //struct;

	virtual ~CreatureBanksExtender();

	static CreatureBanksExtender* Get(PatcherInstance* pi);

};

