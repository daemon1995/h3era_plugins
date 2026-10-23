#include "pch.h"
#include <climits>
#include <cstring>

namespace
{
constexpr int dimensions[] = {36, 72, 108, 144, 180, 216, 252};
constexpr int sizeItems[] = {281, 282, 283, 284, 3003, 3004, 3005};
constexpr int areas[] = {1, 2, 4, 8, 9, 16, 18, 25, 32, 36, 49, 50, 72, 98};
bool installed = false;
bool correcting = false;
int updateDepth = 0;
HMODULE attemptedModule = nullptr;
H3SelectScenarioDialog *pendingRedrawDialog = nullptr;
unsigned pendingRedrawButtons = 0;

struct ButtonState
{
    bool enabled;
    bool selected;
};

void CaptureButtons(H3SelectScenarioDialog *dlg, ButtonState (&state)[8])
{
    for (int i = 0; i < 8; ++i)
    {
        H3DlgItem *item = dlg->GetH3DlgItem(i == 7 ? 285 : sizeItems[i]);
        state[i] = {item && item->IsEnabled() != FALSE, item && item->IsSet(eControlState::SELECTED) != FALSE};
    }
}

void QueueChangedButtons(H3SelectScenarioDialog *dlg, const ButtonState (&previous)[8])
{
    if (pendingRedrawDialog != dlg)
        pendingRedrawButtons = 0;
    pendingRedrawDialog = dlg;
    ButtonState current[8];
    CaptureButtons(dlg, current);
    for (int i = 0; i < 8; ++i)
        if (current[i].enabled != previous[i].enabled || current[i].selected != previous[i].selected)
            pendingRedrawButtons |= 1u << i;
}

void FlushButtonRedraw(H3SelectScenarioDialog *dlg)
{
    if (pendingRedrawDialog != dlg || correcting || updateDepth)
        return;
    const unsigned changed = pendingRedrawButtons;
    pendingRedrawDialog = nullptr;
    pendingRedrawButtons = 0;
    for (int i = 0; i < 8; ++i)
        if (changed & (1u << i))
            if (H3DlgItem *item = dlg->GetH3DlgItem(i == 7 ? 285 : sizeItems[i]))
                if (item->IsVisible())
                {
                    item->Draw();
                    item->Refresh();
                }
}

struct SizeMask
{
    bool valid[2][7] = {};
    bool Supports(int index, int levels) const
    {
        return index >= 0 && index < 7 && levels >= 1 && levels <= 2 && valid[levels - 1][index];
    }
    bool Any(int index) const
    {
        return valid[0][index] || valid[1][index];
    }
};

int SizeIndex(int dimension)
{
    for (int i = 0; i < 7; ++i)
        if (dimension == dimensions[i])
            return i;
    return -1;
}

bool PositiveInteger(const char *text, int &value)
{
    if (!text)
        return false;
    while (*text == ' ' || *text == '\t')
        ++text;
    if (*text == '+')
        ++text;
    if (*text < '0' || *text > '9')
        return false;
    int result = 0;
    while (*text >= '0' && *text <= '9')
    {
        const int digit = *text++ - '0';
        if (result > (INT_MAX - digit) / 10)
            return false;
        result = result * 10 + digit;
    }
    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n')
        ++text;
    if (*text || result <= 0)
        return false;
    value = result;
    return true;
}

void IncludeRange(SizeMask &mask, int minimum, int maximum, int water)
{
    for (int level = 0; level < 2; ++level)
        for (int i = 0; i < 7; ++i)
        {
            const int side = dimensions[i] / 36;
            const int area = side * side * (level + 1);
            const bool ordinary = minimum <= area && area <= maximum;
            // HD doubles the template interval for islands before rounding to
            // supported areas. Do the same here, without integer division that
            // could incorrectly admit an odd area just above the upper bound.
            const bool islands = 2LL * minimum <= area && area <= 2LL * maximum;
            mask.valid[level][i] |= water == 2 ? islands : water == 3 ? ordinary || islands : ordinary;
        }
}

bool ReadMask(H3SelectScenarioDialog *dlg, SizeMask &mask)
{
    if (dlg->waterContentSelected < 0 || dlg->waterContentSelected > 3)
        return false;
    H3TextTable *table = H3TextTable::Load("rmg.txt");
    if (!table)
        return false;
    bool found = false;
    for (UINT row = 0; row < table->CountRows(); ++row)
    {
        auto &cells = (*table)[row];
        int minimum = 0, maximum = 0;
        // Each named template block contributes its own interval; do not fill gaps
        // between different blocks by collapsing them into a single min/max pair.
        if (cells.Count() >= 3 && cells[0] && *cells[0] && PositiveInteger(cells[1], minimum) &&
            PositiveInteger(cells[2], maximum))
        {
            if (minimum > maximum)
            {
                table->UnLoad();
                return false;
            }
            IncludeRange(mask, minimum, maximum, dlg->waterContentSelected);
            found = true;
        }
    }
    table->UnLoad();
    return found;
}

bool CanRefresh(H3SelectScenarioDialog *dlg)
{
    if (!installed || !dlg || !dlg->randomMapGeneration || dlg->loadGameMode || dlg->saveGameMode || P_IsCampaignGame ||
        !dlg->GetH3DlgItem(sizeItems[6]))
        return false;
    if (!IntAt(0x69959C))
        return true;
    const DWORD directPlay = DwordAt(0x69D858);
    return directPlay && THISCALL_1(BOOL8, DwordAt(DwordAt(directPlay) + 144), directPlay);
}

void NativeClick(H3SelectScenarioDialog *dlg, int id)
{
    if (H3DlgItem *item = dlg->GetH3DlgItem(id))
        item->EnableItem(true);
    H3Msg msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.command = eMsgCommand::MOUSE_BUTTON;
    msg.subtype = eMsgSubtype::LBUTTON_CLICK;
    msg.itemId = id;
    THISCALL_2(int, 0x587FD0, dlg, &msg);
}

void CorrectSelection(H3SelectScenarioDialog *dlg, const SizeMask &mask)
{
    const int selected = SizeIndex(dlg->mapDimension);
    if (mask.Supports(selected, dlg->numberLevels))
        return;
    const int currentDimension = static_cast<int>(dlg->mapDimension);
    int target = -1, levels = dlg->numberLevels, distance = INT_MAX;
    // Preserve the chosen size if possible; otherwise prefer the current level count.
    if (selected >= 0 && mask.Any(selected))
    {
        target = selected;
        levels = mask.valid[0][selected] ? 1 : 2;
    }
    else
        for (int pass = 0; pass < 2 && target < 0; ++pass)
        {
            levels = pass ? (dlg->numberLevels == 2 ? 1 : 2) : dlg->numberLevels;
            for (int i = 0; i < 7; ++i)
                if (mask.Supports(i, levels))
                {
                    const int delta = dimensions[i] > currentDimension ? dimensions[i] - currentDimension
                                                                       : currentDimension - dimensions[i];
                    if (delta < distance)
                    {
                        target = i;
                        distance = delta;
                    }
                }
        }
    if (target < 0)
        return;
    // HD's updater is suppressed only during this two-click transaction, so it
    // cannot undo an intermediate size/level combination before both are set.
    correcting = true;
    if (dlg->mapDimension != dimensions[target])
        NativeClick(dlg, sizeItems[target]);
    if (dlg->numberLevels != levels)
        NativeClick(dlg, 285);
    correcting = false;
}

void Refresh(H3SelectScenarioDialog *dlg, bool redraw)
{
    if (correcting || !CanRefresh(dlg))
        return;
    SizeMask mask;
    if (!ReadMask(dlg, mask))
        return; // Unknown/malformed data leaves HD's state intact.
    CorrectSelection(dlg, mask);
    const int selected = SizeIndex(dlg->mapDimension);
    for (int i = 0; i < 8; ++i)
    {
        const int id = i == 7 ? 285 : sizeItems[i];
        // A size may be selected on either level count. Underground toggles only
        // if the opposite level count is valid for the currently selected size.
        const bool enabled = i == 7 ? mask.Supports(selected, dlg->numberLevels == 2 ? 1 : 2) : mask.Any(i);
        H3DlgItem *item = dlg->GetH3DlgItem(id);
        if (item && (item->IsEnabled() != FALSE) != enabled)
        {
            item->EnableItem(enabled);
            if (redraw && item->IsVisible())
            {
                item->Draw();
                item->Refresh();
            }
        }
    }
    if (!mask.Supports(selected, dlg->numberLevels))
        if (H3DlgItem *begin = dlg->GetH3DlgItem(186))
            begin->EnableItem(false);
}

void __stdcall NormalizeSizes(HiHook *, int islands, int *minimum, int *maximum)
{
    if (!minimum || !maximum)
        return;
    if (islands)
    {
        if (*minimum > INT_MAX / 2 || *maximum > INT_MAX / 2)
        {
            *minimum = INT_MAX;
            *maximum = 0;
            return;
        }
        *minimum *= 2;
        *maximum *= 2;
    }
    for (int area : areas)
        if (*minimum <= area)
        {
            *minimum = area;
            break;
        }
    for (int i = 13; i >= 0; --i)
        if (*maximum >= areas[i])
        {
            *maximum = areas[i];
            break;
        }
}

int __stdcall SizeToItem(HiHook *hook, int dimension)
{
    const int index = SizeIndex(dimension);
    return index >= 0 ? sizeItems[index] : STDCALL_1(int, hook->GetDefaultFunc(), dimension);
}

int __stdcall TemplateUpdater(HiHook *hook, H3SelectScenarioDialog *dlg, int errors)
{
    if (correcting)
        return 1;
    const bool outer = updateDepth++ == 0;
    const bool capture = outer && CanRefresh(dlg);
    ButtonState previous[8];
    if (capture)
        CaptureButtons(dlg, previous);
    if (outer)
        Refresh(dlg, false);
    if (capture)
        QueueChangedButtons(dlg, previous);
    const int result = STDCALL_2(int, hook->GetDefaultFunc(), dlg, errors);
    --updateDepth;
    if (outer)
        Refresh(dlg, false);
    // HD can call this from its dropdown's custom widget. Keep the changes
    // made before the original updater, and present them after the modal
    // dialog has restored the parent's background, even for a non-click message.
    if (capture)
        QueueChangedButtons(dlg, previous);
    return result;
}

int __stdcall DialogProc(HiHook *hook, H3SelectScenarioDialog *dlg, H3Msg *msg)
{
    const bool click = msg && msg->command == eMsgCommand::MOUSE_BUTTON && msg->subtype == eMsgSubtype::LBUTTON_CLICK;
    const int id = click ? msg->itemId : 0;
    if (click && !correcting && CanRefresh(dlg))
        for (int i = 0; i < 8; ++i)
            if (id == (i == 7 ? 285 : sizeItems[i]))
                if (H3DlgItem *item = dlg->GetH3DlgItem(id))
                    if (!item->IsEnabled())
                        return 0;
    if (id == 186 && CanRefresh(dlg) && !correcting)
    {
        Refresh(dlg, false);
        if (H3DlgItem *begin = dlg->GetH3DlgItem(186))
            if (!begin->IsEnabled())
                return 0;
    }
    const int result = THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);
    if (click && id != 186 && id != 188 && !updateDepth)
        Refresh(dlg, true);
    if (id == 186 || id == 188)
    {
        if (pendingRedrawDialog == dlg)
        {
            pendingRedrawDialog = nullptr;
            pendingRedrawButtons = 0;
        }
    }
    else
        FlushButtonRedraw(dlg);
    return result;
}

struct HdImage
{
    BYTE *base;
    IMAGE_NT_HEADERS32 *nt;
    explicit HdImage(HMODULE module) : base(reinterpret_cast<BYTE *>(module)), nt(nullptr)
    {
        auto *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(base);
        if (!base || dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
            return;
        auto *header = reinterpret_cast<IMAGE_NT_HEADERS32 *>(base + dos->e_lfanew);
        if (header->Signature == IMAGE_NT_SIGNATURE && header->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
            header->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            nt = header;
    }
    bool Contains(DWORD address, size_t length) const
    {
        return nt && address >= DWORD(base) && length <= nt->OptionalHeader.SizeOfImage &&
               address - DWORD(base) <= nt->OptionalHeader.SizeOfImage - length;
    }
    bool Bytes(DWORD address, const BYTE *bytes, size_t length) const
    {
        return Contains(address, length) && !std::memcmp(reinterpret_cast<void *>(address), bytes, length);
    }
    DWORD Find(const BYTE *pattern, size_t length, const char *mask = nullptr) const
    {
        if (!nt)
            return 0;
        DWORD match = 0;
        const auto *section = IMAGE_FIRST_SECTION(nt);
        for (UINT i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section)
        {
            const DWORD start = DWORD(base) + section->VirtualAddress;
            const size_t size = section->Misc.VirtualSize;
            if (!(section->Characteristics & IMAGE_SCN_MEM_EXECUTE) || size < length || !Contains(start, size))
                continue;
            for (size_t offset = 0; offset <= size - length; ++offset)
            {
                size_t j = 0;
                const auto *data = reinterpret_cast<const BYTE *>(start + offset);
                for (; j < length; ++j)
                    if ((!mask || mask[j] == 'x') && data[j] != pattern[j])
                        break;
                if (j == length)
                {
                    if (match)
                        return 0;
                    match = start + offset;
                }
            }
        }
        return match;
    }
};

bool ApplyTogether(Patch **patches, int count)
{
    bool ok = true;
    for (int i = 0; i < count; ++i)
        if (!patches[i])
            ok = false;
    if (ok)
        for (int i = 0; i < count; ++i)
            if (patches[i]->Apply() < 0 || !patches[i]->IsApplied())
            {
                ok = false;
                break;
            }
    if (!ok)
        for (int i = count - 1; i >= 0; --i)
            if (patches[i])
            {
                if (patches[i]->IsApplied())
                    patches[i]->Undo();
                patches[i]->Destroy();
            }
    return ok;
}

bool Install(HMODULE module)
{
    HdImage image(module);
    static const BYTE normalize[] = {0x55, 0x8B, 0xEC, 0x83, 0xEC, 8,    0x0F, 0xB6, 0x45, 8,    0x85, 0xC0,
                                     0x74, 0x18, 0x8B, 0x4D, 0x0C, 0x8B, 0x11, 0xD1, 0xE2, 0x8B, 0x45, 0x0C,
                                     0x89, 0x10, 0x8B, 0x4D, 0x10, 0x8B, 0x11, 0xD1, 0xE2, 0x8B, 0x45, 0x10,
                                     0x89, 0x10, 0xC7, 0x45, 0xFC, 0,    0,    0,    0};
    static const BYTE mapper[] = {0x55, 0x8B, 0xEC, 0x8B, 0x45, 8, 0x99, 0xB9, 0x24, 0, 0, 0,
                                  0xF7, 0xF9, 5,    0x18, 1,    0, 0,    0x5D, 0xC2, 4, 0};
    static const BYTE clamp[] = {0x81, 0xB9, 0xA0, 0x18, 0,    0,    0,    0,    0, 0,
                                 0x7E, 9,    0xC7, 0x45, 0xF4, 0,    0,    0,    0, 0xEB,
                                 0x0C, 0x8B, 0x55, 0xFC, 0x8B, 0x82, 0xA0, 0x18, 0, 0};
    const DWORD norm = image.Find(normalize, sizeof(normalize));
    const DWORD map = image.Find(mapper, sizeof(mapper));
    const DWORD cap = image.Find(clamp, sizeof(clamp), "xxxxxx????xxxxx????xxxxxxxxxxx");
    const DWORD updater = map + 0x20;
    const BYTE normReturn[] = {0x8B, 0xE5, 0x5D, 0xC2, 0x0C, 0};
    const BYTE updaterStart[] = {0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x60};
    const BYTE updaterReturn[] = {0x8B, 0xE5, 0x5D, 0xC2, 8, 0};
    const BYTE originalAreas[] = {1, 2, 4, 8, 9, 16, 18, 32};
    if (!norm || !map || !cap || !image.Bytes(norm + 0x9E, normReturn, sizeof(normReturn)) ||
        !image.Contains(norm, 0xA4) || !image.Contains(updater, 0x410) ||
        !image.Bytes(updater, updaterStart, sizeof(updaterStart)) ||
        !image.Bytes(updater + 0x40A, updaterReturn, sizeof(updaterReturn)))
        return false;
    const DWORD table = DwordAt(norm + 0x44);
    if (!image.Bytes(table, originalAreas, sizeof(originalAreas)))
        return false;
    for (int offset : {0x55, 0x80, 0x91})
        if (DwordAt(norm + offset) != table)
            return false;
    for (int offset : {0x1C9, 0x28B})
    {
        const DWORD call = updater + offset;
        if (ByteAt(call) != 0xE8 || call + 5 + IntAt(call + 1) != map)
            return false;
    }
    for (int offset : {7, 17, 26, 35})
        if (!image.Contains(DwordAt(updater + offset), 4))
            return false;
    const int oldCap = IntAt(cap + 6);
    if ((oldCap != 144 && oldCap != 252) || IntAt(cap + 15) != oldCap)
        return false;
    for (DWORD address : {norm, map, updater, cap + 6, cap + 15})
        if (globalPatcher->GetFirstPatchAt(address))
            return false;

    Patch *patches[6] = {};
    int count = 0;
    if (oldCap == 144)
    {
        patches[count++] = _PI->CreateDwordPatch(cap + 6, 252);
        patches[count++] = _PI->CreateDwordPatch(cap + 15, 252);
    }
    patches[count++] = _PI->CreateHiHook(norm, SPLICE_, EXTENDED_, STDCALL_, reinterpret_cast<void *>(NormalizeSizes));
    patches[count++] = _PI->CreateHiHook(map, SPLICE_, EXTENDED_, STDCALL_, reinterpret_cast<void *>(SizeToItem));
    patches[count++] =
        _PI->CreateHiHook(updater, SPLICE_, EXTENDED_, STDCALL_, reinterpret_cast<void *>(TemplateUpdater));
    patches[count++] = _PI->CreateHiHook(0x587FD0, SPLICE_, EXTENDED_, THISCALL_, reinterpret_cast<void *>(DialogProc));
    return ApplyTogether(patches, count);
}
} // namespace

void HdTemplateCompatibility_Ensure()
{
    HMODULE module = GetModuleHandleA("HD_WOG.dll");
    if (installed || !module || attemptedModule == module)
        return;
    attemptedModule = module;
    installed = Install(module);
    Era::WriteLog("XXL", "HD template compatibility",
                  installed ? "Installed: extended sizes, size-button mapping and template masks."
                            : "Not installed: HD signatures/contracts differ or a target is already patched; no "
                              "partial patch retained.");
}

void HdTemplateCompatibility_Refresh(H3SelectScenarioDialog *dialog)
{
    Refresh(dialog, false);
}
