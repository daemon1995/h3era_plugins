#define _H3API_PLUGINS_
#include "pch.h"
// #include "expFix.cpp"

using namespace h3;
// #include "..\..\headers\era.cpp"

Patcher *globalPatcher;
PatcherInstance *_PI;

// void CreExpoFix_Apply();

void DeleteAllMapMonoliths()
{

    auto adv = P_AdventureManager->Get();
    const int mapSize = adv->map->mapSize;
    const int hasUnderground = adv->map->hasUnderground;

    for (size_t z = 0; z <= hasUnderground; z++)
    {

        for (size_t x = 0; x < mapSize; x++)
        {
            for (size_t y = 0; y < mapSize; y++)
            {
                auto mapItem = P_AdventureManager->map->GetMapItem(x, y, z);
                if (mapItem->objectType == eObject::MONOLITH_TWO_WAY ||
                    mapItem->objectType == eObject::MONOLITH_ONE_WAY_EXIT ||
                    mapItem->objectType == eObject::MONOLITH_ONE_WAY_ENTRANCE)
                {
                    auto posMixed = H3Position::Pack(x, y, z);
                    THISCALL_4(int, 0x4AA820, P_AdventureManager->Get(), mapItem, posMixed, 1);
                }
            }
        }
    }
}

void ResetHeroesDestionations()
{

    for (auto &hero : P_Game->heroes)
    {
        hero.dest_x = hero.x;
        hero.dest_y = hero.y;
        hero.dest_z = hero.z;
    }
}

void ResetHeroesLevels()
{

    for (auto &hero : P_Game->heroes)
    {
        hero.level = 1;
        hero.experience = h3::NH3Levels::LEVEL_1;
    }
}
void DecreaseCreaturesAIValues()
{

    const int MAX_MON_ID = IntAt(0x4A1657);

    H3CreatureInformation *info = *reinterpret_cast<H3CreatureInformation **>(0x6747B0);
    const int skeletonAI = info[eCreature::SKELETON].aiValue;

    // H3Messagebox();
    for (size_t i = 0; i < MAX_MON_ID; i++)
    {
        info[i].aiValue *= 5;// skeletonAI; // EraJS::read(h3_TextBuffer);//P_CreatureInformation
    }
}
void __stdcall OnKeyPressed_AdvMap(Era::TEvent *event)
{

    if (BOOL isLeftShift = Era::GetAssocVarIntValue("key_leftShift"))
    {
        Era::TXVars *xVars = Era::GetArgXVars();

        const int key = (*xVars)[0];
        switch (key)
        {
        case 0x52: // R
            ResetHeroesLevels();

            break;
        case 0x53: // S
            ResetHeroesDestionations();
            break;

        case 0x54: // T
            DeleteAllMapMonoliths();
            break;
        case 0x56: // V
            DecreaseCreaturesAIValues();
            break;
        default:
            break;
        }
    }
}
int __stdcall AIHeroMapItemWeight(HiHook *h, H3Hero *hero, int a2, int a3)
{

    int result = FASTCALL_3(int, h->GetDefaultFunc(), hero, a2, a3);
    if (true)
    {

    }
    return result;
}
_LHF_(HooksInit)
{
    _PI->WriteHiHook(0x0528520, FASTCALL_, AIHeroMapItemWeight);
    Era::RegisterHandler(OnKeyPressed_AdvMap, "OnKeyPressed_AdvMap");
    // ERI::ExtendedResourcesInfo::Get(); // one of the best crutch
    // new MapScroller(globalPatcher->CreateInstance("MapScroll.ERA.daemon_n.plugin"));
    //	AdventureMapHints::Init(globalPatcher->CreateInstance("EraPlugin.AdventureMapHints.daemon_n"));
    // void CreExpoFix_Apply();

    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{

    static _bool_ plugin_On = 0;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        // if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        if (!plugin_On)

        {
            plugin_On = 1;
            Era::ConnectEra(hModule);

            globalPatcher = GetPatcher();

            _PI = globalPatcher->CreateInstance("EraPlugin.ExperienceFix.daemon_n");
            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
