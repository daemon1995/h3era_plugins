#include "pch.h"
#include <cstdint>
#include <cstring>

void WoGPath_Install();

namespace
{
// Point::MakeLine writes max(abs(dx), abs(dy)) + 1 entries.
// A 252 by 252 map therefore needs 252 points for valid coordinates.
struct WoGPoint
{
    short x, y, z;
};
static_assert(sizeof(WoGPoint) == 6, "WoG Point ABI");
WoGPoint line[252]{};

struct Site
{
    DWORD address;
    DWORD expected;
    DWORD replacementOffset;
    unsigned char prefixLength;
    unsigned char prefix[3];
};

// Absolute operands in h3era.exe with SHA-256 451e208c...90c36181.
// Point::Line[0].y is referenced separately at 0x77A7B0.
const Site sites[] = {
    {0x77958F, 0x2933560, 0, 2, {0x81, 0xC1}}, {0x779645, 0x2933560, 0, 2, {0x81, 0xC2}},
    {0x779705, 0x2933560, 0, 2, {0x81, 0xC2}}, {0x7797BB, 0x2933560, 0, 2, {0x81, 0xC2}},
    {0x779886, 0x2933560, 0, 2, {0x81, 0xC2}}, {0x77993D, 0x2933560, 0, 2, {0x81, 0xC2}},
    {0x7799FE, 0x2933560, 0, 2, {0x81, 0xC2}}, {0x779AB5, 0x2933560, 0, 2, {0x81, 0xC2}},
    {0x7075D8, 0x2933560, 0, 2, {0x81, 0xC2}}, {0x707627, 0x2933560, 0, 2, {0x81, 0xC1}},
    {0x77A730, 0x2933560, 0, 2, {0x81, 0xC2}}, {0x77A780, 0x2933560, 0, 1, {0x05}},
    {0x77A79A, 0x2933560, 0, 3, {0x0F, 0xBF, 0x82}}, {0x77A7B0, 0x2933562, 2, 3, {0x0F, 0xBF, 0x91}},
    {0x77A919, 0x2933560, 0, 2, {0x81, 0xC2}}, {0x77A962, 0x2933560, 0, 2, {0x81, 0xC1}},
    {0x784540, 0x2933560, 0, 1, {0x68}},
};

void Report(const char *message)
{
    OutputDebugStringA(message);
    if (Era::WriteLog)
        Era::WriteLog("XXL", "WoG Point::Line", message);
}

bool HasExpectedBytes(DWORD address, const void *expected, size_t length)
{
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(reinterpret_cast<const void *>(address), &info, sizeof(info)) != sizeof(info) ||
        info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
        return false;
    const auto end = reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize;
    if (static_cast<std::uintptr_t>(address) + length > end)
        return false;
    return std::memcmp(reinterpret_cast<const void *>(address), expected, length) == 0;
}

bool HasExpectedDword(DWORD address, DWORD expected)
{
    return HasExpectedBytes(address, &expected, sizeof(expected));
}

void __stdcall OnAfterWoG(Era::TEvent *)
{
    if (!globalPatcher || reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr)) != 0x400000)
    {
        Report("WoG line expansion skipped: patcher or executable base unavailable.");
        return;
    }
    for (const auto &site : sites)
        if (!HasExpectedBytes(site.address - site.prefixLength, site.prefix, site.prefixLength) ||
            !HasExpectedDword(site.address, site.expected))
        {
            Report("WoG line expansion skipped: incompatible bytes or an existing patch.");
            return;
        }
    const unsigned char pushImmediate = 0x68;
    if (!HasExpectedBytes(0x784538, &pushImmediate, sizeof(pushImmediate)) ||
        !HasExpectedDword(0x784539, 144))
    {
        Report("WoG line expansion skipped: incompatible initialization size.");
        return;
    }

    if (globalPatcher->GetInstance("XXL.WoGLine"))
    {
        Report("WoG line expansion skipped: patch instance already exists.");
        return;
    }
    PatcherInstance *pi = globalPatcher->CreateInstance("XXL.WoGLine");
    if (!pi)
    {
        Report("WoG line expansion skipped: cannot create patch instance.");
        return;
    }
    const DWORD address = reinterpret_cast<DWORD>(line);
    bool installed = true;
    for (const auto &site : sites)
    {
        Patch *patch = pi->WriteDword(site.address, address + site.replacementOffset);
        if (!patch || !patch->IsApplied())
        {
            installed = false;
            break;
        }
    }
    if (installed)
    {
        Patch *patch = pi->WriteDword(0x784539, 252);
        installed = patch && patch->IsApplied();
    }
    if (!installed)
    {
        pi->UndoAll();
        Report("WoG line expansion failed; installed patches were rolled back.");
        return;
    }
    Report("WoG line expanded to 252 points.");
    WoGPath_Install();
}
} // namespace

void *WoGLine_Data()
{
    return line;
}

void WoGLine_Init()
{
    Era::RegisterHandler(OnAfterWoG, "OnAfterWoG");
}
