#include "../../include/Plugin.h"
#include "../../include/WoGCrExpo.h"
#include <climits>
#include <cstring>
#include <string>
namespace gbf
{
namespace crexpo
{
using namespace WoG;
namespace
{
static h3::H3Army *TownUpArmy(h3::H3Town *town)
{
    return THISCALL_1(h3::H3Army *, 0x5C1860, town);
}
} // namespace

// © daemon_n
// отключить выдачу опыта существа при старте игры, есои опыт даётся герою вне битвы
void __stdcall OnGameEnter(Era::TEvent *)
{
    // сбрасываем тип источника выдаваемого опыта на "не битва"
    h3::IntAt(0x2861E14) = 0;
}

// © daemon_n
// Исправлению получения опыта существами, когда максимальный уровень опыта героем достигнут
int crexpoBonusExp = 0;
int __stdcall Hero__AtGiveExperienceAfterLimit(LoHook *, HookContext *c)
{
    // если опыт из битвы и мы превысили лимит
    if (h3::IntAt(0x2861E14) && c->edx > c->esi)
    {
        crexpoBonusExp = IntAt(c->ebp + 0x8); // добавить псевдоопыт
    }

    return EXEC_DEFAULT;
}
void __stdcall CrExpoSet_AddExpo(HiHook *h, h3::H3Hero *hero, int expToAdd, const int oldExp)
{
    if (crexpoBonusExp)
    {
        expToAdd += crexpoBonusExp; // добавить недостающий опыт
        crexpoBonusExp = 0;         // сбросить бонусный опыт
    }
    CDECL_3(void, h->GetDefaultFunc(), hero, expToAdd, oldExp);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// ИСПРАВЛЕНИЕ ОПЫТА ПРИ ПРИСОЕДИНЕНИИ СУЩЕСТВА К АРМИИ ГЕРОЯ ЧЕРЕЗ ДИАЛОГ ПРИСОЕДИНЕНИЯ
/////////////////////////////////////////////////////////////////////////////////////////////////////

static_assert(sizeof(h3::H3Mine) == 0x40, "WoG mine layout");
static_assert(offsetof(h3::H3Mine, army) == 0x04, "WoG mine army offset");
static_assert(offsetof(h3::H3Mine, x) == 0x3C, "WoG mine coordinates offset");

// Экспериментальный исходный вариант: фиктивная шахта остаётся в списке,
// а значение 12222 является отладочным. Установка хука ниже отключена.
static h3::H3Dlg *__stdcall JoinArmyDlgCtor(HiHook *hook, h3::H3Dlg *dialog, h3::H3Hero *hero, h3::H3Army *army, int a4)
{
    h3::H3Mine mine{};
    WoG::CrExpo::UniData data{};

    mine.owner = hero->owner;
    mine.army = *army;
    data.mine.x = mine.x = static_cast<UINT8>(hero->x);
    data.mine.y = mine.y = static_cast<UINT8>(hero->y);
    data.mine.z = mine.z = static_cast<UINT8>(hero->z);

    auto &mines = h3::H3Main::Get()->minesLighthouses;
    THISCALL_4(void, 0x4D0870, &mines, mines.end(), 1, &mine);

    for (int i = 0; i < 7; ++i)
    {
        if (army->type[i] > -1 && army->count[i] > 1)
        {
            data.mine.slot = i;
            WoG::CrExpo::SetNewAndClamp(WoG::CE_MINE, data, army->type[i], army->count[i], 0);
            if (auto *expo = WoG::CrExpo::Find(WoG::CE_MINE, data))
                expo->experience = 12222;
        }
    }

    return THISCALL_4(h3::H3Dlg *, hook->GetDefaultFunc(), dialog, hero, army, a4);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// ИСПРАВЛЕНИЕ РАССЧЁТА ОПЫТА ПРИ ПЕРЕПОЛНЕНИИ INT32
/////////////////////////////////////////////////////////////////////////////////////////////////////

// © daemon_n
// исправление перерасчёта опыта существа при модификации количества существ в отряде через ERM комманды EX или HE:C
static void __cdecl ERM_EX_ValidateSlot(HiHook *h, WoG::CrExpo *this_, const int expOrRankMod, const int type,
                                        const int srcMonId, const int dstMonId, const int old_mon_number,
                                        const int new_mon_number)
{

    auto ClampExp = [](int64_t value) -> int {
        if (value <= 0)
            return 0;
        if (value > INT_MAX)
            return INT_MAX;
        return static_cast<int>(value);
    };

    auto SafeMulDiv = [&](int a, int b, int c) -> int { return c ? ClampExp((static_cast<int64_t>(a) * b) / c) : 0; };

    auto SafeBlend = [&](int oldValue, int oldCount, int newValue, int newCount) -> int {
        if (!newCount)
            return 0;
        return ClampExp(static_cast<int64_t>(newValue) +
                        ((static_cast<int64_t>(oldValue) - newValue) * oldCount) / newCount);
    };

    switch (type)
    {
    case 0:
        if (!new_mon_number)
        {
            this_->experience = 0;
            break;
        }

        if (srcMonId == dstMonId)

            this_->experience = SafeBlend(this_->experience, old_mon_number, expOrRankMod, new_mon_number);
        else
            this_->experience = expOrRankMod;
        break;
    case 1:
        this_->experience = SafeBlend(this_->experience, old_mon_number, expOrRankMod, new_mon_number);
        break;
    case 2:
        this_->experience = ClampExp(static_cast<int64_t>(expOrRankMod));
        break;
    case 3:
        this_->experience = ClampExp(static_cast<int64_t>(this_->experience) + expOrRankMod);
        break;
    case 4:
        this_->experience = WoG::CrExpo::GetCreatureExpByRankAndExp(dstMonId, expOrRankMod, this_->experience);
        break;
    case 5: {
        const double mul = WoG::CrExpo::GetCreatureUpgradeMultiplier(srcMonId);
        const double value = mul * static_cast<double>(this_->experience) + static_cast<double>(expOrRankMod);
        this_->experience = ClampExp(static_cast<int64_t>(value));
        break;
    }
    case 10:
        if (!(new_mon_number + old_mon_number))
        {
            this_->experience = 0;
            break;
        }
        if (srcMonId != dstMonId)
        {
            this_->experience = WoG::CrExpo::GetCreatureExpByRankAndExp(dstMonId, expOrRankMod, 0);
            break;
        }
        // NO BREAK!!
    case 11: {
        if (!new_mon_number)
        {
            this_->experience = 0;
            break;
        }
        const int dstExp = WoG::CrExpo::GetCreatureExpByRankAndExp(dstMonId, expOrRankMod, 0);
        this_->experience = SafeBlend(this_->experience, old_mon_number, dstExp, new_mon_number);
        break;
    }
    case 12:
        this_->experience = WoG::CrExpo::GetCreatureExpByRankAndExp(dstMonId, expOrRankMod, 0);
        break;
    case 13: {
        const int dstLimit = WoG::CrExpo::GetCreatureExpLimit(dstMonId);
        const int srcLimit = WoG::CrExpo::GetCreatureExpLimit(srcMonId);
        this_->experience =
            ClampExp(static_cast<int64_t>(expOrRankMod) + SafeMulDiv(dstLimit, this_->experience, srcLimit));
        break;
    }
    case 14: {
        const int dstLimit = WoG::CrExpo::GetCreatureExpLimit(dstMonId);
        const int srcLimit = WoG::CrExpo::GetCreatureExpLimit(srcMonId);
        const int scaledExpo = SafeMulDiv(dstLimit, this_->experience, srcLimit);
        this_->experience = WoG::CrExpo::GetCreatureExpByRankAndExp(dstMonId, expOrRankMod, scaledExpo);
        break;
    }
    default:
        return;
    }
    // if (dstMonId >= 0)
    //   this_->Clamp(dstMonId);
}

int __stdcall WoG_CrExpo_Recalc(LoHook *, HookContext *c)
{
    WoG::CrExpo *exp = *reinterpret_cast<WoG::CrExpo **>(c->ebp - 0x4);
    const int creatureNum = IntAt(c->ebp + 0x8);
    exp->experience = static_cast<int>(static_cast<INT64>(exp->experience) * exp->number / creatureNum);
    exp->number = creatureNum;

    c->return_address = 0x07184D5;
    return NO_EXEC_DEFAULT;
}
int __stdcall WoG_SetNewExp_AtExpPush(LoHook *, HookContext *c)
{
    const int sourceExpoOffset = IntAt(c->ebp - 0x4);
    const int dstExpoOffset = IntAt(c->ebp - 0x10);
    const int sourceNumber = IntAt(c->ebp + 0x20);
    const int dstNumber = IntAt(c->ebp + 0x24);

    auto *expTable = reinterpret_cast<WoG::CrExpo *>(0x0860550);

    INT64 totalNewExp = static_cast<INT64>(expTable[sourceExpoOffset].experience) * sourceNumber;
    totalNewExp += static_cast<INT64>(expTable[dstExpoOffset].experience) * dstNumber;
    totalNewExp /= static_cast<INT64>(sourceNumber + dstNumber);

    c->eax = static_cast<int>(totalNewExp);

    c->return_address = 0x0719BEA;
    return NO_EXEC_DEFAULT;
}

int __stdcall WoG_CreatureSplitDlg_AtSourceExp(LoHook *, HookContext *c)
{
    auto &srcExpo = *reinterpret_cast<WoG::CrExpo *>(0x28602AC);
    auto &dstExpo = *reinterpret_cast<WoG::CrExpo *>(0x2860294);

    const int srcNum = IntAt(c->ebp - 0x10);

    const int baseNum = IntAt(0x282A77C);
    INT64 totalNewExp = (static_cast<INT64>((srcNum - baseNum)) * srcExpo.experience +
                         static_cast<INT64>(baseNum) * dstExpo.experience) /
                        srcNum;
    IntAt(c->ebp - 0x20) = static_cast<int>(totalNewExp);
    c->return_address = 0x07662D9;
    return NO_EXEC_DEFAULT;
}
int __stdcall WoG_CreatureSplitDlg_AtDestExp(LoHook *, HookContext *c)
{
    auto &srcExpo = *reinterpret_cast<WoG::CrExpo *>(0x28602AC);
    auto &dstExpo = *reinterpret_cast<WoG::CrExpo *>(0x2860294);

    const int baseNum = IntAt(0x282A35C);

    const int dstNum = IntAt(c->ebp - 0x68);

    INT64 totalNewExp = (static_cast<INT64>((dstNum - baseNum)) * dstExpo.experience +
                         static_cast<INT64>(baseNum) * srcExpo.experience) /
                        dstNum;
    IntAt(c->ebp - 0x3C) = static_cast<int>(totalNewExp);
    c->return_address = 0x0766341;
    return NO_EXEC_DEFAULT;
}

struct ArmySlotExperience
{
    int number;
    int experience;
} armySlots[14];

int __stdcall WoG_InTowmArmyMerge_GuardIterator(LoHook *, HookContext *c)
{
    const int slotId = c->eax;

    // сохраняем значения опыта и количества существ для армии защитника
    armySlots[slotId].experience = IntAt(c->ebp - 0x10C);
    armySlots[slotId].number = reinterpret_cast<h3::H3Army *>(0x2846BF0)->count[slotId];

    // заменить значения опыта существа на уникальный индекс для сложения в битсет
    c->ecx = 1 << slotId;

    return EXEC_DEFAULT;
}

int __stdcall WoG_InTowmArmyMerge_VisitorIterator(LoHook *, HookContext *c)
{
    const int slotId = c->ecx + 7;

    // сохраняем значения опыта и количества существ для армии визитёра
    armySlots[slotId].experience = IntAt(c->ebp - 0x10C);
    armySlots[slotId].number = reinterpret_cast<h3::H3Army *>(0x2846C28)->count[slotId - 7];

    // заменить значения опыта существа на уникальный индекс для сложения в битсет
    c->edx = 1 << slotId;

    return EXEC_DEFAULT;
}
int __stdcall WoG_InTowmArmyMerge_ExperienceCreatorIterator(LoHook *, HookContext *c)
{
    INT64 resultExp = 0;

    h3::H3Army &expArmy = *reinterpret_cast<h3::H3Army *>(c->ebp - 0xC8);

    const int slotId = IntAt(c->ebp - 0x18);
    const int slotsMerged = expArmy.count[slotId];

    for (size_t i = 0; i < 14; i++)
    {
        if (slotsMerged & (1 << i))
        {
            // сложение общего опыта, основываясь на начальных значениях
            resultExp += static_cast<INT64>(armySlots[i].experience) * armySlots[i].number;
        }
    }

    // установка итогового опыта делением на итоговое количество существ
    const int totalCreatures = IntAt(c->ebp - 0x14);
    c->eax = totalCreatures ? static_cast<int>(resultExp / totalCreatures) : 0;

    c->return_address = 0x0759A2E;
    return NO_EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// ИСПРАВЛЕНИЕ ОПЫТА ДЛЯ ИИ ПРИ ОБМЕНЕ И ПОКУПКЕ АРМИЙ
/////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GetArmyExperience(const WoG::eExpType type, WoG::CrExpo::UniData baseData, WoG::CrExpo *crexps[7]);

static void DebugArmy(const char *armyName, const h3::H3Army *army, WoG::CrExpo **crexpos = nullptr)
{
    std::string msg = armyName;
    msg += "\n\n\n";
    for (int i = 0; i < 7; i++)
    {
        // msg += "Slot ";
        if (army->type[i] > -1)
        {
            msg += std::to_string(i);
            msg += "{~>CPRSMALL.def:0:";
            msg += std::to_string(army->type[i] + 2) + " valign=bottom}";
            msg += " Count=";
            msg += std::to_string(army->count[i]);
            if (crexpos && crexpos[i])
            {
                msg += " Exp=";
                msg += std::to_string(crexpos[i]->experience);
            }
            msg += "\n\n";
        }
    }
    h3::H3Messagebox::Show(msg.c_str());
}
inline static void DebugHero(const h3::H3Hero *hero, const char *name = nullptr, const int withExp = true)
{
#ifdef _DEBUG

    if (hero->id != 33)
    {
        return;
    }
    WoG::CrExpo *crexps[7] = {0};
    WoG::CrExpo **p_crexps = crexps;

    if (withExp)
    {
        WoG::CrExpo::UniData basePlace;
        basePlace.hero.id = hero->id;
        GetArmyExperience(CE_HERO, basePlace, crexps);
    }
    if (!name)
    {
        name = "Hero Army";
    }
    DebugArmy(name, &hero->army, p_crexps);
#endif // _DEBUG
}

h3::H3Army sourceArmyCopy, targetArmyCopy;

struct ExperienceRecalcContext
{

    h3::H3Army armyCopy;
    WoG::CrExpo *crexps[7];
    WoG::CrExpo::UniData basePlace;

} sourceContext, targetContext;

struct PoolItem
{
    int type;
    int count;
    int64_t expMass; // ❗ вместо DWORD exp
};

struct TakeResult
{
    int taken;        // сколько реально взяли
    int64_t totalExp; // сумма (exp * count)
};
static inline void TakeFromPool(PoolItem *pool, const int poolSize, const int type, const int need,
                                TakeResult &out // ← возврат по ссылке
)
{
    int remaining = need;
    int64_t totalExp = 0;

    for (int i = 0; i < poolSize && remaining > 0; i++)
    {
        PoolItem &p = pool[i];

        if (p.count <= 0)
            continue;

        if (p.type != type)
            continue;

        const int take = std::min(p.count, remaining);

        const int64_t takeMass = (p.expMass * take) / p.count;
        totalExp += takeMass;

        p.count -= take;
        remaining -= take;
    }

    out.taken = need - remaining;
    out.totalExp = totalExp;
}

static int BuildPool(PoolItem *pool, const h3::H3Army &army, WoG::CrExpo *crexpos[7])
{
    int poolSize = 0;

    for (int i = 0; i < 7; i++)
    {
        auto &crexp = crexpos[i];
        if (army.count[i] > 0)
        {
            pool[poolSize++] = {army.type[i], army.count[i], crexp ? (int64_t)crexp->experience * crexp->number : 0};
        }
        if (crexp)
            crexp->Clear();
    }
    return poolSize;
}

static void RemapArmyFromPool(PoolItem *pool, const int poolSize, const h3::H3Army *newArmy,
                              const WoG::eExpType expType, const WoG::CrExpo::UniData basePlace)
{
    for (int i = 0; i < 7; i++)
    {
        const int type = newArmy->type[i];
        const int count = newArmy->count[i];

        // 🔻 пустой слот
        if (count <= 0)
        {
            continue;
        }

        TakeResult res{};
        TakeFromPool(pool, poolSize, type, count, res);

        WoG::CrExpo::UniData data = basePlace;

        if (expType == CE_HERO)
        {
            data.hero.slot = i;
        }
        else
        {
            data.anyGarrison.slot = i;
        }

        const int offset = WoG::CrExpo::SetNewAndClamp(expType, data, type, count, 0);

        if (offset >= 0)
        {
            WoG::CrExpo *exp = &reinterpret_cast<WoG::CrExpo *>(0x0860550)[offset];
            // ⚖️ вычисление опыта
            if (res.taken > 0)
            {
                int64_t avg = (res.totalExp + res.taken / 2) / res.taken; // округление

                if (avg > 0xFFFFFFFF)
                    avg = 0xFFFFFFFF;

                exp->experience = (DWORD)avg;
            }
            else
            {
                // покупка
                exp->experience = 0;
            }
        }
    }
}

BOOL GetArmyExperience(const WoG::eExpType type, WoG::CrExpo::UniData baseData, WoG::CrExpo *crexps[7])
{
    switch (type)
    {
    case CE_HERO:
        for (size_t i = 0; i < 7; i++)
        {
            baseData.hero.slot = static_cast<short>(i);
            crexps[i] = WoG::CrExpo::Find(WoG::CE_HERO, baseData);
        }
        break;
    case CE_TOWN:
    case CE_MINE:
    case CE_HORN:
        for (size_t i = 0; i < 7; i++)
        {
            baseData.anyGarrison.slot = static_cast<short>(i);
            crexps[i] = WoG::CrExpo::Find(type, baseData);
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

// корректировка опыта при управлении армией ИИ в городе
void __stdcall AI_Player_Hero_ManageArmyInTown(HiHook *h, DWORD *aiData, h3::H3Hero *targetHero, h3::H3Town *town)
{

    // если включён опыт существ
    const BOOL stackExpIsEnabled = IntAt(0x2772730);

    // DebugHero(targetHero, "AI_Player_Hero_ManageArmyInTown");

    if (stackExpIsEnabled)
    {
        memcpy(&sourceArmyCopy, TownUpArmy(town), sizeof(h3::H3Army));  // сохраняем данные армии города до обмена
        memcpy(&targetArmyCopy, &targetHero->army, sizeof(h3::H3Army)); // сохраняем данные армии героя до обмена
    }
    // вызываем оригинальную функцию для обмена армиями
    THISCALL_3(void, h->GetDefaultFunc(), aiData, targetHero, town);

    if (stackExpIsEnabled)
    {
        if (memcmp(&sourceArmyCopy, TownUpArmy(town), sizeof(h3::H3Army)) == 0 &&
            memcmp(&targetArmyCopy, &targetHero->army, sizeof(h3::H3Army)) == 0)
            return; // если армия не изменилась, то не нужно ничего делать

        WoG::CrExpo::UniData sourceArmyData{}, targetArmyData{};

        WoG::eExpType sourceArmyType;
        if (town->garrisonHero > -1)
        {
            sourceArmyData.hero.id = town->garrisonHero; // герой-защитник
            sourceArmyType = CE_HERO;
        }
        else
        {
            sourceArmyType = CE_TOWN;
            sourceArmyData.town.x = town->x;
            sourceArmyData.town.y = town->y;
            sourceArmyData.town.z = town->z;
        }
        // сохраняем данные опыта до обмена армиями
        targetArmyData.hero.id = targetHero->id; // герой-визитёр

        WoG::CrExpo *sourceArmyExp[7]{}, *targetArmyExp[7]{};

        GetArmyExperience(sourceArmyType, sourceArmyData, sourceArmyExp);
        GetArmyExperience(CE_HERO, targetArmyData, targetArmyExp);

        PoolItem pool[14];
        int poolSize = BuildPool(pool, sourceArmyCopy, sourceArmyExp);
        poolSize += BuildPool(&pool[poolSize], targetArmyCopy, targetArmyExp);

        // 🔻 герой
        RemapArmyFromPool(pool, poolSize, TownUpArmy(town), sourceArmyType, sourceArmyData);
        RemapArmyFromPool(pool, poolSize, &targetHero->army, CE_HERO, targetArmyData);
    }
    // DebugHero(targetHero);
}

struct
{
    BOOL enabled = FALSE;
    h3::H3Hero *hero = nullptr;
    h3::H3Army armyCopy;

  public:
    void BeforeUpgrade(h3::H3Hero *targetHero)
    {
        if (enabled = IntAt(0x2772730)) // если включён опыт существ
        {
            hero = targetHero;
            memcpy(&armyCopy, &targetHero->army, sizeof(h3::H3Army)); // сохраняем данные армии героя до улучшения
        }
        // DebugHero(targetHero, "BeforeUpgrade");
    }

    void AfterUpgrade()
    {
        if (!enabled)
            return;

        if (memcmp(&armyCopy, &hero->army, sizeof(h3::H3Army)) == 0)
        {
            enabled = FALSE;
            return; // если армия не изменилась, то не нужно ничего делать
        }
        WoG::CrExpo::UniData data{};
        data.hero.id = hero->id;
        WoG::CrExpo *targetArmyExp[7]{};
        GetArmyExperience(CE_HERO, data, targetArmyExp);
        for (size_t i = 0; i < 7; i++)
        {
            const int oldType = armyCopy.type[i];
            const int newType = hero->army.type[i];
            if (armyCopy.count[i] > 0 && hero->army.count[i] > 0 && oldType != newType)
            {
                int newExp = 0;
                if (targetArmyExp[i])
                {
                    const int oldExp = targetArmyExp[i]->experience; // сохраняем опыт, если он был
                    targetArmyExp[i]->Clear();                       // если тип существа изменился, то опыт обнуляется
                    if (oldExp)
                    {
                        newExp = static_cast<int>(static_cast<float>(oldExp) *
                                                  CDECL_1(float, 0x0727E00,
                                                          oldType)); // получение мультипликатора опыта при улучшении
                    }
                }
                data.hero.slot = static_cast<short>(i);
                // создаём новый слот с типом существа после улучшения
                const int offset = WoG::CrExpo::SetNewAndClamp(CE_HERO, data, newType, hero->army.count[i], newExp);
            }
        }
        // DebugHero(hero, "AfterUpgrade");
    }
} armyUpgrader;
// хук перед самим улучшением армий
int __stdcall AI_enter_town_before_creatures_upgrade(LoHook *, HookContext *c)
{
    armyUpgrader.BeforeUpgrade(reinterpret_cast<h3::H3Hero *>(c->edi));
    return EXEC_DEFAULT;
}
// хук после самого улучшения армий
int __stdcall AI_enter_town_after_creatures_upgrade(LoHook *, HookContext *c)
{
    armyUpgrader.AfterUpgrade();
    return EXEC_DEFAULT;
}
// хук при улучшения армии ИИ-героя в форте на холме
void __stdcall AI_AdvMgr_HillFort(HiHook *h, h3::H3Hero *targetHero)
{
    armyUpgrader.BeforeUpgrade(targetHero);
    THISCALL_1(void, h->GetDefaultFunc(), targetHero);
    armyUpgrader.AfterUpgrade();
}

// покупка существа во городском жилище жилище
void __stdcall AI_Town_BuyCreatures(HiHook *h, DWORD _this, h3::H3Town *town)
{

    // DebugHero(h3::H3Main::Get()->GetHero(town->garrisonHero), "AI_Town_BuyCreatures up_hero_id");
    // DebugHero(h3::H3Main::Get()->GetHero(town->visitingHero), "AI_Town_BuyCreatures down_hero_id");

    const BOOL stackExpIsEnabled = IntAt(0x2772730);
    if (stackExpIsEnabled)
    {
        memcpy(&targetArmyCopy, TownUpArmy(town), sizeof(h3::H3Army)); // сохраняем данные армии города до обмена
    }

    THISCALL_2(void, h->GetDefaultFunc(), _this, town);

    if (stackExpIsEnabled)
    {
        if (memcmp(&targetArmyCopy, TownUpArmy(town), sizeof(h3::H3Army)) == 0)
            return; // если армия не изменилась, то не нужно ничего делать

        WoG::CrExpo::UniData targetArmyData{};
        WoG::eExpType targetArmyType;
        if (town->garrisonHero > -1)
        {
            targetArmyData.hero.id = town->garrisonHero; // герой-защитник
            targetArmyType = CE_HERO;
            // DebugHero(h3::H3Main::Get()->GetHero(town->garrisonHero), "AI_Town_BuyCreatures");
        }
        else
        {
            targetArmyType = CE_TOWN;
            targetArmyData.town.x = town->x;
            targetArmyData.town.y = town->y;
            targetArmyData.town.z = town->z;
        }

        WoG::CrExpo *targetArmyExp[7]{};
        GetArmyExperience(targetArmyType, targetArmyData, targetArmyExp);
        PoolItem pool[7];
        const int poolSize = BuildPool(pool, targetArmyCopy, targetArmyExp);
        RemapArmyFromPool(pool, poolSize, TownUpArmy(town), targetArmyType, targetArmyData);
    }
}

// корректировка опыта для ИИ при покупке существ во внешних жилищах
void __stdcall AI_H3Hero_BuyDwellingCreatures(HiHook *h, h3::H3Hero *targetHero, void *dwelling)
{

    const BOOL stackExpIsEnabled = IntAt(0x2772730);
    if (stackExpIsEnabled)
    {
        memcpy(&targetArmyCopy, &targetHero->army, sizeof(h3::H3Army)); // сохраняем данные армии города до обмена
        // DebugHero(targetHero, "AI_H3Hero_BuyDwellingCreatures");
    }

    FASTCALL_2(void, h->GetDefaultFunc(), targetHero, dwelling);

    if (stackExpIsEnabled)
    {
        if (memcmp(&targetArmyCopy, &targetHero->army, sizeof(h3::H3Army)) == 0)
            return; // если армия не изменилась, то не нужно ничего делать
        WoG::CrExpo::UniData targetArmyData{};
        targetArmyData.hero.id = targetHero->id;

        WoG::CrExpo *targetArmyExp[7]{};
        GetArmyExperience(CE_HERO, targetArmyData, targetArmyExp);
        PoolItem pool[7];
        const int poolSize = BuildPool(pool, targetArmyCopy, targetArmyExp);
        RemapArmyFromPool(pool, poolSize, &targetHero->army, CE_HERO, targetArmyData);
        // DebugHero(targetHero, "AI_H3Hero_BuyDwellingCreatures");
    }
}

// корректировка опыта для ИИ при присоединении одного существа извне
void __stdcall AI_H3Hero_JoinOneCreature(HiHook *h, h3::H3Hero *targetHero, const int creatureId, WORD monNum)
{
    //   DebugHero(targetHero, "AI_H3Hero_JoinOneCreature");

    const BOOL stackExpIsEnabled = IntAt(0x2772730);
    if (stackExpIsEnabled)
    {
        memcpy(&targetArmyCopy, &targetHero->army, sizeof(h3::H3Army)); // сохраняем данные армии города до обмена
    }

    FASTCALL_3(void, h->GetDefaultFunc(), targetHero, creatureId, monNum);

    if (stackExpIsEnabled)
    {
        if (memcmp(&targetArmyCopy, &targetHero->army, sizeof(h3::H3Army)) == 0)
            return; // если армия не изменилась, то не нужно ничего делать
        WoG::CrExpo::UniData targetArmyData{};

        WoG::CrExpo *targetArmyExp[7]{};
        targetArmyData.hero.id = targetHero->id;
        GetArmyExperience(CE_HERO, targetArmyData, targetArmyExp);
        PoolItem pool[7];
        const int poolSize = BuildPool(pool, targetArmyCopy, targetArmyExp);
        RemapArmyFromPool(pool, poolSize, &targetHero->army, CE_HERO, targetArmyData);
    }
    // DebugHero(targetHero, "AI_H3Hero_JoinOneCreature");
}
// обмен армиями между ИИ-героем и гарнизоном
void __stdcall AI_H3Hero_VisitGarrison(HiHook *h, h3::H3Hero *targetHero, h3::H3Garrison *sourceGarrison)
{
    // если включе опыт существ
    const BOOL applyChangesToAI =
        IntAt(0x2772730) && sourceGarrison->canRemoveCreatures && sourceGarrison->owner == targetHero->owner;
    // DebugHero(targetHero, "AI_H3Hero_VisitGarrison");
    if (applyChangesToAI)
    {
        memcpy(&sourceArmyCopy, &sourceGarrison->army,
               sizeof(h3::H3Army));                                     // сохраняем данные армии гарнизона до обмена
        memcpy(&targetArmyCopy, &targetHero->army, sizeof(h3::H3Army)); // сохраняем данные армии героя до обмена
    }
    // вызываем оригинальную функцию для обмена армиями
    FASTCALL_2(void, h->GetDefaultFunc(), targetHero, sourceGarrison);

    if (applyChangesToAI)
    {
        if (memcmp(&sourceArmyCopy, &sourceGarrison->army, sizeof(h3::H3Army)) == 0 &&
            memcmp(&targetArmyCopy, &targetHero->army, sizeof(h3::H3Army)) == 0)
            return; // если армия не изменилась, то не нужно ничего делать

        WoG::CrExpo::UniData sourceArmyData{}, targetArmyData{};
        sourceArmyData.garrison.x = sourceGarrison->x;
        sourceArmyData.garrison.y = sourceGarrison->y;
        sourceArmyData.garrison.z = sourceGarrison->z;
        targetArmyData.hero.id = targetHero->id; // герой-визитёр

        WoG::CrExpo *sourceArmyExp[7]{}, *targetArmyExp[7]{};
        GetArmyExperience(CE_HORN, sourceArmyData, sourceArmyExp);
        GetArmyExperience(CE_HERO, targetArmyData, targetArmyExp);

        PoolItem pool[14];
        int poolSize = BuildPool(pool, sourceArmyCopy, sourceArmyExp);
        poolSize += BuildPool(&pool[poolSize], targetArmyCopy, targetArmyExp);

        RemapArmyFromPool(pool, poolSize, &sourceGarrison->army, CE_HORN, sourceArmyData);
        RemapArmyFromPool(pool, poolSize, &targetHero->army, CE_HERO, targetArmyData);
    }
    // DebugHero(targetHero, "AI_H3Hero_VisitGarrison");
}

// корректировка при обмене армиями между двумя ИИ-Героями. Работает, если есть 2 героя
void __stdcall AI_ArmyExchanging_ExchangeArmy(HiHook *h, DWORD *aiData, h3::H3Hero *targetHero, h3::H3Army *sourceArmy,
                                              h3::H3Hero *sourceHero, BOOL hasAlliance)
{
    const BOOL applyChangesToAI = IntAt(0x2772730) && sourceHero;
    if (applyChangesToAI)
    {
        memcpy(&sourceArmyCopy, sourceArmy, sizeof(h3::H3Army));        // сохраняем данные армии визитёра до обмена
        memcpy(&targetArmyCopy, &targetHero->army, sizeof(h3::H3Army)); // сохраняем данные армии источника до обмена
    }
    // DebugHero(targetHero);

    THISCALL_5(void, h->GetDefaultFunc(), aiData, targetHero, sourceArmy, sourceHero, hasAlliance);

    if (applyChangesToAI)
    {
        if (memcmp(&sourceArmyCopy, sourceArmy, sizeof(h3::H3Army)) == 0 &&
            memcmp(&targetArmyCopy, &targetHero->army, sizeof(h3::H3Army)) == 0)
            return; // если армия не изменилась, то не нужно ничего делать

        WoG::CrExpo::UniData sourceArmyData{}, targetArmyData{};
        sourceArmyData.hero.id = sourceHero->id;
        targetArmyData.hero.id = targetHero->id;

        WoG::CrExpo *sourceArmyExp[7]{}, *targetArmyExp[7]{};
        GetArmyExperience(CE_HERO, sourceArmyData, sourceArmyExp);
        GetArmyExperience(CE_HERO, targetArmyData, targetArmyExp);

        PoolItem pool[14];
        int poolSize = BuildPool(pool, sourceArmyCopy, sourceArmyExp);
        poolSize += BuildPool(&pool[poolSize], targetArmyCopy, targetArmyExp);

        RemapArmyFromPool(pool, poolSize, sourceArmy, CE_HERO, sourceArmyData);
        RemapArmyFromPool(pool, poolSize, &targetHero->army, CE_HERO, targetArmyData);
    }
    // DebugHero(targetHero);
}

// помещение героя на карту:
// Если последний аргумент есть, то это инициализация героя -- с этим мы работаем
void __stdcall H3Hero_PutOnMap(HiHook *h, h3::H3Hero *targetHero, const int playerId, const DWORD pos,
                               const bool resetFlags)
{

    const BOOL applyChangesToAI = IntAt(0x2772730) && resetFlags;

    if (applyChangesToAI)
    {
        memcpy(&targetArmyCopy, &targetHero->army, sizeof(h3::H3Army)); // сохраняем данные армии города до обмена
    }
    // DebugHero(targetHero, "H3Hero_PutOnMap");

    THISCALL_4(void, h->GetDefaultFunc(), targetHero, playerId, pos, resetFlags);

    if (applyChangesToAI)
    {
        // я решил отключить проверку, чтобы каждая инициализация героя инициализировал и опыт его армии корректно
        //   if (memcmp(&targetArmyCopy, &targetHero->army, sizeof(h3::H3Army)) == 0)
        //       return; // если армия не изменилась, то не нужно ничего делать
        WoG::CrExpo::UniData targetArmyData{};
        targetArmyData.hero.id = targetHero->id;

        WoG::CrExpo *targetArmyExp[7]{};
        GetArmyExperience(CE_HERO, targetArmyData, targetArmyExp);
        PoolItem pool[7];
        const int poolSize = BuildPool(pool, targetArmyCopy, targetArmyExp);
        RemapArmyFromPool(pool, poolSize, &targetHero->army, CE_HERO, targetArmyData);
    }
    // DebugHero(targetHero, "H3Hero_PutOnMap");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
constexpr int SPELL_COUNT = 81;
constexpr int SPELL_RECORD_SIZE = 0x88;

bool applyingSEMagicResistance = false;
bool injectingSEMagicResistance = false;
bool skipWoGSEMagicResistance = false;

int GetSpellTargetType(int spell)
{
    return h3::H3Spell::Get() ? static_cast<int>(h3::H3Spell::Get()[spell].type) : 1;
}
} // namespace

int __cdecl CrExpBon_DwarfResist_WoG_0071CC3B(HiHook *hook, int stack, signed int resistance, int spell)
{
    if (skipWoGSEMagicResistance && !injectingSEMagicResistance)
    {
        applyingSEMagicResistance = false;
        skipWoGSEMagicResistance = false;
        return resistance;
    }

    return CDECL_3(int, hook->GetDefaultFunc(), stack, resistance, spell);
}

int __stdcall SEMagicResistance_Begin_0075DB2C(LoHook *, HookContext *c)
{
    const int spell = *reinterpret_cast<int *>(0x0286021C);
    const bool isHostileSpell = spell >= 0 && spell < SPELL_COUNT && GetSpellTargetType(spell) <= 0;

    applyingSEMagicResistance = isHostileSpell;
    injectingSEMagicResistance = false;
    skipWoGSEMagicResistance = isHostileSpell;

    return EXEC_DEFAULT;
}

int __stdcall SEMagicResistance_Apply_0044A4C7(LoHook *, HookContext *c)
{
    if (!applyingSEMagicResistance)
        return EXEC_DEFAULT;

    const int stack = *reinterpret_cast<int *>(0x02846884);
    const int spell = *reinterpret_cast<int *>(0x0286021C);

    if (!stack || spell < 0 || spell >= SPELL_COUNT)
    {
        applyingSEMagicResistance = false;
        skipWoGSEMagicResistance = false;
        return EXEC_DEFAULT;
    }

    float &spellChance = *reinterpret_cast<float *>(c->ebp + 0x0C);
    const int spellChancePercent = static_cast<int>(spellChance * 100.0f + 0.5f);
    const int baseResistance = 100 - spellChancePercent;

    injectingSEMagicResistance = true;
    const int combinedResistance = CDECL_3(int, 0x0071CC3B, stack, baseResistance, spell);
    injectingSEMagicResistance = false;

    spellChance = 1.0f - static_cast<float>(combinedResistance) / 100.0f;

    return EXEC_DEFAULT;
}

int __stdcall SEMagicResistance_Finish_0075DB53(LoHook *, HookContext *c)
{
    applyingSEMagicResistance = false;
    injectingSEMagicResistance = false;
    skipWoGSEMagicResistance = false;

    return EXEC_DEFAULT;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void Install(PatcherInstance *pi)
{
    // ОПТИМИЗАЦИИ

    // © daemon_n
    // Оптимизация: убрать повторное вычисление максимального опыта существа при ограничении
    pi->WriteCodePatch(0x07176D1, "%n", 15); // NOPs вместо повтороного вычисления максимального опыта существа

    // ИСПРАВЛЕНИЯ

    // © daemon_n
    // исправление ошибки с ненайденным опытом существа при открытии диалога присоединения армии к герою
    // pi->WriteHiHook(0x05D1620, CALL_, EXTENDED_, THISCALL_, JoinArmyDlgCtor);

    // © daemon_n
    // Ошибка бонуса опыта существ для члена "модификатор", где имеем некорректный тип данных (знаковый) для структуры:
    // Суть в подмене типа копирования символов со знакового на беззнаковое ( MOVSX -> MOVZX )
    pi->WriteByte(0x71C7A7 + 1, 0xB6);
    pi->WriteByte(0x71C7B3 + 1, 0xB6);
    pi->WriteByte(0x71C7D2 + 1, 0xB6);
    // © daemon_n
    // отключить выдачу опыта существа при старте игры, если опыт даётся герою вне битвы
    Era::RegisterHandler(OnGameEnter, "OnGameEnter");
    // © daemon_n
    // Исправлению получения опыта существами, когда максимальный уровень опыта героем достигнут
    pi->WriteLoHook(0x04E36CE,
                    Hero__AtGiveExperienceAfterLimit); // сохранить опыт героя, который он должен был получить
    pi->WriteHiHook(0x076B46D, CALL_, EXTENDED_, CDECL_, CrExpoSet_AddExpo); // добавить опыт существам

    // © daemon_n
    // исправление перерасчёта опыта существа при модификации количества существ в отряде через ERM комманды EX или HE:C
    pi->WriteHiHook(0x071960B, SPLICE_, EXTENDED_, CDECL_, ERM_EX_ValidateSlot);

    // © daemon_n
    // исправление перерасчёта опыта существа при изменении их количества в стеке (при покупке или тп)
    pi->WriteLoHook(0x07184B0, WoG_CrExpo_Recalc);

    // © daemon_n
    // исправление перерасчёта опыта существа при соединении двух отрядов
    pi->WriteLoHook(0x0719BE3, WoG_SetNewExp_AtExpPush);

    // © daemon_n
    // исправление расчёта опыта при передаче отрядов через диалог опыта
    pi->WriteLoHook(0x07662B2, WoG_CreatureSplitDlg_AtSourceExp);
    pi->WriteLoHook(0x076631A, WoG_CreatureSplitDlg_AtDestExp);

    // © daemon_n
    // исправление расчёта опыта при перемещении героя в город
    pi->WriteLoHook(0x0759749, WoG_InTowmArmyMerge_GuardIterator);
    pi->WriteLoHook(0x0759870, WoG_InTowmArmyMerge_VisitorIterator);
    pi->WriteLoHook(0x0759A24, WoG_InTowmArmyMerge_ExperienceCreatorIterator);

    //  отключение сортировки существ в ИИ Армиях, чтобы не ломать опыт
    {
        // забрать Армию из родного города ИИ-героем (если есть очки передвижения)
        pi->WriteHiHook(0x0525985, CALL_, EXTENDED_, THISCALL_, AI_Player_Hero_ManageArmyInTown);

        // хук перед улучшением армии ИИ-героя в городе
        pi->WriteLoHook(0x0525A3C, AI_enter_town_before_creatures_upgrade);

        // хук после улучшения армии ИИ-героя в городе
        pi->WriteLoHook(0x0525B90, AI_enter_town_after_creatures_upgrade);

        // хук при улучшения армии ИИ-героя в форте на холме
        pi->WriteHiHook(0x0528090, SPLICE_, EXTENDED_, THISCALL_, AI_AdvMgr_HillFort);

        // покупка существа в городском жилище
        pi->WriteHiHook(0x0428580, SPLICE_, EXTENDED_, THISCALL_, AI_Town_BuyCreatures);

        // покупка существа во внешнем жилище
        pi->WriteHiHook(0x0527FE0, SPLICE_, EXTENDED_, FASTCALL_, AI_H3Hero_BuyDwellingCreatures);

        // присоединение существ для ИИ-героя бесплатно
        pi->WriteHiHook(0x052C140, SPLICE_, EXTENDED_, FASTCALL_, AI_H3Hero_JoinOneCreature);

        // обмен армиями между ИИ-героем и гарнизоном
        pi->WriteHiHook(0x0524850, SPLICE_, EXTENDED_, FASTCALL_, AI_H3Hero_VisitGarrison);

        // обмен армиями между героями снаружи
        pi->WriteHiHook(0x0526333, CALL_, EXTENDED_, THISCALL_, AI_ArmyExchanging_ExchangeArmy);
        // обмен армиями между героем в городе
        pi->WriteHiHook(0x052587F, CALL_, EXTENDED_, THISCALL_, AI_ArmyExchanging_ExchangeArmy);

        // DEBUG
        // pi->WriteHiHook(0x042C3B0, SPLICE_, EXTENDED_, THISCALL_, AI_ArmyExchanging_ExchangeArmy);

        // помещение героя в на карту -- нужно для покупки и прочее. ПРОВЕРЯТЬ НА СБРОС!
        pi->WriteHiHook(0x04D7B70, SPLICE_, EXTENDED_, THISCALL_, H3Hero_PutOnMap);
    }

    // © daemon_n
    // исправление расчёта опыта при перемещении отряда существ из слота в слот
    const char *srcExpPatch = "8DB0 50058600 8B06 F76D 20 F77E 04 8906 8B4D 20 894E 04"; // слот-источник
    /*
    lea     esi, [WOG_WoG::CrExpo_Table + eax]   ; esi = &Table[src]
    mov     eax, [esi]                        ; eax = Expo
    imul    dword ptr [ebp+source_mon_number] ; edx:eax = Expo * src (64-bit)
    idiv    dword ptr [esi+4]                 ; eax = (Expo*src)/Num

    mov     [esi], eax                        ; Table[src].Expo = eax

    mov     ecx, [ebp+source_mon_number]
    mov     [esi+4], ecx                     ; Table[src].Num = src
    */

    pi->WriteHexPatch(0x0719993, srcExpPatch); // patch experience
    pi->WriteCodePatch(0x07199A9, "%n", 28);   // nop extra code

    const char *dstExpPatch = "8DB1 50058600 8B06 F76D 24 F77E 04 8906 8B4D 24 894E 04"; // слот-назначение
    /*
    lea     esi, [WOG_WoG::CrExpo_Table + ecx]   ; esi = &Table[dst]
    mov     eax, [esi]                        ; eax = Expo
    imul    dword ptr [ebp+dst_mon_number] ; edx:eax = Expo * dst (64-bit)
    idiv    dword ptr [esi+4]                 ; eax = (Expo*dst)/Num

    mov     [esi], eax                        ; Table[dst].Expo = eax

    mov     ecx, [ebp+dst_mon_number]
    mov     [esi+4], ecx                     ; Table[dst].Num = dst
    */

    pi->WriteHexPatch(0x0719A55, dstExpPatch); // patch experience
    pi->WriteCodePatch(0x0719A6B, "%n", 28);   // nop extra code

    PatcherInstance *instance = gbf::patcher->GetInstance("WogSEResistanceFix.era");
    if (instance)
        return;
    instance = gbf::patcher->CreateInstance("WogSEResistanceFix.era");
    if (instance)
    {
        instance->WriteHiHook(0x0071CC3B, SPLICE_, EXTENDED_, CDECL_, CrExpBon_DwarfResist_WoG_0071CC3B);
        instance->WriteLoHook(0x0075DB2C, SEMagicResistance_Begin_0075DB2C);
        instance->WriteLoHook(0x0044A4C7, SEMagicResistance_Apply_0044A4C7);
        instance->WriteLoHook(0x0075DB53, SEMagicResistance_Finish_0075DB53);
    }
}
} // namespace crexpo
void InstallCrExpo(PatcherInstance *pi)
{
    crexpo::Install(pi);
}
} // namespace gbf
