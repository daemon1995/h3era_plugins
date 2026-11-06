#include "pch.h"
#include "TownUpgrades.h"
#include "TownUpgradeLogic.h"
#include <limits>

namespace townupgrades
{
namespace
{
constexpr LPCSTR OPTION = "gem_town_upgrade";
bool installed = false;

LPCSTR Text(LPCSTR key)
{
    return EraJS::read(H3String::Format("gem_plugin.town_upgrade.%s", key).String());
}

bool Enabled()
{
    return installed && Era::GetAssocVarIntValue(OPTION) != 0;
}

H3Player *ActivePlayer()
{
    auto game = P_Game->Get();
    auto player = game ? game->GetPlayer() : nullptr;
    return player && player == P_ActivePlayer->Get() ? player : nullptr;
}

Cost Resources(const H3Resources &resources)
{
    Cost result{};
    for (int i = 0; i < 7; ++i)
        result[i] = resources.asArray[i];
    return result;
}

H3Army *Army(H3Town *town, H3Player *player, int army)
{
    if (!town || !player)
        return nullptr;
    auto game = P_Game->Get();
    if (army == 0)
    {
        if (town->owner != player->ownerID)
            return nullptr;
        auto hero = game->GetHero(town->garrisonHero);
        return hero ? (hero->owner == player->ownerID ? &hero->army : nullptr) : &town->guards;
    }
    auto hero = game->GetHero(town->visitingHero);
    return hero && hero->owner == player->ownerID ? &hero->army : nullptr;
}

bool Describe(H3Army *army, H3Town *town, int slot, bool individual, Upgrade &upgrade)
{
    if (!army || !town || slot < 0 || slot >= 7)
        return false;
    const int type = army->type[slot];
    const int count = army->count[slot];
    const int creatureCount = H3CreatureCount::Get();
    if (type < 0 || type >= creatureCount || count <= 0)
        return false;
    const bool universal = IntAt(0x2771920 + 174 * 4) != 0;
    const int upgraded = FASTCALL_1(int, universal ? 0x74ECD3 : 0x47AAD0, type);
    if (upgraded < 0 || upgraded >= creatureCount)
        return false;
    const auto &creature = P_Creatures[type];
    const bool dwelling = creature.level >= 0 && creature.level < 7 &&
                          THISCALL_3(int, 0x4305A0, town, 37 + creature.level, 1) != 0;
    if (!Eligible({type, upgraded, count, creature.level, creature.town, dwelling}, town->type, universal, individual))
        return false;
    // Use the native routine (including ERA hooks), not H3API's unclamped cost difference.
    // Reject overflowing prices before invoking the original 32-bit multiplication.
    for (int i = 0; i < 7; ++i)
    {
        const auto difference = static_cast<std::int64_t>(P_Creatures[upgraded].cost.asArray[i]) - creature.cost.asArray[i];
        if (difference > 0 && difference * count > (std::numeric_limits<int>::max)())
            return false;
    }
    H3Resources cost;
    FASTCALL_4(void, 0x54EB50, type, upgraded, count, &cost);
    upgrade.slot = slot;
    upgrade.from = type;
    upgrade.to = upgraded;
    upgrade.count = count;
    upgrade.cost = Resources(cost);
    for (int value : upgrade.cost)
        if (value < 0)
            return false;
    return true;
}

Plan TownPlan(H3Town *town, H3Player *player, int mask, bool individual = false)
{
    return MakePlan(mask, Resources(player->playerResources), [&](Upgrade &upgrade) {
        return Describe(Army(town, player, upgrade.army), town, upgrade.slot, individual, upgrade);
    });
}

// WoG's setter is reproduced from igrik's WoG_CrExpo.cpp. Existing creature
// artifacts are retained; calling 0x718AD0 unconditionally would clear them.
void SetExperience(int locationType, int location, const Upgrade &upgrade, int experience)
{
    const int index = CDECL_2(int, 0x718579, locationType, location);
    if (index < 0)
    {
        CDECL_5(void, 0x718AD0, locationType, location, upgrade.to, upgrade.count, experience);
        return;
    }
    const auto entry = DwordAt(0x718B34) + 16 * index;
    IntAt(entry) = experience;
    IntAt(entry + 4) = upgrade.count;
    DwordAt(entry + 8) = (DwordAt(entry + 8) & 0xFFFFE01F) | ((static_cast<BYTE>(upgrade.to) << 5) & 0x1FE0);
    THISCALL_1(void, 0x7176A1, 0x860550 + 16 * index);
}

bool Apply(H3Army *army, H3Town *town, H3Player *player, const Upgrade &expected, bool individual)
{
    Upgrade current;
    if (!Describe(army, town, expected.slot, individual, current) || current.from != expected.from ||
        current.to != expected.to || current.count != expected.count || current.cost != expected.cost)
        return false;
    Cost budget = Resources(player->playerResources);
    if (!TryPay(budget, current.cost))
        return false;
    int locationType = 0;
    int location = 0;
    CDECL_4(void, 0x71A1B7, army, current.slot, &locationType, &location);
    const int experience = CDECL_2(int, 0x718CCD, locationType, location);
    for (int i = 0; i < 7; ++i)
        player->playerResources.asArray[i] = budget[i];
    army->type[current.slot] = current.to;
    if (experience > 0)
        SetExperience(locationType, location, current, UpgradedExperience(experience));
    return true;
}

int ApplyPlan(H3Town *town, H3Player *player, const Plan &plan, bool individual)
{
    int changed = 0;
    for (int i = 0; i < plan.size; ++i)
    {
        const auto &upgrade = plan.stacks[i];
        changed += Apply(Army(town, player, upgrade.army), town, player, upgrade, individual);
    }
    return changed;
}

H3String CostText(const Cost &cost)
{
    H3String text;
    for (int i = 6; i >= 0; --i)
        if (cost[i] > 0)
            text += H3String::Format(Text("resource"), cost[i], i);
    if (text.Empty())
        text = Text("free");
    return text;
}

void Redraw(H3TownManager *manager)
{
    manager->Draw(); // H3API name for TownMgr::UnHighlightArmy (0x5D5930).
    manager->RefreshScreen();
}

H3TownManager *CurrentTownManager()
{
    auto manager = P_TownMgr->Get();
    auto windows = P_WindowManager->Get();
    return manager && manager->town && manager->dlg && windows && windows->lastDlg == manager->dlg ? manager : nullptr;
}

} // namespace

void Init()
{
    if (installed)
        return;
    // Only the ERM script should request upgrades; disable the legacy A+click hook.
    if (auto legacy = globalPatcher->GetInstance("AutoGradeMonInTown"))
        legacy->UndoAll();
    installed = true;
}

H3Town *FindTown(int townId)
{
    if (townId == -1)
    {
        auto manager = CurrentTownManager();
        return manager ? manager->town : nullptr;
    }
    auto game = P_Game->Get();
    return game && townId >= 0 && townId < static_cast<int>(game->towns.Size()) ? &game->towns[townId] : nullptr;
}

int Execute(H3Town *town, H3Player *player, const Plan &plan, bool individual, bool confirm)
{
    if (!plan.size)
    {
        if (confirm)
            H3Messagebox::Show(Text("nothing_to_upgrade"));
        return 0;
    }
    if (confirm)
    {
        const auto cost = CostText(plan.cost);
        const auto question = H3String::Format(Text("confirmation"), plan.size, cost.String());
        if (!H3Messagebox::Choice(question))
            return 0;
        // Scripts can run in the modal dialog. Keep the approved plan and recheck context.
        auto manager = CurrentTownManager();
        if (!Enabled() || ActivePlayer() != player || !manager || manager->town != town)
            return 0;
    }
    const int changed = ApplyPlan(town, player, plan, individual);
    auto manager = CurrentTownManager();
    if (changed && manager && manager->town == town)
        Redraw(manager);
    return changed;
}

int UpgradeTownSelection(int townId, int armyMask, int slot, bool confirm)
{
    auto player = ActivePlayer();
    auto town = FindTown(townId);
    if (!Enabled() || !player || !town || !ValidSelection(armyMask, slot) || (confirm && slot != -1))
        return -1;
    Plan plan;
    if (slot == -1)
        plan = TownPlan(town, player, armyMask);
    else
    {
        auto &upgrade = plan.stacks[0];
        upgrade.army = armyMask == 1 ? 0 : 1;
        if (Describe(Army(town, player, upgrade.army), town, slot, true, upgrade))
            plan.size = 1;
    }
    return Execute(town, player, plan, slot >= 0, confirm);
}

int UpgradeTown(int townId, int armyMask, bool upgradeSkeletons)
{
    auto player = ActivePlayer();
    auto town = FindTown(townId);
    if (!Enabled() || !player || !town || armyMask < 1 || armyMask > 3)
        return -1;
    return Execute(town, player, TownPlan(town, player, armyMask, upgradeSkeletons), upgradeSkeletons, false);
}
} // namespace townupgrades

extern "C" int __stdcall GEM_UpgradeTownCreatures(int townId, int armyMask, int upgradeSkeletons)
{
    return townupgrades::UpgradeTown(townId, armyMask, upgradeSkeletons != 0);
}

extern "C" int __stdcall GEM_UpgradeTownSelection(int townId, int armyMask, int slot, int confirm)
{
    return townupgrades::UpgradeTownSelection(townId, armyMask, slot, confirm != 0);
}
