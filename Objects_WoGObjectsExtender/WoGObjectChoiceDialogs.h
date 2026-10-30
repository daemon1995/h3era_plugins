#pragma once
#include "framework.h"
#include <array>
#include <string>
#include <vector>
#include <functional>

namespace wog { namespace handlers {
struct SkullTarget
{
    int type = -1, value = -1;
};
SkullTarget ChooseSkullTarget();
void ShowSkullSearch(const std::function<void(const SkullTarget &)> &search);
struct SkullService
{
    std::string text;
    int artifact = -1, spell = -1;
    std::vector<std::string> actions;
};
void ShowSkullService(SkullService service, const std::function<void(int, SkullService &)> &action);
bool ConfirmDwellingOffer(const std::vector<std::pair<int, int>> &stacks, int gold);
int ChoosePalaceGame(bool artifactGameAvailable);
bool ShowPalaceSpin(bool artifacts, const std::array<int, 3> &reels, const std::string &message, bool canRepeat);

struct JunkOffer
{
    int artifact = -1, subtype = -1, count = 0;
    bool available = false;
    std::string reward;
    int rewardIcon = 0, rewardFrame = 0; // 1 artifact, 2 resource, 3 creature, 4 primary, 5 spell, 6 mana, 7 scouting.
};
void ShowJunkMerchant(std::vector<JunkOffer> offers,
                      const std::function<void(std::vector<JunkOffer> &)> &refresh,
                      const std::function<bool(const JunkOffer &)> &trade);

struct SummoningOffer
{
    int creature = -1, count = 0;
    std::string town, price;
};
void ShowSummoningOffers(const std::vector<SummoningOffer> &offers, const std::function<bool(int)> &recruit);

struct ArcaneTrade
{
    int paid = -1, wanted = -1, amount = 0;
};
ArcaneTrade ChooseArcaneTrade(H3Hero *hero);

struct AltarRow
{
    int source = -1, count = 0;
    bool eligible = false;
    std::array<int, 9> targets{};
};
struct AltarChoice
{
    int mana = 0, town = -2;
    bool accepted = false;
};
AltarChoice ChooseAltarTransformation(H3Hero *hero, const std::array<AltarRow, 7> &rows);
} }
