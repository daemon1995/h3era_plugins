#include "CombatEmulator.h"

CombatEmulator *CombatEmulator::instance = nullptr;

//
// signed int __thiscall AdvMgr::StartBattle(H3AdventureManager *AdvMan, int PosMixed, H3Hero *attHero, H3Army *attArmy,
//                                          int PlayerIndex, H3Town *defTown, H3Hero *defHero, H3Army *defArmy, int
//                                          seed, signed int a10, int isBank)

// AdvMgr_Hero_BattleMgr_With_Neutral_Monsters(this, hero, v8, &a3, v5, PosMixed, -1, 0, 0, -1, 0, 0);
BOOL StartRealCombat(H3Hero *attHero, H3Hero *defHero, const H3MapItem *mapItem);

DllExport BOOL StartCombatEmulator(H3Hero *attHero, H3Hero *defHero, const H3MapItem *mapItem)
{
    return StartRealCombat(attHero, defHero, mapItem);
}
H3Army someArmy;
H3Hero someHero;

BOOL StartRealCombat(H3Hero *attHero, H3Hero *defHero, const H3MapItem *mapItem)
{
    UINT pos = attHero->mixedPosition;

    if (0)
    {

        if (mapItem)
        {
            pos = mapItem->GetCoordinates().Pack();
        }

        int count = 123;
        auto result = THISCALL_12(int, 0x04AC270, P_AdventureManager->Get(), attHero, 1, &count, nullptr, pos, -1, 0, 0,
                                  -1, 0, 0);

        return 1;
    }

    // P_AdventureManager->SetNextManager(P_CombatManager->Get());
    auto cmb = P_CombatManager->Get();
    auto adv = P_AdventureManager->Get();
    auto win = P_WindowManager->Get();
    auto mos = P_MouseManager->Get();
    //	P_InputManager->SetPreviousManager(P_CombatManager->Get());
    // adv->SetManagers(0, win);

    //    win->SetManagers(cmb, mos);
    //	cmb->SetPreviousManager(adv);
    adv->DemobilizeHero();
    auto patch = _PI->WriteJmp(0x0475CF4, 0x0475D02);
 
	libc::memcpy(&someHero, attHero, sizeof(H3Hero));
	someArmy = attHero->army;
    auto result = THISCALL_11(int, 0x075ADD9, P_AdventureManager->Get(), pos, &someHero, &attHero->army, -1, nullptr,
                              nullptr, &someArmy, -1, 1, 0);

    patch->Destroy();
    // CombatEmulator::GetInstance()->StartCombat(attHero, defHero, mapItem);

    return 1;
}

void CombatEmulator::Init()
{

    if (!instance)
    {
        instance = new CombatEmulator();
    }
}

void CombatEmulator::CreatePatches(PatcherInstance *_pi)
{
}
