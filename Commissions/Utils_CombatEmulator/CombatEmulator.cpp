#include "CombatEmulator.h"

CombatEmulator *CombatEmulator::instance = nullptr;
#pragma comment(linker, "/EXPORT:StartCombatEmulator=_StartCombatEmulator@20")
#pragma comment(linker, "/EXPORT:ApplyBlockingPatches=_ApplyBlockingPatches@12")
#pragma comment(linker, "/EXPORT:UndoBlockingPatches=_UndoBlockingPatches@0")

INT StartPseudoCombat(H3Hero *_attHero, H3Hero *_defHero, const BOOL blockMagic, const BOOL blockRunning,
                      const int specialTerrain);
DllExport INT __stdcall StartCombatEmulator(H3Hero *attHero, H3Hero *defHero, const BOOL blockMagic,
                                            const int specialTerrain, const BOOL blockRunning)
{
    return StartPseudoCombat(attHero, defHero, blockMagic, blockRunning, specialTerrain);
}

DllExport BOOL __stdcall ApplyBlockingPatches(const BOOL blockCombatResultDlg, const BOOL blockMagic,
                                              const BOOL blockRunning)
{
    CombatEmulator::instance->BeforeCombatStart(blockCombatResultDlg, blockMagic, blockRunning);
    return 1;
}

DllExport BOOL __stdcall UndoBlockingPatches()
{
    CombatEmulator::instance->AfterCombatEnd();
    return 1;
}

void CombatEmulator::Init()
{
    if (!instance)
    {
        instance = new CombatEmulator();
        instance->CreatePatches(_PI);
    }
}
struct PseudoCombatManager
{
    H3Hero heroCopies[2];
    WoG::NPC npcCopies[2];
    BOOL inCombat = false;
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
INT StartPseudoCombat(H3Hero *_attHero, H3Hero *_defHero, const BOOL blockMagic, const BOOL blockRunning,
                      const int specialTerrain)
{
    if (ValidateHero(_attHero) != HERO_NO_ERROR || ValidateHero(_defHero) != HERO_NO_ERROR)
    {
        // H3Messagebox("Invalid hero(s) provided to StartCombatEmulator. Combat will not start. Error");
        return -2;
    }

    // copy original NPC
    auto atkNpc = WoG::NPC::Get(_attHero->id);
    libc::memcpy(&pseudoCmb.npcCopies[0], atkNpc, sizeof(WoG::NPC));
    auto defNpc = WoG::NPC::Get(_defHero->id);
    libc::memcpy(&pseudoCmb.npcCopies[1], defNpc, sizeof(WoG::NPC));

    // init passed arguments

    constexpr UINT pos = 0;                  // _attHero->mixedPosition;
    auto atkHero = &pseudoCmb.heroCopies[0]; // create hero copies to avoid modifying original heroes during combat
    libc::memcpy(atkHero, _attHero, sizeof(H3Hero));
    constexpr H3Town *const town = nullptr;
    auto defHero = &pseudoCmb.heroCopies[1];
    libc::memcpy(defHero, _defHero, sizeof(H3Hero));
    constexpr int seed = -1; // generate seed
    constexpr BOOL combatIsLocal = TRUE;
    constexpr BOOL isBank = FALSE;

    // modify combat settings based on passed arguments
    ApplyBlockingPatches(TRUE, blockMagic, blockRunning);
    const BOOL isQuick = IntAt(0x06987CC); // isQuickCombatEnabled
    IntAt(0x06987CC) = true;               // set combat quick

    const int atId = atkHero->id;
    P_AdventureManager->DemobilizeHero();
    atkHero->owner = -1;
    defHero->owner = -1;

    h3::H3HeroInfo *hero_info_table = P_HeroInfo->Get();

    pseudoCmb.inCombat = true; // set flag to indicate combat is active, so hooks can modify behavior accordingly
    auto result = THISCALL_11(int, 0x075ADD9, P_AdventureManager->Get(), pos, atkHero, &atkHero->army, -1, town,
                              defHero, &defHero->army, seed, combatIsLocal, isBank);
    pseudoCmb.inCombat = false;
    IntAt(0x06987CC) = isQuick; // restore original quick combat setting
    UndoBlockingPatches();

    // restore original NPC data to prevent side effects on the rest of the game
    libc::memcpy(atkNpc, &pseudoCmb.npcCopies[0], sizeof(WoG::NPC));
    libc::memcpy(defNpc, &pseudoCmb.npcCopies[1], sizeof(WoG::NPC));
    return result;
}

void CombatEmulator::BeforeCombatStart(const BOOL blockCombatResultDlg, const BOOL blockMagic, const BOOL blockRunning)
{
    BOOL args[] = {blockCombatResultDlg, blockMagic, blockRunning};
    for (size_t i = 0; i < 3; i++)
    {
        if (args[i])
            patches[i]->Apply();
    }
    patchToAfterCombatProc->Apply();
}

void CombatEmulator::AfterCombatEnd()
{
    for (auto &patch : patches)
    {
        patch->Undo();
    }
}

char __stdcall CombatEmulator::GameMgr_CreateSaveGameFile(HiHook *h, H3Game *gameMgr, LPCSTR saveName, const DWORD a3,
                                                          const DWORD a4, const DWORD a5, const DWORD a6)
{
    if (pseudoCmb.inCombat)
        return 1;

    return THISCALL_6(char, h->GetDefaultFunc(), gameMgr, saveName, a3, a4, a5, a6);
}
void __stdcall CombatMgr_ChooseSpecialTerrain(HiHook *h, H3CombatManager *cmb)
{
    if (!pseudoCmb.inCombat)
        return THISCALL_1(void, h->GetDefaultFunc(), cmb);

    cmb->specialTerrain = pseudoCmb.specialTerrain;
}

void CombatEmulator::CreatePatches(PatcherInstance *_pi)
{
    if (isIinited)
        return;
    isIinited = true;

    _pi->WriteHiHook(0x04BEB60, THISCALL_, GameMgr_CreateSaveGameFile);
    _pi->WriteHiHook(0x046381E, THISCALL_, CombatMgr_ChooseSpecialTerrain);

    patchToBlockDlgScreen = _PI->WriteJmp(0x0475CF4, 0x0475D02);
    patchToBlockDlgScreen->Undo();

    patchToBlockAiMagic = _PI->WriteJmp(0x0422F4E, 0x04230BC);
    patchToBlockAiMagic->Undo();

    patchToBlockRun = _PI->WriteJmp(0x041E6FB, 0x041EC2D);
    patchToBlockRun->Undo();

    // block whole after combat proc
    patchToAfterCombatProc = _PI->WriteJmp(0x04AE010, 0x04AE61B);
    patchToAfterCombatProc->Undo();
}
