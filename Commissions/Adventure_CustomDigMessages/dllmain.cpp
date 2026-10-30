#define _H3API_PLUGINS_
#include "../../headers/header.h"

#include <cstring>

Patcher* globalPatcher = nullptr;
PatcherInstance* _PI = nullptr;

namespace
{
struct DigContext
{
    int x, y, z;
};
DigContext* currentDig = nullptr;

H3String Key(const char* kind, const char* field, bool tile)
{
    if (tile && currentDig)
        return H3String::Format("mrart.dig.tiles.%d_%d_%d.%s.%s", currentDig->x, currentDig->y,
                                currentDig->z, kind, field);
    return H3String::Format("mrart.dig.messages.%s.%s", kind, field);
}

LPCSTR ReadText(const char* kind)
{
    bool found = false;
    LPCSTR text = EraJS::read(Key(kind, "text", true).String(), found);
    if (found && text && *text)
        return text;
    text = EraJS::read(Key(kind, "text", false).String(), found);
    if (found && text && *text)
        return text;
    if (!std::strcmp(kind, "insufficient")) return "The hero needs a full movement allowance to dig.";
    if (!std::strcmp(kind, "confirmation")) return "Start digging? The hero will spend the remaining movement.";
    return "There is nothing here.";
}

int ReadColor(const char* kind)
{
    bool found = false;
    int color = EraJS::readInt(Key(kind, "color", true).String(), found);
    if (!found) color = EraJS::readInt(Key(kind, "color", false).String(), found);
    return found && color >= 0 && color < 32 ? color : eTextColor::REGULAR;
}

class DigMessage : public H3Dlg
{
    bool accepted = false;
public:
    DigMessage(const char* kind, bool confirm) : H3Dlg(420, 200, -1, -1, FALSE, FALSE)
    {
        bool found = false;
        int width = EraJS::readInt("mrart.dig.dialog_width", found);
        widthDlg = std::min(H3GameWidth::Get() - 32, std::max(260, found ? width : 420));
        LPCSTR text = ReadText(kind);
        H3FontLoader font(NH3Dlg::Text::MEDIUM);
        const int lineHeight = font.Get() ? font->height + 1 : 18;
        const int lineCount = font.Get() ? font->GetLinesCountInText(text, widthDlg - 48) : 1;
        const int textHeight = std::max(24, lineCount * lineHeight);
        heightDlg = textHeight + 104;
        xDlg = (H3GameWidth::Get() - widthDlg) / 2;
        yDlg = (H3GameHeight::Get() - heightDlg) / 2;
        AddBackground(TRUE, FALSE, H3CurrentPlayerID::Get());
        CreateText(24, 24, widthDlg - 48, textHeight, text, NH3Dlg::Text::MEDIUM, ReadColor(kind), 1);
        CreateOKButton(confirm ? 36 : (widthDlg - 64) / 2, heightDlg - 52);
        if (confirm) CreateCancelButton(widthDlg - 100, heightDlg - 52);
    }
    void OnOK() override { accepted = true; }
    bool Show() { Start(); return accepted; }
};

int __stdcall TryToDig(HiHook* hook, H3AdventureManager* manager, int x, int y, int z)
{
    H3Hero* hero = P_ActivePlayer->GetActiveHero();
    if (!hero) return 1; // The native function dereferences a missing current hero.
    DigContext context = {x == -1 ? hero->x : x, x == -1 ? hero->y : y, x == -1 ? hero->z : z};
    DigContext* previous = currentDig;
    currentDig = &context;
    int result = THISCALL_4(int, hook->GetDefaultFunc(), manager, x, y, z);
    currentDig = previous;
    return result;
}

_LHF_(NotEnoughMovement)
{
    if (!currentDig) return EXEC_DEFAULT;
    DigMessage("insufficient", false).Show();
    c->return_address = 0x40ECA3;
    return NO_EXEC_DEFAULT;
}

_LHF_(ConfirmDig)
{
    if (!currentDig) return EXEC_DEFAULT;
    if (!DigMessage("confirmation", true).Show())
    {
        c->return_address = 0x40F185;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

_LHF_(NothingFound)
{
    if (!currentDig) return EXEC_DEFAULT;
    DigMessage("empty", false).Show();
    c->return_address = 0x40F080;
    return NO_EXEC_DEFAULT;
}

_ERH_(OnAfterWog)
{
    static bool installed = false;
    if (installed) return;
    installed = true;
    _PI->WriteHiHook(0x40EBF0, THISCALL_, TryToDig);
    // All three sites precede argument pushes. Native eligibility, Grail award,
    // hole creation, movement cost, AI and network handling stay in the original.
    _PI->WriteLoHook(0x40EC77, NotEnoughMovement);
    _PI->WriteLoHook(0x40EE79, ConfirmDig);
    _PI->WriteLoHook(0x40F05A, NothingFound);
}
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        globalPatcher = GetPatcher();
        _PI = globalPatcher->CreateInstance("EraPlugin.Adventure_CustomDigMessages.daemon_n");
        Era::ConnectEra(module, "EraPlugin.Adventure_CustomDigMessages.daemon_n");
        _REH_(OnAfterWog);
    }
    return TRUE;
}
