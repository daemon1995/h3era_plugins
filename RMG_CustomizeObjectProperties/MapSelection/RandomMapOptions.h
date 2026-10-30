#pragma once

void RandomMapOptions_Init();

namespace mapselection
{
constexpr int REMEMBER_SETTINGS_ID = 7900; // HD uses 7010..7012 for road icons.
void ToggleRememberSettings(h3::H3SelectScenarioDialog* dlg);

struct HeroCandidates
{
    int heroes[h3::limits::TOTAL_HEROES] = {};
    int count = 0;
};
bool IsNewGame(const h3::H3SelectScenarioDialog* dlg);
bool IsSelectionAuthority();
bool CanControlPlayer(h3::H3SelectScenarioDialog* dlg, int color);
h3::H3ScenarioPlayer* HumanPlayer(h3::H3SelectScenarioDialog* dlg, int color);
h3::H3ScenarioPlayer* Player(h3::H3SelectScenarioDialog* dlg, int color);
int PlayerTown(h3::H3SelectScenarioDialog* dlg, int color);
int SelectedHero(const h3::H3ScenarioPlayer& player);
bool HeroTaken(h3::H3SelectScenarioDialog* dlg, const h3::H3ScenarioPlayer* owner, int hero);
const HeroCandidates& Candidates(h3::H3SelectScenarioDialog* dlg, h3::H3ScenarioPlayer* player);
bool ChooseHero(h3::H3SelectScenarioDialog* dlg, int color, int hero);
void BroadcastPlayerSelection(h3::H3SelectScenarioDialog* dlg);
void ShowPlayerDialog(h3::H3SelectScenarioDialog* dlg, int color);
void RequestHandicap(h3::H3SelectScenarioDialog* dlg, int color);
void InstallPlayerDialog();
}
