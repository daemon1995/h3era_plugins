#include "framework.h"

namespace
{
LPCSTR Text(LPCSTR key, LPCSTR fallback)
{
    bool found = false;
    LPCSTR text = EraJS::read(key, found);
    return found && text && *text ? text : fallback;
}

// Coordinates use H3Position packing. Zero is a valid position (0,0,0).
class ObjectListDlg : public H3Dlg
{
    static constexpr int ROW_COUNT = 8;
    static constexpr int FIRST_ROW = 100;
    std::vector<DWORD> positions;
    std::vector<int> sourceIndexes;
    int page = 0;
    H3DlgFrame* selection = nullptr;
    H3DlgText* pageLabel = nullptr;
    void Refresh()
    {
        for (int i = 0; i < ROW_COUNT; ++i)
        {
            const int index = page * ROW_COUNT + i;
            auto label = GetText(FIRST_ROW + i);
            if (index < int(positions.size()))
            {
                H3Position pos(positions[index]);
                H3String name = H3String::Format("%s %d     (%d, %d, %d)",
                    Text("mrart.teleport_dlg.object_list.destination", "Object"), sourceIndexes[index] + 1,
                    pos.GetX(), pos.GetY(), pos.GetZ());
                label->SetText(name);
                label->ShowActivate();
            }
            else label->HideDeactivate();
        }
        H3String status = H3String::Format("%s %d / %d", Text("mrart.teleport_dlg.object_list.page", "Page"),
            page + 1, std::max(1, (int(positions.size()) + ROW_COUNT - 1) / ROW_COUNT));
        pageLabel->SetText(status);
        GetText(20)->EnableItem(page > 0);
        GetText(20)->color = page > 0 ? eTextColor::WHITE : eTextColor::GRAY;
        GetText(21)->EnableItem((page + 1) * ROW_COUNT < int(positions.size()));
        GetText(21)->color = (page + 1) * ROW_COUNT < int(positions.size()) ? eTextColor::WHITE : eTextColor::GRAY;
        selection->Hide();
        if (chosen >= page * ROW_COUNT && chosen < (page + 1) * ROW_COUNT)
        {
            selection->SetY(76 + (chosen % ROW_COUNT) * 42);
            selection->Show();
        }
        Redraw();
    }
    int chosen = -1;
public:
    ObjectListDlg(const DWORD* list, int count, DWORD current) : H3Dlg(520, 520, -1, -1, FALSE, TRUE)
    {
        for (int i = 0; i < count; ++i)
        {
            if (list[i] == current || list[i] == UINT_MAX ||
                std::find(positions.begin(), positions.end(), list[i]) != positions.end()) continue;
            positions.push_back(list[i]);
            sourceIndexes.push_back(i);
        }
        CreateText(24, 20, widthDlg - 48, 32, Text("mrart.teleport_dlg.object_list.title", "Teleport objects"),
                   NH3Dlg::Text::BIG, eTextColor::GOLD, 1);
        for (int i = 0; i < ROW_COUNT; ++i)
            CreateText(32, 76 + i * 42, widthDlg - 64, 36, "", NH3Dlg::Text::MEDIUM,
                       eTextColor::WHITE, FIRST_ROW + i);
        selection = CreateFrame(28, 76, widthDlg - 56, 36, 2, H3RGB565::Green());
        selection->HideDeactivate();
        CreateText(32, 426, 96, 30, Text("mrart.teleport_dlg.object_list.previous", "Previous"),
                   NH3Dlg::Text::MEDIUM, eTextColor::WHITE, 20)->Activate();
        CreateText(widthDlg - 128, 426, 96, 30, Text("mrart.teleport_dlg.object_list.next", "Next"),
                   NH3Dlg::Text::MEDIUM, eTextColor::WHITE, 21)->Activate();
        CreateFrame(32, 426, 96, 30, 30, H3RGB565::Gold())->DeActivate();
        CreateFrame(widthDlg - 128, 426, 96, 30, 31, H3RGB565::Gold())->DeActivate();
        pageLabel = CreateText(132, 424, widthDlg - 264, 26, "", NH3Dlg::Text::MEDIUM, eTextColor::WHITE, 3);
        CreateOKButton(96, 470)->Disable();
        CreateCancelButton(widthDlg - 160, 470);
        Refresh();
    }
    BOOL DialogProc(H3Msg& msg) override
    {
        if (!msg.IsLeftClick()) return FALSE;
        if (msg.itemId == 20 && page > 0) { --page; Refresh(); }
        else if (msg.itemId == 21 && (page + 1) * ROW_COUNT < int(positions.size())) { ++page; Refresh(); }
        else if (msg.itemId >= FIRST_ROW && msg.itemId < FIRST_ROW + ROW_COUNT)
        {
            int index = page * ROW_COUNT + msg.itemId - FIRST_ROW;
            if (index < int(positions.size()))
            {
                chosen = index;
                GetDefButton(eControlId::OK)->Enable();
                Refresh();
            }
        }
        return FALSE;
    }
    BOOL OnMouseWheel(int direction) override
    {
        if (direction > 0 && page > 0) --page;
        else if (direction < 0 && (page + 1) * ROW_COUNT < int(positions.size())) ++page;
        Refresh();
        return TRUE;
    }
    BOOL OnKeyPress(eVKey key, eMsgFlag) override
    {
        if (key == eVKey::H3VK_LEFT || key == eVKey::H3VK_PAGE_UP) return OnMouseWheel(1);
        if (key == eVKey::H3VK_RIGHT || key == eVKey::H3VK_PAGE_DOWN) return OnMouseWheel(-1);
        return FALSE;
    }
    void OnCancel() override { chosen = -1; }
    int Result() const { return chosen < 0 ? -1 : sourceIndexes[chosen]; }
};
}

#pragma comment(linker, "/EXPORT:DisplayObjectList=_DisplayObjectList@12")
DllExport int __stdcall DisplayObjectList(const DWORD* positions, int count, DWORD currentPosition)
{
    if (!positions || count <= 0 || count > 65536) return -1;
    ObjectListDlg dialog(positions, count, currentPosition);
    dialog.Start();
    return dialog.Result();
}
