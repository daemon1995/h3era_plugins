#include "ActionSwitcher.h"

namespace switcher
{
namespace
{
UINT ReadNumberKeys()
{
    UINT keys = 0;
    if (GetActiveWindow() == H3Hwnd::Get())
        for (int i = 0; i < SwitcherDlgPanel::MAX_ACTIONS; ++i)
            if ((GetAsyncKeyState('1' + i) | GetAsyncKeyState(VK_NUMPAD1 + i)) & 0x8000)
                keys |= 1u << i;
    return keys;
}
} // namespace

// Retained for a future optional interface. The default combat procedure does
// not call this handler: pressing ALT cycles actions without opening a menu.
BOOL SwitcherDlgPanel::HandleHeldAltMenu(H3CombatManager *manager, H3Msg *msg)
{
    const BOOL alt = IsAltHeld(msg);
    const UINT heldKeys = ReadNumberKeys();
    const UINT newKeys = heldKeys & ~heldNumberKeys;
    heldNumberKeys = heldKeys;
    panelShownByAlt = alt;
    SetVisible(alt);
    if (alt && newKeys && visible)
    {
        for (int i = 0; i < MAX_ACTIONS; ++i)
            if (newKeys & (1u << i))
            {
                consumedNumberKeys |= 1u << i;
                SelectAction(static_cast<size_t>(i), manager);
                Refresh();
                return TRUE;
            }
    }

    const bool keyMessage = msg->IsKeyDown() || msg->IsKeyPress() || msg->IsKeyHeld();
    const int number = keyMessage ? static_cast<int>(msg->GetKey()) - static_cast<int>(eVKey::H3VK_1) : -1;
    if (number >= 0 && number < MAX_ACTIONS)
    {
        const UINT bit = 1u << number;
        const bool consumed = (consumedNumberKeys & bit) != 0;
        if (msg->IsKeyPress())
            consumedNumberKeys &= ~bit;
        if (visible || consumed)
        {
            if (visible && !consumed && !msg->IsKeyHeld())
            {
                if (msg->IsKeyDown())
                    consumedNumberKeys |= bit;
                SelectAction(static_cast<size_t>(number), manager);
            }
            Refresh();
            return TRUE;
        }
    }
    if (keyMessage && msg->GetKey() == eVKey::H3VK_ALT)
    {
        if (!alt)
            consumedNumberKeys = 0;
        Refresh();
        return TRUE;
    }
    Refresh();
    return FALSE;
}
} // namespace switcher
