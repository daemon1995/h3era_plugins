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
    // H3API resolves H3Main through this engine-global pointer.
    void* globals = VirtualAlloc(reinterpret_cast<void*>(0x690000), 0x10000,
                                 MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    CHECK(globals == reinterpret_cast<void*>(0x690000));
    auto main = static_cast<H3Main*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(H3Main)));
    CHECK(main);
    *reinterpret_cast<H3Main**>(0x699538) = main;
    main->mainSetup.mapSize = 36;
    main->mainSetup.hasUnderground = false;
    CHECK(IsMapCoordinateValid(0, 0, 0));
    CHECK(IsMapCoordinateValid(35, 35, 0));
    CHECK(!IsMapCoordinateValid(0, 0, 1));
    main->mainSetup.hasUnderground = true;
    CHECK(IsMapCoordinateValid(35, 35, 1));
    CHECK(!IsMapCoordinateValid(0, 0, 2));
    CHECK(!IsMapCoordinateValid(-1, 0, 0));
    CHECK(!IsMapCoordinateValid(0, 36, 0));
    CHECK(!IsMapCoordinateValid(0, 0, -1));
    *reinterpret_cast<H3Main**>(0x699538) = nullptr;
    CHECK(!IsMapCoordinateValid(0, 0, 0));
    HeapFree(GetProcessHeap(), 0, main);
    VirtualFree(globals, 0, MEM_RELEASE);
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
    Stage(path, "first save");
    CHECK(CommitStagedSave());
    CHECK(Contains(path, "first save"));
    Stage(path, "second save");
    CHECK(CommitStagedSave());
    CHECK(Contains(path, "second save"));
    CHECK(Contains(g_file.backupPath, "first save"));
    CHECK(DeleteFileA(g_file.backupPath));
    Stage(path, "third save");
    WriteTestFile(g_file.stagePath, "changed after validation");
    CHECK(!CommitStagedSave());
    CHECK(Contains(path, "second save"));
    Stage(path, "third save");
    HANDLE locked = CreateFileA(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    CHECK(locked != INVALID_HANDLE_VALUE);
    CHECK(!CommitStagedSave());
    CloseHandle(locked);
    CHECK(Contains(path, "second save"));
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

int main()
{
    TestMapLevels();
    TestRepairRecord();
    TestFileReplacement();
    TestVectorAndToggle();
    std::puts("BattleSave: 4 test groups passed.");
}
