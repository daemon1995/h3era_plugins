#include "../pch.h"
#include "../Assets.h"
#include "MenuDialogs.h"
#include "RandomMapOptions.h"

namespace mapselection
{
namespace
{

constexpr int HERO_COLUMNS = 4;
constexpr int HERO_ROWS = 5;
constexpr int PAGE_HEROES = HERO_COLUMNS * HERO_ROWS;
constexpr int GRID_MARGIN = 18;
constexpr int GRID_PADDING = 3;
constexpr int PANEL_GAP = 8;

int CellWidth(const H3SelectScenarioDialog* dlg)
{
    return dlg->itpaDef && dlg->itpaDef->widthDEF > 0 ? dlg->itpaDef->widthDEF : 48;
}
int CellHeight(const H3SelectScenarioDialog* dlg)
{
    return dlg->itpaDef && dlg->itpaDef->heightDEF > 0 ? dlg->itpaDef->heightDEF : 32;
}
int PanelWidth(const H3SelectScenarioDialog* dlg, int columns)
{
    return 2 * GRID_MARGIN + columns * CellWidth(dlg) + (columns - 1) * GRID_PADDING;
}
int PanelHeight(const H3SelectScenarioDialog* dlg)
{
    return 2 * GRID_MARGIN + (HERO_ROWS + 1) * CellHeight(dlg) + HERO_ROWS * GRID_PADDING;
}
bool HasHeroPanel(int color)
{
    return P_Game->mapInfo.playerAttributes[color].generateHeroAtMainTown != 0;
}
int DialogWidth(const H3SelectScenarioDialog* dlg, int color)
{
    return PanelWidth(dlg, 3) + (HasHeroPanel(color) ? PANEL_GAP + PanelWidth(dlg, HERO_COLUMNS) : 0);
}
int DialogHeight(const H3SelectScenarioDialog* dlg)
{
    return PanelHeight(dlg);
}

struct ChoiceMessage
{
    INT32 recipient = -1;
    INT32 sender = 0;
    INT32 type = REQUEST_PLAYER_CHOICE;
    INT32 size = sizeof(ChoiceMessage);
    INT32 reserved = 0;
    INT32 color;
    INT32 kind; // 1 town, 2 hero ID (not an index in a local candidate list)
    INT32 value;
};
static_assert(sizeof(ChoiceMessage) == 32, "RMG player-choice request ABI");

bool CanEdit(H3SelectScenarioDialog* dlg, int color, bool hero)
{
    if (!CanControlPlayer(dlg, color))
        return false;
    return THISCALL_2(BOOL8, hero ? 0x58D280 : 0x58D180, dlg, color) != 0;
}

bool ApplyChoice(H3SelectScenarioDialog* dlg, int color, int kind, int value)
{
    if (!IsNewGame(dlg) || color < 0 || color >= 8 || P_Game->playersInfo.playerType[color] < 0)
        return false;
    auto* player = Player(dlg, color);
    const auto& attributes = P_Game->mapInfo.playerAttributes[color];
    if (kind == 1)
    {
        const unsigned mask = attributes.availableFactionsBitset;
        if ((!attributes.ownsRandomTown && !(mask & (mask - 1))) || value < -1 || value > 8 ||
            (value >= 0 && !(mask & (1u << value))) || (value == 8 && !dlg->gameVersion))
            return false;
        player->town = value;
        player->selectedHeroIndex = -1;
        THISCALL_1(void, 0x583B60, dlg);
        THISCALL_1(void, 0x58BE90, dlg);
        return true;
    }
    if (kind != 2 || PlayerTown(dlg, color) < 0 ||
        !(attributes.generateHeroAtMainTown || attributes.hasRandomHero))
        return false;
    return ChooseHero(dlg, color, value);
}

void RequestChoice(H3SelectScenarioDialog* dlg, int color, int kind, int value)
{
    if (!CanEdit(dlg, color, kind == 2)) return;
    if (IsSelectionAuthority())
    {
        if (ApplyChoice(dlg, color, kind, value))
            BroadcastPlayerSelection(dlg);
    }
    else
    {
        ChoiceMessage message;
        message.color = color;
        message.kind = kind;
        message.value = value;
        FASTCALL_4(int, 0x554950, &message, 0, 0, 1);
    }
}

bool ApplyGuestChoice(H3SelectScenarioDialog* dlg, const ChoiceMessage& request)
{
    if (request.size != sizeof(ChoiceMessage) || request.type != REQUEST_PLAYER_CHOICE ||
        !IsNewGame(dlg) || !IsSelectionAuthority() || request.color < 0 || request.color >= 8)
        return false;
    const auto* human = HumanPlayer(dlg, request.color);
    return human && human->player && human->player == request.sender &&
        ApplyChoice(dlg, request.color, request.kind, request.value);
}

void RedrawPlayerIfVisible(H3SelectScenarioDialog* dlg, int color)
{
    auto* item = dlg->GetH3DlgItem(362 + color);
    if (item && item->IsVisible())
        THISCALL_4(void, 0x58D7E0, dlg, color, true, -1);
}

int __stdcall SelectionNetwork(HiHook* hook, H3SelectScenarioDialog* dlg, INT32* packet, int* result)
{
    // HD rejects guest changes to native RMG controls, but knows nothing about
    // the additional XXL IDs. Apply the same rule before its packet handler.
    if (packet && packet[2] == eNetwork::CLICK && packet[3] >= 24 && IsSelectionAuthority() &&
        packet[5] >= 3000 && packet[5] <= 3005)
    {
        CDECL_1(void, 0x60B0F0, packet);
        return 0;
    }
    if (packet && packet[2] == eNetwork::CLICK && packet[3] >= 24 && IsSelectionAuthority() &&
        packet[5] >= 207 && packet[5] <= 262)
    {
        const int color = (packet[5] - 207) % 8;
        const auto* human = HumanPlayer(dlg, color);
        if (!human || !packet[1] || human->player != packet[1])
        {
            CDECL_1(void, 0x60B0F0, packet);
            return 0;
        }
    }
    if (packet && packet[2] == REQUEST_PLAYER_CHOICE)
    {
        if (packet[3] == sizeof(ChoiceMessage))
        {
            const auto& request = *reinterpret_cast<ChoiceMessage*>(packet);
            if (request.kind == 3 && IsNewGame(dlg) && IsSelectionAuthority() &&
                request.color >= 0 && request.color < 8 &&
                P_Game->playersInfo.playerType[request.color] >= 0 && request.sender &&
                HumanPlayer(dlg, request.color) && HumanPlayer(dlg, request.color)->player == request.sender)
            {
                H3Msg click = {};
                click.itemId = 207 + request.color;
                int result = 0;
                // The host's native handler applies and broadcasts the click.
                THISCALL_4(int, 0x586880, dlg, &click, &result, FALSE);
                CDECL_1(void, 0x60B0F0, packet);
                return 0;
            }
            // Guests can edit only their occupied human slot; rejected choices
            // receive the authoritative snapshot too (e.g. a competing hero).
            const bool applied = ApplyGuestChoice(dlg, request);
            if (applied)
                RedrawPlayerIfVisible(dlg, request.color);
            if (applied || (IsNewGame(dlg) && IsSelectionAuthority()))
                BroadcastPlayerSelection(dlg);
        }
        CDECL_1(void, 0x60B0F0, packet);
        return 0;
    }
    const bool rebuild = packet && packet[2] == eNetwork::UPDATE_PLAYER_POS && IsNewGame(dlg);
    const int status = THISCALL_3(int, hook->GetDefaultFunc(), dlg, packet, result);
    if (rebuild && !status)
    {
        THISCALL_1(void, 0x583B60, dlg);
        for (int color = 0; color < 8; ++color)
            if (P_Game->playersInfo.playerType[color] >= 0)
                RedrawPlayerIfVisible(dlg, color);
    }
    return status;
}

class PlayerDialog : public MenuDialog
{
    int color;
    int page = 0;
    int shown[PAGE_HEROES];
    bool waiting = false;
    bool closeOnSnapshot = false;
    bool heroReleased = false;
    int pressedHero = -1;
    int lastTown = -2;
    int lastHero = -2;
    H3DlgPcx16* panels[2] = {};

    bool CreatePanel(int index, int x, int width, int columns)
    {
        const int height = PanelHeight(parent);
        auto* background = H3LoadedPcx16::Create(width, height);
        if (!background) return false;
        background->BackgroundRegion(0, 0, width, height, true);
        background->FrameRegion(0, 0, width, height, false, 0, true);
        const int cellW = CellWidth(parent), cellH = CellHeight(parent);
        for (int row = 0; row <= HERO_ROWS; ++row)
            for (int column = 0; column < columns; ++column)
                background->SinkArea(GRID_MARGIN + column * (cellW + GRID_PADDING),
                                     GRID_MARGIN + row * (cellH + GRID_PADDING), cellW, cellH);
        auto* item = H3DlgPcx16::Create(x, 0, width, height, 20 + index, nullptr);
        if (!item) { background->Destroy(); return false; }
        item->SetPcx(background);
        item->DeActivate();
        AddItem(item);
        panels[index] = item;
        return true;
    }

    void RefreshPlayer(bool redraw = true)
    {
        auto* player = Player(parent, color);
        const int town = PlayerTown(parent, color);
        const int selected = SelectedHero(*player);
        const auto& list = Candidates(parent, player);
        const bool hasHeroes = HasHeroPanel(color);
        int available[limits::TOTAL_HEROES];
        int availableCount = 0;
        if (hasHeroes && town >= 0)
            for (int index = 0; index < list.count; ++index)
            {
                const int hero = list.heroes[index];
                if (hero >= 0 && hero < limits::TOTAL_HEROES &&
                    (IntAt(0x69959C) || !HeroTaken(parent, player, hero)))
                    available[availableCount++] = hero;
            }
        const int lastPage = availableCount ? (availableCount - 1) / PAGE_HEROES : 0;
        page = std::max(0, std::min(page, lastPage));
        const unsigned mask = P_Game->mapInfo.playerAttributes[color].availableFactionsBitset;
        for (int index = 0; index < 10; ++index)
        {
            const int i = index - 1;
            auto* item = GetH3DlgItem(100 + index);
            const bool valid = i == -1 || ((mask & (1u << i)) && (i != 8 || parent->gameVersion));
            if (item) item->EnableItem(valid && CanEdit(parent, color, false) && !waiting);
            auto* frame = GetH3DlgItem(120 + index);
            if (frame)
            {
                if (town == i) frame->Show(); else frame->Hide();
            }
        }
        const bool canChooseHero = hasHeroes && CanEdit(parent, color, true);
        auto* randomHero = GetH3DlgItem(300);
        if (canChooseHero && !randomHero)
        {
            const int cellW = CellWidth(parent), cellH = CellHeight(parent);
            const int heroX = PanelWidth(parent, 3) + PANEL_GAP;
            randomHero = CreatePcx(heroX + GRID_MARGIN, GRID_MARGIN, 300, rmgassets::HPSRAND1_PCX);
            if (randomHero)
            {
                if (auto* frame = CreateFrame(heroX + GRID_MARGIN - 1, GRID_MARGIN - 1,
                                              cellW + 2, cellH + 2, 650, H3RGB565(H3RGB888::Highlight())))
                    frame->DeActivate()->Hide();
            }
        }
        if (randomHero)
        {
            if (canChooseHero) randomHero->ShowActivate(); else randomHero->HideDeactivate();
            randomHero->EnableItem(canChooseHero && (!waiting || pressedHero == 300));
        }
        if (auto* frame = GetH3DlgItem(650))
        {
            if (canChooseHero && ((waiting && pressedHero == 300) || (!waiting && selected < 0))) frame->Show(); else frame->Hide();
        }
        for (int i = 0; i < PAGE_HEROES; ++i)
        {
            const int index = page * PAGE_HEROES + i;
            const int candidate = index < availableCount ? available[index] : -1;
            const int hero = candidate >= 0 && !HeroTaken(parent, player, candidate) ? candidate : -1;
            shown[i] = hero;
            auto* portrait = GetPcx(400 + i);
            auto* frame = GetH3DlgItem(600 + i);
            if (hero >= 0)
            {
                // Load and release the resources explicitly, as HD does. Sending
                // an item command here can paint before the modal is shown.
                auto* image = H3LoadedPcx::Load(P_HeroInfo[hero].smallPortrait);
                if (!image)
                {
                    if (portrait) portrait->HideDeactivate();
                    if (frame) frame->Hide();
                    continue;
                }
                if (!portrait)
                {
                    const int cellW = CellWidth(parent), cellH = CellHeight(parent);
                    const int heroX = PanelWidth(parent, 3) + PANEL_GAP;
                    const int x = heroX + GRID_MARGIN + (i % HERO_COLUMNS) * (cellW + GRID_PADDING);
                    const int y = GRID_MARGIN + (1 + i / HERO_COLUMNS) * (cellH + GRID_PADDING);
                    portrait = CreatePcx(x, y, 400 + i, rmgassets::HPSRAND1_PCX);
                    if (portrait)
                    {
                        frame = CreateFrame(x - 1, y - 1, cellW + 2, cellH + 2, 600 + i,
                                            H3RGB565(H3RGB888::Highlight()));
                        if (frame) frame->DeActivate()->Hide();
                    }
                }
                if (!portrait)
                {
                    image->Dereference();
                    continue;
                }
                auto* previous = portrait->GetPcx();
                portrait->SetPcx(image);
                if (previous) previous->Dereference();
                portrait->ShowActivate();
                portrait->EnableItem(canChooseHero && (!waiting || pressedHero == 400 + i));
                if (frame)
                {
                    if ((waiting && pressedHero == 400 + i) || (!waiting && hero == selected)) frame->Show(); else frame->Hide();
                }
            }
            else
            {
                if (portrait) portrait->HideDeactivate();
                if (frame) frame->Hide();
            }
        }
        lastTown = town;
        lastHero = selected;
        if (redraw) Redraw();
    }

    virtual BOOL DialogProc(H3Msg& msg) override
    {
        if (P_Game->playersInfo.playerType[color] < 0) { interrupted = true; Stop(); return FALSE; }
        if (changedByNetwork)
        {
            if (receivedSelectionSnapshot)
            {
                waiting = false;
                if (closeOnSnapshot && heroReleased) { Stop(); return FALSE; }
            }
            RefreshPlayer();
        }
        if (msg.IsLeftDown())
        {
            int kind = 0, value = -1;
            if (msg.itemId >= 100 && msg.itemId <= 109)
            {
                kind = 1;
                value = msg.itemId - 101;
            }
            else if (msg.itemId == 300) kind = 2;
            else if (msg.itemId >= 400 && msg.itemId < 400 + PAGE_HEROES)
            {
                kind = 2;
                value = shown[msg.itemId - 400];
            }
            if (kind && !waiting && CanEdit(parent, color, kind == 2))
            {
                auto* item = GetH3DlgItem(msg.itemId);
                if (item && item->IsEnabled())
                {
                    RequestChoice(parent, color, kind, value);
                    if (kind == 1) page = 0;
                    waiting = !IsSelectionAuthority();
                    const bool closeOnRelease = kind == 2 || (kind == 1 && !HasHeroPanel(color));
                    closeOnSnapshot = closeOnRelease && waiting;
                    if (closeOnRelease) { pressedHero = msg.itemId; heroReleased = false; }
                    RefreshPlayer();
                }
            }
        }
        else if (msg.IsLeftClick() && pressedHero >= 0)
        {
            const bool selected = msg.itemId == pressedHero;
            pressedHero = -1;
            if (selected)
            {
                heroReleased = true;
                if (!waiting) { Stop(); return FALSE; }
            }
        }
        return TRUE;
    }

    virtual BOOL OnRightClick(H3DlgItem* item) override
    {
        const int id = item->GetID();
        if (id >= 400 && id < 400 + PAGE_HEROES)
        {
            const int hero = shown[id - 400];
            if (hero >= 0) PreviewHero(hero);
        }
        else if (id == 300 && lastHero >= 0) PreviewHero(lastHero);
        else if (id >= 101 && id <= 109) { H3TownAlignmentDlg preview(id - 101); }
        return TRUE;
    }
    virtual BOOL OnLeftClickOutside() override { Stop(); return FALSE; }
    virtual INT vDialogProc(H3Msg& msg) override
    {
        if (msg.command == eMsgCommand::LCLICK_OUTSIDE && msg.GetX() >= xDlg && msg.GetX() < xDlg + widthDlg &&
            msg.GetY() >= yDlg && msg.GetY() < yDlg + heightDlg)
            return 0;
        return MenuDialog::vDialogProc(msg);
    }
    virtual BOOL OnMouseWheel(INT direction) override
    {
        if (direction) { page += direction > 0 ? -1 : 1; RefreshPlayer(); }
        return FALSE;
    }
    virtual BOOL OnKeyPress(eVKey key, eMsgFlag) override
    {
        if (key == eVKey::H3VK_ESCAPE) { Stop(); return FALSE; }
        if (key == eVKey::H3VK_LEFT || key == eVKey::H3VK_PAGE_UP) return OnMouseWheel(1);
        if (key == eVKey::H3VK_RIGHT || key == eVKey::H3VK_PAGE_DOWN) return OnMouseWheel(-1);
        return TRUE;
    }
    virtual BOOL OnCreate() override { return panels[0] && (!HasHeroPanel(color) || panels[1]); }
  public:
    PlayerDialog(H3SelectScenarioDialog* owner, int playerColor)
        : MenuDialog(owner, DialogWidth(owner, playerColor), DialogHeight(owner), false), color(playerColor)
    {
        xDlg = owner->GetX() + 48;
        const int cellW = CellWidth(owner), cellH = CellHeight(owner);
        const int townX = 0, panelY = 0;
        const int townW = PanelWidth(owner, 3), heroW = PanelWidth(owner, HERO_COLUMNS);
        const int heroX = townX + townW + PANEL_GAP;
        if (!CreatePanel(0, townX, townW, 3) ||
            (HasHeroPanel(color) && !CreatePanel(1, heroX, heroW, HERO_COLUMNS))) return;
        const auto& attributes = P_Game->mapInfo.playerAttributes[color];
        const unsigned mask = attributes.availableFactionsBitset;
        int townCount = 0;
        for (int town = 0; town < 9; ++town)
            if ((mask & (1u << town)) && (town != 8 || owner->gameVersion)) ++townCount;
        const bool randomTown = townCount && (attributes.ownsRandomTown || townCount > 1);
        int townPosition = randomTown ? 3 : 0;
        for (int i = -1; i < 9; ++i)
        {
            if (i < 0 ? !randomTown : !(mask & (1u << i)) || (i == 8 && !owner->gameVersion))
                continue;
            const int position = i < 0 ? 0 : townPosition++;
            const int x = townX + GRID_MARGIN + (position % 3) * (cellW + GRID_PADDING);
            const int y = panelY + GRID_MARGIN + (position / 3) * (cellH + GRID_PADDING);
            if (i < 0)
                CreatePcx(x, y, 100, rmgassets::HPSRAND0_PCX);
            else CreateDef(x, y, 101 + i, rmgassets::ITPA_DEF, 2 * i + 2);
            if (auto* frame = CreateFrame(x - 1, y - 1, cellW + 2, cellH + 2, 121 + i, H3RGB565(H3RGB888::Highlight())))
                frame->DeActivate()->Hide();
        }
        RefreshPlayer(false);
    }
    virtual ~PlayerDialog() override
    {
        for (auto* panel : panels)
            if (panel)
            {
                auto* background = panel->GetPcx();
                panel->SetPcx(nullptr);
                if (background) background->Destroy();
            }
    }
};
}

void ShowPlayerDialog(H3SelectScenarioDialog* dlg, int color)
{
    if (!CanEdit(dlg, color, false) && !CanEdit(dlg, color, true)) return;
    THISCALL_1(void, 0x583B60, dlg);
    {
        PlayerDialog picker(dlg, color);
        picker.Start();
    }
    for (int i = 0; i < 8; ++i)
        if (P_Game->playersInfo.playerType[i] >= 0) THISCALL_4(void, 0x58D7E0, dlg, i, true, -1);
}

void RequestHandicap(H3SelectScenarioDialog* dlg, int color)
{
    if (!CanControlPlayer(dlg, color) || IsSelectionAuthority()) return;
    ChoiceMessage message;
    message.color = color;
    message.kind = 3;
    FASTCALL_4(int, 0x554950, &message, 0, 0, 1);
}

void InstallPlayerDialog()
{
    _PI->WriteHiHook(0x588A70, SPLICE_, EXTENDED_, THISCALL_, SelectionNetwork);
}
}
