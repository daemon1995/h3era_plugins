#pragma once

#include <array>

namespace cbanks
{


	class CreatureBanksExtender : public extender::ObjectsExtender
	{

	private:


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

	private:
		CreatureBanksExtender();
		virtual ~CreatureBanksExtender();

	private:

		virtual void CreatePatches() override;
		virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
		//virtual void GetObjectPreperties() noexcept override  final;
	private:
		const int GetBankSetupsNumberFromJson(const INT16 maxSubtype);
	//	void SetRmgObjectGenData(const int objectSubtype)  noexcept;
		void Resize(UINT16 m_size) noexcept;
		void Reserve(UINT16 m_size) noexcept;
		void ShrinkToFit() noexcept;

	private:

		//static _LHF_(RMG__CreateObjectGenerators);
		static _LHF_(CrBank_BeforeCombatStart);
		static signed int __stdcall CrBank_CombatStart(HiHook* h, UINT AdvMan, UINT PisMixed, UINT attHero, UINT attArmy, int PlayerIndex, UINT defTown, UINT defHero, UINT defArmy, int seed, signed int a10, int isBank);

		static void __stdcall OnAfterReloadLanguageData(Era::TEvent* event);
		//	static int __stdcall CretureBankSetups__Ctor(HiHook*h);
	public:

		UINT Size() const noexcept;

	public:
		static int GetCreatureBankId(const int objType, const int objSubtype) noexcept;
		static int GetCreatureBankObjectType(const int cbId) noexcept;

		static CreatureBanksExtender& Get();

	};

}



