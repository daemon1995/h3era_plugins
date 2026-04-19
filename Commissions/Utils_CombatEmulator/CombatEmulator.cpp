#include "CombatEmulator.h"

CombatEmulator *CombatEmulator::instance = nullptr;

//
// signed int __thiscall AdvMgr::StartBattle(H3AdventureManager *AdvMan, int PosMixed, H3Hero *attHero, H3Army *attArmy,
//                                          int PlayerIndex, H3Town *defTown, H3Hero *defHero, H3Army *defArmy, int
//                                          seed, signed int a10, int isBank)

// AdvMgr_Hero_BattleMgr_With_Neutral_Monsters(this, hero, v8, &a3, v5, PosMixed, -1, 0, 0, -1, 0, 0);
BOOL StartRealCombat(H3Hero *_attHero, H3Hero *_defHero, const BOOL blockMagic, const int specialTerrain,
                     const BOOL blockRunning);
DllExport BOOL StartCombatEmulator(H3Hero *attHero, H3Hero *defHero, const BOOL blockMagic, const int specialTerrain,
                                   const BOOL blockRunning)
{
    return StartRealCombat(attHero, defHero, blockMagic, specialTerrain, blockRunning);
}

DllExport BOOL GameModIsLoaded(LPCSTR modName)
{
    if (!modName)
    {
        return false;
    }

    auto modList = modList::GetEraModList(true);
    return std::find(modList.begin(), modList.end(), modName) != modList.end();
}

struct PseudoCombatManager
{

    H3Hero heroCopies[2];
    BOOL inCombat = false;

    int blockMagic;
    int blockRunning;
    int specialTerrain;

} pseudoCmb;

enum eHeroError
{
    HERO_NO_ERROR,
    HERO_ERROR_NO_HERO,
    HERO_ERROR_WRONG_OWNER
};

eHeroError ValidateHero(H3Hero *hero)
{

    P_CombatManager->specialTerrain = 0;
    if (!hero)
    {
        return HERO_ERROR_NO_HERO;
    }

    if (hero->owner < 0 || hero->owner > 7)
    {

        //   return HERO_ERROR_WRONG_OWNER;
    }

    return HERO_NO_ERROR;
}
BOOL StartRealCombat(H3Hero *_attHero, H3Hero *_defHero, const BOOL blockMagic, const int specialTerrain,
                     const BOOL blockRunning)
{

    if (ValidateHero(_attHero) != HERO_NO_ERROR || ValidateHero(_defHero) != HERO_NO_ERROR)
    {
        // H3Messagebox("Invalid hero(s) provided to StartCombatEmulator. Combat will not start. Error");
        // return 0;
    }
    pseudoCmb.blockMagic = blockMagic;
    pseudoCmb.blockRunning = blockRunning;
    pseudoCmb.specialTerrain = specialTerrain;

    UINT pos = 0; // _attHero->mixedPosition;
    // inCombat = true;
    // if (0)
    //{

    //    if (mapItem)
    //    {
    //        pos = mapItem->GetCoordinates().Pack();
    //    }

    //    int count = 123;
    //    auto result = THISCALL_12(int, 0x04AC270, P_AdventureManager->Get(), _attHero, 1, &count, nullptr, pos, -1, 0,
    //                              0, -1, 0, 0);

    //    return 1;
    //}

    // P_AdventureManager->SetNextManager(P_CombatManager->Get());
    auto cmb = P_CombatManager->Get();
    auto adv = P_AdventureManager->Get();
    auto win = P_WindowManager->Get();
    auto mos = P_MouseManager->Get();
    //	P_InputManager->SetPreviousManager(P_CombatManager->Get());
    // adv->SetManagers(0, win);

    //    win->SetManagers(cmb, mos);
    //	cmb->SetPreviousManager(adv);
    // adv->DemobilizeHero();

    auto atkHero = &pseudoCmb.heroCopies[0];
    auto defHero = &pseudoCmb.heroCopies[1];
    libc::memcpy(atkHero, _attHero, sizeof(H3Hero));
    libc::memcpy(defHero, _defHero, sizeof(H3Hero));
    Patch *patchToBlockDlgScreen = nullptr, *patchToBlockRun = nullptr;
    Patch *patchesToBlockAiMagic[2] = {0, 0}; // nullptr, * patchToBlockRun = nullptr;
    if (atkHero->owner == -1)
    {
        patchToBlockDlgScreen = _PI->WriteJmp(0x0475CF4, 0x0475D02);
        atkHero->owner = 1;
    }
    if (blockRunning)
    {
        patchToBlockRun = _PI->WriteJmp(0x041E6FB, 0x041EC2D);
    }

    if (defHero->owner == -1)
    {
        defHero->owner = 1;
    }
    if (blockMagic)
    {
        patchesToBlockAiMagic[0] = _PI->WriteCodePatch(0x0478191, "%n", 5);
        patchesToBlockAiMagic[1] = _PI->WriteCodePatch(0x04781AD, "%n", 5);
    }

    pseudoCmb.inCombat = true;
    const BOOL isQuick = IntAt(0x06987CC);
    IntAt(0x06987CC) = true;
    auto result = THISCALL_11(int, 0x075ADD9, P_AdventureManager->Get(), pos, atkHero, &atkHero->army, 0, nullptr,
                              defHero, &defHero->army, -1, 1, 0);
    IntAt(0x06987CC) = isQuick;

    pseudoCmb.inCombat = false;
    if (patchToBlockDlgScreen)
    {
        patchToBlockDlgScreen->Destroy();
    }

    if (patchToBlockRun)

        patchToBlockRun->Destroy();

    for (auto &patch : patchesToBlockAiMagic)
    {
        if (patch)
        {
            patch->Destroy();
        }
    }
    // CombatEmulator::GetInstance()->StartCombat(attHero, defHero, mapItem);

    return 1;
}

void CombatEmulator::Init()
{

    if (!instance)
    {
        instance = new CombatEmulator();
        CreatePatches(_PI);
    }
}

void CombatEmulator::BeforeCombatStart()
{
}

char __stdcall CombatEmulator::GameMgr_CreateSaveGameFile(HiHook *h, H3Game *gameMgr, LPCSTR saveName, const DWORD a3,
                                                          const DWORD a4, const DWORD a5, const DWORD a6)
{

    if (pseudoCmb.inCombat)
        return 0;

    return THISCALL_6(char, h->GetDefaultFunc(), gameMgr, saveName, a3, a4, a5, a6);
}
void __stdcall CombatMgr_ChooseSpecialTerrain(HiHook *h, H3CombatManager *cmb)
{
    if (pseudoCmb.inCombat)
    {
        return THISCALL_1(void, h->GetDefaultFunc(), cmb);
    }
    cmb->specialTerrain = pseudoCmb.specialTerrain;
}
void CombatEmulator::CreatePatches(PatcherInstance *_pi)
{

    _pi->WriteHiHook(0x04BEB60, THISCALL_, GameMgr_CreateSaveGameFile);
    _pi->WriteHiHook(0x046381E, THISCALL_, CombatMgr_ChooseSpecialTerrain);
}
