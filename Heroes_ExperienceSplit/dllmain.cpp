// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"
#include "..\headers\Era\era.h"

namespace dllText {
	//const char* PLUGIN_VERSION = "1.24";
	const char* INSTANCE_NAME = "EraPlugin.HeroExperienceSplit.daemon_n";
	//const char* PLUGIN_AUTHOR = "daemon_n";
	//// const char* PROJECT_NAME = "$(ProjectName)";
	//const char* PLUGIN_DATA = __DATE__;
} // namespace dllText
Patcher* globalPatcher = nullptr;
PatcherInstance* _PI = nullptr;

using namespace h3;
constexpr int MAX_HERO_LEVEL = 6424; //!< Maximum level for a hero
static int __stdcall Hero_GiveExperienceAll(HiHook* h, H3Hero* hero, const int experience, const int a3, const int a4)
{
	// Check if the experience exceeds the limit
	if (hero->level == MAX_HERO_LEVEL) {
		return 0;
	}
	// Call the original function with the same parameters
	return THISCALL_4(int, h->GetDefaultFunc(), hero, experience, a3, a4);
}



// This function is called when a hero gains experience, and it distributes the experience to all heroes of the same player
static int __stdcall Hero_GiveExperience(HiHook* h, H3Hero* hero, const int experience, const int a3, const int a4)
{

	// Call the original function with the same parameters
	const int result = THISCALL_4(int, h->GetDefaultFunc(), hero, experience, a3, a4);

	const auto& player = P_Game->players[hero->owner];
	if (player.is_human)
	{

		for (auto& i : player.heroIDs)
		{
			if (H3Hero* currentHero = P_Game->GetHero(i))
			{
				if (currentHero != hero)
				{
					THISCALL_4(int, 0x04E3620, currentHero, experience, true, a4);
				}
			}
		}

		for (auto& i : P_Game->towns)
		{
			if (i.owner == hero->owner)
			{
				if (H3Hero* currentHero = i.GetGarrisonHero())
				{
					if (currentHero != hero)
					{
						THISCALL_4(int, 0x04E3620, currentHero, experience, true, a4);
					}
				}
			}
		}
	}

	return result;
}



void __stdcall OnAfterWog(Era::TEvent* e)
{
	// This function is called after
	if (atoi(Era::tr("tlrq.heroes.splitExperience")))
	{
		_PI->WriteHiHook(0x0477278, THISCALL_, Hero_GiveExperience); // CALL_ combat finish
		_PI->WriteHiHook(0x04A06B8, THISCALL_, Hero_GiveExperience); // CALL_ pandora/event pick up
		_PI->WriteHiHook(0x04A617C, THISCALL_, Hero_GiveExperience); // CALL_ chest pick up
		_PI->WriteHiHook(0x057408D, THISCALL_, Hero_GiveExperience); // CALL_ quest experience

	}

	//_PI->WriteHiHook(0x04E3620, THISCALL_, Hero_GiveExperienceAll); // SPLICE_
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	static BOOL plugin_On = 0; //!< Flag to indicate if the plugin is on
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if (!plugin_On)
		{
			plugin_On = 1;
			//! Connect to the Era framework
			//! Get the global patcher
			globalPatcher = GetPatcher();
			_PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
			Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
			Era::RegisterHandler(OnAfterWog, "OnAfterWog");

		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

