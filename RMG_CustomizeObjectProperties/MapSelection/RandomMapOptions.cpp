#include "../pch.h"
#include "../Assets.h"

#include "MenuDialogs.h"
#include "RandomMapOptions.h"

namespace mapselection
{
constexpr int PLAYER_COUNT = 8;

struct PlayerChoice
{
    int town = -1;
    int hero = -1;
    int bonus = 3;
};

struct LastGame
{
    PlayerChoice players[PLAYER_COUNT];
};

LastGame lastGame;
bool settingsLoaded = false;
bool restorePlayers = false;
// True only while this plugin starts a new standalone scenario on the authority.
bool startingLocalGame = false;
constexpr char settingsPath[] = "Runtime\\PluginSettings\\MapSelection.ini";

// Keep the native 0x7C-byte slot and packet 1039 unchanged. All eligible
// heroes live here; a choice beyond the native window is projected into slot 15.
// Guests receive the actual hero ID in that slot, never a process-local index.
H3SelectScenarioDialog *candidateDialog = nullptr;
HeroCandidates candidateLists[16];
bool collectingHeroes = false;

// In the scenario constructor, the preceding network test already selected
// the text-button path for network games. Force that same path for local games
// before the native allocation, instead of drawing a separate overlay.
_LHF_(CreateHandicapTextButton)
{
    c->return_address = 0x57B40F;
    return NO_EXEC_DEFAULT;
}

int CandidateSlot(H3SelectScenarioDialog *dlg, H3ScenarioPlayer *player)
{
    for (int i = 0; i < PLAYER_COUNT; ++i)
    {
        if (player == &dlg->mapPlayersHuman[i])
            return i;
        if (player == &dlg->mapPlayersComputer[i])
            return PLAYER_COUNT + i;
    }
    return -1;
}

const HeroCandidates &Candidates(H3SelectScenarioDialog *dlg, H3ScenarioPlayer *player)
{
    static const HeroCandidates empty;
    const int slot = CandidateSlot(dlg, player);
    return dlg == candidateDialog && slot >= 0 ? candidateLists[slot] : empty;
}

_LHF_(CollectStartingHero)
{
    if (!collectingHeroes || !candidateDialog)
        return EXEC_DEFAULT;
    auto *player = reinterpret_cast<H3ScenarioPlayer *>(IntAt(c->ebp - 0x18));
    const int slot = CandidateSlot(candidateDialog, player);
    if (slot < 0)
        return EXEC_DEFAULT;
    auto &list = candidateLists[slot];
    bool allowed = c->edx < limits::TOTAL_HEROES;
    if (allowed && candidateDialog->randomMapGeneration)
    {
        const auto &hero = P_HeroInfo[c->edx];
        // TRmgGenerator writes the inverse of these flags into the generated
        // header. The synthetic lobby header otherwise marks every hero free.
        allowed = !hero.campaignHero && (candidateDialog->gameVersion ? hero.expansionHero : hero.roeHero);
        const auto *mask = candidateDialog->mapInfo.expansionHeroes;
        bool hasMask = false;
        for (int i = 0; i < 5; ++i)
            hasMask |= mask[i].Get() != 0;
        if (hasMask)
            allowed = allowed && mask[c->edx / 32].GetState(c->edx % 32);
    }
    if (allowed && list.count < limits::TOTAL_HEROES)
    {
        list.heroes[list.count++] = c->edx;
        if (player->heroesCount < 16)
            player->heroes[player->heroesCount++] = c->edx;
    }
    // Resume the original hero loop, bypassing only its sixteen-entry stop.
    // The map's hero-owner, hero-class and per-player availability tests ran above.
    c->return_address = 0x583DAA;
    return NO_EXEC_DEFAULT;
}

bool IsNewGame(const H3SelectScenarioDialog *dlg)
{
    return dlg && !dlg->loadGameMode && !dlg->saveGameMode && !P_IsCampaignGame;
}

bool IsNewLocalGame(const H3SelectScenarioDialog *dlg)
{
    return IsNewGame(dlg) && !IntAt(0x69959C);
}

bool IsSelectionAuthority()
{
    if (!IntAt(0x69959C))
        return true;
    const auto directPlay = DwordAt(0x69D858);
    return directPlay && THISCALL_1(BOOL8, DwordAt(DwordAt(directPlay) + 144), directPlay);
}

bool IsRandomMap(const H3SelectScenarioDialog *dlg)
{
    return IsNewLocalGame(dlg) && dlg->randomMapGeneration;
}

H3ScenarioPlayer *HumanPlayer(H3SelectScenarioDialog *dlg, int color)
{
    for (auto &player : dlg->mapPlayersHuman)
        if (player.player2 == color)
            return &player;
    return nullptr;
}

bool CanControlPlayer(H3SelectScenarioDialog *dlg, int color)
{
    if (!IsNewGame(dlg) || color < 0 || color >= PLAYER_COUNT || P_Game->playersInfo.playerType[color] < 0)
        return false;
    if (!IntAt(0x69959C))
        return true;
    const auto *human = HumanPlayer(dlg, color);
    if (!human)
        return IsSelectionAuthority();
    const int localId = IntAt(0x69D658);
    return localId && human->player == localId;
}

H3ScenarioPlayer *Player(H3SelectScenarioDialog *dlg, int color)
{
    auto *human = HumanPlayer(dlg, color);
    return human ? human : &dlg->mapPlayersComputer[color];
}

int PlayerTown(H3SelectScenarioDialog *dlg, int color)
{
    const auto &attributes = P_Game->mapInfo.playerAttributes[color];
    const unsigned mask = attributes.availableFactionsBitset;
    // For a town fixed by the map, native code derives its type from the mask;
    // the dialog's town field can remain -1. Match that rule without changing it.
    if (!attributes.ownsRandomTown && mask && !(mask & (mask - 1)))
        for (int town = 0; town < 9; ++town)
            if (mask == (1u << town))
                return town;
    return Player(dlg, color)->town;
}

int SelectedHero(const H3ScenarioPlayer &player)
{
    const int i = player.selectedHeroIndex;
    const int hero = i >= 0 && i < player.heroesCount && i < 16 ? player.heroes[i] : -1;
    return hero >= 0 && hero < limits::TOTAL_HEROES ? hero : -1;
}

bool HeroTaken(H3SelectScenarioDialog *dlg, const H3ScenarioPlayer *owner, int hero)
{
    if (hero < 0)
        return false;
    for (int color = 0; color < PLAYER_COUNT; ++color)
    {
        if (P_Game->playersInfo.playerType[color] < 0)
            continue;
        const auto *other = Player(dlg, color);
        if (other != owner && SelectedHero(*other) == hero)
            return true;
    }
    return false;
}

bool ChooseHero(H3SelectScenarioDialog *dlg, int color, int hero)
{
    if (!IsNewGame(dlg) || color < 0 || color >= PLAYER_COUNT)
        return false;
    auto *player = Player(dlg, color);
    if (hero == -1)
    {
        player->selectedHeroIndex = -1;
        return true;
    }
    const auto &list = Candidates(dlg, player);
    if (HeroTaken(dlg, player, hero))
        return false;
    for (int i = 0; i < list.count; ++i)
        if (list.heroes[i] == hero)
        {
            for (int j = 0; j < player->heroesCount && j < 16; ++j)
                if (player->heroes[j] == hero)
                {
                    player->selectedHeroIndex = j;
                    return true;
                }
            if (player->heroesCount == 16)
            {
                player->heroes[15] = hero;
                player->selectedHeroIndex = 15;
                return true;
            }
        }
    return false;
}

bool HasComputerHeroChoice(H3SelectScenarioDialog *dlg, int color)
{
    if (!IsNewGame(dlg) || color < 0 || color >= PLAYER_COUNT || HumanPlayer(dlg, color) ||
        P_Game->playersInfo.playerType[color] < 0)
        return false;
    const auto &attributes = P_Game->mapInfo.playerAttributes[color];
    return PlayerTown(dlg, color) >= 0 && (attributes.generateHeroAtMainTown || attributes.hasRandomHero);
}

bool CanChooseComputerHero(H3SelectScenarioDialog *dlg, int color)
{
    return IsSelectionAuthority() && HasComputerHeroChoice(dlg, color);
}

// Native message 1039 is already used by ShowAdvancedOptions/StartNewGame.
// Use raw bytes: H3ScenarioPlayer has a game-side constructor, but its wire
// representation is a fixed 0x7C-byte record without owning pointers.
struct PlayerSelectionMessage
{
    INT32 recipient = -1;
    INT32 sender = 0;
    INT32 type = eNetwork::UPDATE_PLAYER_POS;
    INT32 size = sizeof(PlayerSelectionMessage);
    INT32 reserved = 0;
    BYTE humans[sizeof(H3ScenarioPlayer) * PLAYER_COUNT];
    BYTE computers[sizeof(H3ScenarioPlayer) * PLAYER_COUNT];
};
static_assert(sizeof(PlayerSelectionMessage) == 2004, "Native selection-message ABI");

void CapturePlayerSelection(PlayerSelectionMessage &msg, H3SelectScenarioDialog *dlg)
{
    memcpy(msg.humans, dlg->mapPlayersHuman, sizeof(msg.humans));
    memcpy(msg.computers, dlg->mapPlayersComputer, sizeof(msg.computers));
}

void BroadcastPlayerSelection(H3SelectScenarioDialog *dlg)
{
    if (!IntAt(0x69959C) || !IsSelectionAuthority())
        return;
    PlayerSelectionMessage msg;
    CapturePlayerSelection(msg, dlg);
    // ECX=packet, EDX=all recipients, then native compression/reliable flags.
    FASTCALL_4(int, 0x554950, &msg, 0, 1, 1);
}

int ReadInt(LPCSTR section, LPCSTR key, int fallback)
{
    char text[4096];
    int value = fallback;
    return Era::ReadStrFromIni(key, section, settingsPath, text) && sscanf_s(text, "%d", &value) == 1 ? value
                                                                                                      : fallback;
}

void WriteInt(LPCSTR section, LPCSTR key, int value)
{
    char text[24];
    sprintf_s(text, "%d", value);
    if (!Era::WriteStrToIni(key, text, section, settingsPath))
        OutputDebugStringA("RMG: cannot save last random-map settings.\n");
}

bool RememberSettings()
{
    return settingsPath[0] && ReadInt("XXL", "RememberSettings", 0) != 0;
}

bool SetRememberSettings(bool enabled)
{
    const bool previous = RememberSettings();
    if (!Era::WriteStrToIni("RememberSettings", enabled ? "1" : "0", "XXL", settingsPath))
        return false;
    if (Era::SaveIni(settingsPath))
        return true;
    Era::WriteStrToIni("RememberSettings", previous ? "1" : "0", "XXL", settingsPath);
    return false;
}

void ToggleRememberSettings(H3SelectScenarioDialog *dlg)
{
    if (!IsNewLocalGame(dlg))
        return;
    auto *checkbox = dlg->GetDef(REMEMBER_SETTINGS_ID);
    if (!checkbox || !checkbox->IsVisible() || !checkbox->IsEnabled())
        return;
    const bool enabled = !RememberSettings();
    if (!SetRememberSettings(enabled))
    {
        H3Messagebox(Text("rememberSettingsError"));
        return;
    }
    // Do not apply an old snapshot when enabling the option in an open lobby.
    restorePlayers = false;
    checkbox->SetFrame(enabled ? 1 : 0);
    // Repaint the background/items before the native text and player overlays.
    // Drawing only the new DEF frame leaves pixels from its previous frame.
    dlg->H3BaseDlg::Redraw();
    dlg->Redraw();
}

void CreateRememberSettingsControl(H3SelectScenarioDialog *dlg)
{
    if (!IsNewLocalGame(dlg) || dlg->GetH3DlgItem(REMEMBER_SETTINGS_ID))
        return;
    const bool enabled = RememberSettings();
    auto *checkbox = H3DlgDef::Create(0, 0, REMEMBER_SETTINGS_ID, rmgassets::RMG_CHBX_DEF, enabled ? 1 : 0, 0);
    if (!checkbox)
        return;
    checkbox->SetHint(Text("rememberSettingsHint"));
    checkbox->HideDeactivate();
    dlg->AddItem(checkbox);
}

void SetRememberSettingsVisible(H3SelectScenarioDialog *dlg, bool visible)
{
    auto *flag = dlg->GetH3DlgItem(263);
    auto *button = dlg->GetH3DlgItem(REMEMBER_SETTINGS_ID);
    if (!button)
        return;
    if (visible && flag && IsNewLocalGame(dlg))
    {
        const int x = flag->GetX() + (flag->GetWidth() - button->GetWidth()) / 2;
        const int y = flag->GetY() - button->GetHeight() - 4;
        button->SetX(x);
        button->SetY(y);
        button->ShowActivate();
    }
    else
        button->HideDeactivate();
}

// ShowAdvancedOptions has finished positioning/showing player controls.
// EBX is the dialog; the native instruction sets mapSelectShown at +0x37C.
_LHF_(ShowPlayerSettings)
{
    SetRememberSettingsVisible(reinterpret_cast<H3SelectScenarioDialog *>(c->ebx), true);
    return EXEC_DEFAULT;
}

// HideMapSelection has hidden the player controls. ESI is still the dialog;
// the native instruction clears the same flag before returning to its caller.
_LHF_(HidePlayerSettings)
{
    SetRememberSettingsVisible(reinterpret_cast<H3SelectScenarioDialog *>(c->esi), false);
    return EXEC_DEFAULT;
}

void ReadLastGame()
{
    lastGame = LastGame{};
    if (!RememberSettings() || ReadInt("XXL", "Version", 0) != 1)
        return;
    for (int color = 0; color < PLAYER_COUNT; ++color)
    {
        char section[24];
        sprintf_s(section, "Player%d", color);
        auto &p = lastGame.players[color];
        p.town = ReadInt(section, "Town", -1);
        p.hero = ReadInt(section, "Hero", -1);
        p.bonus = ReadInt(section, "Bonus", 3);
        if (p.town < -1 || p.town > 8)
            p.town = -1;
        if (p.hero < -1 || p.hero >= limits::TOTAL_HEROES)
            p.hero = -1;
        if (p.bonus < 0 || p.bonus > 3)
            p.bonus = 3;
    }
}

LastGame Capture(H3SelectScenarioDialog *dlg)
{
    LastGame s;
    for (int color = 0; color < PLAYER_COUNT; ++color)
    {
        const auto *p = Player(dlg, color);
        s.players[color] = {p->town, SelectedHero(*p), p->bonusType};
    }
    return s;
}

void SaveLastGame(const LastGame &s)
{
    if (!RememberSettings())
        return;
    WriteInt("XXL", "RememberSettings", 1);
    WriteInt("XXL", "Version", 1);
    for (int color = 0; color < PLAYER_COUNT; ++color)
    {
        char section[24];
        sprintf_s(section, "Player%d", color);
        WriteInt(section, "Town", s.players[color].town);
        WriteInt(section, "Hero", s.players[color].hero);
        WriteInt(section, "Bonus", s.players[color].bonus);
    }
    if (!Era::SaveIni(settingsPath))
        OutputDebugStringA("RMG: cannot save last random-map settings.\n");
}

H3SelectScenarioDialog *__stdcall ScenarioCtor(HiHook *h, H3SelectScenarioDialog *dlg, int mode)
{
    // TEMPORARY multiplayer test: expose campaign heroes to both the lobby
    // and the native random-map generator for this process. Remove after test.
    for (int hero = 0; hero < limits::TOTAL_HEROES; ++hero)
        if (P_HeroInfo[hero].campaignHero)
        {
            P_HeroInfo[hero].campaignHero = 0;
            P_HeroInfo[hero].expansionHero = 1;
        }
    InitHdMenu();
    candidateDialog = nullptr;
    settingsLoaded = false;
    restorePlayers = false;
    auto *result = THISCALL_2(H3SelectScenarioDialog *, h->GetDefaultFunc(), dlg, mode);
    if (IsNewLocalGame(dlg) && ReadInt("XXL", "RememberSettings", -1) == -1)
        if (!SetRememberSettings(false))
            OutputDebugStringA("RMG: cannot initialize RememberSettings=0.\n");
    CreateRememberSettingsControl(dlg);
    SetRememberSettingsVisible(dlg, dlg->mapSelectShown != FALSE);
    UpdateMenuControls(dlg);
    return result;
}

void __stdcall ShowRandomSettings(HiHook *h, H3SelectScenarioDialog *dlg)
{
    if (!settingsLoaded && !dlg->loadGameMode && !dlg->saveGameMode && !IntAt(0x69959C))
    {
        settingsLoaded = true;
        ReadLastGame();
        restorePlayers = RememberSettings();
    }
    THISCALL_1(void, h->GetDefaultFunc(), dlg);
    UpdateMenuControls(dlg);
}

// Native FillStartingHeroesList skips colors absent from mapPlayersHuman.
// At this branch EBX=dialog, EDX=color; the second native lookup already falls back to AI.
_LHF_(FillComputerHeroes)
{
    auto *dlg = reinterpret_cast<H3SelectScenarioDialog *>(c->ebx);
    if (IsNewGame(dlg) && c->edx < PLAYER_COUNT)
    {
        c->eax = reinterpret_cast<int>(&dlg->mapPlayersComputer[c->edx]);
        c->return_address = 0x583BDD;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

void __stdcall FillHeroes(HiHook *h, H3SelectScenarioDialog *dlg)
{
    const bool random = IsRandomMap(dlg);
    const bool restore = random && restorePlayers;
    if (IsNewGame(dlg))
    {
        for (int color = 0; color < PLAYER_COUNT; ++color)
        {
            auto *p = Player(dlg, color);
            if (!restore || P_Game->playersInfo.playerType[color] < 0)
                continue;
            const auto &saved = lastGame.players[color];
            const auto mask = P_Game->mapInfo.playerAttributes[color].availableFactionsBitset;
            if (p->town == -1 && saved.town >= 0 && (mask & (1u << saved.town)) &&
                (saved.town != 8 || dlg->gameVersion != 0))
                p->town = saved.town;
            if (p->bonusType == 3)
                p->bonusType = saved.bonus;
        }
    }
    int previousHeroes[PLAYER_COUNT];
    for (int color = 0; color < PLAYER_COUNT; ++color)
        previousHeroes[color] = SelectedHero(*Player(dlg, color));
    if (IsNewGame(dlg))
        for (int color = 0; color < PLAYER_COUNT; ++color)
            Player(dlg, color)->heroesCount = 0;
    candidateDialog = dlg;
    for (auto &list : candidateLists)
        list.count = 0;
    collectingHeroes = IsNewGame(dlg);
    THISCALL_1(void, h->GetDefaultFunc(), dlg);
    collectingHeroes = false;
    if (IsNewGame(dlg))
    {
        for (int color = 0; color < PLAYER_COUNT; ++color)
            Player(dlg, color)->selectedHeroIndex = -1;
        for (int color = 0; color < PLAYER_COUNT; ++color)
        {
            ChooseHero(dlg, color, previousHeroes[color]);
        }
    }
    if (restore)
    {
        restorePlayers = false;
        // The random-map header and player slots are already initialized by 0x580700.
        // Opening Advanced Options must not reset the restored town/hero selection again.
        dlg->newMapSelected = false;
        for (int color = 0; color < PLAYER_COUNT; ++color)
        {
            if (P_Game->playersInfo.playerType[color] < 0)
                continue;
            auto *p = Player(dlg, color);
            const int hero = lastGame.players[color].hero;
            if (p->town < 0 || p->selectedHeroIndex != -1 || hero < 0 || HeroTaken(dlg, p, hero))
                continue;
            ChooseHero(dlg, color, hero);
        }
    }
}

BOOL8 __stdcall CanChooseHero(HiHook *h, H3SelectScenarioDialog *dlg, int color)
{
    if (CanChooseComputerHero(dlg, color))
        return true;
    return THISCALL_2(BOOL8, h->GetDefaultFunc(), dlg, color);
}

void __stdcall ScrollHero(HiHook *h, H3SelectScenarioDialog *dlg, int step, H3ScenarioPlayer *p)
{
    if (!IsNewGame(dlg))
    {
        THISCALL_3(void, h->GetDefaultFunc(), dlg, step, p);
        return;
    }
    const auto &list = Candidates(dlg, p);
    const int count = list.count;
    if (count < 1)
    {
        p->selectedHeroIndex = -1;
        return;
    }
    int i = -1;
    const int selected = SelectedHero(*p);
    for (int j = 0; j < count; ++j)
        if (list.heroes[j] == selected)
            i = j;
    for (int attempts = 0; attempts <= count; ++attempts)
    {
        i += step < 0 ? -1 : 1;
        if (i < -1)
            i = count - 1;
        if (i >= count)
            i = -1;
        if (i == -1 || !HeroTaken(dlg, p, list.heroes[i]))
        {
            int color = 0;
            for (; color < PLAYER_COUNT && Player(dlg, color) != p; ++color)
            {
            }
            if (color < PLAYER_COUNT)
                ChooseHero(dlg, color, i == -1 ? -1 : list.heroes[i]);
            BroadcastPlayerSelection(dlg);
            return;
        }
    }
    p->selectedHeroIndex = -1;
}

int __stdcall LeftClick(HiHook *h, H3SelectScenarioDialog *dlg, H3Msg *msg, int *result, BOOL8 remote)
{
    if (msg->itemId >= 231 && msg->itemId <= 246)
    {
        const int color = (msg->itemId - 231) % PLAYER_COUNT;
        // A remote 1043 click must not let a guest select an AI hero through the host.
        if (IntAt(0x69959C) && !HumanPlayer(dlg, color) && (remote || !IsSelectionAuthority()))
            return 0;
        if (CanChooseComputerHero(dlg, color))
        {
            THISCALL_3(void, 0x58B1F0, dlg, msg->itemId < 239 ? -1 : 1, Player(dlg, color));
            THISCALL_4(void, 0x58D7E0, dlg, color, true, -1);
            return 0;
        }
    }
    return THISCALL_4(int, h->GetDefaultFunc(), dlg, msg, result, remote);
}

BOOL8 __stdcall StartGame(HiHook *h, H3SelectScenarioDialog *dlg)
{
    const bool random = IsRandomMap(dlg);
    const bool remember = random && RememberSettings();
    const auto settings = remember ? Capture(dlg) : LastGame{};
    const bool previousStart = startingLocalGame;
    startingLocalGame = IsNewGame(dlg) && IsSelectionAuthority();
    const auto result = THISCALL_1(BOOL8, h->GetDefaultFunc(), dlg);
    startingLocalGame = previousStart;
    if (remember && result)
        SaveLastGame(settings);
    return result;
}

// Native StartNewGame builds startingHeroOfPlayer from human slots only.
// This site follows that loop and precedes LoadScenario; EBX still holds the dialog.
_LHF_(StartingComputerHeroes)
{
    auto *dlg = reinterpret_cast<H3SelectScenarioDialog *>(c->ebx);
    if (IsNewGame(dlg) && IsSelectionAuthority())
        for (int color = 0; color < PLAYER_COUNT; ++color)
            if (!HumanPlayer(dlg, color) && P_Game->playersInfo.playerType[color] >= 0)
                IntAt(0x69FB80 + color * sizeof(int)) =
                    CanChooseComputerHero(dlg, color) ? SelectedHero(*Player(dlg, color)) : -1;
    return EXEC_DEFAULT;
}

// GameMgr::StartNewGame (0x4BFB30) reserves selected heroes before loading
// map objects. Native code gates this on player.isHuman. Keep its remaining
// checks and writes (heroOwner + playersInfo.heroMaybe), including for AI.
_LHF_(ReserveComputerStartingHero)
{
    if (startingLocalGame)
    {
        c->return_address = 0x4BFC2C;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

// LoadMap_CreateTownRandHeroes (0x4C9D00) also ignores the requested array
// for AI. The non-null array check at 0x4C9DFF has already succeeded here.
// Resume at the native array read; -1 still takes the native random path.
_LHF_(UseComputerStartingHero)
{
    if (startingLocalGame)
    {
        c->return_address = 0x4C9E0B;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

} // namespace mapselection

void RandomMapOptions_Init()
{
    using namespace mapselection;
    const BYTE handicapBranch[] = {0x83, 0x3D, 0x40, 0x8A, 0x69, 0x00, 0x03, 0x75, 0x4B};
    if (!std::memcmp(reinterpret_cast<const void *>(0x57B406), handicapBranch, sizeof(handicapBranch)))
        _PI->WriteLoHook(0x57B406, CreateHandicapTextButton);
    else
        Era::WriteLog("RMG", "Map selection", "Handicap button constructor differs; native button was left unchanged.");
    _PI->WriteLoHook(0x583D92, CollectStartingHero);
    _PI->WriteHiHook(0x579CE0, SPLICE_, EXTENDED_, THISCALL_, ScenarioCtor);
    _PI->WriteHiHook(0x580180, SPLICE_, EXTENDED_, THISCALL_, ShowRandomSettings);
    _PI->WriteLoHook(0x581C4F, ShowPlayerSettings);
    _PI->WriteLoHook(0x582039, HidePlayerSettings);
    _PI->WriteLoHook(0x583BCA, FillComputerHeroes);
    _PI->WriteHiHook(0x583B60, SPLICE_, EXTENDED_, THISCALL_, FillHeroes);
    _PI->WriteHiHook(0x58D280, SPLICE_, EXTENDED_, THISCALL_, CanChooseHero);
    _PI->WriteHiHook(0x58B1F0, SPLICE_, EXTENDED_, THISCALL_, ScrollHero);
    _PI->WriteHiHook(0x586880, SPLICE_, EXTENDED_, THISCALL_, LeftClick);
    _PI->WriteHiHook(0x58C840, SPLICE_, EXTENDED_, THISCALL_, StartGame);
    _PI->WriteLoHook(0x58C987, StartingComputerHeroes);
    _PI->WriteLoHook(0x4BFC27, ReserveComputerStartingHero);
    _PI->WriteLoHook(0x4C9E03, UseComputerStartingHero);
    InstallPlayerDialog();
}
