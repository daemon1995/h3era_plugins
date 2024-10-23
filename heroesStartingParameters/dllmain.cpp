// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "framework.h"
Patcher* globalPatcher;
PatcherInstance* _PI;
using namespace h3;

//H3String jsonKeys[12] = {"hasSpellBook",};

constexpr int HEROES_COUNT = h3::limits::HEROES;
void ChangeHeroStartingParameters(H3HeroInfo& heroInfo, int id)
{

	//std::string jsonKeyStart = "gem.%d";

	heroInfo.campaignHero = false;
	heroInfo.expansionHero = true;
	sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.hasSpellBook", id);
	std::string trResult = Era::tr(h3_TextBuffer);

	if (trResult != h3_TextBuffer)
		heroInfo.hasSpellbook = atoi(trResult.c_str());

	sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.startingSpell", id);
	trResult = Era::tr(h3_TextBuffer);

	if (trResult != h3_TextBuffer)
	{
		int spellId = atoi(trResult.c_str());
		spellId = Clamp(eSpell::NONE, spellId, eSpell::AIR_ELEMENTAL);
		heroInfo.startingSpell = spellId;
	}


	for (size_t i = 0; i < 2; i++)
	{
		sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.skill%d.type", id, i);
		trResult = Era::tr(h3_TextBuffer);
		if (trResult != h3_TextBuffer)
		{
			int skillId = atoi(trResult.c_str());
			skillId = Clamp(eSecondary::NONE, skillId, eSecondary::FIRST_AID);
			heroInfo.sskills[i].type = (eSecondary)skillId;
			if (skillId != eSecondary::NONE)
			{
				sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.skill%d.level", id, i);
				int skillLvl = eSecSkillLevel::BASIC;
				trResult = Era::tr(h3_TextBuffer);
				if (trResult != h3_TextBuffer)
				{
					skillLvl = atoi(trResult.c_str());
					skillLvl = Clamp(eSecSkillLevel::BASIC, skillLvl, eSecSkillLevel::EXPERT);

				}
				heroInfo.sskills[i].level = (eSecSkillLevel)skillLvl;
			}
		}
	}

	const int MAX_CREATURE_ID = IntAt(0x4A1657);

	for (size_t i = 0; i < 3; i++)
	{
		sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.army%d.type", id, i);
		trResult = Era::tr(h3_TextBuffer);
		if (trResult != h3_TextBuffer)
		{
			int armyType = atoi(trResult.c_str());
			armyType = Clamp(eCreature::UNDEFINED, armyType, MAX_CREATURE_ID);
			heroInfo.armyType[i] = armyType;
			if (armyType != eCreature::UNDEFINED)
			{
				sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.army%d.low", id, i);
				int low = heroInfo.creatureAmount[i].lowAmount;
				trResult = Era::tr(h3_TextBuffer);
				if (trResult != h3_TextBuffer)
				{
					low = atoi(trResult.c_str());
					low = Clamp(1, low, INT32_MAX);
				}
				heroInfo.creatureAmount[i].lowAmount = low;
				sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.army%d.high", id, i);
				int high = heroInfo.creatureAmount[i].highAmount;
				trResult = Era::tr(h3_TextBuffer);
				if (trResult != h3_TextBuffer)
				{
					high = atoi(trResult.c_str());
					high = Clamp(1, high, INT32_MAX);
				}
				if (high < low) high = low;
				heroInfo.creatureAmount[i].highAmount = high;

			}
		}
	}

	return;
}

typedef typename WoG::NPC NPC;
constexpr DWORD NPC_BASE_STATS_ADDRESSES[7] = { 0x769B01,0x769B0B,0x769B33,0x769B1F,0x769B29,0x769B15,0x769B3D };

struct NPCStats
{
private:
	int stats[7];

public:

	NPCStats(const UINT id)
	{

		for (size_t i = 0; i < 7; i++)
		{
			stats[i] = IntAt(NPC_BASE_STATS_ADDRESSES[i]);
		}

		ReadJsonData(id);
	}
public:

	void Assign(NPC* npc)
	{
		libc::memcpy(&(npc->parameters), stats, sizeof(stats));
	}

	void ReadJsonData(const UINT id)
	{
		bool readSucces = false;
		for (size_t i = 0; i < 7; i++)
		{
			sprintf(h3_TextBuffer, "gem.NPCStartingParameters.%d.%d", id, i);
			const int value = EraJS::readInt(h3_TextBuffer, readSucces);

			if (readSucces && value >= 0)
			{
				stats[i] = value;

			}
		}
	}

public:
	static void SetDefault()
	{
		bool readSucces = false;

		for (size_t i = 0; i < 7; i++)
		{
			sprintf(h3_TextBuffer, "gem.NPCStartingParameters.-1.%d", i);
			const int value = EraJS::readInt(h3_TextBuffer, readSucces);

			if (readSucces && value >= 0)
			{
				_PI->WriteDword(NPC_BASE_STATS_ADDRESSES[i], value);

			}
		}
	}
};
std::vector< NPCStats> npcStatsVec;

NPC* __stdcall NPC__Init(HiHook* h, NPC* npc)
{

	auto result = THISCALL_1(NPC*, h->GetDefaultFunc(), npc); // get default function


	if (npc->id != eHero::RANDOM)
	{
		npcStatsVec.at(npc->id).Assign(npc);
	}

	return result;

}
_LHF_(HooksInit)
{
	// Фикс Димера - герой имеет продвинутую разведку на старте
	h3::H3HeroInfo* hero_info_table = P_HeroInfo->Get();

	hero_info_table[eHero::DEEMER].sskills[1].level = eSecSkillLevel::BASIC;

	NPCStats::SetDefault();
	npcStatsVec.reserve(HEROES_COUNT);

	for (size_t i = 0; i < HEROES_COUNT; i++)
	{
		ChangeHeroStartingParameters(hero_info_table[i], i);
		npcStatsVec.emplace_back(NPCStats{ i });

	}


	return EXEC_DEFAULT;
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	static bool plugin_On = 0;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		if (!plugin_On)

		{
			plugin_On = 1;

			globalPatcher = GetPatcher();
			_PI = globalPatcher->CreateInstance("EraPlugin.HeroesStartingParameters.daemon_n");
			Era::ConnectEra();
			_PI->WriteLoHook(0x4EEAF2, HooksInit);
			_PI->WriteHiHook(0x769AB4, THISCALL_, NPC__Init);

		}

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;

	}
	return TRUE;

}

