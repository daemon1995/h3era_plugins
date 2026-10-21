#include "CombatEmulator.h"

CombatEmulator *CombatEmulator::instance = nullptr;
#pragma comment(linker, "/EXPORT:StartCombatEmulator=_StartCombatEmulator@32")
#pragma comment(linker, "/EXPORT:ApplyBlockingPatches=_ApplyBlockingPatches@12")
#pragma comment(linker, "/EXPORT:UndoBlockingPatches=_UndoBlockingPatches@0")

INT __stdcall StartPseudoCombat(H3Hero *_attHero, H3Hero *_defHero, const int x, const int y, const int z,
                                const BOOL blockMagic, const BOOL blockRunning, const int specialTerrain);
DllExport INT __stdcall StartCombatEmulator(H3Hero *attHero, H3Hero *defHero, const int x, const int y, const int z,
                                            const BOOL blockMagic, const BOOL blockRunning, const int specialTerrain)
{
    return StartPseudoCombat(attHero, defHero, x, y, z, blockMagic, blockRunning, specialTerrain);
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

static H3Town *FindTownForHero(const H3Hero *hero)
{
    if (!P_Game || !hero)
        return nullptr;

    for (auto &town : P_Game->towns)
    {
        if (town.garrisonHero == hero->id || town.visitingHero == hero->id)
            return &town;
    }
    return nullptr;
}
HANDLE systemOptionsPlugin = nullptr;
static BOOL GetQuickCombatState()
{

    typedef int(__stdcall * TGetOptionValue)(const char *);

    static TGetOptionValue GetOptionValue = nullptr;
    if (!GetOptionValue)
    {
        auto dll = GetModuleHandleA("ERA_SystemOptionsExtension.era");
        if (!dll)
            return -1;
        GetOptionValue = (TGetOptionValue)GetProcAddress(dll, "GetOptionValue");
        if (!GetOptionValue)
            return -1;
    }
    return GetOptionValue("Combat.QuickCombatType");
}
static BOOL SetQuickCombatState(const int state)
{

    typedef int(__stdcall * TSetOptionValue)(const char *, const int);
    static TSetOptionValue SetOptionValue = nullptr;
    if (!SetOptionValue)
    {
        auto dll = GetModuleHandleA("ERA_SystemOptionsExtension.era");
        if (!dll)
            return -1;
        SetOptionValue = (TSetOptionValue)GetProcAddress(dll, "SetOptionValue");
        if (!SetOptionValue)
            return -1;
    }
    return SetOptionValue("Combat.QuickCombatType", state);
}

INT __stdcall StartPseudoCombat(H3Hero *_attHero, H3Hero *_defHero, const int x, const int y, const int z,
                                const BOOL blockMagic, const BOOL blockRunning, const int specialTerrain)
{
    if (!CombatEmulator::instance || pseudoCmb.inCombat || ValidateHero(_attHero) != HERO_NO_ERROR ||
        ValidateHero(_defHero) != HERO_NO_ERROR)
    {
        // H3Messagebox("Invalid hero(s) provided to StartCombatEmulator. Combat will not start. Error");
        return -2;
    }

    if (specialTerrain < -1 || specialTerrain > 9)
        return -2;

    // This API receives only two hero armies and cannot safely reproduce a
    // town battle (town guards and garrison state are additional inputs).
    // Reject that case instead of reporting a misleading field-battle result.
    // if (FindTownForHero(_defHero))
    //    return -2;

    // copy original NPC
    auto atkNpc = WoG::NPC::Get(_attHero->id);
    auto defNpc = WoG::NPC::Get(_defHero->id);
    if (!atkNpc || !defNpc)
        return -2;

    pseudoCmb.npcCopies[0] = *atkNpc;
    pseudoCmb.npcCopies[1] = *defNpc;

    pseudoCmb.specialTerrain = specialTerrain;

    // init passed arguments

    const int battleX = x; // _defHero->x;
    const int battleY = y; //_defHero->y;
    const int battleZ = z; //_defHero->z;
    const BOOL hasDefenderPosition =
        battleX >= 0 && battleX <= 0x3FF && battleY >= 0 && battleY <= 0x3FF && battleZ >= 0 && battleZ <= 1;
    // StartBattle needs the real map position: falling back to (0, 0, 0) can
    // silently select different terrain and a different deterministic seed.
    if (!hasDefenderPosition)
        return -2;

    const UINT pos = H3Position::Pack(battleX, battleY, battleZ);
    auto atkHero = &pseudoCmb.heroCopies[0]; // create hero copies to avoid modifying original heroes during combat
    CopyHero(atkHero, _attHero);
    constexpr H3Town *const town = nullptr;
    auto defHero = &pseudoCmb.heroCopies[1];
    CopyHero(defHero, _defHero);

    // Use a stable battle seed without reseeding the game's global RNG. The
    // battle setup may also derive its battlefield seed from `pos`.
    const int seed = 0x3C907 * battleX + 0x4386D * battleY + 0x4BB5F * battleZ + 0x25EA7;

    constexpr BOOL combatIsLocal = TRUE;
    constexpr BOOL isBank = FALSE;

    // The skipped StartBattle tail normally restores these adventure globals.
    const int activePlayerId = IntAt(0x69CCF4);
    const int adventureShown = IntAt(0x698A10);
    const int combatActive = IntAt(0x699590);
    const BYTE enemyFled = ByteAt(0x6985F3);
    const BYTE enemySurrendered = ByteAt(0x697794);
    const BOOL heroWasMobilized = P_AdventureManager->centeredHero;

    // modify combat settings based on passed arguments
    ApplyBlockingPatches(TRUE, blockMagic, blockRunning);

    const int quickCombatState = GetQuickCombatState();

    const BOOL isQuick = IntAt(0x06987CC); // isQuickCombatEnabled
    if (quickCombatState == -1)
    {
        IntAt(0x06987CC) = true; // set combat quick
    }
    else
    {
        SetQuickCombatState(1 + bool(blockMagic));
    }

    P_AdventureManager->DemobilizeHero();

    pseudoCmb.inCombat = true; // set flag to indicate combat is active, so hooks can modify behavior accordingly
    const int enemyOwner =
        _defHero->owner; // FIXED owner variable type (signed int should be passed to StartBattle, not signed char)
    int result = THISCALL_11(int, 0x075ADD9, P_AdventureManager->Get(), pos, atkHero, &atkHero->army, enemyOwner, town,
                             defHero, &defHero->army, seed, combatIsLocal, isBank);
    pseudoCmb.inCombat = false;

    if (quickCombatState == -1)
    {
        IntAt(0x06987CC) = isQuick; // restore original quick combat setting
    }
    else
    {
        SetQuickCombatState(quickCombatState); // restore original quick combat setting
    }

    IntAt(0x69CCF4) = activePlayerId;
    IntAt(0x698A10) = adventureShown;
    IntAt(0x699590) = combatActive;
    ByteAt(0x6985F3) = enemyFled;
    ByteAt(0x697794) = enemySurrendered;

    // restore original NPC data to prevent side effects on the rest of the game
    *atkNpc = pseudoCmb.npcCopies[0];
    *defNpc = pseudoCmb.npcCopies[1];

    UndoBlockingPatches();
    pseudoCmb.specialTerrain = -1;

    // Use the native post-combat arguments: H3API's MobilizeHero passes a
    // final zero, which omits the route refresh and queued mouse movement.
    if (heroWasMobilized)
        THISCALL_4(void, 0x417540, P_AdventureManager->Get(), 0, 0, 1);
    if (P_MouseManager && !IntAt(reinterpret_cast<ADDRESS>(P_MouseManager->Get()) + 0x38))
        THISCALL_1(void, 0x4194D0, P_AdventureManager->Get());

    return result;
}

void CombatEmulator::BeforeCombatStart(const BOOL blockCombatResultDlg, const BOOL blockMagic, const BOOL blockRunning)
{
    if (!blockingPatchesApplied)
    {
        if (P_MouseManager)
            mouseNoChangePointer = IntAt(reinterpret_cast<ADDRESS>(P_MouseManager->Get()) + 0x38);
        combatShowCursorShadow = IntAt(0x698810);
        blockingPatchesApplied = true;
    }

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
    RestoreMouseState();
    blockingPatchesApplied = false;
}

void CombatEmulator::RestoreMouseState()
{
    if (!blockingPatchesApplied)
        return;

    if (P_MouseManager)
        IntAt(reinterpret_cast<ADDRESS>(P_MouseManager->Get()) + 0x38) = mouseNoChangePointer;
    IntAt(0x698810) = combatShowCursorShadow;
}

int __stdcall CombatEmulator::CombatMgr_SkipResults(LoHook *h, HookContext *c)
{
    // 0x476DA0 also clears bNoChangePointer at 0x477123. A hidden battle
    // leaves it set by 0x462627, so restore it before returning to the map.
    instance->RestoreMouseState();
    c->return_address = 0x475D02;
    return NO_EXEC_DEFAULT;
}

void __stdcall CombatEmulator::MouseMgr_SetCursor(HiHook *h, H3MouseManager *mouse, const int frame, const int type)
{
    // Stock SetCursor does nothing while bNoChangePointer is set. HD's hook
    // still writes type/frame after that rejected call and can cache images
    // from the old DEF. Reject the call before it enters the HD hook chain.
    if (instance && instance->blockingPatchesApplied && IntAt(reinterpret_cast<ADDRESS>(mouse) + 0x38))
        return;

    THISCALL_3(void, h->GetDefaultFunc(), mouse, frame, type);
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

    // -1 means "use the game's normal terrain selection". Forcing -1 into
    // the manager disabled terrain effects instead of preserving map rules.
    if (pseudoCmb.specialTerrain < 0)
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
    _pi->WriteHiHook(0x50CEA0, THISCALL_, MouseMgr_SetCursor);

    patchToBlockDlgScreen = _pi->CreateLoHook(0x475CF4, CombatMgr_SkipResults);

    patchToBlockAiMagic = _pi->WriteJmp(0x0422F4E, 0x04230BC);
    patchToBlockAiMagic->Undo();

    patchToBlockRun = _pi->WriteJmp(0x041E6FB, 0x041EC2D);
    patchToBlockRun->Undo();

    // block whole after combat proc
    patchToAfterCombatProc = _pi->WriteJmp(0x04AE010, 0x04AE61B);
    patchToAfterCombatProc->Undo();
}
