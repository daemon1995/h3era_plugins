#include "CombatEmulator.h"

CombatEmulator *CombatEmulator::instance = nullptr;
#pragma comment(linker, "/EXPORT:StartCombatEmulator=_StartCombatEmulator@20")
#pragma comment(linker, "/EXPORT:ApplyBlockingPatches=_ApplyBlockingPatches@12")
#pragma comment(linker, "/EXPORT:UndoBlockingPatches=_UndoBlockingPatches@0")

INT __stdcall StartPseudoCombat(H3Hero *_attHero, H3Hero *_defHero, const BOOL blockMagic, const BOOL blockRunning,
                                const int specialTerrain);
DllExport INT __stdcall StartCombatEmulator(H3Hero *attHero, H3Hero *defHero, const BOOL blockMagic,
                                            const BOOL blockRunning, const int specialTerrain)
{
    return StartPseudoCombat(attHero, defHero, blockMagic, blockRunning, specialTerrain);
}

DllExport BOOL __stdcall ApplyBlockingPatches(const BOOL blockCombatResultDlg, const BOOL blockMagic,
                                              const BOOL blockRunning)
{
    if (!CombatEmulator::instance)
        return FALSE;

    CombatEmulator::instance->BeforeCombatStart(blockCombatResultDlg, blockMagic, blockRunning);
    return TRUE;
}

DllExport BOOL __stdcall UndoBlockingPatches()
{
    if (!CombatEmulator::instance)
        return FALSE;

    CombatEmulator::instance->AfterCombatEnd();
    return TRUE;
}

void CombatEmulator::Init()
{
    if (!instance && _PI)
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
    int specialTerrain = -1;

} pseudoCmb;

static void CopyHero(H3Hero *destination, const H3Hero *source)
{
    auto *destinationBytes = reinterpret_cast<unsigned char *>(destination);
    auto *sourceBytes = reinterpret_cast<unsigned char *>(const_cast<H3Hero *>(source));

    const size_t mixedPositionOffset = offsetof(H3Hero, mixedPosition);
    const size_t armyOffset = offsetof(H3Hero, army);
    const size_t biographyOffset = offsetof(H3Hero, biography);
    const size_t biographyEnd = biographyOffset + sizeof(destination->biography);

    // Copy the plain data around H3Position, H3Army and H3String. These three
    // members have non-trivial copy/lifetime semantics in H3API.
    libc::memcpy(destinationBytes, sourceBytes, mixedPositionOffset);
    destination->mixedPosition = source->mixedPosition;

    libc::memcpy(destinationBytes + mixedPositionOffset + sizeof(destination->mixedPosition),
                 sourceBytes + mixedPositionOffset + sizeof(source->mixedPosition),
                 armyOffset - mixedPositionOffset - sizeof(destination->mixedPosition));
    destination->army = source->army;

    libc::memcpy(destinationBytes + armyOffset + sizeof(destination->army),
                 sourceBytes + armyOffset + sizeof(source->army),
                 biographyOffset - armyOffset - sizeof(destination->army));
    destination->biography.Erase();
    if (!source->biography.Empty())
        destination->biography = source->biography;

    libc::memcpy(destinationBytes + biographyEnd, sourceBytes + biographyEnd, sizeof(H3Hero) - biographyEnd);
}

enum eHeroError
{
    HERO_NO_ERROR,
    HERO_ERROR_NO_HERO,
    HERO_ERROR_WRONG_ID,
    HERO_ERROR_WRONG_OWNER
};

eHeroError ValidateHero(H3Hero *hero)
{

    if (!hero)
    {
        return HERO_ERROR_NO_HERO;
    }

    if (hero->id < 0 || hero->id >= h3::limits::HEROES)
    {
        return HERO_ERROR_WRONG_ID;
    }

    if (hero->owner < 0 || hero->owner > 7)
    {
        return HERO_ERROR_WRONG_OWNER;
    }

    return HERO_NO_ERROR;
}
INT __stdcall StartPseudoCombat(H3Hero *_attHero, H3Hero *_defHero, const BOOL blockMagic, const BOOL blockRunning,
                                const int specialTerrain)
{
    if (!CombatEmulator::instance || pseudoCmb.inCombat || ValidateHero(_attHero) != HERO_NO_ERROR ||
        ValidateHero(_defHero) != HERO_NO_ERROR)
    {
        // H3Messagebox("Invalid hero(s) provided to StartCombatEmulator. Combat will not start. Error");
        return -2;
    }

    if (specialTerrain < -1 || specialTerrain > 9)
        return -2;

    // copy original NPC
    auto atkNpc = WoG::NPC::Get(_attHero->id);
    auto defNpc = WoG::NPC::Get(_defHero->id);
    if (!atkNpc || !defNpc)
        return -2;

    pseudoCmb.npcCopies[0] = *atkNpc;
    pseudoCmb.npcCopies[1] = *defNpc;

    pseudoCmb.specialTerrain = specialTerrain;

    // init passed arguments

    const BOOL hasDefenderPosition = _defHero->x >= 0 && _defHero->x <= 0x3FF && _defHero->y >= 0 &&
                                     _defHero->y <= 0x3FF && _defHero->z >= 0 && _defHero->z <= 1;
    const UINT pos = hasDefenderPosition ? H3Position::Pack(_defHero->x, _defHero->y, _defHero->z) : 0;
    auto atkHero = &pseudoCmb.heroCopies[0]; // create hero copies to avoid modifying original heroes during combat
    CopyHero(atkHero, _attHero);
    constexpr H3Town *const town = nullptr;
    auto defHero = &pseudoCmb.heroCopies[1];
    CopyHero(defHero, _defHero);

    int seed = -1; // generate seed

    if (hasDefenderPosition)
    {
        seed = 0x3C907 * _defHero->x + 0x4386D * _defHero->y + 0x4BB5F * _defHero->z + 0x25EA7;
        THISCALL_1(void, 0x50C7B0, seed);
    }

    constexpr BOOL combatIsLocal = TRUE;
    constexpr BOOL isBank = FALSE;

    // modify combat settings based on passed arguments
    ApplyBlockingPatches(TRUE, blockMagic, blockRunning);
    const BOOL isQuick = IntAt(0x06987CC); // isQuickCombatEnabled
    IntAt(0x06987CC) = true;               // set combat quick

    P_AdventureManager->DemobilizeHero();

    pseudoCmb.inCombat = true; // set flag to indicate combat is active, so hooks can modify behavior accordingly
    int result = THISCALL_11(int, 0x075ADD9, P_AdventureManager->Get(), pos, atkHero, &atkHero->army, atkHero->owner,
                             town, defHero, &defHero->army, seed, combatIsLocal, isBank);
    pseudoCmb.inCombat = false;
    IntAt(0x06987CC) = isQuick; // restore original quick combat setting
    UndoBlockingPatches();
    pseudoCmb.specialTerrain = -1;

    // restore original NPC data to prevent side effects on the rest of the game
    *atkNpc = pseudoCmb.npcCopies[0];
    *defNpc = pseudoCmb.npcCopies[1];
    return result;
}

void CombatEmulator::BeforeCombatStart(const BOOL blockCombatResultDlg, const BOOL blockMagic, const BOOL blockRunning)
{
    BOOL args[] = {blockCombatResultDlg, blockMagic, blockRunning};
    for (size_t i = 0; i < 3; i++)
    {
        if (args[i] && patches[i])
            patches[i]->Apply();
    }
    if (patchToAfterCombatProc)
        patchToAfterCombatProc->Apply();
}

void CombatEmulator::AfterCombatEnd()
{
    for (auto &patch : patches)
    {
        if (patch)
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
    if (isInited)
        return;
    isInited = true;

    _pi->WriteHiHook(0x04BEB60, THISCALL_, GameMgr_CreateSaveGameFile);
    _pi->WriteHiHook(0x046381E, THISCALL_, CombatMgr_ChooseSpecialTerrain);

    patchToBlockDlgScreen = _pi->WriteJmp(0x0475CF4, 0x0475D02);
    patchToBlockDlgScreen->Undo();

    patchToBlockAiMagic = _pi->WriteJmp(0x0422F4E, 0x04230BC);
    patchToBlockAiMagic->Undo();

    patchToBlockRun = _pi->WriteJmp(0x041E6FB, 0x041EC2D);
    patchToBlockRun->Undo();

    // block whole after combat proc
    patchToAfterCombatProc = _pi->WriteJmp(0x04AE010, 0x04AE61B);
    patchToAfterCombatProc->Undo();
}
