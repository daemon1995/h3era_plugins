#include "pch.h"
#include "WoGPathCore.h"
#include <cstdint>
#include <cstring>

void *WoGLine_Data();

namespace {
using namespace wog_path;
Grid ai[3]{}, monster{}, monsterSteps{};
Search search{};

int mapSize() { return static_cast<int>(h3::H3MapSize::Get()); }
int findAI(Point seed, Point origin, int index, bool shortway) {
    if (index < 0 || index >= 3) return -1;
    return search.run(ai[index], mapSize(), seed, origin, shortway,
        [origin](int x, int y) { return CDECL_3(int, 0x706D80, x, y, origin.z) != 0; });
}
int findMonster(Point origin, Point seed, int where) {
    return search.run(monster, mapSize(), seed, origin, false,
        [origin, where](int x, int y) { return CDECL_4(int, 0x779B03, x, y, origin.z, where) != 0; });
}
void makeLine(Point &from, Point &to) { THISCALL_2(void, 0x779470, &from, &to); }
Point *line() { return static_cast<Point *>(WoGLine_Data()); }
int lineCount() { int n = *reinterpret_cast<const short *>(0x2933558); return n >= 0 && n <= maxSize ? n : 0; }
bool same(Point a, Point b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
DWORD pack(Point p) { return h3::H3Position::Pack(p.x, p.y, p.z); }

int __cdecl FindWayAI(HiHook *, Point *seed, Point *origin, int index, int shortway) {
    if (!seed || !origin) return 0;
    return findAI(*seed, *origin, index, shortway != 0) == 1 ? 1 : 0;
}
DWORD __cdecl MakeAIWay(HiHook *, Point *from, Point *to, int steps, int index) {
    if (!from || !to || index < 0 || index >= 3) return 0;
    Point result{};
    return trace(ai[index], nullptr, mapSize(), *from, *to, steps, result) ? pack(result) : pack(*from);
}
int __cdecl FindStepAI(HiHook *, int x, int y, int z, int tx, int ty, int tz, DWORD *mixed, int steps) {
    const int size = mapSize();
    if (!mixed || x < 0 || y < 0 || z < 0 || tx < 0 || ty < 0 || tz < 0 ||
        x >= size || y >= size || tx >= size || ty >= size || z > 1 || tz > 1 || size > maxSize) return 0;
    Point from{static_cast<short>(x), static_cast<short>(y), static_cast<short>(z)};
    Point to{static_cast<short>(tx), static_cast<short>(ty), static_cast<short>(tz)};
    if (same(from, to)) return 2;
    Point result{};
    if (z == tz && ai[0].value[cell(x, y)] > 0 && trace(ai[0], nullptr, size, from, to, steps, result)) {
        *mixed = pack(result); return 1;
    }
    makeLine(from, to);
    for (int i = 0, n = lineCount(); i < n; ++i) {
        Point candidate = line()[i];
        // The seed must be the candidate: tracing starts at the hero and
        // follows decreasing distances toward that candidate.
        if (findAI(candidate, from, 2, true) == 1) return 2;
        if (trace(ai[2], nullptr, size, from, candidate, steps, result)) {
            *mixed = pack(result); return 1;
        }
    }
    return 0;
}
int __cdecl GetDistAI(HiHook *, Point p, int index) {
    const int size = mapSize();
    if (size < 1 || size > maxSize || index < 0 || index >= 3 || !valid(p, size)) return -1;
    const Grid &g = ai[index];
    const int c = cell(p.x, p.y), value = g.value[c];
    if (!value) return -1;
    if (value > 0) return value;
    int best = -1;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy) {
            if (!dx && !dy) continue;
            const int d = g.value[c + dx * width + dy];
            if (d > 0 && (best < 0 || d < best)) best = d;
        }
    return best < 0 ? -1 : best + 1;
}
int __cdecl FindWayMonster(HiHook *, Point *from, Point *to, int where) {
    if (!from || !to) return 0;
    return findMonster(*from, *to, where) == 1 ? 1 : 0;
}
void __cdecl MakeWayMonster(HiHook *, Point *from, Point *to) {
    if (!from || !to) return;
    Point result{}; trace(monster, &monsterSteps, mapSize(), *from, *to, 0, result);
}
DWORD __cdecl MakeWayMonsterSteps(HiHook *, Point *from, Point *to, int steps) {
    if (!from || !to) return 0;
    Point result{};
    return trace(monster, &monsterSteps, mapSize(), *from, *to, steps, result) ? pack(result) : pack(*from);
}
int __cdecl FindStepMonster(HiHook *, Point *fromPtr, Point *toPtr, int where) {
    const int size = mapSize();
    if (!fromPtr || !toPtr || size < 1 || size > maxSize || !valid(*fromPtr, size) || !valid(*toPtr, size)) return 0;
    Point from = *fromPtr, to = *toPtr, result{};
    if (from.z == to.z && findMonster(from, to, where) == 1) return 3;
    if (from.z != to.z || monster.value[cell(from.x, from.y)] <= 0) {
        makeLine(from, to);
        for (int i = 0, n = lineCount(); i < n; ++i) {
            Point candidate = line()[i];
            if (findMonster(from, candidate, where) == 1) return 6;
            if (!trace(monster, &monsterSteps, size, from, candidate, 0, result)) continue;
            return monsterSteps.value[cell(candidate.x, candidate.y)] == 1 ? 5 : 4;
        }
        return 0;
    }
    if (!trace(monster, &monsterSteps, size, from, to, 0, result)) return 0;
    return monsterSteps.value[cell(to.x, to.y)] == 1 ? 2 : 1;
}
int __cdecl FindStepMonsterCoords(HiHook *, int x, int y, int z, int tx, int ty, int tz,
                                    DWORD *mixed, int steps, int where) {
    const int size = mapSize();
    if (!mixed || size < 1 || size > maxSize || x < 0 || y < 0 || tx < 0 || ty < 0 ||
        x >= size || y >= size || tx >= size || ty >= size || z < 0 || tz < 0 || z > 1 || tz > 1) return 0;
    Point from{static_cast<short>(x), static_cast<short>(y), static_cast<short>(z)};
    Point to{static_cast<short>(tx), static_cast<short>(ty), static_cast<short>(tz)}, result{};
    if (z == tz && findMonster(from, to, where) == 1) return 2;
    if (z == tz && monster.value[cell(x, y)] > 0 && trace(monster, &monsterSteps, size, from, to, steps, result)) {
        *mixed = pack(result); return 1;
    }
    makeLine(from, to);
    for (int i = 0, n = lineCount(); i < n; ++i) {
        Point candidate = line()[i];
        if (findMonster(from, candidate, where) == 1) return 2;
        if (trace(monster, &monsterSteps, size, from, candidate, steps, result)) {
            *mixed = pack(result); return 1;
        }
    }
    return 0;
}
int __cdecl WhereToMonster(HiHook *, int index) {
    if (index < 0 || index >= 1000) return 0;
    auto *object = reinterpret_cast<unsigned char *>(0x292F6D8) + index * 16;
    const int size = mapSize(), x = object[7], y = object[8], z = object[9];
    if (size < 1 || size > maxSize || x >= size || y >= size) return 0;
    // WoG's original priority: east, southeast, northeast, south, north,
    // west, southwest, northwest (y direction follows map coordinates).
    static const int dx[] = {1, 1, 1, 0, 0, -1, -1, -1};
    static const int dy[] = {0, -1, 1, -1, 1, 0, -1, 1};
    int nx = x, ny = y;
    for (int i = 0; i < 8; ++i) {
        const int xx = x + dx[i], yy = y + dy[i];
        if (xx >= 0 && yy >= 0 && xx < size && yy < size && monsterSteps.value[cell(xx, yy)] != 0) {
            nx = xx; ny = yy; break;
        }
    }
    object[10] = static_cast<unsigned char>(nx);
    object[11] = static_cast<unsigned char>(ny);
    object[12] = static_cast<unsigned char>(z);
    return nx != x || ny != y;
}

struct HookSite { DWORD address; unsigned char bytes[6]; void *handler; };
const HookSite hooks[] = {
    {0x706DDB, {0x55,0x8B,0xEC,0x83,0xEC,0x20}, reinterpret_cast<void *>(FindWayAI)},
    {0x70727E, {0x55,0x8B,0xEC,0x83,0xEC,0x20}, reinterpret_cast<void *>(MakeAIWay)},
    {0x7074FA, {0x55,0x8B,0xEC,0x83,0xEC,0x1C}, reinterpret_cast<void *>(FindStepAI)},
    {0x7083C0, {0x55,0x8B,0xEC,0x83,0xEC,0x30}, reinterpret_cast<void *>(GetDistAI)},
    {0x779E42, {0x55,0x8B,0xEC,0x83,0xEC,0x20}, reinterpret_cast<void *>(FindWayMonster)},
    {0x77A1FD, {0x55,0x8B,0xEC,0x83,0xEC,0x20}, reinterpret_cast<void *>(MakeWayMonster)},
    {0x77A41E, {0x55,0x8B,0xEC,0x83,0xEC,0x20}, reinterpret_cast<void *>(MakeWayMonsterSteps)},
    {0x77A66A, {0x55,0x8B,0xEC,0x51,0x53,0x56}, reinterpret_cast<void *>(FindStepMonster)},
    {0x77A832, {0x55,0x8B,0xEC,0x83,0xEC,0x1C}, reinterpret_cast<void *>(FindStepMonsterCoords)},
    {0x77B250, {0x55,0x8B,0xEC,0x83,0xEC,0x18}, reinterpret_cast<void *>(WhereToMonster)},
};
void report(const char *message) {
    OutputDebugStringA(message);
    if (Era::WriteLog) Era::WriteLog("XXL", "WoG pathfinding", message);
}
bool readable(const HookSite &site) {
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(reinterpret_cast<void *>(site.address), &info, sizeof(info)) != sizeof(info) ||
        info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    const auto end = reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize;
    return static_cast<std::uintptr_t>(site.address) + sizeof(site.bytes) <= end &&
        std::memcmp(reinterpret_cast<void *>(site.address), site.bytes, sizeof(site.bytes)) == 0;
}
}

void WoGPath_Install() {
    if (!globalPatcher || !WoGLine_Data() || globalPatcher->GetInstance("XXL.WoGPath")) return;
    for (const auto &site : hooks) if (!readable(site)) {
        report("WoG pathfinding skipped: incompatible executable or existing hook."); return;
    }
    auto *pi = globalPatcher->CreateInstance("XXL.WoGPath");
    if (!pi) { report("WoG pathfinding skipped: cannot create patch instance."); return; }
    for (const auto &site : hooks) {
        auto *hook = pi->WriteHiHook(site.address, SPLICE_, EXTENDED_, CDECL_, site.handler);
        if (!hook || !hook->IsApplied()) {
            pi->UndoAll(); report("WoG pathfinding installation failed; hooks rolled back."); return;
        }
    }
    report("WoG AI and monster pathfinding expanded to 252; runtime verification required.");
}
