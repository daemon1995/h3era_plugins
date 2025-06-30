// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "framework.h"
Patcher *globalPatcher;
PatcherInstance *_PI;
using namespace h3;

// H3String jsonKeys[12] = {"hasSpellBook",};

int heroStartList[8][MAX_HEROES_PER_TOWN]; // Castle town has 19 possible heroes, plus the random -1 value.
// h3::H3ScenarioPlayer* current_player = nullptr;
int current_player = 0;
int current_hero[8] = {};

void ChangeHeroStartingParameters(H3HeroInfo &heroInfo, int id)
{

    // std::string jsonKeyStart = "gem.%d";

    heroInfo.campaignHero = false;
    heroInfo.expansionHero = true;
    sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.hasSpellBook", id);
    std::string trResult = Era::tr(h3_TextBuffer);

    if (trResult != h3_TextBuffer)
        heroInfo.hasSpellbook = atoi(trResult.c_str());

    sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.startingSpell", id);
    trResult = Era::tr(h3_TextBuffer);

    if (trResult != h3_TextBuffer)
    {
        int spellId = atoi(trResult.c_str());
        spellId = Clamp(eSpell::NONE, spellId, eSpell::AIR_ELEMENTAL);
        heroInfo.startingSpell = spellId;
    }

    for (size_t i = 0; i < 2; i++)
    {
        sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.skill%d.type", id, i);
        trResult = Era::tr(h3_TextBuffer);
        if (trResult != h3_TextBuffer)
        {
            int skillId = atoi(trResult.c_str());
            skillId = Clamp(eSecondary::NONE, skillId, eSecondary::FIRST_AID);
            heroInfo.sskills[i].type = (eSecondary)skillId;
            if (skillId != eSecondary::NONE)
            {
                sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.skill%d.level", id, i);
                int skillLvl = eSecSkillLevel::BASIC;
                trResult = Era::tr(h3_TextBuffer);
                if (trResult != h3_TextBuffer)
                {
                    skillLvl = atoi(trResult.c_str());
                    skillLvl = Clamp(eSecSkillLevel::BASIC, skillLvl, eSecSkillLevel::EXPERT);
                }
                heroInfo.sskills[i].level = (eSecSkillLevel)skillLvl;
            }
        }
    }

    const int MAX_CREATURE_ID = IntAt(0x4A1657);

    for (size_t i = 0; i < 3; i++)
    {
        sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.army%d.type", id, i);
        trResult = Era::tr(h3_TextBuffer);
        if (trResult != h3_TextBuffer)
        {
            int armyType = atoi(trResult.c_str());
            armyType = Clamp(eCreature::UNDEFINED, armyType, MAX_CREATURE_ID);
            heroInfo.armyType[i] = armyType;
            if (armyType != eCreature::UNDEFINED)
            {
                sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.army%d.low", id, i);
                int low = heroInfo.creatureAmount[i].lowAmount;
                trResult = Era::tr(h3_TextBuffer);
                if (trResult != h3_TextBuffer)
                {
                    low = atoi(trResult.c_str());
                    low = Clamp(1, low, INT32_MAX);
                }
                heroInfo.creatureAmount[i].lowAmount = low;
                sprintf(h3_TextBuffer, "gem.heroesStartingParameters.%d.army%d.high", id, i);
                int high = heroInfo.creatureAmount[i].highAmount;
                trResult = Era::tr(h3_TextBuffer);
                if (trResult != h3_TextBuffer)
                {
                    high = atoi(trResult.c_str());
                    high = Clamp(1, high, INT32_MAX);
                }
                if (high < low)
                    high = low;
                heroInfo.creatureAmount[i].highAmount = high;
            }
        }
    }

    return;
}

typedef typename WoG::NPC NPC;
constexpr DWORD NPC_BASE_STATS_ADDRESSES[7] = {0x769B01, 0x769B0B, 0x769B33, 0x769B1F, 0x769B29, 0x769B15, 0x769B3D};

struct NPCStats
{
  private:
    int stats[7];

  public:
    NPCStats(const UINT id)
    {

        for (size_t i = 0; i < 7; i++)
        {
            stats[i] = IntAt(NPC_BASE_STATS_ADDRESSES[i]);
        }

        ReadJsonData(id);
    }

  public:
    void Assign(NPC *npc)
    {
        libc::memcpy(&(npc->parameters), stats, sizeof(stats));
    }

    void ReadJsonData(const UINT id)
    {
        bool readSucces = false;
        for (size_t i = 0; i < 7; i++)
        {
            sprintf(h3_TextBuffer, "gem.NPCStartingParameters.%d.%d", id, i);
            const int value = EraJS::readInt(h3_TextBuffer, readSucces);

            if (readSucces && value >= 0)
            {
                stats[i] = value;
            }
        }
    }

  public:
    static void SetDefault()
    {
        bool readSucces = false;

        for (size_t i = 0; i < 7; i++)
        {
            sprintf(h3_TextBuffer, "gem.NPCStartingParameters.-1.%d", i);
            const int value = EraJS::readInt(h3_TextBuffer, readSucces);

            if (readSucces && value >= 0)
            {
                _PI->WriteDword(NPC_BASE_STATS_ADDRESSES[i], value);
            }
        }
    }
};
std::vector<NPCStats> npcStatsVec;

NPC *__stdcall NPC__Init(HiHook *h, NPC *npc)
{

    auto result = THISCALL_1(NPC *, h->GetDefaultFunc(), npc); // get default function

    if (npc->id != eHero::RANDOM)
    {
        npcStatsVec.at(npc->id).Assign(npc);
    }

    return result;
}

enum eItemIds
{
    HERO_FIRST = 362,
    HERO_RED = HERO_FIRST,
    HERO_BLUE = 363,
    HERO_TAN = 364,
    HERO_GREEN = 365,
    HERO_ORANGE = 366,
    HERO_PURPLE = 367,
    HERO_TEAL = 368,
    HERO_PINK = 369,
    HERO_LAST = HERO_PINK,
    TOWN_FIRST = 370,
    TOWN_RED = TOWN_FIRST,
    TOWN_BLUE = 371,
    TOWN_TAN = 372,
    TOWN_GREEN = 373,
    TOWN_ORANGE = 374,
    TOWN_PURPLE = 375,
    TOWN_TEAL = 376,
    TOWN_PINK = 377,
    TOWN_LAST = TOWN_PINK
};
void StartPlayerDlg(H3SelectScenarioDialog* dlg, H3ScenarioPlayer* player, int playerId, int humanPLayerId)
{





}
int __stdcall H3SelectScenarioDialog__Proc(HiHook *h, H3SelectScenarioDialog *dlg, H3Msg *msg)
{

    //  int result = THISCALL_2(int, h->GetDefaultFunc(), dlg, msg);
    if (msg->itemId >= HERO_FIRST && msg->itemId <= TOWN_LAST && msg->command == h3::eMsgCommand::MOUSE_BUTTON)
    {
        const int itemId = msg->itemId;

        const bool isTownClick = (itemId >= TOWN_FIRST && itemId <= TOWN_LAST);
        const int playerId = itemId - (isTownClick ? TOWN_FIRST : HERO_FIRST);

        H3ScenarioPlayer &player = dlg->mapPlayersHuman[playerId];

        int humanPLayerId = 0;
        for (size_t i = 0; i < limits::PLAYERS; i++)
        {
            if (dlg->mapPlayersHuman[i].player2 != playerId)
            {
                humanPLayerId++;
            }
            else
            {
                player = dlg->mapPlayersHuman[i];
                break;
            }
            if (humanPLayerId >= limits::PLAYERS)
            {
                return THISCALL_2(int, h->GetOriginalFunc(), dlg, msg);
            }
        }

        if (msg->IsRightClick() && !isTownClick && player.selectedHeroIndex >= 0)
        {

            const int heroId =
                *reinterpret_cast<UINT8 *>(reinterpret_cast<UINT8 *>(&player.heroes) + player.selectedHeroIndex);
            {

                dlg::TownSelectionDlg::ShowStaringHeroDlg(heroId);
            }
            return 1;
        }
        if (msg->IsLeftDown())
        {

           // StartPlayerDlg()

        }

        // if (!dlg->mapPlayersHuman[playerIndex].player2 == playerIndex)
        {
            return THISCALL_2(int, h->GetOriginalFunc(), dlg, msg);
        }

        //  Era::ExecErmCmd("IF:L^%y1^");
    }

    return THISCALL_2(int, h->GetDefaultFunc(), dlg, msg);
}
int GetRealPlayerId(const H3SelectScenarioDialog *dlg, const int clickedPlayerId)
{
    if (clickedPlayerId >= 0 && clickedPlayerId < 8)
    {
        // v36 = &this->mapPlayersHuman[0].Player2;

        volatile int counter = 0;
        while (dlg->mapPlayersHuman[counter].player2 != clickedPlayerId)
        {
            // ++counter;
            // v36 += 31;
            if (++counter >= 8)
                return -1;
        }
        return counter;
    }

    return -1;
}
_LHF_(H3SelectScenarioDialog__NewGame_LeftClick)
{
    if (auto msg = reinterpret_cast<H3Msg *>(c->esi))
    {
        if (auto dlg = reinterpret_cast<H3SelectScenarioDialog *>(msg->GetDlg()))
        {
            // dlg->se
            const int itemId = msg->itemId;
            if (itemId < HERO_FIRST || itemId > TOWN_LAST)
            {
                return EXEC_DEFAULT;
            }
            const bool isTownClick = (itemId >= TOWN_FIRST && itemId <= TOWN_LAST);
            const int playerIndex = itemId - (isTownClick ? TOWN_FIRST : HERO_FIRST);
            if (playerIndex < ePlayer::RED || playerIndex > ePlayer::PINK)
            {
                return EXEC_DEFAULT;
            }
            H3ScenarioPlayer &player = dlg->mapPlayersHuman[playerIndex];

            int humanPLayerId = 0;
            for (size_t i = 0; i < limits::PLAYERS; i++)
            {
                if (dlg->mapPlayersHuman[i].player2 != playerIndex)
                {
                    humanPLayerId++;
                }
                else
                {
                    player = dlg->mapPlayersHuman[i];
                    break;
                }
                if (humanPLayerId >= limits::PLAYERS)
                {
                    return EXEC_DEFAULT;
                }
            }

            //  if (player.heroesCount > 16)

            const auto itpa = dlg->itpaDef;
            const int townsCount = CharAt(0x05134F7 + 2);
            const int cellWidth = itpa->widthDEF;
            const int cellHeight = itpa->heightDEF;
            constexpr int padding = dlg::DlgData::padding;
            constexpr int margin = dlg::DlgData::margin;
            dlg::DlgData townData, heroData;
            constexpr int townsPerRow = 3;
            townData.columns = townsPerRow;

            const int townsWidth = townsPerRow * cellWidth + (townsPerRow - 1) * padding + 2 * margin;
            const int townsPerCol = townsCount / townsPerRow + 1 + 2;
            townData.rows = townsPerCol;

            const int townsHeight = townsPerCol * cellHeight + (townsPerCol - 1) * padding + 2 * margin;

            constexpr int heroesPerRow = 4;
            heroData.columns = heroesPerRow;
            heroData.rows = townData.rows;

            const int heroesWidth = heroesPerRow * cellWidth + (heroesPerRow - 1) * padding + 2 * margin;

            constexpr int distance = dlg::DlgData::distance;
            const int dlgWidth = townsWidth + distance + heroesWidth;
            const int dlgHeight = townsHeight;
            townData.rect = {0, 0, townsWidth, dlgHeight};
            heroData.rect = {townsWidth + distance, 0, dlgWidth, dlgHeight};

            dlg::TownSelectionDlg selectionDlg(dlgWidth, townData, heroData, -1, -1, dlg, player);
            selectionDlg.Start();

            // refresh player selsection
            THISCALL_4(void, 0x058D7E0, dlg, playerIndex, 1, -1);
            // if it is network game
            if (IntAt(0x69959C))
            {

                struct NewHeroData
                {
                    int msgId = 1036;
                    int recipient = -1;
                    int _f_10 = 28;
                };
            }
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(HooksInit)
{
    // Фикс Димера - герой имеет продвинутую разведку на старте
    if (h3::H3HeroInfo *hero_info_table = P_HeroInfo->Get())
    {
        hero_info_table[eHero::DEEMER].sskills[1].level = eSecSkillLevel::BASIC;

        NPCStats::SetDefault();
        npcStatsVec.reserve(HEROES_COUNT);

        for (size_t i = 0; i < HEROES_COUNT; i++)
        {
            ChangeHeroStartingParameters(hero_info_table[i], i);
            npcStatsVec.emplace_back(NPCStats{i});
        }
    }

    _PI->WriteLoHook(0x0058692A, H3SelectScenarioDialog__NewGame_LeftClick);
    //_PI->WriteLoHook(0x0058260B, hook_0058260B);

    _PI->WriteHiHook(0x0587FD0, THISCALL_, H3SelectScenarioDialog__Proc);
    //  if (auto wogPathch = globalPatcher->GetInstance("HD.TE"))
    {
        globalPatcher->UndoAllAt(0x058285C);
        _PI->WriteCodePatch(0x058284F,
                            (char *)"0FB67C082C83FFFF0F84F50400008BC7C1E00603C78D04C08D9C4220160200807E6500");
    }

    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool plugin_On = 0;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        if (!plugin_On)

        {
            plugin_On = 1;

            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance("EraPlugin.HeroesStartingParameters.daemon_n");
            // Era::ConnectEra();
            _PI->WriteLoHook(0x4EEAF2, HooksInit);
            //   _PI->WriteHiHook(0x769AB4, THISCALL_, NPC__Init);
        }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
