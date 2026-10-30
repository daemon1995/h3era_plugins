#include "../pch.h"
#include "../Assets.h"
#include "MenuDialogs.h"
#include "RandomMapOptions.h"
#include "HdTemplateCompatibility.h"
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace mapselection
{
namespace
{
DWORD hdBase = 0;
std::deque<void*> deferredPackets;
int modalDepth = 0;
constexpr int TEMPLATE_MAX_ROWS = 20;

int TemplateRows() { return std::max(1, std::min(TEMPLATE_MAX_ROWS, H3GameHeight::Get() / 24 - 1)); }
int TemplateWidth(H3SelectScenarioDialog* owner)
{
    auto* button = owner ? owner->GetH3DlgItem(7001) : nullptr;
    return (button && button->GetWidth() > 24 ? button->GetWidth() : 160) + 16;
}

LPCSTR __stdcall ScenarioName(HiHook* hook, H3SelectScenarioDialog* dlg, int index)
{
    if (hdBase && IsNewGame(dlg) && dlg->randomMapGeneration)
    {
        const int choice = IntAt(hdBase + 0xFAE88);
        const int count = IntAt(hdBase + 0x28C32C);
        auto** labels = reinterpret_cast<LPCSTR*>(DwordAt(hdBase + 0x28C2CC));
        if (labels && choice >= 0 && choice < count && count <= 4096 && labels[choice] && labels[choice][0])
            return labels[choice];
    }
    return THISCALL_2(LPCSTR, hook->GetDefaultFunc(), dlg, index);
}

// patcher_x86 FASTCALL_ passes both ECX and EDX to the extended callback,
// even though the native receive routine only uses CL.
void* __stdcall ReceivePacket(HiHook* hook, int queued, int)
{
    if (!modalDepth && !deferredPackets.empty())
    {
        void* packet = deferredPackets.front();
        deferredPackets.pop_front();
        return packet;
    }
    return FASTCALL_1(void*, hook->GetDefaultFunc(), queued);
}

class TemplateDialog : public MenuDialog
{
    static constexpr int ROW_HEIGHT = 24;
    int rows;
    int top = 0;
    int count;
    int choice;
    LPCSTR* labels;
    H3DlgEdit* search = nullptr;
    H3DlgPcx16* backgroundItem = nullptr;
    std::string query;
    std::vector<int> filtered;
    struct ScrollbarAccess : H3DlgScrollbar { void Range(int n) const { vSetTickCount(n); } };
    static std::string Lower(std::string value)
    {
        if (!value.empty()) CharLowerBuffA(&value[0], static_cast<DWORD>(value.size()));
        return value;
    }
    static void __fastcall Scroll(INT32 tick, H3BaseDlg* owner)
    {
        static_cast<TemplateDialog*>(owner)->SetTop(tick);
    }
    void SetTop(int value)
    {
        top = std::max(0, std::min(std::max(0, int(filtered.size()) - rows), value));
        if (auto* bar = GetScrollbar(200))
        {
            bar->SetTick(top);
            if (bar->GetTicksCount() > 1) bar->SetButtonPosition();
        }
        if (auto* frame = GetH3DlgItem(400)) frame->Hide();
        RefreshRows();
    }
    void RefreshRows(bool redraw = true)
    {
        for (int i = 0; i < rows; ++i)
        {
            auto* text = GetText(100 + i);
            const int index = top + i;
            const int source = index < int(filtered.size()) ? filtered[index] : -1;
            if (!text) continue;
            text->SetText(source >= 0 && labels[source] ? labels[source] : "");
            text->EnableItem(source >= 0);
            text->color = source == choice ? eTextColor::GOLD : eTextColor::REGULAR;
        }
        if (redraw) Redraw();
    }
    void Filter(bool redraw)
    {
        filtered.clear();
        const std::string needle = Lower(query);
        for (int i = 0; i < count; ++i)
            if (labels[i] && Lower(labels[i]).find(needle) != std::string::npos)
                filtered.push_back(i);
        top = 0;
        if (auto* bar = GetScrollbar(200))
        {
            const int ticks = std::max(1, int(filtered.size()) - rows + 1);
            bar->SetTick(0);
            if (ticks > 1) { reinterpret_cast<ScrollbarAccess*>(bar)->Range(ticks); bar->SetButtonPosition(); bar->ShowActivate(); }
            else bar->HideDeactivate();
        }
        if (auto* frame = GetH3DlgItem(400)) frame->Hide();
        RefreshRows(redraw);
    }
    virtual INT vShow(INT zorder, BOOL8 draw) override
    {
        const int result = H3Dlg::vShow(zorder, draw);
        if (search) search->SetFocus(TRUE);
        return result;
    }
    virtual INT vDialogProc(H3Msg& msg) override
    {
        const int result = MenuDialog::vDialogProc(msg);
        if (!endDialog && search)
        {
            const std::string next = search->GetText() ? search->GetText() : "";
            if (next != query) { query = next; Filter(true); }
        }
        return result;
    }
    virtual BOOL DialogProc(H3Msg& msg) override
    {
        if (!IsSelectionAuthority()) { interrupted = true; Stop(); return FALSE; }
        if (msg.IsLeftDown())
        {
            if (msg.itemId >= 100 && msg.itemId < 100 + rows && top + msg.itemId - 100 < int(filtered.size()))
            {
                choice = filtered[top + msg.itemId - 100];
                IntAt(hdBase + 0xFAE88) = choice;
                Stop();
            }
            else if (msg.itemId == 2) Stop();
        }
        return endDialog ? FALSE : TRUE;
    }
    virtual BOOL OnLeftClickOutside() override { Stop(); return FALSE; }
    virtual BOOL OnMouseWheel(INT direction) override
    {
        if (direction) SetTop(top + (direction > 0 ? -1 : 1));
        return FALSE;
    }
    virtual BOOL OnMouseHover(INT32 x, INT32 y) override
    {
        auto* frame = GetH3DlgItem(400);
        if (!frame) return TRUE;
        const int localX = x - xDlg, localY = y - yDlg;
        const int row = (localY - ROW_HEIGHT) / ROW_HEIGHT;
        if (localX >= 2 && localX < widthDlg - 18 && localY >= ROW_HEIGHT && row >= 0 && row < rows && top + row < int(filtered.size()))
        {
            const int frameY = ROW_HEIGHT + row * ROW_HEIGHT;
            if (!frame->IsVisible() || frame->GetY() != frameY)
            { frame->SetY(frameY); frame->Show(); Redraw(); }
        }
        else if (frame->IsVisible()) { frame->Hide(); Redraw(); }
        return TRUE;
    }
  public:
    TemplateDialog(H3SelectScenarioDialog* owner, int x, int y)
        : MenuDialog(owner, TemplateWidth(owner), (TemplateRows() + 1) * ROW_HEIGHT, false), rows(TemplateRows()), count(IntAt(hdBase + 0x28C32C)),
          choice(IntAt(hdBase + 0xFAE88)), labels(reinterpret_cast<LPCSTR*>(DwordAt(hdBase + 0x28C2CC)))
    {
        if (count < 0 || count > 4096 || !labels) count = 0;
        xDlg = std::max(0, std::min(x, H3GameWidth::Get() - widthDlg));
        yDlg = std::max(0, std::min(y, H3GameHeight::Get() - heightDlg));
        auto* background = H3LoadedPcx16::Create(widthDlg, heightDlg);
        if (background)
        {
            background->BackgroundRegion(0, 0, widthDlg, heightDlg, true);
            background->DarkenArea(0, 0, widthDlg, heightDlg, 50);
            for (int row = 0; row <= rows; ++row)
                background->BevelArea(1, row * ROW_HEIGHT + 1, widthDlg - 18, ROW_HEIGHT - 2);
            backgroundItem = H3DlgPcx16::Create(0, 0, widthDlg, heightDlg, 10, nullptr);
            if (backgroundItem) { backgroundItem->SetPcx(background); backgroundItem->DeActivate(); AddItem(backgroundItem); }
            else background->Destroy();
        }
        search = CreateEdit(3, 2, widthDlg - 22, ROW_HEIGHT - 4, 100, "", NH3Dlg::Text::SMALL,
                            eTextColor::REGULAR, eTextAlignment::MIDDLE_LEFT, nullptr, 11, TRUE, 2, 2);
        if (search) search->SetAutoredraw(TRUE);
        for (int i = 0; i < rows; ++i)
            CreateText(4, ROW_HEIGHT + i * ROW_HEIGHT, widthDlg - 22, ROW_HEIGHT, "", NH3Dlg::Text::SMALL,
                       eTextColor::REGULAR, 100 + i, eTextAlignment::MIDDLE_LEFT);
        if (auto* frame = CreateFrame(1, ROW_HEIGHT, widthDlg - 18, ROW_HEIGHT, 400, H3RGB565(H3RGB888::Highlight())))
            frame->DeActivate()->Hide();
        CreateScrollbar(widthDlg - 16, ROW_HEIGHT, 16, rows * ROW_HEIGHT, 200,
                        std::max(1, count - rows + 1), Scroll, TRUE, rows, TRUE);
        Filter(false);
    }
    virtual ~TemplateDialog() override
    {
        if (backgroundItem)
        {
            auto* pcx = backgroundItem->GetPcx();
            backgroundItem->SetPcx(nullptr);
            if (pcx) pcx->Destroy();
        }
    }
};

int __stdcall Templates(HiHook*, int x, int y)
{
    auto* owner = reinterpret_cast<H3SelectScenarioDialog*>(DwordAt(0x69FC44));
    if (IsNewGame(owner) && IsSelectionAuthority())
    {
        TemplateDialog dlg(owner, x, y);
        dlg.Start();
    }
    return 1;
}

int AlliancePlayerCount()
{
    int count = 0;
    for (int color = 0; color < 8; ++color)
        if (P_Game->playersInfo.playerType[color] >= 0) ++count;
    return count;
}
int AllianceWidth() { return std::max(63, 31 * AlliancePlayerCount()) + 132; }
int AllianceHeight() { return 204 + 31 * AlliancePlayerCount(); }

class AllianceDialog : public MenuDialog
{
    BYTE teams[8];
    int rows[8];
    int count = 0;
    void RefreshTeams(bool redraw = true)
    {
        for (int column = 0; column < count; ++column)
            if (auto* flag = GetH3DlgItem(300 + column))
            {
                flag->SetX(111 + column * 31);
                flag->SetY(82 + teams[rows[column]] * 31);
            }
        if (redraw) Redraw();
    }
    virtual void OnOK() override
    {
        if (interrupted || !IsSelectionAuthority()) { Stop(); return; }
        BYTE mapping[8];
        std::memset(mapping, 0xFF, sizeof(mapping));
        BYTE next = 0;
        for (int team = 0; team < 8; ++team)
            for (int row = 0; row < count; ++row)
                if (teams[rows[row]] == team && mapping[team] == 0xFF) mapping[team] = next++;
        for (int row = 0; row < count; ++row) teams[rows[row]] = mapping[teams[rows[row]]];
        std::memcpy(reinterpret_cast<void*>(hdBase + 0xF4C28), teams, sizeof(teams));
        Stop();
    }
    virtual BOOL DialogProc(H3Msg& msg) override
    {
        if (!IsSelectionAuthority()) { interrupted = true; Stop(); return FALSE; }
        if (msg.IsLeftDown())
        {
            if (msg.itemId >= 100 && msg.itemId < 100 + count * 8 &&
                (msg.itemId - 100) % 8 < count)
            {
                teams[rows[(msg.itemId - 100) % 8]] = (msg.itemId - 100) / 8;
                RefreshTeams();
            }
        }
        return endDialog ? FALSE : TRUE;
    }
  public:
    AllianceDialog(H3SelectScenarioDialog* owner) : MenuDialog(owner, AllianceWidth(), AllianceHeight())
    {
        flags |= 0x10;
        std::memcpy(teams, reinterpret_cast<void*>(hdBase + 0xF4C28), sizeof(teams));
        CreateText(0, 32, AllianceWidth(), 42, Text("title.alliances"), NH3Dlg::Text::MEDIUM,
                   eTextColor::GOLD, 10);
        for (int color = 0; color < 8; ++color)
        {
            if (P_Game->playersInfo.playerType[color] < 0) continue;
            if (teams[color] >= 8) teams[color] = color;
            rows[count] = color;
            ++count;
        }
        BYTE mapping[8];
        std::memset(mapping, 0xFF, sizeof(mapping));
        BYTE next = 0;
        for (int team = 0; team < 8; ++team)
            for (int column = 0; column < count; ++column)
                if (teams[rows[column]] == team && mapping[team] == 0xFF) mapping[team] = next++;
        for (int column = 0; column < count; ++column)
            teams[rows[column]] = mapping[teams[rows[column]]];
        for (int row = 0; row < count; ++row)
        {
            char label[16];
            sprintf_s(label, "%d", row + 1);
            CreateText(32, 74 + row * 31, 68, 31, label, NH3Dlg::Text::MEDIUM,
                       eTextColor::REGULAR, 20 + row);
            for (int column = 0; column < count; ++column)
            {
                CreatePcx(100 + column * 31, 74 + row * 31, 31, 31,
                          100 + row * 8 + column, rmgassets::TEAMPLSL_PCX);
            }
        }
        for (int column = 0; column < count; ++column)
            if (auto* flag = CreateDef(111 + column * 31, 82, 300 + column, rmgassets::ITGFLAGS_DEF, rows[column]))
                flag->DeActivate();
        CreateButton(AllianceWidth() / 2 - 65, AllianceHeight() - 60, eControlId::OK, rmgassets::RMG_OKAY_DEF, 0, 1, TRUE, eVKey::H3VK_ENTER);
        CreateButton(AllianceWidth() / 2 + 10, AllianceHeight() - 60, eControlId::CANCEL, rmgassets::RMG_CNCL_DEF, 0, 1, TRUE, eVKey::H3VK_ESCAPE);
        RefreshTeams(false);
    }
};

int __stdcall Alliances(HiHook*)
{
    auto* owner = reinterpret_cast<H3SelectScenarioDialog*>(DwordAt(0x69FC44));
    if (IsNewGame(owner) && IsSelectionAuthority())
    {
        AllianceDialog dlg(owner);
        dlg.Start();
    }
    return 1;
}

void HideComputerOnly(H3SelectScenarioDialog* dlg)
{
    if (!dlg || !dlg->randomMapGeneration) return;
    // Header and zero..eight/random buttons for computer-only players.
    for (int id = 306; id <= 315; ++id)
        if (auto* item = dlg->GetH3DlgItem(id)) item->HideDeactivate();
}

int PlayerControlColor(int id)
{
    if (id >= 207 && id <= 262) return (id - 207) % 8;
    if (id >= 362 && id <= 385) return (id - 362) % 8;
    return -1;
}

int PlayerControlAt(H3SelectScenarioDialog* dlg, H3Msg* msg)
{
    const int x = msg->GetX() - dlg->GetX(), y = msg->GetY() - dlg->GetY();
    for (int id = 207; id <= 385; ++id)
    {
        const int color = PlayerControlColor(id);
        if (color < 0) continue;
        const auto* item = dlg->GetH3DlgItem(id);
        if (item && item->IsVisible() && x >= item->GetX() && y >= item->GetY() &&
            x < item->GetX() + item->GetWidth() && y < item->GetY() + item->GetHeight())
            return color;
    }
    return -1;
}

int __stdcall MenuProc(HiHook* hook, H3SelectScenarioDialog* dlg, H3Msg* msg)
{
    if (msg && IsNewGame(dlg))
    {
        const int id = msg->itemId;
        if (IntAt(0x69959C) && msg->command == eMsgCommand::MOUSE_WHEEL)
        {
            const int color = PlayerControlAt(dlg, msg);
            if (color >= 0 && !CanControlPlayer(dlg, color)) return 1;
        }
        if (msg->command == eMsgCommand::MOUSE_BUTTON)
        {
            if (id == REMEMBER_SETTINGS_ID && msg->IsLeftDown())
            {
                ToggleRememberSettings(dlg);
                return 1;
            }
            const int color = PlayerControlColor(id);
            const bool left = msg->IsLeftDown() || msg->IsLeftClick();
            if (left && IntAt(0x69959C) && color >= 0 && !CanControlPlayer(dlg, color)) return 1;
            if (left && !IsSelectionAuthority() && (id == 1221 || id == 1223 || id == 1225)) return 1;
            if (msg->IsLeftClick() && id >= 207 && id <= 214 && !IsSelectionAuthority())
            {
                RequestHandicap(dlg, id - 207);
                return 1;
            }
            if (left && ((dlg->randomMapGeneration && id >= 306 && id <= 315) ||
                (!IsSelectionAuthority() && (id == 7001 || id == 7002 || id == 7005 ||
                    (id >= 7007 && id <= 7009) || (id >= 3000 && id <= 3005)))))
                return 1;
            if (msg->IsLeftClick() && id >= 362 && id <= 377)
            {
                ShowPlayerDialog(dlg, (id - 362) % 8);
                return 0; // Match HD's handled town/hero click result.
            }
            if (hdBase && msg->IsLeftDown() && id >= 7007 && id <= 7009)
            {
                auto* item = dlg->GetH3DlgItem(id);
                if (item && item->IsVisible() && item->IsEnabled())
                {
                    BYTE* roads = reinterpret_cast<BYTE*>(hdBase + 0xF4BD8);
                    roads[id - 7007] = !roads[id - 7007];
                    // The native HD road control's frame is a dword at +0x34.
                    IntAt(DWORD(item) + 0x34) = roads[id - 7007];
                    item->Draw(); item->Refresh();
                    CDECL_6(int, hdBase + 0xB1DF0, eNetwork::HD_LOBBY_MESSAGE, 4, 6, int(roads[0]), int(roads[1]), int(roads[2]));
                }
                return 1;
            }
        }
    }
    // Preserve HD's folder browser, timer controls, map validation and backend
    // notifications. Its former native hook is undone; this is the sole entry.
    const int result = hdBase ? STDCALL_3(int, hdBase + 0xA3750, hook, dlg, msg)
                              : THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);
    UpdateMenuControls(dlg);
    return result;
}

struct HdPatch
{
    Patch* patch = nullptr;
    int position = 0;
};

HdPatch FindHdAt(DWORD address, int type)
{
    HdPatch found;
    int position = 0;
    for (Patch* patch = globalPatcher->GetFirstPatchAt(address); patch; patch = patch->GetAppliedAfter(), ++position)
    {
        if (patch->GetType() == type && !std::strcmp(patch->GetOwner(), "HD.TE"))
            found = {patch, position};
    }
    return found;
}
}

LPCSTR Text(LPCSTR key)
{
    char path[128];
    sprintf_s(path, "RMG.mapSelection.%s", key);
    bool found = false;
    LPCSTR value = EraJS::read(path, found);
    if (found && value && *value) return value;
    static const std::pair<LPCSTR, LPCSTR> defaults[] = {
        {"title.templates", "Random map template"}, {"title.alliances", "Player alliances"},
        {"title.player", "Starting town and hero"}, {"random", "Random"}, {"heroes", "Heroes"},
        {"rememberSettingsError", "Cannot save player settings preference."},
        {"rememberSettingsHint", "Remember starting towns, heroes and bonuses"}};
    for (const auto &entry : defaults)
        if (!std::strcmp(key, entry.first)) return entry.second;
    return key;
}

void InstallScenarioName()
{
    _PI->WriteHiHook(0x5842E0, SPLICE_, EXTENDED_, THISCALL_, ScenarioName);
}

void UpdateMenuControls(H3SelectScenarioDialog* dlg)
{
    if (!IsNewGame(dlg)) return;
    HideComputerOnly(dlg);
    for (int color = 0; color < 8; ++color)
    {
        auto* item = dlg->GetH3DlgItem(207 + color);
        const bool visible = item && item->IsVisible() && P_Game->playersInfo.playerType[color] >= 0;
        if (!visible) continue;
        if (!item->IsEnabled()) item->EnableItem(true);
        if (!item->IsActive()) item->Activate();
    }
}

MenuDialog::MenuDialog(H3SelectScenarioDialog* owner, int width, int height, bool makeBackground)
    : H3Dlg(width, height, -1, -1, 0, 0, IntAt(0x69CCF4)), parent(owner)
{
    // TownSelectionDlg uses the dialog's exact bounds, without the extra shadow.
    flags &= ~0x10;
    if (makeBackground) AddBackground(0, 0, width, height, true, false, IntAt(0x69CCF4), true);
    ++modalDepth;
}

MenuDialog::~MenuDialog()
{
    --modalDepth;
}

INT MenuDialog::vDialogProc(H3Msg& msg)
{
    if (interrupted) return msg.CloseDialog();
    changedByNetwork = false;
    receivedSelectionSnapshot = false;
    if (IntAt(0x69959C))
    {
        auto* packet = FASTCALL_1(INT32*, 0x5547F0, 1);
        if (packet)
        {
            // Only selection changes can safely run while a modal picker exists.
            // Start, disconnect, map/tab changes and HD packets return to the
            // normal parent loop, after this picker has restored its background.
            const int type = packet[2];
            if (type == eNetwork::REQUEST_HERO_FACE || type == eNetwork::REQUEST_HERO_FACE_REPLY || type == eNetwork::SETAGR || type == eNetwork::UPDATE_PLAYER_POS || type == eNetwork::TOWN_UPDATE || type == REQUEST_PLAYER_CHOICE)
            {
                int closeParent = 0;
                THISCALL_3(int, 0x588A70, parent, packet, &closeParent);
                if (closeParent) { interrupted = true; Stop(); }
                changedByNetwork = true;
                receivedSelectionSnapshot = type == eNetwork::UPDATE_PLAYER_POS;
                Redraw();
            }
            else
            {
                deferredPackets.push_back(packet);
                // Chat, keep-alives and unrelated HD notifications can wait,
                // just as in HD's modal loop. A changing session/map must return
                // to the parent immediately, before the picker accepts a choice.
                const bool sessionChanges = type == eNetwork::GAME_TRANSMIT_INIT || type == eNetwork::PLAYER_DROPPED || type == eNetwork::SET_AS_HOST ||
                    type == eNetwork::GAME_HEADER_INFO || type == eNetwork::GAME_HEADER_INFO_INIT || type == eNetwork::MAP_FILE_NAME || type == eNetwork::NEW_PLAYER ||
                    type == eNetwork::CLICK || type == eNetwork::BAD_VERSION || type == eNetwork::SETUP_PING || type == eNetwork::SETUP_PING_RESPONSE;
                const bool hdChangesMap = type == eNetwork::HD_LOBBY_MESSAGE && packet[3] >= 28 &&
                    (packet[6] == 5 || packet[6] == 8);
                if (sessionChanges || hdChangesMap || deferredPackets.size() >= 256)
                {
                    interrupted = true;
                    Stop();
                }
            }
        }
    }
    if (interrupted) return msg.CloseDialog();
    return H3Dlg::vDialogProc(msg);
}

void PreviewHero(int hero)
{
    if (hdBase && hero >= 0 && hero < limits::TOTAL_HEROES)
        STDCALL_1(int, hdBase + 0x97180, hero);
}

bool InstallHdMenu(HMODULE module)
{
    if (hdBase) return true;
    auto* base = reinterpret_cast<BYTE*>(module);
    if (!base) return false;
    const auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    const auto* nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    // Verified against D29DFFE0...7DB9AF1A. Old IDB addresses are never used.
    static const BYTE preview[] = {0x55,0x8B,0xEC,0x81,0xEC,0xC0,1,0,0,0x56,0x69,0x45,8,0x92,4,0};
    static const BYTE templates[] = {0x55,0x8B,0xEC,0x83,0xEC,0x4C,0xC6,0x45,0xFD,0xE8};
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        nt->FileHeader.TimeDateStamp != 0x6ADDE7AD || nt->OptionalHeader.SizeOfImage != 0x2B1000 ||
        std::memcmp(base + 0x97180, preview, sizeof(preview)) ||
        std::memcmp(base + 0xA1920, templates, sizeof(templates)))
    {
        Era::WriteLog("RMG", "Map selection", "HD menu backend differs; version-specific replacements were not installed.");
        return false;
    }
    HdPatch previous[] = {FindHdAt(0x587FD0, HIHOOK_), FindHdAt(0x586103, LOHOOK_), FindHdAt(0x5854BE, LOHOOK_)};
    if (!previous[0].patch) return false; // HD.TE can finish initialization later.
    hdBase = DWORD(base);
    Patch* replacements[] = {
        _PI->CreateHiHook(0x587FD0, SPLICE_, EXTENDED_, THISCALL_, reinterpret_cast<void*>(MenuProc)),
        _PI->CreateHiHook(hdBase + 0xA1920, SPLICE_, EXTENDED_, STDCALL_, reinterpret_cast<void*>(Templates)),
        _PI->CreateHiHook(hdBase + 0xA1FD0, SPLICE_, EXTENDED_, CDECL_, reinterpret_cast<void*>(Alliances)),
        _PI->CreateHiHook(0x5547F0, SPLICE_, EXTENDED_, FASTCALL_, reinterpret_cast<void*>(ReceivePacket))};
    bool ok = true;
    for (auto* patch : replacements) if (!patch) ok = false;
    if (ok)
        for (const auto& saved : previous)
            if (saved.patch)
            {
                saved.patch->Undo();
                if (saved.patch->IsApplied()) ok = false;
            }
    if (ok)
        for (auto* patch : replacements)
            if (patch->Apply() < 0 || !patch->IsApplied()) { ok = false; break; }
    if (!ok)
    {
        for (int i = 3; i >= 0; --i)
            if (replacements[i]) replacements[i]->Destroy();
        for (const auto& saved : previous)
            if (saved.patch && !saved.patch->IsApplied()) saved.patch->ApplyInsert(saved.position);
        hdBase = 0;
        return false;
    }
    Era::WriteLog("RMG", "Map selection", "Menu, template and alliance pickers installed; duplicate HD.TE hooks removed.");
    return true;
}

bool InitHdMenu()
{
    HMODULE module = GetModuleHandleA("HD_WOG.dll");
    if (module) return InstallHdMenu(module);
    static bool nativeInstalled = false;
    if (!nativeInstalled)
    {
        _PI->WriteHiHook(0x587FD0, SPLICE_, EXTENDED_, THISCALL_, MenuProc);
        _PI->WriteHiHook(0x5547F0, SPLICE_, EXTENDED_, FASTCALL_, ReceivePacket);
        nativeInstalled = true;
    }
    return true;
}
}
