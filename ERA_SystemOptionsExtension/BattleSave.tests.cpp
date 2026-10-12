// Exercise the production implementation. Unused game hooks are discarded by
// /Gy /OPT:REF; these checks never call the game's fixed-address functions.
#include "BattleSave.cpp"
#include <cstdlib>

// Hook/lifecycle entry points are linked but never invoked outside the game.
Patcher* globalPatcher = nullptr;
namespace Era
{
decltype(RegisterHandler) RegisterHandler = nullptr;
decltype(WriteSavegameSection) WriteSavegameSection = nullptr;
decltype(ReadSavegameSection) ReadSavegameSection = nullptr;
}

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "Line %d: %s\n", __LINE__, #condition); std::exit(1); \
} } while (0)

void TestMapLevels()
{
    auto main = static_cast<H3Main*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(H3Main)));
    CHECK(main);
    main->mainSetup.mapSize = 36;
    main->mainSetup.hasUnderground = false;
    CHECK(IsMapCoordinateValid(main, 0, 0, 0));
    CHECK(IsMapCoordinateValid(main, 35, 35, 0));
    CHECK(!IsMapCoordinateValid(main, 0, 0, 1));
    main->mainSetup.hasUnderground = true;
    CHECK(IsMapCoordinateValid(main, 35, 35, 1));
    CHECK(!IsMapCoordinateValid(main, 0, 0, 2));
    CHECK(!IsMapCoordinateValid(main, -1, 0, 0));
    CHECK(!IsMapCoordinateValid(main, 0, 36, 0));
    CHECK(!IsMapCoordinateValid(main, 0, 0, -1));
    CHECK(!IsMapCoordinateValid(nullptr, 0, 0, 0));
    HeapFree(GetProcessHeap(), 0, main);
}

void TestRepairRecord()
{
    auto hero = static_cast<H3Hero*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(H3Hero)));
    CHECK(hero);
    hero->id = 12;
    hero->owner = 2;
    hero->x = 8;
    hero->y = 9;
    hero->movement = 1500;
    hero->maxMovement = 2000;
    ResetMovementSequence();
    g_move.hero = hero;
    g_move.heroId = hero->id;
    g_move.owner = hero->owner;
    g_move.movementMax = hero->maxMovement;
    g_move.movementCurrent = hero->movement;
    std::memcpy(g_move.heroState, hero, sizeof(g_move.heroState));
    SaveRepairRecord record = {};
    bool needed = true;
    CHECK(BuildRepairRecord(hero, record, needed) && !needed);
    hero->x = 10;
    hero->movement = 1400;
    CHECK(BuildRepairRecord(hero, record, needed) && needed);
    CHECK(IsRepairRecordSane(record));
    CHECK(record.savedX == 10 && record.movementCurrent == 1500);
    CHECK(*reinterpret_cast<short*>(record.heroState) == 8);
    record.owner = 8;
    CHECK(!IsRepairRecordSane(record));
    hero->flags = IN_BOAT_FLAG; // An unproven embark transition must be refused.
    CHECK(!BuildRepairRecord(hero, record, needed));
    hero->flags = 0;
    g_move.inBoat = true;
    CHECK(!BuildRepairRecord(hero, record, needed));
    g_move.boatStateValid = true;
    CHECK(BuildRepairRecord(hero, record, needed));
    CHECK((record.flags & (REPAIR_PRE_IN_BOAT | REPAIR_BOAT_STATE)) ==
                          (REPAIR_PRE_IN_BOAT | REPAIR_BOAT_STATE));
    HeapFree(GetProcessHeap(), 0, hero);
    ResetMovementSequence();
}

void WriteTestFile(const char* path, const char* contents)
{
    FILE* file = nullptr;
    CHECK(fopen_s(&file, path, "wb") == 0);
    CHECK(std::fwrite(contents, 1, std::strlen(contents), file) == std::strlen(contents));
    CHECK(std::fclose(file) == 0);
}

bool Contains(const char* path, const char* expected)
{
    FILE* file = nullptr;
    if (fopen_s(&file, path, "rb") != 0)
        return false;
    char buffer[64] = {};
    const size_t size = std::fread(buffer, 1, sizeof(buffer), file);
    std::fclose(file);
    return size == std::strlen(expected) && !std::memcmp(buffer, expected, size);
}

void Stage(const char* finalPath, const char* contents)
{
    ClearFileTransaction(true);
    CHECK(BuildOwnedPaths(finalPath));
    WriteTestFile(g_file.stagePath, contents);
    CHECK(ReadFileFingerprint(g_file.stagePath, g_file.readySize, g_file.readyWriteTime));
    g_file.ready = true;
}

void TestFileReplacement()
{
    char path[MAX_PATH] = {};
    CHECK(GetModuleFileNameA(nullptr, path, MAX_PATH));
    char* name = std::strrchr(path, '\\');
    CHECK(name);
    CHECK(_snprintf_s(name + 1, MAX_PATH - (name + 1 - path), _TRUNCATE,
                      "BattleSave-check-%lu.gm1", GetCurrentProcessId()) >= 0);
    char marked[MARKED_SAVEGAME_NAME_CAPACITY] = {};
    char lastName[LOGICAL_NAME_SIZE] = {};
    const SaveSelectionBuffers selection = {marked, lastName};
    CHECK(CopyString(marked, MARKED_SAVEGAME_NAME_CAPACITY, "previous.gm1"));
    CHECK(CopyString(lastName, LOGICAL_NAME_SIZE, "previous"));
    Stage(path, "first save");
    CHECK(!std::strcmp(marked, "previous.gm1") && !std::strcmp(lastName, "previous"));
    CHECK(CommitStagedSave(selection));
    CHECK(Contains(path, "first save"));
    CHECK(!std::strcmp(marked, name + 1));
    CHECK(!std::strcmp(lastName, BATTLE_SAVE_NAME));
    Stage(path, "second save");
    CHECK(CommitStagedSave(selection));
    CHECK(Contains(path, "second save"));
    CHECK(Contains(g_file.backupPath, "first save"));
    CHECK(DeleteFileA(g_file.backupPath));
    CHECK(CopyString(marked, MARKED_SAVEGAME_NAME_CAPACITY, "selected-before-error.gm1"));
    CHECK(CopyString(lastName, LOGICAL_NAME_SIZE, "selected-before-error"));
    Stage(path, "third save");
    WriteTestFile(g_file.stagePath, "changed after validation");
    CHECK(!CommitStagedSave(selection));
    CHECK(Contains(path, "second save"));
    CHECK(!std::strcmp(marked, "selected-before-error.gm1"));
    CHECK(!std::strcmp(lastName, "selected-before-error"));
    Stage(path, "third save");
    HANDLE locked = CreateFileA(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    CHECK(locked != INVALID_HANDLE_VALUE);
    CHECK(!CommitStagedSave(selection));
    CloseHandle(locked);
    CHECK(Contains(path, "second save"));
    CHECK(!std::strcmp(marked, "selected-before-error.gm1"));
    CHECK(!std::strcmp(lastName, "selected-before-error"));
    ClearFileTransaction(true);
    CHECK(DeleteFileA(path));
}

void TestVectorAndToggle()
{
    RawExeVector vector = {};
    unsigned int count = 99;
    CHECK(ReadVectorCount(&vector, 4, count) && count == 0);
    unsigned char storage[16] = {};
    vector.first = storage;
    vector.last = storage + 8;
    vector.capacity = storage + 16;
    CHECK(ReadVectorCount(&vector, 4, count) && count == 2);
    vector.last = storage + 7;
    CHECK(!ReadVectorCount(&vector, 4, count));
    vector.last = storage + 8;
    vector.capacity = storage + 4;
    CHECK(!ReadVectorCount(&vector, 4, count));
    available = true;
    AdditionalConfig::Get().battleSave.value = 1;
    CHECK(IsEnabled());
    AdditionalConfig::Get().battleSave.value = 0;
    CHECK(!IsEnabled());
    available = false;
    AdditionalConfig::Get().battleSave.value = 1;
    CHECK(!IsEnabled());
}

void TestBattleCandidateIdentity()
{
    auto hero = static_cast<H3Hero*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(H3Hero)));
    auto other = static_cast<H3Hero*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(H3Hero)));
    CHECK(hero && other);
    hero->id = other->id = 12;
    hero->owner = other->owner = 2;
    const unsigned int point = 5u | (9u << 16) | (1u << 26);
    available = true;
    AdditionalConfig::Get().battleSave.value = 1;
    g_preBattleDepth = g_combatDepth = 0;
    g_candidate.pending = g_candidate.bound = true;
    g_candidate.threadId = GetCurrentThreadId();
    g_candidate.hero = hero;
    g_candidate.heroId = hero->id;
    g_candidate.owner = hero->owner;
    g_candidate.point = point;

    CHECK(MatchesBattleCandidate(hero, point));
    // Packed coordinates have unused bits. Different padding is the same
    // battle, while changing any coordinate must reject its staged save.
    CHECK(MatchesBattleCandidate(hero, point ^ ~POINT_COORDINATE_MASK));
    CHECK(!MatchesBattleCandidate(hero, point ^ 1u));
    CHECK(!MatchesBattleCandidate(hero, point ^ (1u << 16)));
    CHECK(!MatchesBattleCandidate(hero, point ^ (1u << 26)));
    CHECK(!MatchesBattleCandidate(nullptr, point));
    CHECK(!MatchesBattleCandidate(other, point));
    ++hero->id;
    CHECK(!MatchesBattleCandidate(hero, point));
    --hero->id;
    ++hero->owner;
    CHECK(!MatchesBattleCandidate(hero, point));
    --hero->owner;
    g_candidate.threadId ^= 1u;
    CHECK(!MatchesBattleCandidate(hero, point));
    g_candidate.threadId = GetCurrentThreadId();
    g_preBattleDepth = 1;
    CHECK(!MatchesBattleCandidate(hero, point));
    g_preBattleDepth = 0;
    g_combatDepth = 1;
    CHECK(!MatchesBattleCandidate(hero, point));
    g_combatDepth = 0;
    AdditionalConfig::Get().battleSave.value = 0;
    CHECK(!MatchesBattleCandidate(hero, point));
    AdditionalConfig::Get().battleSave.value = 1;
    available = false;
    CHECK(!MatchesBattleCandidate(hero, point));
    available = true;
    g_candidate.pending = false;
    CHECK(!MatchesBattleCandidate(hero, point));
    DiscardStagedBattle();
    HeapFree(GetProcessHeap(), 0, other);
    HeapFree(GetProcessHeap(), 0, hero);
}

void TestPreBattleCandidateLifecycle()
{
    char path[MAX_PATH] = {};
    CHECK(GetModuleFileNameA(nullptr, path, MAX_PATH));
    char* name = std::strrchr(path, '\\');
    CHECK(name);
    CHECK(_snprintf_s(name + 1, MAX_PATH - (name + 1 - path), _TRUNCATE,
                      "BattleSave-candidate-check-%lu.gm1", GetCurrentProcessId()) >= 0);
    WriteTestFile(path, "previous battle");
    available = true;
    AdditionalConfig::Get().battleSave.value = 1;

    Stage(path, "prepared battle");
    g_candidate.bound = true;
    FinishPreBattleCandidate(true, true, true);
    CHECK(g_candidate.pending && g_file.ready);
    CHECK(Contains(path, "previous battle")); // Preparation must not commit.
    CHECK(Contains(g_file.stagePath, "prepared battle"));
    DiscardStagedBattle();

    // ERM cancellation, abnormal return, failed staging, lost binding,
    // incomplete file, disabled setting and failed writer all preserve BATTLE!.
    for (int scenario = 0; scenario < 7; ++scenario)
    {
        Stage(path, "rejected battle");
        char stagePath[PHYSICAL_PATH_SIZE] = {};
        CHECK(CopyString(stagePath, sizeof(stagePath), g_file.stagePath));
        g_candidate.bound = scenario != 3;
        g_file.ready = scenario != 4;
        AdditionalConfig::Get().battleSave.value = scenario != 5;
        g_file.failed = scenario == 6;
        FinishPreBattleCandidate(scenario != 1, scenario != 2, scenario != 0);
        CHECK(!g_candidate.pending && !g_candidate.bound && !g_file.ready);
        CHECK(GetFileAttributesA(stagePath) == INVALID_FILE_ATTRIBUTES);
        CHECK(Contains(path, "previous battle"));
    }
    AdditionalConfig::Get().battleSave.value = 1;

    Stage(path, "active writer");
    char stagePath[PHYSICAL_PATH_SIZE] = {};
    CHECK(CopyString(stagePath, sizeof(stagePath), g_file.stagePath));
    g_file.armed = true;
    g_file.threadId = GetCurrentThreadId();
    FileTransaction expected = g_file;
    expected.failed = true;
    g_candidate.pending = g_candidate.bound = true;
    DiscardStagedBattle();
    CHECK(!g_candidate.pending && !g_candidate.bound);
    CHECK(std::memcmp(&g_file, &expected, sizeof(g_file)) == 0);
    CHECK(Contains(stagePath, "active writer"));
    g_file.armed = false;
    DiscardStagedBattle();
    CHECK(GetFileAttributesA(stagePath) == INVALID_FILE_ATTRIBUTES);
    CHECK(Contains(path, "previous battle"));
    CHECK(DeleteFileA(path));
}

void TestSaveSelection()
{
    char marked[MARKED_SAVEGAME_NAME_CAPACITY] = {};
    char lastName[LOGICAL_NAME_SIZE] = {};
    const SaveSelectionBuffers selection = {marked, lastName};
    // The stock lookup code is absent in this test process. Requiring its
    // exact bytes would silently suppress these updates with patched dialogs.
    MarkBattleSaveForLoadDialog(".\\GAMES\\BATTLE!.gm1", selection);
    CHECK(!std::strcmp(marked, "BATTLE!.gm1"));
    CHECK(!std::strcmp(lastName, "BATTLE!"));
    MarkBattleSaveForLoadDialog("D:/games/HoMM 3 ERA/Games/BATTLE!.gm2", selection);
    CHECK(!std::strcmp(marked, "BATTLE!.gm2"));
    MarkBattleSaveForLoadDialog("BATTLE!.cgm1", selection);
    CHECK(!std::strcmp(marked, "BATTLE!.cgm1"));
    CHECK(CopyString(lastName, LOGICAL_NAME_SIZE, "unchanged"));
    MarkBattleSaveForLoadDialog(nullptr, selection);
    MarkBattleSaveForLoadDialog("", selection);
    MarkBattleSaveForLoadDialog(".\\GAMES\\", selection);
    char tooLong[MARKED_SAVEGAME_NAME_CAPACITY + 1];
    std::memset(tooLong, 'x', sizeof(tooLong) - 1);
    tooLong[sizeof(tooLong) - 1] = 0;
    MarkBattleSaveForLoadDialog(tooLong, selection);
    CHECK(!std::strcmp(marked, "BATTLE!.cgm1"));
    CHECK(!std::strcmp(lastName, "unchanged"));
}

void TestCombatLifecycle()
{
    char path[MAX_PATH] = {};
    CHECK(GetModuleFileNameA(nullptr, path, MAX_PATH));
    char *name = std::strrchr(path, '\\');
    CHECK(name);
    CHECK(_snprintf_s(name + 1, MAX_PATH - (name + 1 - path), _TRUNCATE,
                      "BattleSave-combat-check-%lu.gm1", GetCurrentProcessId()) >= 0);
    char marked[MARKED_SAVEGAME_NAME_CAPACITY] = {};
    char lastName[LOGICAL_NAME_SIZE] = {};
    const SaveSelectionBuffers selection = {marked, lastName};
    auto hero = static_cast<H3Hero*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(H3Hero)));
    CHECK(hero);
    hero->id = 12;
    hero->owner = 2;
    available = true;
    const int point = 5 | (9 << 16);

    // Commit only the matching, enabled, local battle at native entry.
    // A nested combat, changed point or failed file must preserve the old slot.
    for (int scenario = 0; scenario < 6; ++scenario)
    {
        WriteTestFile(path, "previous battle");
        Stage(path, "accepted battle");
        char stagePath[PHYSICAL_PATH_SIZE] = {};
        CHECK(CopyString(stagePath, sizeof(stagePath), g_file.stagePath));
        g_combatDepth = scenario == 3 ? 1 : 0;
        AdditionalConfig::Get().battleSave.value = scenario != 4;
        g_candidate.pending = g_candidate.bound = true;
        g_candidate.threadId = GetCurrentThreadId();
        g_candidate.hero = hero;
        g_candidate.heroId = hero->id;
        g_candidate.owner = hero->owner;
        g_candidate.point = point;
        if (scenario == 5)
            WriteTestFile(stagePath, "changed after validation");
        const bool matches = MatchesBattleCandidate(hero, scenario == 2 ? point ^ 1 : point) && scenario != 1;
        BeginCombat(matches, selection);
        CHECK(g_combatDepth == (scenario == 3 ? 2u : 1u));
        CHECK(!g_candidate.pending && !g_candidate.bound && !g_file.ready);
        CHECK(GetFileAttributesA(stagePath) == INVALID_FILE_ATTRIBUTES);
        CHECK(Contains(path, scenario == 0 ? "accepted battle" : "previous battle"));
        CHECK(Y_CombatExit(nullptr, nullptr) == EXEC_DEFAULT);
        CHECK(g_combatDepth == (scenario == 3 ? 1u : 0u));
        if (scenario == 3)
            Y_CombatExit(nullptr, nullptr);
        CHECK(g_combatDepth == 0);
    }
    AdditionalConfig::Get().battleSave.value = 1;
    CHECK(DeleteFileA(path));
    char backupPath[PHYSICAL_PATH_SIZE] = {};
    CHECK(_snprintf_s(backupPath, sizeof(backupPath), _TRUNCATE, "%s.BattleSave.old", path) >= 0);
    CHECK(DeleteFileA(backupPath));

    // FastQuit/loading can unwind a battle without either native return.
    BeginCombat(false, selection);
    BeginCombat(false, selection);
    CHECK(g_combatDepth == 2);
    ResetRuntimeState(false, false);
    CHECK(g_combatDepth == 0);
    Y_CombatExit(nullptr, nullptr);
    CHECK(g_combatDepth == 0); // A late exit cannot underflow the cleared depth.
    HeapFree(GetProcessHeap(), 0, hero);
}

int main()
{
    TestMapLevels();
    TestRepairRecord();
    TestFileReplacement();
    TestVectorAndToggle();
    TestBattleCandidateIdentity();
    TestPreBattleCandidateLifecycle();
    TestSaveSelection();
    TestCombatLifecycle();
    std::puts("BattleSave: 8 test groups passed.");
    return 0;
}
