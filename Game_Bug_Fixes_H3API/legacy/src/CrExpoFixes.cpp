namespace CrExpo
{

// © daemon_n
// отключить выдачу опыта существа при старте игры, есои опыт даётся герою вне битвы
_ERH_(OnGameEnter)
{
    // сбрасываем тип источника выдаваемого опыта на "не битва"
    o_CreExpoCombatFlag = 0;
}

// © daemon_n
// Исправлению получения опыта существами, когда максимальный уровень опыта героем достигнут
int crexpoBonusExp = 0;
_LHF_(Hero__AtGiveExperienceAfterLimit)
{
    // если опыт из битвы и мы превысили лимит
    if (o_CreExpoCombatFlag && c->edx > c->esi)
    {
        crexpoBonusExp = IntAt(c->ebp + 0x8); // добавить псевдоопыт
    }

    return EXEC_DEFAULT;
}
void __stdcall CrExpoSet_AddExpo(HiHook *h, _Hero_ *hero, int expToAdd, const int oldExp)
{
    if (crexpoBonusExp)
    {
        expToAdd += crexpoBonusExp; // добавить недостающий опыт
        crexpoBonusExp = 0;         // сбросить бонусный опыт
    }
    CALL_3(void, __cdecl, h->GetDefaultFunc(), hero, expToAdd, oldExp);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// ИСПРАВЛЕНИЕ ОПЫТА ПРИ ПРИСОЕДИНЕНИИ СУЩЕСТВА К АРМИИ ГЕРОЯ ЧЕРЕЗ ДИАЛОГ ПРИСОЕДИНЕНИЯ
/////////////////////////////////////////////////////////////////////////////////////////////////////

static _Dlg_ *__stdcall JoinArmyDlgCtor(HiHook *h, _Dlg_ *this_, _Hero_ *hero, _Army_ *army, int a4)
{

    _Mine_ mine;
    _CrExpo_::UniData data;

    mine.owner = hero->owner_id;
    mine.army = *army;
    data.mine.x = mine.x = hero->x;
    data.mine.y = mine.y = hero->y;
    data.mine.z = mine.z = hero->z;
    auto minesVector = o_GameMgr->Offset(0x4E388);
    auto lastMine = o_GameMgr->Offset(0x4E390);
    CALL_4(void, __thiscall, 0x4D0870, (char *)o_GameMgr + 0x4E388, IntAt((char *)o_GameMgr + 0x4E390), 1, &mine);

    for (size_t i = 0; i < 7; i++)
    {
        if (army->type[i] > -1 && army->count[i] > 1)
        {
            data.mine.slot = i;
            _CrExpo_::SetNewAndClamp(eExpType::CE_MINE, data, army->type[i], army->count[i], 0);
            auto expo = _CrExpo_::Find(eExpType::CE_MINE, data);
            expo->experience = 12222;
        }
    }

    // o_GameMgr->map_header

    auto result = CALL_4(_Dlg_ *, __thiscall, h->GetDefaultFunc(), this_, hero, army, a4);

    return result;
}

// © daemon_n

/////////////////////////////////////////////////////////////////////////////////////////////////////
// ИСПРАВЛЕНИЕ РАССЧЁТА ОПЫТА ПРИ ПЕРЕПОЛНЕНИИ INT32
/////////////////////////////////////////////////////////////////////////////////////////////////////

// © daemon_n
// исправление перерасчёта опыта существа при модификации количества существ в отряде через ERM комманды EX или HE:C
static void __cdecl ERM_EX_ValidateSlot(HiHook *h, _CrExpo_ *this_, const int expOrRankMod, const int type,
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
        this_->experience = _CrExpo_::GetCreatureExpByRankAndExp(dstMonId, expOrRankMod, this_->experience);
        break;
    case 5: {
        const double mul = _CrExpo_::GetCreatureUpgradeMultiplier(srcMonId);
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
            this_->experience = _CrExpo_::GetCreatureExpByRankAndExp(dstMonId, expOrRankMod, 0);
            break;
        }
        // NO BREAK!!
    case 11: {
        if (!new_mon_number)
        {
            this_->experience = 0;
            break;
        }
        const int dstExp = _CrExpo_::GetCreatureExpByRankAndExp(dstMonId, expOrRankMod, 0);
        this_->experience = SafeBlend(this_->experience, old_mon_number, dstExp, new_mon_number);
        break;
    }
    case 12:
        this_->experience = _CrExpo_::GetCreatureExpByRankAndExp(dstMonId, expOrRankMod, 0);
        break;
    case 13: {
        const int dstLimit = _CrExpo_::GetCreatureExpLimit(dstMonId);
        const int srcLimit = _CrExpo_::GetCreatureExpLimit(srcMonId);
        this_->experience =
            ClampExp(static_cast<int64_t>(expOrRankMod) + SafeMulDiv(dstLimit, this_->experience, srcLimit));
        break;
    }
    case 14: {
        const int dstLimit = _CrExpo_::GetCreatureExpLimit(dstMonId);
        const int srcLimit = _CrExpo_::GetCreatureExpLimit(srcMonId);
        const int scaledExpo = SafeMulDiv(dstLimit, this_->experience, srcLimit);
        this_->experience = _CrExpo_::GetCreatureExpByRankAndExp(dstMonId, expOrRankMod, scaledExpo);
        break;
    }
    default:
        return;
    }
    // if (dstMonId >= 0)
    //   this_->Clamp(dstMonId);
}

_LHF_(WoG_CrExpo_Recalc)
{
    _CrExpo_ *exp = *reinterpret_cast<_CrExpo_ **>(c->ebp - 0x4);
    const int creatureNum = IntAt(c->ebp + 0x8);
    exp->experience = static_cast<int>(static_cast<INT64>(exp->experience) * exp->number / creatureNum);
    exp->number = creatureNum;

    c->return_address = 0x07184D5;
    return NO_EXEC_DEFAULT;
}
_LHF_(WoG_SetNewExp_AtExpPush)
{
    const int sourceExpoOffset = IntAt(c->ebp - 0x4);
    const int dstExpoOffset = IntAt(c->ebp - 0x10);
    const int sourceNumber = IntAt(c->ebp + 0x20);
    const int dstNumber = IntAt(c->ebp + 0x24);

    auto *expTable = reinterpret_cast<_CrExpo_ *>(0x0860550);

    INT64 totalNewExp = static_cast<INT64>(expTable[sourceExpoOffset].experience) * sourceNumber;
    totalNewExp += static_cast<INT64>(expTable[dstExpoOffset].experience) * dstNumber;
    totalNewExp /= static_cast<INT64>(sourceNumber + dstNumber);

    c->eax = static_cast<int>(totalNewExp);

    c->return_address = 0x0719BEA;
    return NO_EXEC_DEFAULT;
}

_LHF_(WoG_CreatureSplitDlg_AtSourceExp)
{
    auto &srcExpo = *reinterpret_cast<_CrExpo_ *>(0x28602AC);
    auto &dstExpo = *reinterpret_cast<_CrExpo_ *>(0x2860294);

    const int srcNum = IntAt(c->ebp - 0x10);

    const int baseNum = IntAt(0x282A77C);
    INT64 totalNewExp = (static_cast<INT64>((srcNum - baseNum)) * srcExpo.experience +
                         static_cast<INT64>(baseNum) * dstExpo.experience) /
                        srcNum;
    IntAt(c->ebp - 0x20) = static_cast<int>(totalNewExp);
    c->return_address = 0x07662D9;
    return NO_EXEC_DEFAULT;
}
_LHF_(WoG_CreatureSplitDlg_AtDestExp)
{
    auto &srcExpo = *reinterpret_cast<_CrExpo_ *>(0x28602AC);
    auto &dstExpo = *reinterpret_cast<_CrExpo_ *>(0x2860294);

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

_LHF_(WoG_InTowmArmyMerge_GuardIterator)
{
    const int slotId = c->eax;

    // сохраняем значения опыта и количества существ для армии защитника
    armySlots[slotId].experience = IntAt(c->ebp - 0x10C);
    armySlots[slotId].number = reinterpret_cast<_Army_ *>(0x2846BF0)->count[slotId];

    // заменить значения опыта существа на уникальный индекс для сложения в битсет
    c->ecx = 1 << slotId;

    return EXEC_DEFAULT;
}

_LHF_(WoG_InTowmArmyMerge_VisitorIterator)
{
    const int slotId = c->ecx + 7;

    // сохраняем значения опыта и количества существ для армии визитёра
    armySlots[slotId].experience = IntAt(c->ebp - 0x10C);
    armySlots[slotId].number = reinterpret_cast<_Army_ *>(0x2846C28)->count[slotId - 7];

    // заменить значения опыта существа на уникальный индекс для сложения в битсет
    c->edx = 1 << slotId;

    return EXEC_DEFAULT;
}
_LHF_(WoG_InTowmArmyMerge_ExperienceCreatorIterator)
{
    INT64 resultExp = 0;

    _Army_ &expArmy = *reinterpret_cast<_Army_ *>(c->ebp - 0xC8);

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
BOOL GetArmyExperience(const eExpType type, _CrExpo_::UniData baseData, _CrExpo_ *crexps[7]);

static void DebugArmy(const char *armyName, const _Army_ *army, _CrExpo_ **crexpos = nullptr)
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
    o_MsgBox((char *)msg.c_str());
}
inline static void DebugHero(const _Hero_ *hero, const char *name = nullptr, const int withExp = true)
{
#ifdef _DEBUG

    if (hero->id != HID_DEEMER)
    {
        return;
    }
    _CrExpo_ *crexps[7] = {0};
    _CrExpo_ **p_crexps = crexps;

    if (withExp)
    {
        _CrExpo_::UniData basePlace;
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

_Army_ sourceArmyCopy, targetArmyCopy;

struct ExperienceRecalcContext
{

    _Army_ armyCopy;
    _CrExpo_ *crexps[7];
    _CrExpo_::UniData basePlace;

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

        const int take = min(p.count, remaining);

        const int64_t takeMass = (p.expMass * take) / p.count;
        totalExp += takeMass;

        p.count -= take;
        remaining -= take;
    }

    out.taken = need - remaining;
    out.totalExp = totalExp;
}

static int BuildPool(PoolItem *pool, const _Army_ &army, _CrExpo_ *crexpos[7])
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

static void RemapArmyFromPool(PoolItem *pool, const int poolSize, const _Army_ *newArmy, const eExpType expType,
                              const _CrExpo_::UniData basePlace)
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

        _CrExpo_::UniData data = basePlace;

        if (expType == CE_HERO)
        {
            data.hero.slot = i;
        }
        else
        {
            data.anyGarrison.slot = i;
        }

        const int offset = _CrExpo_::SetNewAndClamp(expType, data, type, count, 0);

        if (offset >= 0)
        {
            _CrExpo_ *exp = &reinterpret_cast<_CrExpo_ *>(0x0860550)[offset];
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

BOOL GetArmyExperience(const eExpType type, _CrExpo_::UniData baseData, _CrExpo_ *crexps[7])
{
    switch (type)
    {
    case CE_HERO:
        for (size_t i = 0; i < 7; i++)
        {
            baseData.hero.slot = static_cast<short>(i);
            crexps[i] = _CrExpo_::Find(CE_HERO, baseData);
        }
        break;
    case CE_TOWN:
    case CE_MINE:
    case CE_HORN:
        for (size_t i = 0; i < 7; i++)
        {
            baseData.anyGarrison.slot = static_cast<short>(i);
            crexps[i] = _CrExpo_::Find(type, baseData);
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

// корректировка опыта при управлении армией ИИ в городе
void __stdcall AI_Player_Hero_ManageArmyInTown(HiHook *h, DWORD *aiData, _Hero_ *targetHero, _Town_ *town)
{

    // если включён опыт существ
    const BOOL stackExpIsEnabled = IntAt(0x2772730);

    // DebugHero(targetHero, "AI_Player_Hero_ManageArmyInTown");

    if (stackExpIsEnabled)
    {
        memcpy(&sourceArmyCopy, town->GetUpArmy(), sizeof(_Army_)); // сохраняем данные армии города до обмена
        memcpy(&targetArmyCopy, &targetHero->army, sizeof(_Army_)); // сохраняем данные армии героя до обмена
    }
    // вызываем оригинальную функцию для обмена армиями
    CALL_3(void, __thiscall, h->GetDefaultFunc(), aiData, targetHero, town);

    if (stackExpIsEnabled)
    {
        if (memcmp(&sourceArmyCopy, town->GetUpArmy(), sizeof(_Army_)) == 0 &&
            memcmp(&targetArmyCopy, &targetHero->army, sizeof(_Army_)) == 0)
            return; // если армия не изменилась, то не нужно ничего делать

        _CrExpo_::UniData sourceArmyData{}, targetArmyData{};

        eExpType sourceArmyType;
        if (town->up_hero_id > -1)
        {
            sourceArmyData.hero.id = town->up_hero_id; // герой-защитник
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

        _CrExpo_ *sourceArmyExp[7]{}, *targetArmyExp[7]{};

        GetArmyExperience(sourceArmyType, sourceArmyData, sourceArmyExp);
        GetArmyExperience(CE_HERO, targetArmyData, targetArmyExp);

        PoolItem pool[14];
        int poolSize = BuildPool(pool, sourceArmyCopy, sourceArmyExp);
        poolSize += BuildPool(&pool[poolSize], targetArmyCopy, targetArmyExp);

        // 🔻 герой
        RemapArmyFromPool(pool, poolSize, town->GetUpArmy(), sourceArmyType, sourceArmyData);
        RemapArmyFromPool(pool, poolSize, &targetHero->army, CE_HERO, targetArmyData);
    }
    // DebugHero(targetHero);
}

struct
{
    BOOL enabled = FALSE;
    _Hero_ *hero = nullptr;
    _Army_ armyCopy;

  public:
    void BeforeUpgrade(_Hero_ *targetHero)
    {
        if (enabled = IntAt(0x2772730)) // если включён опыт существ
        {
            hero = targetHero;
            memcpy(&armyCopy, &targetHero->army, sizeof(_Army_)); // сохраняем данные армии героя до улучшения
        }
        // DebugHero(targetHero, "BeforeUpgrade");
    }

    void AfterUpgrade()
    {
        if (!enabled)
            return;

        if (memcmp(&armyCopy, &hero->army, sizeof(_Army_)) == 0)
        {
            enabled = FALSE;
            return; // если армия не изменилась, то не нужно ничего делать
        }
        _CrExpo_::UniData data{};
        data.hero.id = hero->id;
        _CrExpo_ *targetArmyExp[7]{};
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
                                                  CALL_1(float, __cdecl, 0x0727E00,
                                                         oldType)); // получение мультипликатора опыта при улучшении
                    }
                }
                data.hero.slot = static_cast<short>(i);
                // создаём новый слот с типом существа после улучшения
                const int offset = _CrExpo_::SetNewAndClamp(CE_HERO, data, newType, hero->army.count[i], newExp);
            }
        }
        // DebugHero(hero, "AfterUpgrade");
    }
} armyUpgrader;
// хук перед самим улучшением армий
_LHF_(AI_enter_town_before_creatures_upgrade)
{
    armyUpgrader.BeforeUpgrade(reinterpret_cast<_Hero_ *>(c->edi));
    return EXEC_DEFAULT;
}
// хук после самого улучшения армий
_LHF_(AI_enter_town_after_creatures_upgrade)
{
    armyUpgrader.AfterUpgrade();
    return EXEC_DEFAULT;
}
// хук при улучшения армии ИИ-героя в форте на холме
void __stdcall AI_AdvMgr_HillFort(HiHook *h, _Hero_ *targetHero)
{
    armyUpgrader.BeforeUpgrade(targetHero);
    CALL_1(void, __thiscall, h->GetDefaultFunc(), targetHero);
    armyUpgrader.AfterUpgrade();
}

// покупка существа во городском жилище жилище
void __stdcall AI_Town_BuyCreatures(HiHook *h, DWORD _this, _Town_ *town)
{

    // DebugHero(o_GameMgr->GetHero(town->up_hero_id), "AI_Town_BuyCreatures up_hero_id");
    // DebugHero(o_GameMgr->GetHero(town->down_hero_id), "AI_Town_BuyCreatures down_hero_id");

    const BOOL stackExpIsEnabled = IntAt(0x2772730);
    if (stackExpIsEnabled)
    {
        memcpy(&targetArmyCopy, town->GetUpArmy(), sizeof(_Army_)); // сохраняем данные армии города до обмена
    }

    CALL_2(void, __thiscall, h->GetDefaultFunc(), _this, town);

    if (stackExpIsEnabled)
    {
        if (memcmp(&targetArmyCopy, town->GetUpArmy(), sizeof(_Army_)) == 0)
            return; // если армия не изменилась, то не нужно ничего делать

        _CrExpo_::UniData targetArmyData{};
        eExpType targetArmyType;
        if (town->up_hero_id > -1)
        {
            targetArmyData.hero.id = town->up_hero_id; // герой-защитник
            targetArmyType = CE_HERO;
            // DebugHero(o_GameMgr->GetHero(town->up_hero_id), "AI_Town_BuyCreatures");
        }
        else
        {
            targetArmyType = CE_TOWN;
            targetArmyData.town.x = town->x;
            targetArmyData.town.y = town->y;
            targetArmyData.town.z = town->z;
        }

        _CrExpo_ *targetArmyExp[7]{};
        GetArmyExperience(targetArmyType, targetArmyData, targetArmyExp);
        PoolItem pool[7];
        const int poolSize = BuildPool(pool, targetArmyCopy, targetArmyExp);
        RemapArmyFromPool(pool, poolSize, town->GetUpArmy(), targetArmyType, targetArmyData);
    }
}

// корректировка опыта для ИИ при покупке существ во внешних жилищах
void __stdcall AI_H3Hero_BuyDwellingCreatures(HiHook *h, _Hero_ *targetHero, _Dwelling_ *dwelling)
{

    const BOOL stackExpIsEnabled = IntAt(0x2772730);
    if (stackExpIsEnabled)
    {
        memcpy(&targetArmyCopy, &targetHero->army, sizeof(_Army_)); // сохраняем данные армии города до обмена
        // DebugHero(targetHero, "AI_H3Hero_BuyDwellingCreatures");
    }

    CALL_2(void, __fastcall, h->GetDefaultFunc(), targetHero, dwelling);

    if (stackExpIsEnabled)
    {
        if (memcmp(&targetArmyCopy, &targetHero->army, sizeof(_Army_)) == 0)
            return; // если армия не изменилась, то не нужно ничего делать
        _CrExpo_::UniData targetArmyData{};
        targetArmyData.hero.id = targetHero->id;

        _CrExpo_ *targetArmyExp[7]{};
        GetArmyExperience(CE_HERO, targetArmyData, targetArmyExp);
        PoolItem pool[7];
        const int poolSize = BuildPool(pool, targetArmyCopy, targetArmyExp);
        RemapArmyFromPool(pool, poolSize, &targetHero->army, CE_HERO, targetArmyData);
        // DebugHero(targetHero, "AI_H3Hero_BuyDwellingCreatures");
    }
}

// корректировка опыта для ИИ при присоединении одного существа извне
void __stdcall AI_H3Hero_JoinOneCreature(HiHook *h, _Hero_ *targetHero, const int creatureId, WORD monNum)
{
    //   DebugHero(targetHero, "AI_H3Hero_JoinOneCreature");

    const BOOL stackExpIsEnabled = IntAt(0x2772730);
    if (stackExpIsEnabled)
    {
        memcpy(&targetArmyCopy, &targetHero->army, sizeof(_Army_)); // сохраняем данные армии города до обмена
    }

    CALL_3(void, __fastcall, h->GetDefaultFunc(), targetHero, creatureId, monNum);

    if (stackExpIsEnabled)
    {
        if (memcmp(&targetArmyCopy, &targetHero->army, sizeof(_Army_)) == 0)
            return; // если армия не изменилась, то не нужно ничего делать
        _CrExpo_::UniData targetArmyData{};

        _CrExpo_ *targetArmyExp[7]{};
        targetArmyData.hero.id = targetHero->id;
        GetArmyExperience(CE_HERO, targetArmyData, targetArmyExp);
        PoolItem pool[7];
        const int poolSize = BuildPool(pool, targetArmyCopy, targetArmyExp);
        RemapArmyFromPool(pool, poolSize, &targetHero->army, CE_HERO, targetArmyData);
    }
    // DebugHero(targetHero, "AI_H3Hero_JoinOneCreature");
}
// обмен армиями между ИИ-героем и гарнизоном
void __stdcall AI_H3Hero_VisitGarrison(HiHook *h, _Hero_ *targetHero, _H3Garrison_ *sourceGarrison)
{
    // если включе опыт существ
    const BOOL applyChangesToAI =
        IntAt(0x2772730) && sourceGarrison->armyRemovable && sourceGarrison->owner == targetHero->owner_id;
    // DebugHero(targetHero, "AI_H3Hero_VisitGarrison");
    if (applyChangesToAI)
    {
        memcpy(&sourceArmyCopy, &sourceGarrison->army, sizeof(_Army_)); // сохраняем данные армии гарнизона до обмена
        memcpy(&targetArmyCopy, &targetHero->army, sizeof(_Army_));     // сохраняем данные армии героя до обмена
    }
    // вызываем оригинальную функцию для обмена армиями
    CALL_2(void, __fastcall, h->GetDefaultFunc(), targetHero, sourceGarrison);

    if (applyChangesToAI)
    {
        if (memcmp(&sourceArmyCopy, &sourceGarrison->army, sizeof(_Army_)) == 0 &&
            memcmp(&targetArmyCopy, &targetHero->army, sizeof(_Army_)) == 0)
            return; // если армия не изменилась, то не нужно ничего делать

        _CrExpo_::UniData sourceArmyData{}, targetArmyData{};
        sourceArmyData.garrison.x = sourceGarrison->x;
        sourceArmyData.garrison.y = sourceGarrison->y;
        sourceArmyData.garrison.z = sourceGarrison->z;
        targetArmyData.hero.id = targetHero->id; // герой-визитёр

        _CrExpo_ *sourceArmyExp[7]{}, *targetArmyExp[7]{};
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
void __stdcall AI_ArmyExchanging_ExchangeArmy(HiHook *h, DWORD *aiData, _Hero_ *targetHero, _Army_ *sourceArmy,
                                              _Hero_ *sourceHero, BOOL hasAlliance)
{
    const BOOL applyChangesToAI = IntAt(0x2772730) && sourceHero;
    if (applyChangesToAI)
    {
        memcpy(&sourceArmyCopy, sourceArmy, sizeof(_Army_));        // сохраняем данные армии визитёра до обмена
        memcpy(&targetArmyCopy, &targetHero->army, sizeof(_Army_)); // сохраняем данные армии источника до обмена
    }
    // DebugHero(targetHero);

    CALL_5(void, __thiscall, h->GetDefaultFunc(), aiData, targetHero, sourceArmy, sourceHero, hasAlliance);

    if (applyChangesToAI)
    {
        if (memcmp(&sourceArmyCopy, sourceArmy, sizeof(_Army_)) == 0 &&
            memcmp(&targetArmyCopy, &targetHero->army, sizeof(_Army_)) == 0)
            return; // если армия не изменилась, то не нужно ничего делать

        _CrExpo_::UniData sourceArmyData{}, targetArmyData{};
        sourceArmyData.hero.id = sourceHero->id;
        targetArmyData.hero.id = targetHero->id;

        _CrExpo_ *sourceArmyExp[7]{}, *targetArmyExp[7]{};
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
void __stdcall H3Hero_PutOnMap(HiHook *h, _Hero_ *targetHero, const int playerId, const DWORD pos,
                               const bool resetFlags)
{

    const BOOL applyChangesToAI = IntAt(0x2772730) && resetFlags;

    if (applyChangesToAI)
    {
        memcpy(&targetArmyCopy, &targetHero->army, sizeof(_Army_)); // сохраняем данные армии города до обмена
    }
    // DebugHero(targetHero, "H3Hero_PutOnMap");

    CALL_4(void, __thiscall, h->GetDefaultFunc(), targetHero, playerId, pos, resetFlags);

    if (applyChangesToAI)
    {
        // я решил отключить проверку, чтобы каждая инициализация героя инициализировал и опыт его армии корректно
        //   if (memcmp(&targetArmyCopy, &targetHero->army, sizeof(_Army_)) == 0)
        //       return; // если армия не изменилась, то не нужно ничего делать
        _CrExpo_::UniData targetArmyData{};
        targetArmyData.hero.id = targetHero->id;

        _CrExpo_ *targetArmyExp[7]{};
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
    return o_Spells ? o_Spells[spell].type : 1;
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

    return CALL_3(int, __cdecl, hook->GetDefaultFunc(), stack, resistance, spell);
}

_LHF_(SEMagicResistance_Begin_0075DB2C)
{
    const int spell = *reinterpret_cast<int *>(0x0286021C);
    const bool isHostileSpell = spell >= 0 && spell < SPELL_COUNT && GetSpellTargetType(spell) <= 0;

    applyingSEMagicResistance = isHostileSpell;
    injectingSEMagicResistance = false;
    skipWoGSEMagicResistance = isHostileSpell;

    return EXEC_DEFAULT;
}

_LHF_(SEMagicResistance_Apply_0044A4C7)
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
    const int combinedResistance = CALL_3(int, __cdecl, 0x0071CC3B, stack, baseResistance, spell);
    injectingSEMagicResistance = false;

    spellChance = 1.0f - static_cast<float>(combinedResistance) / 100.0f;

    return EXEC_DEFAULT;
}

_LHF_(SEMagicResistance_Finish_0075DB53)
{
    applyingSEMagicResistance = false;
    injectingSEMagicResistance = false;
    skipWoGSEMagicResistance = false;

    return EXEC_DEFAULT;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CrExpoFixes(PatcherInstance *_PI)
{
    // ОПТИМИЗАЦИИ

    // © daemon_n
    // Оптимизация: убрать повторное вычисление максимального опыта существа при ограничении
    _PI->WriteCodePatch(0x07176D1, "%n", 15); // NOPs вместо повтороного вычисления максимального опыта существа

    // ИСПРАВЛЕНИЯ

    // © daemon_n
    // исправление ошибки с ненайденным опытом существа при открытии диалога присоединения армии к герою
    //  _PI->WriteHiHook(0x05D1620, CALL_, EXTENDED_, THISCALL_, JoinArmyDlgCtor); // добавить опыт существам

    // © daemon_n
    // Ошибка бонуса опыта существ для члена "модификатор", где имеем некорректный тип данных (знаковый) для структуры:
    // Суть в подмене типа копирования символов со знакового на беззнаковое ( MOVSX -> MOVZX )
    _PI->WriteByte(0x71C7A7 + 1, 0xB6);
    _PI->WriteByte(0x71C7B3 + 1, 0xB6);
    _PI->WriteByte(0x71C7D2 + 1, 0xB6);
    // © daemon_n
    // отключить выдачу опыта существа при старте игры, если опыт даётся герою вне битвы
    Era::RegisterHandler(OnGameEnter, "OnGameEnter");
    // © daemon_n
    // Исправлению получения опыта существами, когда максимальный уровень опыта героем достигнут
    _PI->WriteLoHook(0x04E36CE,
                     Hero__AtGiveExperienceAfterLimit); // сохранить опыт героя, который он должен был получить
    _PI->WriteHiHook(0x076B46D, CALL_, EXTENDED_, CDECL_, CrExpoSet_AddExpo); // добавить опыт существам

    // © daemon_n
    // исправление перерасчёта опыта существа при модификации количества существ в отряде через ERM комманды EX или HE:C
    _PI->WriteHiHook(0x071960B, SPLICE_, EXTENDED_, CDECL_, ERM_EX_ValidateSlot);

    // © daemon_n
    // исправление перерасчёта опыта существа при изменении их количества в стеке (при покупке или тп)
    _PI->WriteLoHook(0x07184B0, WoG_CrExpo_Recalc);

    // © daemon_n
    // исправление перерасчёта опыта существа при соединении двух отрядов
    _PI->WriteLoHook(0x0719BE3, WoG_SetNewExp_AtExpPush);

    // © daemon_n
    // исправление расчёта опыта при передаче отрядов через диалог опыта
    _PI->WriteLoHook(0x07662B2, WoG_CreatureSplitDlg_AtSourceExp);
    _PI->WriteLoHook(0x076631A, WoG_CreatureSplitDlg_AtDestExp);

    // © daemon_n
    // исправление расчёта опыта при перемещении героя в город
    _PI->WriteLoHook(0x0759749, WoG_InTowmArmyMerge_GuardIterator);
    _PI->WriteLoHook(0x0759870, WoG_InTowmArmyMerge_VisitorIterator);
    _PI->WriteLoHook(0x0759A24, WoG_InTowmArmyMerge_ExperienceCreatorIterator);

    // !BUG!
    // тестируем отключение сортировки существ в ИИ Армиях, чтобы не ломать опыт
    if (true)
    {
        // забрать Армию из родного города ИИ-героем (если есть очки передвижения)
        _PI->WriteHiHook(0x0525985, CALL_, EXTENDED_, THISCALL_, AI_Player_Hero_ManageArmyInTown);

        // хук перед улучшением армии ИИ-героя в городе
        _PI->WriteLoHook(0x0525A3C, AI_enter_town_before_creatures_upgrade);

        // хук после улучшения армии ИИ-героя в городе
        _PI->WriteLoHook(0x0525B90, AI_enter_town_after_creatures_upgrade);

        // хук при улучшения армии ИИ-героя в форте на холме
        _PI->WriteHiHook(0x0528090, SPLICE_, EXTENDED_, THISCALL_, AI_AdvMgr_HillFort);

        // покупка существа в городском жилище
        _PI->WriteHiHook(0x0428580, SPLICE_, EXTENDED_, THISCALL_, AI_Town_BuyCreatures);

        // покупка существа во внешнем жилище
        _PI->WriteHiHook(0x0527FE0, SPLICE_, EXTENDED_, FASTCALL_, AI_H3Hero_BuyDwellingCreatures);

        // присоединение существ для ИИ-героя бесплатно
        _PI->WriteHiHook(0x052C140, SPLICE_, EXTENDED_, FASTCALL_, AI_H3Hero_JoinOneCreature);

        // обмен армиями между ИИ-героем и гарнизоном
        _PI->WriteHiHook(0x0524850, SPLICE_, EXTENDED_, FASTCALL_, AI_H3Hero_VisitGarrison);

        // обмен армиями между героями снаружи
        _PI->WriteHiHook(0x0526333, CALL_, EXTENDED_, THISCALL_, AI_ArmyExchanging_ExchangeArmy);
        // обмен армиями между героем в городе
        _PI->WriteHiHook(0x052587F, CALL_, EXTENDED_, THISCALL_, AI_ArmyExchanging_ExchangeArmy);

        // DEBUG
        // _PI->WriteHiHook(0x042C3B0, SPLICE_, EXTENDED_, THISCALL_, AI_ArmyExchanging_ExchangeArmy);

        // помещение героя в на карту -- нужно для покупки и прочее. ПРОВЕРЯТЬ НА СБРОС!
        _PI->WriteHiHook(0x04D7B70, SPLICE_, EXTENDED_, THISCALL_, H3Hero_PutOnMap);
    }

    // © daemon_n
    // исправление расчёта опыта при перемещении отряда существ из слота в слот
    char *srcExpPatch = "8DB0 50058600 8B06 F76D 20 F77E 04 8906 8B4D 20 894E 04"; // слот-источник
    /*
    lea     esi, [WOG__CrExpo__Table + eax]   ; esi = &Table[src]
    mov     eax, [esi]                        ; eax = Expo
    imul    dword ptr [ebp+source_mon_number] ; edx:eax = Expo * src (64-bit)
    idiv    dword ptr [esi+4]                 ; eax = (Expo*src)/Num

    mov     [esi], eax                        ; Table[src].Expo = eax

    mov     ecx, [ebp+source_mon_number]
    mov     [esi+4], ecx                     ; Table[src].Num = src
    */

    _PI->WriteHexPatch(0x0719993, srcExpPatch); // patch experience
    _PI->WriteCodePatch(0x07199A9, "%n", 28);   // nop extra code

    char *dstExpPatch = "8DB1 50058600 8B06 F76D 24 F77E 04 8906 8B4D 24 894E 04"; // слот-назначение
    /*
    lea     esi, [WOG__CrExpo__Table + ecx]   ; esi = &Table[dst]
    mov     eax, [esi]                        ; eax = Expo
    imul    dword ptr [ebp+dst_mon_number] ; edx:eax = Expo * dst (64-bit)
    idiv    dword ptr [esi+4]                 ; eax = (Expo*dst)/Num

    mov     [esi], eax                        ; Table[dst].Expo = eax

    mov     ecx, [ebp+dst_mon_number]
    mov     [esi+4], ecx                     ; Table[dst].Num = dst
    */

    _PI->WriteHexPatch(0x0719A55, dstExpPatch); // patch experience
    _PI->WriteCodePatch(0x0719A6B, "%n", 28);   // nop extra code

    PatcherInstance *instance = _P->GetInstance("WogSEResistanceFix.era");
    if (instance)
        return;
    instance = _P->CreateInstance("WogSEResistanceFix.era");
    if (instance)
    {
        instance->WriteHiHook(0x0071CC3B, SPLICE_, EXTENDED_, CDECL_, CrExpBon_DwarfResist_WoG_0071CC3B);
        instance->WriteLoHook(0x0075DB2C, SEMagicResistance_Begin_0075DB2C);
        instance->WriteLoHook(0x0044A4C7, SEMagicResistance_Apply_0044A4C7);
        instance->WriteLoHook(0x0075DB53, SEMagicResistance_Finish_0075DB53);
    }
}
} // namespace CrExpo
