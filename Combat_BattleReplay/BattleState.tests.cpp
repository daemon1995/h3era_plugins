// Standalone regression checks; never linked into the game plugin.
#include "BattleState.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <new>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct Army
{
    int type[7]{};
    int count[7]{};
};

struct Hero
{
    Army army;
    int mana = 30;
    std::string biography = "original biography";
};

static Army MakeArmy(int count)
{
    Army army;
    army.type[0] = 42;
    army.count[0] = count;
    return army;
}

static void SeparateNetworkArmy()
{
    Hero hero;
    hero.army = MakeArmy(91);
    Army networkArmy = MakeArmy(47);
    const battleReplay::SideSnapshot<Hero, Army> state(&hero, &networkArmy);
    for (int attempt = 0; attempt < 3; ++attempt)
    {
        networkArmy = Army{}; // Losing attempt empties the actual combat army.
        hero.army = Army{};
        hero.mana = 0;
        hero.biography.assign(2000, 'x');
        state.Restore();
        assert(networkArmy.count[0] == 47);
        assert(networkArmy.type[0] == 42);
        assert(hero.army.count[0] == 91);
        assert(hero.mana == 30);
        assert(hero.biography == "original biography");
    }
    // Accepting a final attempt must preserve its losses, with no rollback on destruction.
    networkArmy.count[0] = 11;
}

static void AliasedHeroArmy()
{
    Hero hero;
    hero.army = MakeArmy(65);
    const battleReplay::SideSnapshot<Hero, Army> state(&hero, &hero.army);
    hero.army = Army{};
    state.Restore();
    assert(hero.army.count[0] == 65);
}

static void TownAndFieldArmies()
{
    Hero attacker, defender;
    Army attackerArmy = MakeArmy(100), defenderArmy = MakeArmy(40), townGuards = MakeArmy(12);
    const battleReplay::SideSnapshot<Hero, Army> attackerState(&attacker, &attackerArmy);
    const battleReplay::SideSnapshot<Hero, Army> defenderState(&defender, &defenderArmy);
    const battleReplay::Snapshot<Army> townState(&townGuards);
    attackerArmy = defenderArmy = townGuards = Army{};
    attackerState.Restore();
    defenderState.Restore();
    townState.Restore();
    assert(attackerArmy.count[0] == 100);
    assert(defenderArmy.count[0] == 40);
    assert(townGuards.count[0] == 12);
}

static void NoHeroAndNullObjects()
{
    Army neutral = MakeArmy(50);
    const battleReplay::SideSnapshot<Hero, Army> state(nullptr, &neutral);
    neutral = Army{};
    state.Restore();
    assert(neutral.count[0] == 50);
    const battleReplay::SideSnapshot<Hero, Army> empty(nullptr, nullptr);
    empty.Restore();
}

static void AcceptedBattleIsNotRestored()
{
    Hero hero;
    Army army = MakeArmy(10);
    {
        const battleReplay::SideSnapshot<Hero, Army> state(&hero, &army);
        army.count[0] = 3;
    }
    assert(army.count[0] == 3);
}

static void Eligibility()
{
    assert(!battleReplay::IsPlayer(-1));
    assert(battleReplay::IsPlayer(0));
    assert(battleReplay::IsPlayer(7));
    assert(!battleReplay::IsPlayer(8));
    for (int network = 0; network < 2; ++network)
        for (int attacker = 0; attacker < 2; ++attacker)
            for (int defender = 0; defender < 2; ++defender)
            {
                const bool allowed = battleReplay::CanReplay(network != 0, attacker != 0, defender != 0);
                if (!attacker && !defender)
                    assert(!allowed);
                else if (network && attacker && defender)
                    assert(!allowed);
                else
                    assert(allowed);
            }
}

struct HdVariable
{
    unsigned int value = 0;
    unsigned int GetValue() const { return value; }
    void SetValue(unsigned int newValue) { value = newValue; }
};

static void LoadInvalidatesAbandonedContext()
{
    battleReplay::ReplayRuntime runtime;
    battleReplay::ReplayContext oldContext, nestedContext, newContext;
    using Scope = battleReplay::ReplayRuntime::Scope;
    alignas(Scope) char oldScopeStorage[sizeof(Scope)];
    auto oldScope = new (oldScopeStorage) Scope(runtime, oldContext);
    oldContext.allowed = true;
    oldContext.requested = true;
    {
        Scope nested(runtime, nestedContext);
        assert(runtime.Current() == &nestedContext);
    }
    assert(runtime.Current() == &oldContext);
    // ERA abandons the old native frame. The reset must work without its dtor.
    runtime.Reset();
    assert(runtime.Current() == nullptr && !oldScope->IsCurrent());
    {
        Scope loadedBattle(runtime, newContext);
        assert(loadedBattle.IsCurrent());
        oldScope->~Scope(); // Late unwind must not detach the loaded battle.
        assert(runtime.Current() == &newContext);
    }
    assert(runtime.Current() == nullptr);
}

static void QuickCombatSurvivesLoading()
{
    battleReplay::ReplayRuntime runtime;
    battleReplay::QuickBattleBackup<HdVariable> backup;
    int gameValue = 1;
    HdVariable hd;
    hd.value = 7; // Game and HD values deliberately differ.
    using Scope = battleReplay::QuickBattleBackup<HdVariable>::Scope;
    alignas(Scope) char oldScopeStorage[sizeof(Scope)];
    auto oldScope = new (oldScopeStorage) Scope(runtime, backup, gameValue, &hd);
    oldScope->Disable();
    assert(gameValue == 0 && hd.value == 0);
    runtime.Reset();
    backup.Restore(gameValue); // Lifecycle handler runs before loading.
    assert(gameValue == 1 && hd.value == 7);
    // Loading replaces settings; the abandoned frame must never overwrite them.
    gameValue = 3;
    hd.value = 9;
    backup.Restore(gameValue); // Repeated OnGameLeft/OnBeforeLoad resets are harmless.
    assert(gameValue == 3 && hd.value == 9);
    {
        Scope loadedBattle(runtime, backup, gameValue, &hd);
        loadedBattle.Disable();
        oldScope->Disable();
        oldScope->~Scope();
        assert(gameValue == 0 && hd.value == 0);
    }
    assert(gameValue == 3 && hd.value == 9);
    {
        Scope noHd(runtime, backup, gameValue, nullptr);
        noHd.Disable();
        assert(gameValue == 0);
    }
    assert(gameValue == 3);
}

constexpr DWORD FAST_QUIT_TEST_EXCEPTION = 0xE0421377;
static void RaiseFastQuit(battleReplay::ReplayRuntime &runtime, int &destroyed)
{
    struct DestructionProbe
    {
        int &count;
        ~DestructionProbe() { ++count; }
    } probe{destroyed};
    battleReplay::ReplayContext context;
    battleReplay::ReplayRuntime::Scope scope(runtime, context);
    RaiseException(FAST_QUIT_TEST_EXCEPTION, 0, 0, nullptr);
}

static bool CatchFastQuit(battleReplay::ReplayRuntime &runtime, int &destroyed)
{
    __try
    {
        RaiseFastQuit(runtime, destroyed);
    }
    __except (GetExceptionCode() == FAST_QUIT_TEST_EXCEPTION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return true;
    }
    return false;
}

static void SehUnwindsOwnedSnapshots()
{
    battleReplay::ReplayRuntime runtime;
    int destroyed = 0;
    assert(CatchFastQuit(runtime, destroyed));
    assert(destroyed == 1 && runtime.Current() == nullptr);
}

int main()
{
    SeparateNetworkArmy();
    AliasedHeroArmy();
    TownAndFieldArmies();
    NoHeroAndNullObjects();
    AcceptedBattleIsNotRestored();
    Eligibility();
    LoadInvalidatesAbandonedContext();
    QuickCombatSurvivesLoading();
    SehUnwindsOwnedSnapshots();
    std::puts("BattleReplay regression checks passed (9 groups).");
}
