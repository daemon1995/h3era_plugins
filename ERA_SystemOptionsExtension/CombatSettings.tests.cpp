// Run the production battle logic against isolated game data. No game
// process, dialogs, registry writer, or fixed-address game function is invoked.
#include "CombatSettings.cpp"
#include <climits>
#include <cstdio>
#include <cstdlib>

Patcher *globalPatcher = nullptr;
namespace Era
{
int testV[2] = {};
int testY[2] = {};
TErmZVar testZ[5] = {};
int *v = testV;
int *y = testY;
TErmZVar *z = testZ;
decltype(ExecErmCmd) ExecErmCmd = nullptr;
decltype(RegisterHandler) RegisterHandler = nullptr;
decltype(FastQuitToGameMenu) FastQuitToGameMenu = nullptr;
static_str tr(const char *key)
{
    return const_cast<char *>(key);
}
} // namespace Era

#define CHECK(condition)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            std::fprintf(stderr, "Line %d: %s\n", __LINE__, #condition);                                               \
            std::exit(1);                                                                                              \
        }                                                                                                              \
    } while (0)

OriginalConfig testConfig = {};
H3Main *testMain = nullptr;
int attackerPlayerId = -1;
int defenderPlayerId = -1;
bool autoSolo = false;
bool networkGame = false;

struct CombatTestAccess : cmbsttngs::CombatSettings
{
    using CombatSettings::AskQuickCombatType;
    static void BeforeBattle()
    {
        ConfigureBattle(testConfig, testMain, attackerPlayerId, defenderPlayerId, autoSolo, networkGame);
    }
    static void AfterBattle()
    {
        RestoreQuickCombatOptions(testConfig);
    }
    static void ContextChanged()
    {
        ResetGameContext(testConfig);
    }
};

int menuResult = 1;
int menuCalls = 0;
int expectedMenuInitialMask = 0;
bool menuLoadsGame = false;
bool menuThrows = false;
void __stdcall TestMenu(const char *command)
{
    CHECK(!std::strcmp(command, "IF:G1/1/y1/1/2/3/4"));
    CHECK(Era::v[1] == 0);
    CHECK(Era::y[1] == 1 || Era::y[1] == 2 || Era::y[1] == 4);
    if (expectedMenuInitialMask)
        CHECK(Era::y[1] == expectedMenuInitialMask);
    ++menuCalls;
    Era::v[1] = menuResult;
    if (menuLoadsGame)
    {
        CombatTestAccess::ContextChanged();
        Era::v[1] = 900;
        Era::y[1] = 901;
        std::strcpy(Era::z[1], "loaded game");
    }
    if (menuThrows)
        RaiseException(0xE0000001, 0, 0, nullptr);
}

void TestState()
{
    using cmbsttngs::QuickCombatState;
    for (int quick = 0; quick < 2; ++quick)
        for (int spells = 0; spells < 2; ++spells)
            for (int battleQuick = 0; battleQuick < 2; ++battleQuick)
                for (int battleSpells = 0; battleSpells < 2; ++battleSpells)
                {
                    QuickCombatState state;
                    int q = quick, s = spells;
                    state.SetQuickCombat(q, battleQuick);
                    state.SetAutoSpells(s, battleSpells);
                    CHECK(state.PersistentQuickCombat(q) == quick);
                    CHECK(state.PersistentAutoSpells(s) == spells);
                    state.FinishInstantly(q, s);
                    state.FinishInstantly(q, s); // Repeated Q/A keeps the first backup.
                    CHECK(q == 1 && s == 0);
                    state.BeginSettingsWrite(q, s);
                    CHECK(q == quick && s == spells);
                    state.BeginSettingsWrite(q, s);
                    state.EndSettingsWrite(q, s);
                    CHECK(q == quick && s == spells);
                    state.EndSettingsWrite(q, s);
                    CHECK(q == 1 && s == 0);
                    state.SetPersistentQuickCombat(q, !quick);
                    state.SetPersistentAutoSpells(s, !spells);
                    state.RestoreInstantFinish(q, s);
                    CHECK(q == battleQuick && s == battleSpells);
                    CHECK(state.PersistentQuickCombat(q) == !quick);
                    CHECK(state.PersistentAutoSpells(s) == !spells);
                    state.Restore(q, s);
                    CHECK(q == !quick && s == !spells);
                    state.Restore(q, s);
                    CHECK(q == !quick && s == !spells);
                }

    QuickCombatState state;
    int q = 0, s = 1;
    state.SetQuickCombat(q, 0); // Manual battle reserves only quick combat.
    s = 0;                      // An untouched setting changed in the battle dialog must survive.
    state.FinishInstantly(q, s);
    state.SetPersistentAutoSpells(s, 1);
    state.RestoreInstantFinish(q, s);
    CHECK(q == 0 && s == 1);
    state.Restore(q, s);
    CHECK(q == 0 && s == 1);

    state.FinishInstantly(q, s);
    state.BeginSettingsWrite(q, s);
    state.SetPersistentQuickCombat(q, 1);
    state.SetPersistentAutoSpells(s, 0);
    CHECK(q == 1 && s == 0);
    state.Restore(q, s); // Loading while a settings writer is on the stack.
    q = 0;
    s = 1;
    state.EndSettingsWrite(q, s);
    CHECK(q == 0 && s == 1); // The old writer cannot resurrect its overrides.

    state.BeginSettingsWrite(q, s);
    state.SetQuickCombat(q, 1);
    state.SetAutoSpells(s, 0);
    CHECK(q == 0 && s == 1);
    state.EndSettingsWrite(q, s);
    CHECK(q == 1 && s == 0);
    state.Restore(q, s);
    CHECK(q == 0 && s == 1);
}

void TestMenuState()
{
    Era::v[1] = 12345;
    Era::y[1] = 54321;
    for (int i = 1; i < 5; ++i)
        std::memset(Era::z[i], 'a' + i, sizeof(Era::TErmZVar));
    Era::TErmZVar before[5];
    std::memcpy(before, Era::z, sizeof(before));
    const struct
    {
        int mask;
        int selectedIndex;
        int initialMask;
    } results[] = {{1, 0, 1}, {2, 1, 1}, {4, 2, 2}, {1, 0, 4}, {4, 2, 1}, {0, 0, 4},      {-1, 0, 4},
                   {3, 0, 4}, {5, 0, 4}, {6, 0, 4}, {7, 0, 4}, {8, 0, 4}, {INT_MAX, 0, 4}};
    for (const auto &result : results)
    {
        menuResult = result.mask;
        expectedMenuInitialMask = result.initialMask;
        CHECK(CombatTestAccess::AskQuickCombatType() == result.selectedIndex);
        CHECK(Era::v[1] == 12345 && Era::y[1] == 54321);
        CHECK(!std::memcmp(before, Era::z, sizeof(before)));
    }
    expectedMenuInitialMask = 0;
    menuThrows = true;
    bool caught = false;
    __try
    {
        CombatTestAccess::AskQuickCombatType();
    }
    __except (GetExceptionCode() == 0xE0000001 ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        caught = true;
    }
    menuThrows = false;
    CHECK(caught && Era::v[1] == 12345 && Era::y[1] == 54321);
    CHECK(!std::memcmp(before, Era::z, sizeof(before)));

    menuLoadsGame = true;
    CHECK(CombatTestAccess::AskQuickCombatType() == -1);
    menuLoadsGame = false;
    CHECK(Era::v[1] == 900 && Era::y[1] == 901);
    CHECK(!std::strcmp(Era::z[1], "loaded game"));
}

void TestBattleSelection(H3Main *main)
{
    auto &config = testConfig;
    auto &mode = AdditionalConfig::Get().quickCombatType;
    for (int type = 0; type <= 3; ++type)
        for (int defender = 0; defender < 2; ++defender)
            for (int selected = 1; selected <= (type == 3 ? 3 : 1); ++selected)
            {
                CombatTestAccess::AfterBattle();
                std::memset(main->players, 0, sizeof(main->players));
                attackerPlayerId = defender ? 1 : 0;
                defenderPlayerId = defender ? 0 : -1;
                main->players[0].isHuman = true;
                main->players[0].isLocal = true;
                mode.value = type;
                config.quickCombat = type == 0; // Deliberately stale/opposite native flag.
                config.autoSpells = 1;
                menuResult = 1 << (selected - 1);
                menuCalls = 0;
                CombatTestAccess::BeforeBattle();
                const int effective = type == 3 ? selected - 1 : type;
                CHECK(config.quickCombat == (effective != 0));
                CHECK(config.autoSpells == (effective == 2 ? 0 : 1));
                CHECK(CombatTestAccess::PersistentQuickCombat() == (type != 0));
                CHECK(mode.value == type && menuCalls == (type == 3));
                CombatTestAccess::AfterBattle();
                CHECK(config.quickCombat == (type != 0) && config.autoSpells == 1);
                CHECK(mode.value == type);
            }

    mode.value = 3;
    menuCalls = 0;
    main->players[0].isHuman = main->players[1].isHuman = true;
    attackerPlayerId = 1;
    defenderPlayerId = 0;
    CombatTestAccess::BeforeBattle();
    CHECK(config.quickCombat == 0 && menuCalls == 0); // Human vs human.
    CombatTestAccess::AfterBattle();
    main->players[1].isHuman = false;
    main->players[0].isLocal = false;
    networkGame = true;
    CombatTestAccess::BeforeBattle();
    CHECK(menuCalls == 0); // Remote human must not see this PC's question.
    main->players[0].isLocal = true;
    menuResult = 1;
    CombatTestAccess::BeforeBattle();
    CHECK(menuCalls == 1 && config.quickCombat == 0 && mode.value == 3);
    CombatTestAccess::AfterBattle();
    menuCalls = 0;
    networkGame = false;

    main->players[0].isLocal = true;
    main->players[0].isHuman = false;
    CombatTestAccess::BeforeBattle();
    CHECK(menuCalls == 0); // AI vs AI.
    main->players[0].isHuman = true;
    attackerPlayerId = -1;
    menuResult = 2;
    CombatTestAccess::BeforeBattle();
    CHECK(menuCalls == 1 && config.quickCombat == 1); // Human defender, no attacking hero.
    CombatTestAccess::AfterBattle();
    menuCalls = 0;

    const int invalidIds[] = {-128, -2, 8, 127};
    for (const int id : invalidIds)
    {
        attackerPlayerId = id;
        defenderPlayerId = 0;
        CombatTestAccess::BeforeBattle();
        attackerPlayerId = 0;
        defenderPlayerId = id;
        CombatTestAccess::BeforeBattle();
        CHECK(menuCalls == 0);
    }

    defenderPlayerId = -1;
    main->inTutorial = true;
    CombatTestAccess::BeforeBattle();
    CHECK(config.quickCombat == 0 && menuCalls == 0);
    main->inTutorial = false;
    CombatTestAccess::ContextChanged();
    CHECK(config.quickCombat == 1 && mode.value == 3);

    mode.value = 2;
    config.autoSpells = 1;
    CombatTestAccess::BeforeBattle();
    CHECK(config.autoSpells == 0);
    CombatTestAccess::ContextChanged();
    CHECK(config.autoSpells == 1 && config.quickCombat == 1 && mode.value == 2);
    config.quickCombat = 0; // A later native reload cannot redefine the configured mode.
    CHECK(CombatTestAccess::PersistentQuickCombat() == 1);
    CombatTestAccess::ContextChanged();
    CHECK(config.quickCombat == 1);

    mode.value = 0;
    autoSolo = true;
    CombatTestAccess::BeforeBattle();
    CHECK(config.quickCombat == 1 && CombatTestAccess::PersistentQuickCombat() == 0);
    autoSolo = false;
    CombatTestAccess::AfterBattle();
    CHECK(config.quickCombat == 0);
    testMain = nullptr;
    CombatTestAccess::BeforeBattle();
    CHECK(menuCalls == 0);
    testMain = main;
}

int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS *exception) -> LONG {
        std::fprintf(stderr, "Unhandled exception %08lX at %p\n", exception->ExceptionRecord->ExceptionCode,
                     exception->ExceptionRecord->ExceptionAddress);
        return EXCEPTION_EXECUTE_HANDLER;
    });
    TestState();
    auto main = static_cast<H3Main *>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(H3Main)));
    CHECK(main);
    testMain = main;
    Era::ExecErmCmd = TestMenu;
    TestMenuState();
    TestBattleSelection(main);
    HeapFree(GetProcessHeap(), 0, main);
    std::puts("Combat settings regression checks passed.");
}
