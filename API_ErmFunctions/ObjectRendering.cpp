#include "framework.h"
#include "ObjectRendering.h"
#include "HdMinimapLayout.h"

namespace erm_functions
{
namespace rendering
{
// 0x410DCA: EDI points to H3ObjectAttributes, EBX to H3ObjectDraw.
// Jump to the game's existing flagged-DEF branch; it still handles clipping and animation.
_LHF_(DrawRegisteredObject)
{
    const auto attributes = c->Edi<H3ObjectAttributes *>();
    if (runtime::GetMap() && !IntAt(0x698A44) && runtime::FindObjectType(attributes->type, attributes->subtype))
    {
        c->return_address = 0x410EC1;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

// 0x410F7C follows the native GetFlagColor call. EDI is still the drawn object's attributes.
_LHF_(DrawRegisteredObjectOwner)
{
    auto map = runtime::GetMap();
    const auto attributes = c->Edi<H3ObjectAttributes *>();
    if (map && runtime::FindObjectType(attributes->type, attributes->subtype))
    {
        c->eax = runtime::ReadOwnerAtPosition(c->Local<std::uint32_t>(0x44 / 4));
    }
    return EXEC_DEFAULT;
}

bool RegisteredMinimapColor(H3MapItem *tile, UINT16 &color)
{
    auto map = runtime::GetMap();
    if (!map || !tile)
        return false;
    const int actualType = THISCALL_1(int, 0x4FD220, tile);
    for (int i = int(tile->objectDrawing.Count()) - 1; i >= 0; --i)
    {
        int x = -1, y = -1, z = -1;
        const H3ObjectAttributes *attributes = nullptr;
        if (!runtime::ResolveDrawObject(*map, tile->objectDrawing[i], x, y, z, attributes))
            continue;
        if (!IsMinimapObjectCell(actualType, attributes->type, tile->mirror, tile->access))
            continue;
        const bool *minimap = runtime::FindObjectType(attributes->type, attributes->subtype);
        if (!minimap || !*minimap)
            continue;
        const int owner = runtime::ReadOwnerAtPosition(H3Position::Pack(x, y, z));
        // Native code reads the game palette (0x6AAD18), not the UI palette at 0x6AAD10.
        // Index 63 is the native neutral-owner color; 64..71 are the eight players.
        color = H3GamePalette::Get()->color[64 + owner].GetBits();
        return true;
    }
    return false;
}

// Vanilla visible-tile branch, after hero markers, before ordinary object colors.
_LHF_(MinimapRegisteredObject)
{
    UINT16 color = 0;
    if (RegisteredMinimapColor(c->Ebx<H3MapItem *>(), color))
    {
        c->SI() = color;
        c->return_address = 0x4130DF;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

hd_minimap::Layout hdLayout;
std::uintptr_t hdCodeBase = 0;
HMODULE checkedHdModule = nullptr;

_LHF_(HdMinimap16)
{
    UINT16 color = 0;
    if (!RegisteredMinimapColor(c->Local<H3MapItem *>(0x84 / 4), color))
        return EXEC_DEFAULT;
    c->Local<UINT16>(0x58 / 4) = color;
    // Reuse HD's mine marker append/scaling path. No access to a fake native mine index.
    c->return_address = hdCodeBase + hdLayout.markers16;
    return NO_EXEC_DEFAULT;
}

_LHF_(HdMinimap32)
{
    UINT16 color = 0;
    if (!RegisteredMinimapColor(c->Local<H3MapItem *>(0x88 / 4), color))
        return EXEC_DEFAULT;
    c->Local<UINT16>(0x48 / 4) = color;
    // HD converts RGB565 to its 32-bit buffer and preserves object markers after downscaling.
    c->return_address = hdCodeBase + hdLayout.markers32;
    return NO_EXEC_DEFAULT;
}

void EnsureHdMinimapHooks()
{
    const auto module = GetModuleHandleA("HD_WOG.dll");
    // HD loads after OnAfterWoG in some configurations: retry when the module first appears.
    if (!module || module == checkedHdModule || !_PI)
        return;
    checkedHdModule = module;
    const auto base = reinterpret_cast<const BYTE *>(module);
    const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 0x10000)
        return;
    const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS32 *>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        return;
    const auto section = IMAGE_FIRST_SECTION(nt);
    hd_minimap::Layout candidate;
    std::uintptr_t candidateBase = 0;
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i)
    {
        if (!(section[i].Characteristics & IMAGE_SCN_MEM_EXECUTE))
            continue;
        const auto rva = section[i].VirtualAddress;
        const auto size = section[i].Misc.VirtualSize;
        if (rva >= nt->OptionalHeader.SizeOfImage || size > nt->OptionalHeader.SizeOfImage - rva)
            return;
        const auto found = hd_minimap::FindLayout(base + rva, size);
        if (found)
        {
            if (candidate)
                return; // Ambiguous layout: do not alter this HD build.
            candidate = found;
            candidateBase = reinterpret_cast<std::uintptr_t>(base + rva);
        }
    }
    if (!candidate)
        return;
    const auto site16 = candidateBase + candidate.objects16;
    const auto site32 = candidateBase + candidate.objects32;
    // These private HD seams must be unmodified. Never combine incompatible local-frame hooks.
    if (globalPatcher->GetFirstPatchAt(site16) || globalPatcher->GetFirstPatchAt(site32))
        return;
    auto hook16 = _PI->CreateLoHook(site16, HdMinimap16);
    auto hook32 = _PI->CreateLoHook(site32, HdMinimap32);
    if (!hook16 || !hook32)
    {
        if (hook16) hook16->Destroy();
        if (hook32) hook32->Destroy();
        return;
    }
    hdLayout = candidate;
    hdCodeBase = candidateBase;
    hook16->Apply();
    hook32->Apply();
    if (!hook16->IsApplied() || !hook32->IsApplied())
    {
        if (hook16->IsApplied()) hook16->Undo();
        if (hook32->IsApplied()) hook32->Undo();
        hook16->Destroy();
        hook32->Destroy();
        hdLayout = hd_minimap::Layout{};
        hdCodeBase = 0;
        return;
    }
}

void InstallHooks()
{
    _PI->WriteLoHook(0x410DCA, DrawRegisteredObject);
    _PI->WriteLoHook(0x410F7C, DrawRegisteredObjectOwner);
    _PI->WriteLoHook(0x412F33, MinimapRegisteredObject);
    EnsureHdMinimapHooks();
}
}
}
