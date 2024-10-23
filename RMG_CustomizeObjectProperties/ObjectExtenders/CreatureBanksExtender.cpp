#include "..\pch.h"
namespace cbanks
{



	CreatureBanksExtender::CreatureBanksExtender()
		:ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.CreatureBanksExtender.daemon_n"))
	{
		bankCombatCheck = false;

		CreatePatches();

	}

	CreatureBanksExtender& CreatureBanksExtender::Get()
	{
		static CreatureBanksExtender _instance;
		return _instance;
	}

	_LHF_(CrBanksTxt_Ctor)
	{


		return EXEC_DEFAULT;
	}


	void CreatureBanksExtender::CreatePatches()
	{
		//m_isInited = true;

		if (m_isInited) return;

		m_isInited = true;




		//	_PI->WriteLoHook(0x47A3B0, CrBanksTxt_Ctor);



		_PI->WriteLoHook(0x4ABAD3, CrBank_BeforeCombatStart);
		_PI->WriteHiHook(0x4ABBCB, THISCALL_, CrBank_CombatStart);


		_PI->WriteHiHook(0x418580, THISCALL_, LoopSoundManager::AdvMgr_MapItem_Select_Sound);
		Era::RegisterHandler(LoopSoundManager::OnGameLeave, "OnGameLeave");
	}
	//#define _UNIQUE


	void __stdcall CreatureBanksExtender::LoopSoundManager::OnGameLeave(Era::TEvent* event)
	{

		//if (instance->soundManager.loopSoundChanged)
		{
			P_AdventureManager->loopSounds[7] = Get().soundManager.defaultWav;
			Get().soundManager.defaultWav = nullptr;
			Get().soundManager.loopSoundChanged = false;
		}
	}



	int __stdcall CreatureBanksExtender::LoopSoundManager::AdvMgr_MapItem_Select_Sound(HiHook* h, H3AdventureManager* adv, const int x, const int y, const int z)
	{


		H3MapItem* mapItem = adv->GetMapItem(x, y, z);

		int result = THISCALL_4(int, h->GetDefaultFunc(), adv, x, y, z);

		if (result == -1 && mapItem->objectType == 16)
		{

		}



		int crBankId = GetCreatureBankId(mapItem->objectType, mapItem->objectSubtype);
		if (crBankId >= Get().defaultBanksNumber
			&& Get().soundManager.loopSoundNames[crBankId] != h3_NullString)
		{



			if (!Get().soundManager.loopSounds[crBankId])
				Get().soundManager.loopSounds[crBankId] = H3WavFile::Load(Get().soundManager.loopSoundNames[crBankId]);

			P_AdventureManager->loopSounds[7] = Get().soundManager.loopSounds[crBankId];
			result = 7;
			Get().soundManager.loopSoundChanged = true;


			return result;

		}
		else if (Get().soundManager.loopSoundChanged)
		{
			if (!Get().soundManager.defaultWav)
				Get().soundManager.defaultWav = H3WavFile::Load("LoopCave.wav");
			P_AdventureManager->loopSounds[7] = Get().soundManager.defaultWav;// H3WavFile::Load("LoopCave.wav");
			Get().soundManager.loopSoundChanged = false;
		}
		return result;

		return EXEC_DEFAULT;
	}



	_LHF_(CreatureBanksExtender::CrBank_BeforeCombatStart)
	{
		H3MapItem* mapItem = reinterpret_cast<H3MapItem*>(c->ecx);
		if (mapItem && mapItem->objectType == eObject::CREATURE_BANK && Get().creatureBanks.isNotBank[mapItem->objectSubtype])
			Get().bankCombatCheck = true;


		return EXEC_DEFAULT;
	}

	signed int __stdcall  CreatureBanksExtender::CrBank_CombatStart(HiHook* h, UINT AdvMan, UINT PosMixed, UINT attHero, UINT attArmy, int PlayerIndex,
		UINT defTown, UINT defHero, UINT defArmy, int seed, signed int a10, int isBank)
	{

		if (Get().bankCombatCheck)
		{
			isBank = false;
			Get().bankCombatCheck = false;
		}
		return  THISCALL_11(signed int, h->GetDefaultFunc(), AdvMan, PosMixed, attHero, attArmy, PlayerIndex, defTown, defHero, defArmy, seed, a10, isBank);
	}

	const int CreatureBanksExtender::GetCreatureBankId(const int objType, const int objSubtype) noexcept
	{
		int cbId = -1;
		switch (objType)
		{
		case	eObject::CREATURE_BANK:
			cbId = objSubtype;
			break;
		case	eObject::DERELICT_SHIP:
			cbId = 8;
			break;
		case	eObject::DRAGON_UTOPIA:
			cbId = 10;
			break;
		case	eObject::CRYPT:
			cbId = 9;
			break;
		case	eObject::SHIPWRECK:
			cbId = 7;
			break;
		default:
			break;
		}

		return cbId;
	}

	const int CreatureBanksExtender::GetCreatureBankObjectType(const int cbId) noexcept
	{

		// get CB object type for some edits later
		eObject objectType = eObject::NO_OBJ;
		if (cbId >= 0)
		{
			switch (cbId)
			{
			case	7:
				objectType = eObject::SHIPWRECK;
				break;
			case	8:
				objectType = eObject::DERELICT_SHIP;
				break;
			case	9:
				objectType = eObject::CRYPT;
				break;
			case	10:
				objectType = eObject::DRAGON_UTOPIA;
				break;
			default:
				objectType = eObject::CREATURE_BANK;
				break;
			}
		}

		return objectType;
	}



	extern "C" __declspec(dllexport) void Name(size_t id, char* buff)
	{

		;
		if (id < CreatureBanksExtender::Get().Size())
		{
			sprintf(buff, "%s", H3CreatureBankSetup::Get()[id].name.String());
		}
		//return creatureBankExtender.m_size;
	}


	// SOUND FIND HERE 00418BB6

	//H3WavFile* loop = trSuccess ? H3WavFile::Load(loooSoundName.String()) : nullptr;



	void CreatureBanksExtender::AfterLoadingObjectTxtProc(const INT16* maxSubtypes)
	{

		// Get Default Banks Number from H3Vector<H3CreatureBankSetup::Ctor>::Size()
		defaultBanksNumber = ByteAt(0x47A3BA + 0x1);

		Resize(defaultBanksNumber);
		creatureBanks.CopyDefaultData(defaultBanksNumber);

		const int maxCreatureBankSubtype = maxSubtypes[eObject::CREATURE_BANK];


		// init vector sizes!
		int banksAdded = GetBanksSetupFromJson(maxCreatureBankSubtype);

		if (banksAdded)
		{		// set new Creature Bank Setups data at native array address
			//IntAt(0x67029C) = (int)instance->creatureBanks.setups.data();
			const DWORD newCbArrayAddress = DWORD(Get().creatureBanks.setups.data());
			// set new address for the using CB array ptr
			_pi->WriteDword(0x67029C, newCbArrayAddress);

			// set new address for the H3CreatureBankSetup init vector to use H3CreatureBankSetup::Get(); properly with new setup
			_pi->WriteDword(0x47A3C1 + 1, newCbArrayAddress);
		}
	}




	const int CreatureBanksExtender::GetBanksSetupFromJson(const INT16 maxSubtype)
	{

		addedBanksNumber = 0;


		auto& b = creatureBanks;
		//const int MAX_MON_ID = IntAt(0x4A1657);

		Reserve(30);
		//const H3String baseKey = "RMG.objectGeneration.16.%s.";

		H3String key;
		bool trSuccess = false;
		//return EXEC_DEFAULT;


		//// change rmg data
		//for (INT16 i = 0; i < maxSubtype; i++)
		//	SetRmgObjectGenData(i);

		for (INT16 i = defaultBanksNumber; i < maxSubtype; i++)
		{
			key = H3String::Format("RMG.objectGeneration.16.%d.isEnabled", i);

			int isEnabled = EraJS::readInt(key.String(), trSuccess);

			//if (isEnabled)
			{

				//SetRmgObjectGenData(i);

				LPCSTR loooSoundName = EraJS::read(H3String::Format("RMG.objectGeneration.16.%d.sound.loop", i).String(), trSuccess);
				soundManager.loopSoundNames.emplace_back(trSuccess ? loooSoundName : h3_NullString);

				H3WavFile* ptr = nullptr;
				soundManager.loopSounds.emplace_back(ptr);


				// states

				H3CreatureBankSetup setup;


				// assign new CB name from json/default
				H3String name = EraJS::read(H3String::Format("RMG.objectGeneration.16.%d.name", i).String(), trSuccess);
				if (!trSuccess && setup.name.Empty())
					name = H3ObjectName::Get()[eObject::CREATURE_BANK];
				setup.name = name;

				for (size_t state = 0; state < 4; state++)
				{

					setup.states[state].creatureRewardType = EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.states.%d.creatureRewardType", i, state).String());
					setup.states[state].creatureRewardCount = EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.states.%d.creatureRewardCount", i, state).String());
					setup.states[state].chance = EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.states.%d.chance", i, state).String());
					setup.states[state].upgrade = EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.states.%d.upgrade", i, state).String());

					for (size_t artLvl = 0; artLvl < 4; artLvl++)
						setup.states[state].artifactTypeCounts[artLvl] = EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.states.%d.artifactTypeCounts.%d", i, state, artLvl).String());

					for (size_t j = 0; j < 7; j++)
					{

						setup.states[state].guardians.type[j] = EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.states.%d.guardians.type.%d", i, state, j).String());
						setup.states[state].guardians.count[j] = EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.states.%d.guardians.count.%d", i, state, j).String());
						setup.states[state].resources.asArray[j] = EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.states.%d.resources.%d", i, state, j).String());
					}

				}

				int isNotBank = EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.isNotBank", i).String());
				b.isNotBank.emplace_back(isNotBank);
				b.setups.emplace_back(setup);
				b.monsterAwards.emplace_back(setup.states[0].creatureRewardType);

				std::array<int, 5> tempArr = { -1,-1,-1,-1,-1 };
				memcpy(&tempArr[0], &setup.states[0].guardians.type[0], sizeof(tempArr));

				b.monsterGuards.emplace_back(tempArr);
				addedBanksNumber++;
			}

		}

		ShrinkToFit();

		return addedBanksNumber;


	}


	const UINT CreatureBanksExtender::Size() const noexcept
	{
		return creatureBanks.m_size;
	}

	void CreatureBanksExtender::CreatureBank::CopyDefaultData(const size_t defaultSize)
	{

		int* currentCreatureRewardsArray = reinterpret_cast<int*>(IntAt(0x47A4A8 + 3));
		memcpy(monsterAwards.data(), currentCreatureRewardsArray, sizeof(int) * defaultSize);
		IntAt(0x47A4A8 + 3) = (int)monsterAwards.data();
		//instance->_pi->WriteWord(0x47A4A8 + 3, (int)monsterAwards.data());

		int* currentGuardiansArray = reinterpret_cast<int*>(IntAt(0x47A4AF + 3));
		memcpy(monsterGuards[0].data(), currentGuardiansArray, sizeof(int) * defaultSize * 5);
		IntAt(0x47A4AF + 3) = (int)monsterGuards[0].data();

		// get original bank setups data array
		H3CreatureBankSetup* originalBanks = *reinterpret_cast<H3CreatureBankSetup**>(0x67029C);
		if (originalBanks)
		{
			// iterate default data and copy into current array
			for (size_t i = 0; i < defaultSize; i++)
			{
				setups[i] = originalBanks[i];
			}
		}
	}



	CreatureBanksExtender::~CreatureBanksExtender()
	{
		creatureBanks.monsterAwards.clear();
		creatureBanks.monsterGuards.clear();
		creatureBanks.setups.clear();
		creatureBanks.isNotBank.clear();

		soundManager.loopSoundNames.clear();
		soundManager.loopSounds.clear();

	}




	void CreatureBanksExtender::Resize(UINT16 m_size) noexcept
	{
		creatureBanks.monsterAwards.resize(m_size);
		creatureBanks.monsterGuards.resize(m_size);
		creatureBanks.setups.resize(m_size);
		creatureBanks.isNotBank.resize(m_size);

		creatureBanks.m_size = m_size;

		soundManager.loopSoundNames.resize(m_size);
		soundManager.loopSounds.resize(m_size);

	}

	void CreatureBanksExtender::Reserve(UINT16 m_size) noexcept
	{
		creatureBanks.monsterAwards.reserve(m_size);
		creatureBanks.monsterGuards.reserve(m_size);
		creatureBanks.setups.reserve(m_size);
		creatureBanks.isNotBank.reserve(m_size);

		soundManager.loopSoundNames.reserve(m_size);
		soundManager.loopSounds.reserve(m_size);
	}

	void CreatureBanksExtender::ShrinkToFit() noexcept
	{
		creatureBanks.monsterAwards.shrink_to_fit();
		creatureBanks.monsterGuards.shrink_to_fit();
		creatureBanks.setups.shrink_to_fit();
		creatureBanks.isNotBank.shrink_to_fit();

		soundManager.loopSoundNames.shrink_to_fit();
		soundManager.loopSounds.shrink_to_fit();

	}

}