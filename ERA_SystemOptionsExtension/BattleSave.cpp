// Based on igrik.BattleSave; adapted to the shared H3API and system options.
// Original repository license: BattleSave.LICENSE.
#include "BattleSave.h"
#include "framework.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
namespace
{
constexpr char INSTANCE_NAME[] = "igrik.BattleSave";
PatcherInstance *modulePatcher = nullptr;
bool initialized = false;
bool available = false;

struct NativeObject
{
    template <class T> T &Field(size_t offset)
    {
        return *reinterpret_cast<T *>(reinterpret_cast<unsigned char *>(this) + offset);
    }
};

H3Player *ActivePlayer()
{
    return *reinterpret_cast<H3Player **>(0x69CCFC);
}

unsigned char &HeroFacing(H3Hero *hero)
{
    // Original hero::facing, the byte protected as _f_47 in this H3API revision.
    return *(reinterpret_cast<unsigned char *>(hero) + 0x47);
}

bool IsEnabled()
{
    return available && AdditionalConfig::Get().battleSave.value != 0;
}

static_assert(sizeof(H3Hero) == 0x492 && offsetof(H3Hero, maxMovement) == 0x49 && offsetof(H3Hero, dest_x) == 0x35 &&
                  offsetof(H3Hero, flags) == 0x105,
              "Unexpected hero layout");
static_assert(offsetof(H3Main, mainSetup) + offsetof(H3MainSetup, mapSize) == 0x1FC44, "Unexpected map layout");
static_assert(offsetof(H3Main, mainSetup) + offsetof(H3MainSetup, hasUnderground) == 0x1FC48,
              "Unexpected underground flag layout");
constexpr _ptr_ MOVE_HERO = 0x47FF00;
constexpr _ptr_ DO_EVENT = 0x4AA710;
constexpr _ptr_ DO_COMBAT = 0x4AD160;
constexpr _ptr_ GAME_SAVE = 0x4BE0B0;
constexpr _ptr_ SAVE_PATH_READY = 0x4BED76;
constexpr _ptr_ SAVE_CLOSE_DONE = 0x4BEDA9;
constexpr _ptr_ SAVE_GAME = 0x4BEB60;
constexpr _ptr_ GET_HERO_BOAT = 0x4CE5C0;
constexpr _ptr_ OBSCURING_OBJECT_HIDE = 0x4D7950;
constexpr _ptr_ HERO_WIELDS_ARTIFACT = 0x4D9460;
constexpr _ptr_ HERO_CAN_LAND = 0x4E5F50;
constexpr _ptr_ GZ_CTOR = 0x4D6EB0;
constexpr _ptr_ GZ_CLOSE = 0x4D6FC0;
constexpr _ptr_ GZ_READ = 0x4D6FE0;
constexpr _ptr_ MARKED_SAVEGAME_NAME = 0x68338C;
constexpr _ptr_ MARKED_SAVE_LOOKUP_SITE = 0x5833AB;

constexpr int MAX_HEROES = 156;
constexpr int MAX_PLAYERS = 8;
constexpr int LOGICAL_NAME_SIZE = 352;
constexpr int PHYSICAL_PATH_SIZE = 1024;
constexpr int ENGINE_SAVE_PATH_EBP_OFFSET = 0x454;
constexpr int ENGINE_SAVE_PATH_CAPACITY = 352;
constexpr int GZIP_BUFFER_SIZE = 64 * 1024;
constexpr int MARKED_SAVEGAME_NAME_CAPACITY = 260;
constexpr int BOAT_PAYLOAD_OFFSET = 24;
constexpr int BOAT_PAYLOAD_SIZE = 16;
constexpr int OBJECT_HERO = 34;
constexpr int ARTIFACT_ANGEL_WINGS = 72;
constexpr int ARTIFACT_BOOTS_OF_LEVITATION = 90;
constexpr unsigned int IN_BOAT_FLAG = 0x00040000;
constexpr _ptr_ BOAT_POOL_OFFSET = 0x4E3B8;
constexpr _ptr_ RECORDED_EVENTS_OFFSET = 0x4E7AC;
constexpr unsigned int BOAT_RECORD_SIZE = 40;
constexpr int GZ_READ_ERROR_CODE = -1;
constexpr int Z_DATA_ERROR_CODE = -3;

constexpr unsigned int REPAIR_MAGIC = 0x31525342; // BSR1
constexpr unsigned short REPAIR_VERSION = 1;
constexpr unsigned int REPAIR_ACTIVE = 0x00000001;
constexpr unsigned int REPAIR_PRE_IN_BOAT = 0x00000002;
constexpr unsigned int REPAIR_SAVED_IN_BOAT = 0x00000004;
constexpr unsigned int REPAIR_BOAT_STATE = 0x00000008;
const char REPAIR_SECTION[] = "BattleSave.Repair.v1";
const char DISCARD_SAVE_PATH[] = ".\\GAMES\\~BattleSave.failed.new";
const char BATTLE_SAVE_NAME[] = "BATTLE!";

const unsigned char MOVE_HERO_PROLOG[] = {0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x48, 0x53, 0x56, 0x57, 0x8B, 0xF9};
const unsigned char DO_EVENT_PROLOG[] = {0x55, 0x8B, 0xEC, 0xA1, 0xFC, 0xCC, 0x69, 0x00, 0x53, 0x56, 0x8B, 0xF1};
const unsigned char DO_COMBAT_PROLOG[] = {0x55, 0x8B, 0xEC, 0x6A, 0xFF, 0x68, 0x8F, 0xB9, 0x62, 0x00};
const unsigned char GAME_SAVE_PROLOG[] = {0x55, 0x8B, 0xEC, 0x6A, 0xFF, 0x68, 0x93, 0xC5, 0x62, 0x00};
const unsigned char SAVE_PATH_INSTRUCTION[] = {0x8D, 0x8D, 0xAC, 0xFB, 0xFF, 0xFF};
const unsigned char SAVE_CLOSE_INSTRUCTION[] = {0xFF, 0x15, 0x54, 0xA3, 0x63, 0x00};
const unsigned char MARKED_SAVE_LOOKUP_INSTRUCTION[] = {0x68, 0x8C, 0x33, 0x68, 0x00, 0x8B,
                                                        0xCB, 0xE8, 0xF9, 0xA2, 0x00, 0x00};

struct RawExeVector
{
    void *allocator;
    unsigned char *first;
    unsigned char *last;
    unsigned char *capacity;
};

enum MoveState
{
    MOVE_IDLE,
    MOVE_ACTIVE,
    MOVE_EVENT_PENDING,
    MOVE_EVENT_ACTIVE,
    MOVE_COMBAT_HANDLED
};

struct MovementTransaction
{
    MoveState state;
    DWORD threadId;
    H3MapItem *eventCell;
    H3Hero *hero;
    int heroId;
    int owner;
    int movementMax;
    int movementCurrent;
    int targetX;
    int targetY;
    int targetZ;
    int flyCastPower;
    int waterwalkCastPower;
    unsigned int tempModFlags;
    unsigned char facing;
    bool inBoat;
    bool boatStateValid;
    bool eventCountValid;
    bool safeAnchor;
    unsigned int eventCount;
    unsigned char heroState[24];
    unsigned char boatState[40];
};

struct MovementChain
{
    bool active;
    DWORD threadId;
    H3Hero *hero;
    int heroId;
    int owner;
    short endX;
    short endY;
    short endZ;
};

#pragma pack(push, 1)
struct SaveRepairRecord
{
    unsigned int magic;
    unsigned short version;
    unsigned short recordSize;
    unsigned int flags;
    int heroId;
    int owner;
    int savedX;
    int savedY;
    int savedZ;
    int movementMax;
    int movementCurrent;
    int targetX;
    int targetY;
    int targetZ;
    int flyCastPower;
    int waterwalkCastPower;
    unsigned int tempModFlags;
    unsigned char facing;
    unsigned char reserved[3];
    unsigned char heroState[24];
    unsigned char boatState[40];
};
#pragma pack(pop)
static_assert(sizeof(SaveRepairRecord) == 132, "Preserve BattleSave.Repair.v1 compatibility");

struct FileTransaction
{
    bool armed;
    bool redirected;
    bool ready;
    bool engineSaveCalled;
    bool engineSaveSucceeded;
    bool gzipCloseCalled;
    bool gzipCloseSucceeded;
    bool failed;
    DWORD threadId;
    unsigned int serial;
    unsigned __int64 readySize;
    FILETIME readyWriteTime;
    char finalPath[PHYSICAL_PATH_SIZE];
    char stagePath[PHYSICAL_PATH_SIZE];
    char backupPath[PHYSICAL_PATH_SIZE];
};

MovementTransaction g_move = {};
MovementTransaction g_anchor = {};
MovementChain g_chain = {};
FileTransaction g_file = {};
SaveRepairRecord g_writeRepair = {};
SaveRepairRecord g_pendingRepair = {};
bool g_writeRepairActive = false;
bool g_writeRepairEmitted = false;
bool g_pendingRepairActive = false;
unsigned int g_fileSerial = 0;
unsigned int g_eventDepth = 0;
bool g_recoveryDone = false;
unsigned char g_gzipBuffer[GZIP_BUFFER_SIZE];

HiHook *g_moveHook = nullptr;
HiHook *g_eventHook = nullptr;
HiHook *g_combatHook = nullptr;
HiHook *g_gameSaveHook = nullptr;
LoHook *g_savePathHook = nullptr;
LoHook *g_saveCloseHook = nullptr;

size_t BoundedLength(const char *text, size_t limit)
{
    if (!text)
        return limit;

    size_t length = 0;
    while (length < limit && text[length])
        ++length;
    return length;
}

bool CopyString(char *destination, size_t destinationSize, const char *source)
{
    const size_t length = BoundedLength(source, destinationSize);
    if (!destination || !destinationSize || length >= destinationSize)
        return false;

    std::memcpy(destination, source, length + 1);
    return true;
}

bool BytesEqual(_ptr_ address, const unsigned char *expected, size_t size)
{
    bool equal = false;
    __try
    {
        equal = std::memcmp(reinterpret_cast<const void *>(address), expected, size) == 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        equal = false;
    }
    return equal;
}

void MarkBattleSaveForLoadDialog()
{
    // The stock dialog consumes this buffer only after the list is rebuilt.
    // Validate that the loaded code still pushes this exact buffer into the
    // native lookup routine before updating the optional UI state.
    if (!BytesEqual(MARKED_SAVE_LOOKUP_SITE, MARKED_SAVE_LOOKUP_INSTRUCTION, sizeof(MARKED_SAVE_LOOKUP_INSTRUCTION)))
        return;

    __try
    {
        CopyString(reinterpret_cast<char *>(MARKED_SAVEGAME_NAME), MARKED_SAVEGAME_NAME_CAPACITY, BATTLE_SAVE_NAME);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

bool ValidatePe32Image()
{
    HMODULE module = GetModuleHandleA(nullptr);
    if (reinterpret_cast<_ptr_>(module) != 0x400000)
        return false;

    bool valid = false;
    __try
    {
        const IMAGE_DOS_HEADER *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(module);
        const IMAGE_NT_HEADERS32 *nt = reinterpret_cast<const IMAGE_NT_HEADERS32 *>(
            reinterpret_cast<const unsigned char *>(module) + dos->e_lfanew);
        valid = dos->e_magic == IMAGE_DOS_SIGNATURE && nt->Signature == IMAGE_NT_SIGNATURE &&
                nt->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
                nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC && nt->OptionalHeader.ImageBase == 0x400000 &&
                nt->OptionalHeader.SizeOfImage >= 0x2500000;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        valid = false;
    }
    return valid;
}

bool ValidateHiHookSite(_ptr_ address, const unsigned char *original, size_t originalSize, const char *allowedOwner)
{
    Patch *patch = globalPatcher->GetFirstPatchAt(address);
    if (!patch)
        return BytesEqual(address, original, originalSize);

    for (; patch; patch = patch->GetAppliedAfter())
    {
        if (!patch->IsApplied() || patch->GetAddress() != address || patch->GetType() != HIHOOK_)
            return false;

        const char *owner = patch->GetOwner();
        if (!allowedOwner || !owner || std::strcmp(owner, allowedOwner) != 0)
            return false;
    }
    return true;
}

bool ValidateSaveSites()
{
    return globalPatcher->GetFirstPatchAt(SAVE_PATH_READY) == nullptr &&
           BytesEqual(SAVE_PATH_READY, SAVE_PATH_INSTRUCTION, sizeof(SAVE_PATH_INSTRUCTION)) &&
           globalPatcher->GetFirstPatchAt(SAVE_CLOSE_DONE) == nullptr &&
           BytesEqual(SAVE_CLOSE_DONE, SAVE_CLOSE_INSTRUCTION, sizeof(SAVE_CLOSE_INSTRUCTION));
}

bool IsLocalHumanHero(H3Hero *hero)
{
    if (!hero || !H3Main::Get() || hero->id < 0 || hero->id >= MAX_HEROES || hero->owner < 0 ||
        hero->owner >= MAX_PLAYERS)
        return false;

    H3Player *player = &H3Main::Get()->players[hero->owner];
    return player && player->isLocal && player->isHuman && H3Main::Get()->GetPlayerID() == hero->owner;
}

bool ReadVectorCount(RawExeVector *vector, unsigned int itemSize, unsigned int &count)
{
    if (!vector || !itemSize)
        return false;

    bool valid = false;
    __try
    {
        if (!vector->first && !vector->last && !vector->capacity)
        {
            count = 0;
            valid = true;
        }
        else if (vector->first && vector->last && vector->capacity && vector->first <= vector->last &&
                 vector->last <= vector->capacity)
        {
            const size_t used = static_cast<size_t>(vector->last - vector->first);
            if (used % itemSize == 0)
            {
                count = static_cast<unsigned int>(used / itemSize);
                valid = true;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        valid = false;
    }
    return valid;
}

RawExeVector *RecordedEvents()
{
    return reinterpret_cast<RawExeVector *>(reinterpret_cast<unsigned char *>(H3Main::Get()) + RECORDED_EVENTS_OFFSET);
}

RawExeVector *BoatPool()
{
    return reinterpret_cast<RawExeVector *>(reinterpret_cast<unsigned char *>(H3Main::Get()) + BOAT_POOL_OFFSET);
}

bool CaptureRecordedEventCount(unsigned int &count)
{
    return H3Main::Get() && ReadVectorCount(RecordedEvents(), sizeof(void *), count);
}

bool BuildOwnedPaths(const char *finalPath)
{
    if (!CopyString(g_file.finalPath, sizeof(g_file.finalPath), finalPath))
        return false;

    const int stageResult =
        _snprintf_s(g_file.stagePath, sizeof(g_file.stagePath), _TRUNCATE, "%s.BattleSave.%lu.%u.new", g_file.finalPath,
                    static_cast<unsigned long>(GetCurrentProcessId()), ++g_fileSerial);
    const int backupResult =
        _snprintf_s(g_file.backupPath, sizeof(g_file.backupPath), _TRUNCATE, "%s.BattleSave.old", g_file.finalPath);

    return stageResult >= 0 && backupResult >= 0 &&
           BoundedLength(g_file.stagePath, sizeof(g_file.stagePath)) < sizeof(g_file.stagePath) &&
           BoundedLength(g_file.backupPath, sizeof(g_file.backupPath)) < sizeof(g_file.backupPath);
}

bool ReadFileFingerprint(const char *path, unsigned __int64 &size, FILETIME &writeTime)
{
    WIN32_FILE_ATTRIBUTE_DATA data = {};
    if (!path || !GetFileAttributesExA(path, GetFileExInfoStandard, &data))
        return false;

    ULARGE_INTEGER fileSize = {};
    fileSize.HighPart = data.nFileSizeHigh;
    fileSize.LowPart = data.nFileSizeLow;
    if (!fileSize.QuadPart)
        return false;

    size = fileSize.QuadPart;
    writeTime = data.ftLastWriteTime;
    return true;
}

bool MatchesFileFingerprint(const char *path, unsigned __int64 expectedSize, const FILETIME &expectedWriteTime)
{
    unsigned __int64 actualSize = 0;
    FILETIME actualWriteTime = {};
    return ReadFileFingerprint(path, actualSize, actualWriteTime) && actualSize == expectedSize &&
           CompareFileTime(&actualWriteTime, &expectedWriteTime) == 0;
}

bool FileHasSize(const char *path, unsigned __int64 expectedSize)
{
    unsigned __int64 actualSize = 0;
    FILETIME actualWriteTime = {};
    return ReadFileFingerprint(path, actualSize, actualWriteTime) && actualSize == expectedSize;
}

bool QuickCheckSaveFile(const char *path)
{
    HANDLE file =
        CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    LARGE_INTEGER size = {};
    unsigned char header[2] = {};
    unsigned int trailer[2] = {};
    DWORD bytesRead = 0;
    LARGE_INTEGER trailerOffset = {};
    trailerOffset.QuadPart = -8;

    const bool valid =
        GetFileSizeEx(file, &size) != FALSE && size.QuadPart >= 18 &&
        ReadFile(file, header, sizeof(header), &bytesRead, nullptr) != FALSE && bytesRead == sizeof(header) &&
        header[0] == 0x1F && header[1] == 0x8B && SetFilePointerEx(file, trailerOffset, nullptr, FILE_END) != FALSE &&
        ReadFile(file, trailer, sizeof(trailer), &bytesRead, nullptr) != FALSE && bytesRead == sizeof(trailer);
    CloseHandle(file);
    return valid;
}

bool ReadGzipStoredSize(const char *path, unsigned int &storedSize)
{
    HANDLE file =
        CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    LARGE_INTEGER offset = {};
    offset.QuadPart = -4;
    DWORD bytesRead = 0;
    const bool success = SetFilePointerEx(file, offset, nullptr, FILE_END) != FALSE &&
                         ReadFile(file, &storedSize, sizeof(storedSize), &bytesRead, nullptr) != FALSE &&
                         bytesRead == sizeof(storedSize);
    CloseHandle(file);
    return success;
}

bool ValidateGzip(const char *path)
{
    unsigned int storedSize = 0;
    if (!QuickCheckSaveFile(path) || !ReadGzipStoredSize(path, storedSize))
        return false;

    __declspec(align(4)) unsigned char gzipObject[8] = {};
    bool opened = false;
    bool completed = false;
    int closeResult = -1;
    unsigned __int64 totalRead = 0;

    __try
    {
        THISCALL_3(void, GZ_CTOR, gzipObject, path, "rb");
        opened = true;

        int readResult = 0;
        do
        {
            readResult = THISCALL_3(int, GZ_READ, gzipObject, g_gzipBuffer, GZIP_BUFFER_SIZE);
            if (readResult > 0)
                totalRead += static_cast<unsigned int>(readResult);
        } while (readResult > 0);

        // Legacy H3 saves consistently reach the declared ISIZE before zlib
        // reports their historical footer-CRC error.  Accept that exact
        // terminal shape, but reject every early read error/truncation.
        completed =
            (readResult == 0 || readResult == GZ_READ_ERROR_CODE) && static_cast<unsigned int>(totalRead) == storedSize;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        completed = false;
    }

    if (opened)
    {
        __try
        {
            closeResult = THISCALL_1(int, GZ_CLOSE, gzipObject);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            closeResult = -1;
        }
    }

    return completed && (closeResult == 0 || closeResult == Z_DATA_ERROR_CODE);
}

bool RestoreBackup(const char *finalPath, const char *backupPath)
{
    if (!ValidateGzip(backupPath))
        return false;

    if (GetFileAttributesA(finalPath) != INVALID_FILE_ATTRIBUTES)
    {
        if (ReplaceFileA(finalPath, backupPath, nullptr, 0, nullptr, nullptr))
            return ValidateGzip(finalPath);
        return false;
    }

    return MoveFileExA(backupPath, finalPath, 0) != FALSE && ValidateGzip(finalPath);
}

void RecoverOneBackup(const char *backupPath)
{
    char finalPath[PHYSICAL_PATH_SIZE] = {};
    if (!CopyString(finalPath, sizeof(finalPath), backupPath))
        return;

    const char suffix[] = ".BattleSave.old";
    const size_t pathLength = std::strlen(finalPath);
    const size_t suffixLength = sizeof(suffix) - 1;
    if (pathLength <= suffixLength || std::strcmp(finalPath + pathLength - suffixLength, suffix) != 0)
        return;

    finalPath[pathLength - suffixLength] = 0;
    if (ValidateGzip(finalPath))
        DeleteFileA(backupPath);
    else
        RestoreBackup(finalPath, backupPath);
}

void RecoverOwnedFiles()
{
    if (g_recoveryDone)
        return;
    g_recoveryDone = true;
    DeleteFileA(DISCARD_SAVE_PATH);

    WIN32_FIND_DATAA findData = {};
    HANDLE search = FindFirstFileA(".\\GAMES\\*.BattleSave.old", &findData);
    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            char path[PHYSICAL_PATH_SIZE] = {};
            if (_snprintf_s(path, sizeof(path), _TRUNCATE, ".\\GAMES\\%s", findData.cFileName) >= 0)
                RecoverOneBackup(path);
        } while (FindNextFileA(search, &findData));
        FindClose(search);
    }

    search = FindFirstFileA(".\\GAMES\\*.BattleSave.*.new", &findData);
    if (search == INVALID_HANDLE_VALUE)
        return;

    do
    {
        char stagePath[PHYSICAL_PATH_SIZE] = {};
        char finalPath[PHYSICAL_PATH_SIZE] = {};
        if (_snprintf_s(stagePath, sizeof(stagePath), _TRUNCATE, ".\\GAMES\\%s", findData.cFileName) < 0 ||
            !CopyString(finalPath, sizeof(finalPath), stagePath))
            continue;

        char *marker = std::strstr(finalPath, ".BattleSave.");
        if (!marker)
            continue;
        *marker = 0;

        char backupPath[PHYSICAL_PATH_SIZE] = {};
        if (_snprintf_s(backupPath, sizeof(backupPath), _TRUNCATE, "%s.BattleSave.old", finalPath) < 0)
            continue;

        if (ValidateGzip(finalPath) || RestoreBackup(finalPath, backupPath))
            DeleteFileA(stagePath);
        else if (!ValidateGzip(stagePath))
            DeleteFileA(stagePath);
    } while (FindNextFileA(search, &findData));
    FindClose(search);
}

bool CommitStagedSave()
{
    if (!g_file.ready || !MatchesFileFingerprint(g_file.stagePath, g_file.readySize, g_file.readyWriteTime))
        return false;

    const bool finalExists = GetFileAttributesA(g_file.finalPath) != INVALID_FILE_ATTRIBUTES;
    bool replaced = false;

    if (finalExists)
    {
        if (GetFileAttributesA(g_file.backupPath) != INVALID_FILE_ATTRIBUTES && !DeleteFileA(g_file.backupPath))
            return false;

        replaced = ReplaceFileA(g_file.finalPath, g_file.stagePath, g_file.backupPath, 0, nullptr, nullptr) != FALSE;
    }
    else
    {
        replaced = MoveFileExA(g_file.stagePath, g_file.finalPath, 0) != FALSE;
    }

    if (replaced && FileHasSize(g_file.finalPath, g_file.readySize))
        return true;

    RestoreBackup(g_file.finalPath, g_file.backupPath);
    return false;
}

void ClearFileTransaction(bool deleteStage)
{
    if (deleteStage && g_file.stagePath[0])
        DeleteFileA(g_file.stagePath);
    if (deleteStage)
        DeleteFileA(DISCARD_SAVE_PATH);

    const unsigned int serial = g_file.serial;
    std::memset(&g_file, 0, sizeof(g_file));
    g_file.serial = serial;
}

bool CreateStagedBattleSave()
{
    if (g_file.armed || g_file.redirected || !g_savePathHook || !g_savePathHook->IsApplied())
        return false;

    RecoverOwnedFiles();
    ClearFileTransaction(false);
    g_file.armed = true;
    g_file.threadId = GetCurrentThreadId();
    g_file.serial = ++g_fileSerial;

    char saveName[LOGICAL_NAME_SIZE] = {};
    std::memcpy(saveName, BATTLE_SAVE_NAME, sizeof(BATTLE_SAVE_NAME));
    char saveResult = 0;

    __try
    {
        saveResult = THISCALL_6(char, SAVE_GAME, H3Main::Get(), saveName, 1, 1, 1, 0);
    }
    __finally
    {
        g_file.armed = false;
    }

    if (!saveResult || g_file.failed || !g_file.engineSaveCalled || !g_file.engineSaveSucceeded ||
        !g_file.gzipCloseCalled || !g_file.gzipCloseSucceeded || !g_file.redirected || !g_file.finalPath[0] ||
        !g_file.stagePath[0] || !ValidateGzip(g_file.stagePath))
    {
        ClearFileTransaction(true);
        return false;
    }

    g_file.ready = ReadFileFingerprint(g_file.stagePath, g_file.readySize, g_file.readyWriteTime);
    if (!g_file.ready)
    {
        ClearFileTransaction(true);
        return false;
    }
    return true;
}

void ResetMovement()
{
    std::memset(&g_move, 0, sizeof(g_move));
    g_move.state = MOVE_IDLE;
}

void ResetMovementSequence()
{
    ResetMovement();
    std::memset(&g_anchor, 0, sizeof(g_anchor));
    std::memset(&g_chain, 0, sizeof(g_chain));
    g_eventDepth = 0;
}

bool CaptureMovement(H3Hero *hero);

bool IsStableMovementAnchor(H3Hero *hero)
{
    if (!hero)
        return false;
    if (g_move.inBoat)
        return g_move.boatStateValid;

    bool safe = false;
    __try
    {
        const bool flightActive =
            g_move.flyCastPower != -1 || THISCALL_2(BOOL, HERO_WIELDS_ARTIFACT, hero, ARTIFACT_ANGEL_WINGS) != 0;
        const bool waterWalkActive = g_move.waterwalkCastPower != -1 ||
                                     THISCALL_2(BOOL, HERO_WIELDS_ARTIFACT, hero, ARTIFACT_BOOTS_OF_LEVITATION) != 0;
        safe = (!flightActive && !waterWalkActive) || THISCALL_1(BOOL8, HERO_CAN_LAND, hero) != 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        safe = false;
    }
    return safe;
}

bool CanContinueFromAnchor(H3Hero *hero)
{
    return hero && g_anchor.safeAnchor && g_chain.active && g_chain.threadId == GetCurrentThreadId() &&
           g_chain.hero == hero && g_chain.heroId == hero->id && g_chain.owner == hero->owner &&
           g_chain.endX == hero->x && g_chain.endY == hero->y && g_chain.endZ == hero->z && g_anchor.hero == hero &&
           g_anchor.heroId == hero->id && g_anchor.owner == hero->owner;
}

bool PrepareMovement(H3Hero *hero)
{
    if (!CaptureMovement(hero))
        return false;

    if (IsStableMovementAnchor(hero))
    {
        g_move.safeAnchor = true;
        g_anchor = g_move;
    }
    else if (CanContinueFromAnchor(hero))
    {
        g_move = g_anchor;
        g_move.state = MOVE_ACTIVE;
        g_move.threadId = GetCurrentThreadId();
        g_move.eventCell = nullptr;
        g_move.safeAnchor = true;
    }
    else
    {
        // Preserve an unsafe transaction through the matching event so that
        // DoCombat fails closed instead of falling back to the moved state.
        g_move.safeAnchor = false;
    }
    return true;
}

void ContinueMovementChain(H3Hero *hero)
{
    std::memset(&g_chain, 0, sizeof(g_chain));
    if (!hero)
        return;

    g_chain.active = true;
    g_chain.threadId = GetCurrentThreadId();
    g_chain.hero = hero;
    g_chain.heroId = hero->id;
    g_chain.owner = hero->owner;
    g_chain.endX = hero->x;
    g_chain.endY = hero->y;
    g_chain.endZ = hero->z;
}

bool GetCurrentMoveHero(H3Hero *&hero)
{
    hero = nullptr;
    if (!H3Main::Get() || !ActivePlayer() || H3Main::Get()->GetPlayer() != ActivePlayer())
        return false;

    const int heroId = ActivePlayer()->currentHero;
    if (heroId < 0 || heroId >= MAX_HEROES)
        return false;

    hero = &H3Main::Get()->heroes[heroId];
    return hero->id == heroId && IsLocalHumanHero(hero);
}

bool CaptureBoatState(H3Hero *hero)
{
    if (!hero || !H3Main::Get() || !g_move.inBoat)
        return !g_move.inBoat;

    bool valid = false;
    __try
    {
        NativeObject *boat = THISCALL_3(NativeObject *, GET_HERO_BOAT, H3Main::Get(), hero->id, TRUE);
        valid = boat && boat->Field<BOOL8>(24) && boat->Field<BOOL8>(36) && boat->Field<int>(32) == hero->id &&
                boat->Field<short>(0) == hero->x && boat->Field<short>(2) == hero->y &&
                boat->Field<short>(4) == hero->z;
        if (valid)
            std::memcpy(g_move.boatState, boat, sizeof(g_move.boatState));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        valid = false;
    }
    return valid;
}

bool CaptureMovement(H3Hero *hero)
{
    if (!hero)
        return false;

    g_move.state = MOVE_ACTIVE;
    g_move.threadId = GetCurrentThreadId();
    g_move.hero = hero;
    g_move.heroId = hero->id;
    g_move.owner = hero->owner;
    g_move.movementMax = hero->maxMovement;
    g_move.movementCurrent = hero->movement;
    g_move.targetX = hero->dest_x;
    g_move.targetY = hero->dest_y;
    g_move.targetZ = hero->dest_z;
    g_move.flyCastPower = hero->flyPower;
    g_move.waterwalkCastPower = hero->waterwalkPower;
    g_move.tempModFlags = hero->flags;
    g_move.facing = HeroFacing(hero);
    g_move.inBoat = (hero->flags & IN_BOAT_FLAG) != 0;
    std::memcpy(g_move.heroState, hero, sizeof(g_move.heroState));
    g_move.eventCountValid = CaptureRecordedEventCount(g_move.eventCount);
    g_move.boatStateValid = CaptureBoatState(hero);

    // A failed boat lookup must still remain movement-scoped.  Keeping the
    // transaction with safeAnchor=false prevents DoCombat from falling back
    // to a post-disembark save that would be known to be inconsistent.
    return true;
}

bool IsRepairRecordSane(const SaveRepairRecord &record)
{
    return record.magic == REPAIR_MAGIC && record.version == REPAIR_VERSION &&
           record.recordSize == sizeof(SaveRepairRecord) && (record.flags & REPAIR_ACTIVE) != 0 && record.heroId >= 0 &&
           record.heroId < MAX_HEROES && record.owner >= 0 && record.owner < MAX_PLAYERS;
}

bool BuildRepairRecord(H3Hero *hero, SaveRepairRecord &record, bool &needed)
{
    needed = false;
    std::memset(&record, 0, sizeof(record));
    if (!hero || hero != g_move.hero || hero->id != g_move.heroId || hero->owner != g_move.owner)
        return false;

    const short startX = *reinterpret_cast<const short *>(g_move.heroState + 0);
    const short startY = *reinterpret_cast<const short *>(g_move.heroState + 2);
    const short startZ = *reinterpret_cast<const short *>(g_move.heroState + 4);
    const bool savedInBoat = (hero->flags & IN_BOAT_FLAG) != 0;
    needed = hero->x != startX || hero->y != startY || hero->z != startZ || savedInBoat != g_move.inBoat ||
             hero->maxMovement != g_move.movementMax || hero->movement != g_move.movementCurrent ||
             hero->dest_x != g_move.targetX || hero->dest_y != g_move.targetY || hero->dest_z != g_move.targetZ ||
             hero->flyPower != g_move.flyCastPower || hero->waterwalkPower != g_move.waterwalkCastPower ||
             hero->flags != g_move.tempModFlags || HeroFacing(hero) != g_move.facing;
    if (!needed)
        return true;

    if ((g_move.inBoat && (savedInBoat || !g_move.boatStateValid)) || (!g_move.inBoat && savedInBoat))
        return false;

    record.magic = REPAIR_MAGIC;
    record.version = REPAIR_VERSION;
    record.recordSize = sizeof(record);
    record.flags = REPAIR_ACTIVE;
    if (g_move.inBoat)
        record.flags |= REPAIR_PRE_IN_BOAT | REPAIR_BOAT_STATE;
    if (savedInBoat)
        record.flags |= REPAIR_SAVED_IN_BOAT;
    record.heroId = g_move.heroId;
    record.owner = g_move.owner;
    record.savedX = hero->x;
    record.savedY = hero->y;
    record.savedZ = hero->z;
    record.movementMax = g_move.movementMax;
    record.movementCurrent = g_move.movementCurrent;
    record.targetX = g_move.targetX;
    record.targetY = g_move.targetY;
    record.targetZ = g_move.targetZ;
    record.flyCastPower = g_move.flyCastPower;
    record.waterwalkCastPower = g_move.waterwalkCastPower;
    record.tempModFlags = g_move.tempModFlags;
    record.facing = g_move.facing;
    std::memcpy(record.heroState, g_move.heroState, sizeof(record.heroState));
    if (g_move.inBoat)
        std::memcpy(record.boatState, g_move.boatState, sizeof(record.boatState));
    return true;
}

bool SaveConfirmedBattle(H3Hero *hero, bool movementScoped)
{
    RawExeVector *vector = nullptr;
    unsigned char *savedLast = nullptr;
    bool shortenEvents = false;
    SaveRepairRecord repair = {};
    bool repairNeeded = false;

    if (movementScoped)
    {
        if (!g_move.safeAnchor || !g_move.eventCountValid || !hero || hero != g_move.hero ||
            hero->id != g_move.heroId || hero->owner != g_move.owner)
            return false;

        if (!BuildRepairRecord(hero, repair, repairNeeded))
            return false;

        vector = RecordedEvents();
        unsigned int currentCount = 0;
        if (!ReadVectorCount(vector, sizeof(void *), currentCount) || g_move.eventCount > currentCount)
            return false;
        savedLast = vector->last;
        shortenEvents = true;
    }

    bool staged = false;
    g_writeRepairActive = repairNeeded;
    g_writeRepairEmitted = false;
    if (repairNeeded)
        g_writeRepair = repair;
    else
        std::memset(&g_writeRepair, 0, sizeof(g_writeRepair));

    __try
    {
        if (shortenEvents && vector->first)
            vector->last = vector->first + g_move.eventCount * sizeof(void *);
        staged = CreateStagedBattleSave();
    }
    __finally
    {
        if (shortenEvents)
            vector->last = savedLast;
        g_writeRepairActive = false;
        std::memset(&g_writeRepair, 0, sizeof(g_writeRepair));
    }

    const bool repairWasWritten = !repairNeeded || g_writeRepairEmitted;
    g_writeRepairEmitted = false;
    if (!staged || !repairWasWritten)
    {
        ClearFileTransaction(true);
        return false;
    }

    const bool committed = CommitStagedSave();
    if (committed)
        MarkBattleSaveForLoadDialog();
    ClearFileTransaction(true);
    return committed;
}

bool IsMapCoordinateValid(int x, int y, int z)
{
    if (!H3Main::Get())
        return false;
    const int mapSize = static_cast<int>(H3Main::Get()->mainSetup.mapSize);
    const int mapDepth = static_cast<int>(1 + !!H3Main::Get()->mainSetup.hasUnderground);
    return x >= 0 && y >= 0 && z >= 0 && x < mapSize && y < mapSize && z < mapDepth;
}

bool BoatMatchesCapturedIdentity(NativeObject *boat, const SaveRepairRecord &record, short startX, short startY,
                                 short startZ)
{
    return boat && boat->Field<BOOL8>(24) && boat->Field<unsigned char>(25) == record.boatState[25] &&
           boat->Field<unsigned char>(26) == record.boatState[26] &&
           boat->Field<unsigned char>(28) == record.boatState[28] && boat->Field<int>(32) == record.heroId &&
           boat->Field<short>(0) == startX && boat->Field<short>(2) == startY && boat->Field<short>(4) == startZ;
}

NativeObject *FindCapturedBoat(const SaveRepairRecord &record, short startX, short startY, short startZ)
{
    if (!H3Main::Get() || !record.boatState[24] || !record.boatState[36] ||
        *reinterpret_cast<const int *>(record.boatState + 32) != record.heroId ||
        *reinterpret_cast<const short *>(record.boatState + 0) != startX ||
        *reinterpret_cast<const short *>(record.boatState + 2) != startY ||
        *reinterpret_cast<const short *>(record.boatState + 4) != startZ)
        return nullptr;

    RawExeVector *pool = BoatPool();
    unsigned int count = 0;
    if (!ReadVectorCount(pool, BOAT_RECORD_SIZE, count) || !pool->first)
        return nullptr;

    NativeObject *match = nullptr;
    const unsigned int capturedIndex = record.boatState[25];
    if (capturedIndex < count)
    {
        NativeObject *candidate = reinterpret_cast<NativeObject *>(pool->first + capturedIndex * BOAT_RECORD_SIZE);
        if (BoatMatchesCapturedIdentity(candidate, record, startX, startY, startZ))
            match = candidate;
    }

    // The byte id is normally the vector index, but scan the validated pool as
    // a compatibility fallback.  Reject ambiguity rather than binding another
    // boat to the hero.
    for (unsigned int i = 0; i < count; ++i)
    {
        NativeObject *candidate = reinterpret_cast<NativeObject *>(pool->first + i * BOAT_RECORD_SIZE);
        if (!BoatMatchesCapturedIdentity(candidate, record, startX, startY, startZ))
            continue;
        if (match && match != candidate)
            return nullptr;
        match = candidate;
    }
    return match;
}

bool RepairPostconditionMatches(H3Hero *hero, NativeObject *boat, const SaveRepairRecord &record, short startX,
                                short startY, short startZ, bool restoreBoat)
{
    if (!hero || hero->x != startX || hero->y != startY || hero->z != startZ ||
        hero->maxMovement != record.movementMax || hero->movement != record.movementCurrent ||
        hero->dest_x != record.targetX || hero->dest_y != record.targetY || hero->dest_z != record.targetZ ||
        hero->flyPower != record.flyCastPower || hero->waterwalkPower != record.waterwalkCastPower ||
        hero->flags != record.tempModFlags || HeroFacing(hero) != record.facing)
        return false;

    if (!restoreBoat)
        return true;

    return BoatMatchesCapturedIdentity(boat, record, startX, startY, startZ) && !boat->Field<BOOL8>(6) &&
           boat->Field<BOOL8>(36) &&
           std::memcmp(reinterpret_cast<unsigned char *>(boat) + BOAT_PAYLOAD_OFFSET,
                       record.boatState + BOAT_PAYLOAD_OFFSET, BOAT_PAYLOAD_SIZE) == 0;
}

bool ApplyPendingRepair()
{
    if (!g_pendingRepairActive || !IsRepairRecordSane(g_pendingRepair) || !H3Main::Get())
        return false;

    SaveRepairRecord &record = g_pendingRepair;
    H3Hero *hero = H3Main::Get()->GetHero(record.heroId);
    if (!hero || hero->id != record.heroId || hero->owner != record.owner)
        return false;

    const short startX = *reinterpret_cast<const short *>(record.heroState + 0);
    const short startY = *reinterpret_cast<const short *>(record.heroState + 2);
    const short startZ = *reinterpret_cast<const short *>(record.heroState + 4);
    if (!IsMapCoordinateValid(record.savedX, record.savedY, record.savedZ) ||
        !IsMapCoordinateValid(startX, startY, startZ))
        return false;

    const bool savedInBoat = (record.flags & REPAIR_SAVED_IN_BOAT) != 0;
    const bool preInBoat = (record.flags & REPAIR_PRE_IN_BOAT) != 0;
    const bool heroInBoat = (hero->flags & IN_BOAT_FLAG) != 0;
    const bool atSavedState =
        hero->x == record.savedX && hero->y == record.savedY && hero->z == record.savedZ && heroInBoat == savedInBoat;
    const bool atAnchorState = hero->x == startX && hero->y == startY && hero->z == startZ && heroInBoat == preInBoat;
    if (!atSavedState && !atAnchorState)
        return false;

    // Depending on how the save was loaded, the selected hero may already be
    // mobilized by the adventure manager.  A mobilized hero is intentionally
    // hidden from the map-cell stack (visible == false); rejecting that state
    // made the boat repair depend on load/event ordering.  Preserve the
    // representation found at this point: visible heroes are moved with
    // Hide/Show, while mobile heroes remain hidden and will be shown later by
    // the game's normal demobilization path at their repaired coordinates.
    const bool heroWasVisible = hero->isVisible != 0;

    const bool restoreBoat =
        (record.flags & (REPAIR_PRE_IN_BOAT | REPAIR_BOAT_STATE)) == (REPAIR_PRE_IN_BOAT | REPAIR_BOAT_STATE);
    NativeObject *boat = nullptr;
    if (restoreBoat)
    {
        __try
        {
            // A FastQuit load can expose the boat as visible+empty or retain
            // its hidden+occupied transitional form.  Resolve it by stable
            // pool identity instead of requiring either presentation state.
            boat = FindCapturedBoat(record, startX, startY, startZ);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            boat = nullptr;
        }
        if (!boat)
            return false;
    }

    if (RepairPostconditionMatches(hero, boat, record, startX, startY, startZ, restoreBoat))
        return true;

    bool repaired = false;
    __try
    {
        if (heroWasVisible)
            hero->Hide();
        if (restoreBoat)
        {
            if (boat->Field<BOOL8>(6))
                THISCALL_1(void, OBSCURING_OBJECT_HIDE, boat);
            hero->x = startX;
            hero->y = startY;
            hero->z = startZ;
            if (heroWasVisible)
                hero->Show();
            std::memcpy(reinterpret_cast<unsigned char *>(boat) + BOAT_PAYLOAD_OFFSET,
                        record.boatState + BOAT_PAYLOAD_OFFSET, BOAT_PAYLOAD_SIZE);
        }
        else
        {
            hero->x = startX;
            hero->y = startY;
            hero->z = startZ;
            if (heroWasVisible)
                hero->Show();
        }

        hero->maxMovement = record.movementMax;
        hero->movement = record.movementCurrent;
        hero->dest_x = record.targetX;
        hero->dest_y = record.targetY;
        hero->dest_z = record.targetZ;
        hero->flyPower = record.flyCastPower;
        hero->waterwalkPower = record.waterwalkCastPower;
        hero->flags = record.tempModFlags;
        HeroFacing(hero) = record.facing;
        repaired = RepairPostconditionMatches(hero, boat, record, startX, startY, startZ, restoreBoat);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        repaired = false;
    }
    return repaired;
}

int __stdcall Y_MoveHero(HiHook *hook, int This, int direction, BOOL8 standEnd, int triggerPoint, int noMove,
                         BOOL8 computerMove, int foughtBattle, BOOL8 remoteMove)
{
    if (!IsEnabled())
    {
        ResetMovementSequence();
        return THISCALL_8(int, hook->GetDefaultFunc(), This, direction, standEnd, triggerPoint, noMove, computerMove,
                          foughtBattle, remoteMove);
    }

    H3MapItem *result = nullptr;
    bool captured = false;
    bool defaultReturned = false;
    short stepStartX = 0;
    short stepStartY = 0;
    short stepStartZ = 0;

    if (g_move.state != MOVE_IDLE)
        ResetMovementSequence();
    else
        ResetMovement();

    H3Hero *hero = nullptr;
    if (!computerMove && !remoteMove && GetCurrentMoveHero(hero))
    {
        stepStartX = hero->x;
        stepStartY = hero->y;
        stepStartZ = hero->z;
        captured = PrepareMovement(hero);
        if (!captured)
            ResetMovementSequence();
    }
    else
    {
        // A movement whose local hero cannot be proven (including AI/remote
        // replay) must suppress the stationary combat fallback for its exact
        // call/event scope. Missing an autosave is safer than binding it to a
        // moved or unrelated hero.
        g_move.state = MOVE_ACTIVE;
        g_move.threadId = GetCurrentThreadId();
        g_move.safeAnchor = false;
        captured = true;
    }

    __try
    {
        result = THISCALL_8(H3MapItem *, hook->GetDefaultFunc(), This, direction, standEnd, triggerPoint, noMove,
                            computerMove, foughtBattle, remoteMove);
        defaultReturned = true;
    }
    __finally
    {
        if (captured && defaultReturned && result && g_move.state == MOVE_ACTIVE)
        {
            g_move.state = MOVE_EVENT_PENDING;
            g_move.eventCell = result;
            std::memset(&g_anchor, 0, sizeof(g_anchor));
            std::memset(&g_chain, 0, sizeof(g_chain));
        }
        else if (captured && defaultReturned && !result && g_move.state == MOVE_ACTIVE && hero &&
                 hero->id == g_move.heroId && hero->owner == g_move.owner &&
                 (hero->x != stepStartX || hero->y != stepStartY || hero->z != stepStartZ))
        {
            ContinueMovementChain(hero);
            ResetMovement();
        }
        else
        {
            ResetMovementSequence();
        }
    }
    return reinterpret_cast<int>(result);
}

void __stdcall Y_DoEvent(HiHook *hook, int This, H3MapItem *eventCell, int point)
{
    const DWORD threadId = GetCurrentThreadId();
    const bool ownsTransaction =
        g_move.state == MOVE_EVENT_PENDING && g_move.threadId == threadId && g_move.eventCell == eventCell;
    const bool nestedTransaction = g_move.state == MOVE_EVENT_ACTIVE && g_move.threadId == threadId;
    const bool scopedTransaction = ownsTransaction || nestedTransaction;

    // A different event must not turn the pending movement into an unsafe
    // stationary fallback.  Keep it pending and fail closed until the exact
    // returned event cell arrives or the next movement/lifecycle reset clears it.
    if (ownsTransaction)
        g_move.state = MOVE_EVENT_ACTIVE;
    if (scopedTransaction)
        ++g_eventDepth;

    __try
    {
        THISCALL_3(void, hook->GetDefaultFunc(), This, eventCell, point);
    }
    __finally
    {
        if (scopedTransaction && g_eventDepth)
            --g_eventDepth;
        if (ownsTransaction)
            ResetMovementSequence();
    }
}

int __stdcall Y_DoCombat(HiHook *hook, int This, int point, H3Hero *leftHero, int leftArmy, int rightPlayer,
                         int rightTown, int rightHero, int rightArmy, int seed, int finishHeroes, int alternateLayout)
{
    if (IsEnabled() && IsLocalHumanHero(leftHero))
    {
        const bool synchronousMovement = g_move.state == MOVE_ACTIVE && g_eventDepth == 0;
        const bool matchingEventScope = g_move.state == MOVE_EVENT_ACTIVE && g_eventDepth == 1;
        if ((synchronousMovement || matchingEventScope) && g_move.threadId == GetCurrentThreadId())
        {
            const bool matchingHero =
                leftHero == g_move.hero && leftHero->id == g_move.heroId && leftHero->owner == g_move.owner;
            if (matchingHero)
            {
                SaveConfirmedBattle(leftHero, true);
                g_move.state = MOVE_COMBAT_HANDLED;
            }
        }
        else if (g_move.state == MOVE_IDLE)
        {
            SaveConfirmedBattle(leftHero, false);
        }
    }

    return THISCALL_11(int, hook->GetDefaultFunc(), This, point, leftHero, leftArmy, rightPlayer, rightTown, rightHero,
                       rightArmy, seed, finishHeroes, alternateLayout);
}

int __stdcall Y_SavePathReady(LoHook *, HookContext *context)
{
    if (!g_file.armed || g_file.threadId != GetCurrentThreadId())
        return EXEC_DEFAULT;

    char *enginePath = reinterpret_cast<char *>(context->ebp - ENGINE_SAVE_PATH_EBP_OFFSET);

    // A second path resolution means a save was re-entered while this battle
    // transaction was active. Redirect every additional writer to a disposable
    // file and refuse the transaction, so none can reach the existing slot.
    if (g_file.redirected)
    {
        g_file.failed = true;
        DeleteFileA(DISCARD_SAVE_PATH);
        std::memcpy(enginePath, DISCARD_SAVE_PATH, sizeof(DISCARD_SAVE_PATH));
        return EXEC_DEFAULT;
    }

    const size_t length = BoundedLength(enginePath, ENGINE_SAVE_PATH_CAPACITY);
    if (length >= ENGINE_SAVE_PATH_CAPACITY || !BuildOwnedPaths(enginePath))
    {
        g_file.failed = true;
        CopyString(g_file.stagePath, sizeof(g_file.stagePath), DISCARD_SAVE_PATH);
        DeleteFileA(DISCARD_SAVE_PATH);
        std::memcpy(enginePath, DISCARD_SAVE_PATH, sizeof(DISCARD_SAVE_PATH));
        g_file.redirected = true;
        return EXEC_DEFAULT;
    }

    DeleteFileA(g_file.stagePath);
    const size_t stageLength = BoundedLength(g_file.stagePath, sizeof(g_file.stagePath));
    if (stageLength >= ENGINE_SAVE_PATH_CAPACITY)
    {
        g_file.failed = true;
        CopyString(g_file.stagePath, sizeof(g_file.stagePath), DISCARD_SAVE_PATH);
        DeleteFileA(DISCARD_SAVE_PATH);
        std::memcpy(enginePath, DISCARD_SAVE_PATH, sizeof(DISCARD_SAVE_PATH));
        g_file.redirected = true;
        return EXEC_DEFAULT;
    }

    std::memcpy(enginePath, g_file.stagePath, stageLength + 1);
    g_file.redirected = true;
    return EXEC_DEFAULT;
}

int __stdcall Y_GameSave(HiHook *hook, int This, NativeObject *gzipFile)
{
    const bool owned = g_file.armed && g_file.threadId == GetCurrentThreadId();
    int result = -1;
    bool returned = false;

    if (owned)
    {
        g_file.engineSaveCalled = true;
        g_file.engineSaveSucceeded = false;
    }

    __try
    {
        result = THISCALL_2(int, hook->GetDefaultFunc(), This, gzipFile);
        returned = true;
    }
    __finally
    {
        if (owned)
            g_file.engineSaveSucceeded = returned && result == 0;
    }
    return result;
}

int __stdcall Y_SaveCloseDone(LoHook *, HookContext *context)
{
    if (g_file.armed && g_file.threadId == GetCurrentThreadId())
    {
        g_file.gzipCloseCalled = true;
        g_file.gzipCloseSucceeded = static_cast<int>(context->eax) == 0;
    }
    return EXEC_DEFAULT;
}

void __stdcall OnSavegameWrite(Era::TEvent *)
{
    if (g_writeRepairActive && IsRepairRecordSane(g_writeRepair))
    {
        Era::WriteSavegameSection(sizeof(g_writeRepair), &g_writeRepair, REPAIR_SECTION);
        g_writeRepairEmitted = true;
    }
}

void __stdcall OnSavegameRead(Era::TEvent *)
{
    std::memset(&g_pendingRepair, 0, sizeof(g_pendingRepair));
    g_pendingRepairActive = Era::ReadSavegameSection(sizeof(g_pendingRepair), &g_pendingRepair, REPAIR_SECTION) ==
                                sizeof(g_pendingRepair) &&
                            IsRepairRecordSane(g_pendingRepair);
}

void ResetRuntimeState(bool clearPendingRepair, bool recoverFiles)
{
    ResetMovementSequence();
    ClearFileTransaction(true);
    g_writeRepairActive = false;
    g_writeRepairEmitted = false;
    std::memset(&g_writeRepair, 0, sizeof(g_writeRepair));
    if (clearPendingRepair)
    {
        g_pendingRepairActive = false;
        std::memset(&g_pendingRepair, 0, sizeof(g_pendingRepair));
    }
    if (recoverFiles)
    {
        g_recoveryDone = false;
        RecoverOwnedFiles();
    }
}

void __stdcall OnBeforeLoadGame(Era::TEvent *)
{
    ResetRuntimeState(true, true);
}

void __stdcall OnAfterLoadGame(Era::TEvent *)
{
    ResetMovementSequence();
    ClearFileTransaction(true);
    // This pass is provisional. FastQuit reuses advManager and later load
    // handlers can still mobilize the selected hero or change boat display
    // state, so keep the marker through OnGameEnter and verify it there again.
    ApplyPendingRepair();
}

void __stdcall OnGameEnter(Era::TEvent *)
{
    ResetRuntimeState(false, true);
    if (g_pendingRepairActive)
    {
        // OnGameEnter is fired after LoadSavegame returns. Reapply idempotently
        // in case later load handlers changed the provisional representation,
        // and consume the marker only after the verified postcondition holds.
        if (ApplyPendingRepair())
        {
            g_pendingRepairActive = false;
            std::memset(&g_pendingRepair, 0, sizeof(g_pendingRepair));
        }
    }
}

void __stdcall OnGameLeave(Era::TEvent *)
{
    ResetRuntimeState(true, false);
    g_recoveryDone = false;
}

bool InstallHooks()
{
    if (!ValidatePe32Image() || !ValidateHiHookSite(MOVE_HERO, MOVE_HERO_PROLOG, sizeof(MOVE_HERO_PROLOG), nullptr) ||
        !ValidateHiHookSite(DO_EVENT, DO_EVENT_PROLOG, sizeof(DO_EVENT_PROLOG), nullptr) ||
        !ValidateHiHookSite(DO_COMBAT, DO_COMBAT_PROLOG, sizeof(DO_COMBAT_PROLOG), "HD.WoG") ||
        !ValidateHiHookSite(GAME_SAVE, GAME_SAVE_PROLOG, sizeof(GAME_SAVE_PROLOG), "wzx_HW") || !ValidateSaveSites())
        return false;

    g_savePathHook = modulePatcher->WriteLoHook(SAVE_PATH_READY, Y_SavePathReady);
    g_saveCloseHook = modulePatcher->WriteLoHook(SAVE_CLOSE_DONE, Y_SaveCloseDone);
    g_moveHook = modulePatcher->WriteHiHook(MOVE_HERO, SPLICE_, EXTENDED_, THISCALL_, Y_MoveHero);
    g_eventHook = modulePatcher->WriteHiHook(DO_EVENT, SPLICE_, EXTENDED_, THISCALL_, Y_DoEvent);
    g_gameSaveHook = modulePatcher->WriteHiHook(GAME_SAVE, SPLICE_, EXTENDED_, THISCALL_, Y_GameSave);
    g_combatHook = modulePatcher->CreateHiHook(DO_COMBAT, SPLICE_, EXTENDED_, THISCALL_, Y_DoCombat);

    // Apply last in the chain so an already installed HD.WoG wrapper executes
    // first, matching the safe order when HD.WoG is loaded after BattleSave.
    // ApplyInsert returns the resulting zero-based chain position, so position
    // zero is success and must not be interpreted as a Boolean failure.
    if (g_combatHook)
        g_combatHook->ApplyInsert(-1);

    if (!g_savePathHook || !g_saveCloseHook || !g_moveHook || !g_eventHook || !g_gameSaveHook || !g_combatHook ||
        !g_savePathHook->IsApplied() || !g_saveCloseHook->IsApplied() || !g_moveHook->IsApplied() ||
        !g_eventHook->IsApplied() || !g_combatHook->IsApplied() || !g_gameSaveHook->IsApplied())
    {
        modulePatcher->UndoAll();
        return false;
    }
    return true;
}

bool StartPlugin()
{
    if (!InstallHooks())
        return false;

    Era::RegisterHandler(OnSavegameWrite, "OnSavegameWrite");
    Era::RegisterHandler(OnSavegameRead, "OnSavegameRead");
    Era::RegisterHandler(OnBeforeLoadGame, "OnBeforeLoadGame");
    Era::RegisterHandler(OnAfterLoadGame, "OnAfterLoadGame");
    Era::RegisterHandler(OnGameEnter, "OnGameEnter");
    Era::RegisterHandler(OnGameLeave, "OnGameLeave");
    Era::RegisterHandler(OnGameLeave, "OnGameLeft");
    return true;
}
} // namespace

namespace battleSave
{
void Initialize() noexcept
{
    if (initialized)
        return;
    initialized = true;
    // Keep module patches separate: failure must never undo other settings hooks.
    if (!globalPatcher || globalPatcher->GetInstance(INSTANCE_NAME))
        return;
    modulePatcher = globalPatcher->CreateInstance(INSTANCE_NAME);
    if (modulePatcher)
        available = StartPlugin();
}

bool IsAvailable() noexcept
{
    return available;
}
} // namespace battleSave
