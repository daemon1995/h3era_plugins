#include "pch.h"

CreatureBanksExtender* CreatureBanksExtender::instance = nullptr;


CreatureBanksExtender::CreatureBanksExtender(PatcherInstance* pi)
	:ObjectsExtender(pi)
{
	bankCombatCheck = false;

	CreatePatches();

}

CreatureBanksExtender* CreatureBanksExtender::Get(PatcherInstance* pi)
{
	if (!instance)
		instance = new CreatureBanksExtender(pi);
	return instance;
}

void CreatureBanksExtender::CreatePatches()
{
	//m_isInited = true;

	if (m_isInited) return;

	 m_isInited = true;

	_PI->WriteLoHook(0x4EDE42, CrBanksTxt_BeforeLoad);
	//_PI->WriteLoHook(0x4EDE4F, CrBanksTxt_AfterLoad);

	_PI->WriteLoHook(0x515038, LoadObjectsTxt);
	_PI->WriteLoHook(0x539C73, CreatureBanksListCreation);

	_PI->WriteLoHook(0x4ABAD3, CrBank_BeforeCombatStart);
	_PI->WriteHiHook(0x4ABBCB, THISCALL_, CrBank_CombatStart);

	_PI->WriteLoHook(0x4AA757, LoopSoundManager::MapItemSound_BeforeVisit);

	_PI->WriteLoHook(0x4185F9, LoopSoundManager::MapItemSound_BeforeCrBankTypeSelect);
	Era::RegisterHandler(LoopSoundManager::OnGameLeave, "OnGameLeave");
}


_LHF_(CreatureBanksExtender::CreatureBanksListCreation)
{

	H3Vector<H3RmgObjectGenerator*>* rmgObjecsList = reinterpret_cast<H3Vector<H3RmgObjectGenerator*>*>(c->esi);
	if (rmgObjecsList)
	{
		for (size_t i = instance->defaultBanksNumber; i < instance->totalBanksNumber; i++)
		{

			int s = instance->banks.cbId[0];

			std::unique_ptr<H3RmgObjectGenerator> crBankGen(new H3RmgObjectGenerator);
			//H3RmgObjectGenerator * crBankGen =  new H3RmgObjectGenerator;
			THISCALL_5(H3RmgObjectGenerator*, 0x534640, crBankGen.get(), 16, instance->banks.cbId[i], instance->banks.values[i], instance->banks.densities[i]);
			rmgObjecsList->Push(crBankGen.get());
		}
		for (size_t i = 11; i < 21; i++)
		{
			//std::unique_ptr<H3RmgObjectGenerator> crBankGen(new H3RmgObjectGenerator);
			//THISCALL_5(H3RmgObjectGenerator*, 0x534640, crBankGen.get(), 16, i, rand() % 20 *200 +700, rand() %10 *10 +10);
			//rmgObjecsList->Push(crBankGen.get());
		}

	}

	return EXEC_DEFAULT;
}

void __stdcall CreatureBanksExtender::LoopSoundManager::OnGameLeave(Era::TEvent* event)
{

	//if (instance->soundManager.loopSoundChanged)
	{
		P_AdventureManager->loopSounds[7] = instance->soundManager.defaultWav;
		instance->soundManager.defaultWav = nullptr;
		instance->soundManager.loopSoundChanged = false;
	}
}


_LHF_(CreatureBanksExtender::LoopSoundManager::MapItemSound_BeforeVisit)
{


	H3MapItem* mapItem = *reinterpret_cast<H3MapItem**>(c->ebp + 0x8);

	if (mapItem->objectType == eObject::CREATURE_BANK && mapItem->objectSubtype >= instance->defaultBanksNumber && instance->soundManager.enterSoundNames[mapItem->objectSubtype]!=h3_NullString)
	{

		instance->_pi->WriteDword(0x4AB21D + 1, instance->soundManager.enterSoundNames[mapItem->objectSubtype]);
		instance->_pi->WriteDword(0x4AB22D + 1, instance->soundManager.enterSoundNames[mapItem->objectSubtype]);
		instance->soundManager.enterSoundChanged = true;
	}
	else if (instance->soundManager.enterSoundChanged = true)
	{
		instance->soundManager.enterSoundChanged = false;
		instance->_pi->WriteDword(0x4AB21D + 1, 0x677854);
		instance->_pi->WriteDword(0x4AB22D + 1, 0x677854);
	}


	return EXEC_DEFAULT;
}



_LHF_(CreatureBanksExtender::LoopSoundManager::MapItemSound_BeforeCrBankTypeSelect)
{


	int crBankId = c->eax;
	if (crBankId >= instance->defaultBanksNumber
		&& instance->soundManager.loopSoundNames[crBankId] != h3_NullString)
	{



		if (!instance->soundManager.loopSounds[crBankId])
			instance->soundManager.loopSounds[crBankId] = H3WavFile::Load(instance->soundManager.loopSoundNames[crBankId]);

		P_AdventureManager->loopSounds[7] = instance->soundManager.loopSounds[crBankId];

		instance->soundManager.loopSoundChanged = true;
		c->return_address = 0x41862A;

		return NO_EXEC_DEFAULT;

	}
	else if (instance->soundManager.loopSoundChanged)
	{
		if (!instance->soundManager.defaultWav)
			instance->soundManager.defaultWav = H3WavFile::Load("LoopCave.wav");
		P_AdventureManager->loopSounds[7] = instance->soundManager.defaultWav;// H3WavFile::Load("LoopCave.wav");
		instance->soundManager.loopSoundChanged = false;
	}

	return EXEC_DEFAULT;
}



_LHF_(CreatureBanksExtender::CrBank_BeforeCombatStart)
{
	H3MapItem* mapItem = reinterpret_cast<H3MapItem*>(c->ecx);
	if (mapItem && mapItem->objectType == eObject::CREATURE_BANK && instance->banks.isNotBank[mapItem->objectSubtype])
		instance->bankCombatCheck = true;


	return EXEC_DEFAULT;
}

signed int __stdcall  CreatureBanksExtender::CrBank_CombatStart(HiHook* h, UINT AdvMan, UINT PosMixed, UINT attHero, UINT attArmy, int PlayerIndex,
	UINT defTown, UINT defHero, UINT defArmy, int seed, signed int a10, int isBank)
{
	if (instance->bankCombatCheck)
	{
		isBank = false;
		instance->bankCombatCheck = false;
	}
	return THISCALL_11(signed int, h->GetDefaultFunc(), AdvMan, PosMixed, attHero, attArmy, PlayerIndex, defTown, defHero, defArmy, seed, a10, isBank);
}

int CreatureBanksExtender::GetCreatureBankId(int objType, int objSubtype)
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






extern "C" __declspec(dllexport) int Size()
{
	return CreatureBanksExtender::instance->banks.setups.size();
}

extern "C" __declspec(dllexport) void Name(int id, char* buff)
{
	//if (id > -1 && id < creatureBankExtender.size)
	{
		//sprintf(buff, "%s", creatureBankExtender.bankSetups[id].name.String());
	}
	//return creatureBankExtender.size;
}

_LHF_(CreatureBanksExtender::CrBanksTxt_AfterLoad)
{



	return EXEC_DEFAULT;


}

struct RMGLimit
{
//	005467AB
	eObject objectId;
	int number;
};

// SOUND FIND HERE 00418BB6
bool ss = 0;
_LHF_(CreatureBanksExtender::LoadObjectsTxt)
{


	const UINT32 PROPERTIES_ADDED = instance->banks.properties.size();
	if (PROPERTIES_ADDED)
	{
		GemH3TextFile* objectTxt = *reinterpret_cast<GemH3TextFile**>(c->ebp + 0x8);

		// iterate new bankcs array
		for (auto it = instance->banks.properties.begin(); it != instance->banks.properties.end(); ++it)
		{
			//if (it->first == instance->banks.cbId[bankId])
			{

				objectTxt->AddLine(it->second);				// add new txt entry

			}
		}

		c->eax+= PROPERTIES_ADDED;

	}





	return EXEC_DEFAULT;

}

//H3WavFile* loop = trSuccess ? H3WavFile::Load(loooSoundName.String()) : nullptr;

int CreatureBanksExtender::GetBanksSetupFromJson()
{

	auto &b = banks;
	//const int MAX_MON_ID = IntAt(0x4A1657);

	Reserve(30);
	const H3String baseKey = "RMG.objectGeneration.16.%s.";
	constexpr int BANKS_MAX = 50;
	
	H3String key;
	bool trSuccess = false;
	//return EXEC_DEFAULT;
	addedBanksNumber = 0;
	for (size_t i = defaultBanksNumber; i < BANKS_MAX; i++)
	{
		key = H3String::Format("RMG.objectGeneration.16.%d.isEnabled", i);

		bool isEnabled = EraJS::readInt(key.String(), trSuccess);

		//if (isEnabled)
		{


			int subtype = i;

			// rmg
			//int vId = b.cbId.size();
			b.cbId.emplace_back(subtype);
			b.types.emplace_back(eObject::CREATURE_BANK);
			b.subtypes.emplace_back(subtype);

			int value = EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.value", i).String());
			b.values.emplace_back(value);
			int density = EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.density", i).String());
			b.densities.emplace_back(density);

			// sound

			LPCSTR loooSoundName = EraJS::read(H3String::Format("RMG.objectGeneration.16.%d.sound.loop", i).String(), trSuccess);
			soundManager.loopSoundNames.emplace_back(trSuccess ? loooSoundName : h3_NullString);

			LPCSTR enterSoundName = EraJS::read(H3String::Format("RMG.objectGeneration.16.%d.sound.enter", i).String(), trSuccess);
			soundManager.enterSoundNames.emplace_back(trSuccess ? enterSoundName : h3_NullString);
			H3WavFile* ptr = nullptr;
			soundManager.loopSounds.emplace_back(ptr);


			// properties
			bool propSuccess = false;
			int counter{};
			do
			{
				H3String str = EraJS::read(H3String::Format("RMG.objectGeneration.16.%d.properties.%d", i, counter++).String(), propSuccess);
				if (propSuccess)
				{
					char* sharedMemoryString = _strdup(str.String()); //_strdup(H3String::Format(str.String(), vId).String());
					b.properties.insert(std::make_pair(i, sharedMemoryString));
				}
			} while (propSuccess);

			// states

			H3CreatureBankSetup setup;
			key = H3String::Format("RMG.objectGeneration.16.%d.name", i);



			// assign new CB name from json/default
			H3String name = EraJS::read(key.String(), trSuccess);
			if (!trSuccess)
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

			bool isNotBank = EraJS::readInt(H3String::Format("RMG.objectGeneration.16.%d.isNotBank", i).String());
			b.isNotBank.push_back(isNotBank);
			b.setups.emplace_back(setup);
			b.monsterAwards.emplace_back(setup.states[0].creatureRewardType);

			std::array<int, 5> tempArr = { -1,-1,-1,-1,-1 };
			memcpy(&tempArr[0], &setup.states[0].guardians.type[0], sizeof(int) * 5);

			b.monsterGuards.emplace_back(tempArr);
			addedBanksNumber++;

		}

	}
	totalBanksNumber = defaultBanksNumber + addedBanksNumber;
	banks.size = totalBanksNumber;
	ShrinkToFit();

	return addedBanksNumber;


}


void CreatureBanksExtender::CreatureBank::CopyDefaultData(int defaultSize)
{

	int* currentCreatureRewardsArray = reinterpret_cast<int*>(IntAt(0x47A4A8 + 3));
	memcpy(monsterAwards.data(), currentCreatureRewardsArray, sizeof(int) * defaultSize);
	IntAt(0x47A4A8 + 3) = (int)monsterAwards.data();
	//instance->_pi->WriteWord(0x47A4A8 + 3, (int)monsterAwards.data());

	int* currentGuardiansArray = reinterpret_cast<int*>(IntAt(0x47A4AF + 3));
	memcpy(monsterGuards[0].data(), currentGuardiansArray, sizeof(int) * defaultSize * 5);
	IntAt(0x47A4AF + 3) = (int)monsterGuards[0].data();
	//instance->_pi->WriteWord(0x47A4AF + 3, (int)monsterGuards[0].data());

}
_LHF_(CreatureBanksExtender::CrBanksTxt_BeforeLoad)
{

	//RMGLimit* zoneLimits;
	//h->Undo();
    // Init Default Data
	int defaultBanksNumber = ByteAt(0x47A3BA + 0x1);
	instance->defaultBanksNumber = defaultBanksNumber;
	//return EXEC_DEFAULT;
	instance->totalBanksNumber = instance->defaultBanksNumber;
	instance->Resize(defaultBanksNumber);

	// init vector sizes!
	int banksAdded = instance->GetBanksSetupFromJson();
	if (banksAdded)
	{



		instance->banks.CopyDefaultData(defaultBanksNumber);

		// Set new States
		ByteAt(0x47A3BA + 0x1) = instance->totalBanksNumber; // increase ctor
		ByteAt(0x47A3E5 + 0x1) = instance->totalBanksNumber; // increase dtor
		instance->banks.monsterGuards[0].data();

		IntAt(0x47A3C1 + 1) = (int)instance->banks.setups.data();
		IntAt(0x47A3EC + 1) = (int)instance->banks.setups.data();
		IntAt(0x47A4B6 + 3) = (int)&instance->banks.setups[0].name.m_string;
		//ByteAt(0x47A362 + 2) = instance->defaultBanksNumber;
		//CDECL_0(void, 0x47A350);
		IntAt(0x47A68F + 1) = (int)(&instance->banks.setups[instance->defaultBanksNumber].name.m_string);

		IntAt(0x67029C) = (int)instance->banks.setups.data();

		// assign original CBs ID's
		for (size_t i = 0; i < 7; i++)
			instance->banks.cbId[i] = i;
		for (size_t i = 11; i < 21; i++)
			instance->banks.cbId[i] = i;

		//instance->banks.cbId[7] = ;

	}
	else
	{
		instance->_pi->UndoAll();
		//delete instance;
		//instance = nullptr;
	}



	return EXEC_DEFAULT;
}





CreatureBanksExtender::~CreatureBanksExtender()
{
	banks.properties.clear();
	banks.monsterAwards.clear();
	banks.monsterGuards.clear();
	banks.setups.clear();
	banks.values.clear();
	banks.densities.clear();
	banks.isNotBank.clear();
	banks.cbId.clear();

	soundManager.enterSoundNames.clear();
	soundManager.loopSoundNames.clear();
	soundManager.loopSounds.clear();

}

void GemH3TextFile::AddLine(LPCSTR txt)
{
	this->text.Add(txt);
}


void CreatureBanksExtender::Resize(int size) noexcept
{
	banks.monsterAwards.resize(size);
	banks.monsterGuards.resize(size);
	banks.setups.resize(size);
	banks.values.resize(size);
	banks.densities.resize(size);
	banks.cbId.resize(size);
	banks.isNotBank.resize(size);
	banks.size = size;

	soundManager.enterSoundNames.resize(size);
	soundManager.loopSoundNames.resize(size);
	soundManager.loopSounds.resize(size);

}

void CreatureBanksExtender::Reserve(int size) noexcept
{
	banks.monsterAwards.reserve(size);
	banks.monsterGuards.reserve(size);
	banks.setups.reserve(size);
	banks.values.reserve(size);
	banks.densities.reserve(size);
	banks.cbId.reserve(size);
	banks.isNotBank.reserve(size);

	soundManager.enterSoundNames.reserve(size);
	soundManager.loopSoundNames.reserve(size);
	soundManager.loopSounds.reserve(size);
}

void CreatureBanksExtender::ShrinkToFit() noexcept
{
	banks.monsterAwards.shrink_to_fit();
	banks.monsterGuards.shrink_to_fit();
	banks.setups.shrink_to_fit();
	banks.values.shrink_to_fit();
	banks.densities.shrink_to_fit();
	banks.isNotBank.shrink_to_fit();
	banks.cbId.shrink_to_fit();

	soundManager.enterSoundNames.shrink_to_fit();
	soundManager.loopSoundNames.shrink_to_fit();
	soundManager.loopSounds.shrink_to_fit();

}
