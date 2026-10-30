/////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// ПАТЧИ ////////////////////////////////////////////////
namespace scouting
{
bool scoutingCheckAfterCombat = false;
typedef _int8_ INT8;

typedef _int16_ INT16;
typedef _Hero_ H3Hero;
static inline void OpenArea(INT8 playerNum, INT16 x, INT16 y, INT16 z, int radius, int a6)
{
    // Тут еще какая-то H3NetworkData формируется
    // if (a6 && MEMORY[0x69959C] && !MEMORY[0x698450])// a6 && o_NetworkGame && !StartGame_SkipDialogs
    //{
    //    v10.recipientId = -1;
    //    v10._f_04 = 0;
    //    v10.bufferSize = 32;
    //    v10.Data[0] = 0;
    //    v10.msgId = 1021;
    //    v10.Data[3] = radius;
    //    LOWORD(v9) = ((x ^ v8) & 0x3FF ^ v8) & 0x3FF;
    //    HIWORD(v9) = y & 0x3FF | ((z & 0xF) << 10) & 0x3FFF;
    //    v10.Data[1] = v9;
    //    v10.Data[2] = owner;
    //    MEMORY[0x481CB0](&v10);                     // H3NetworkData::SendShortCommand
    //    v6 = x;
    //}

    // H3Game::OpenArea
    //->
    CALL_7(bool, __thiscall, 0x49CDD0, o_GameMgr, x, y, z, playerNum, radius, 0);
}

static inline void OpenArea(H3Hero *hero)
{
    if (hero->owner_id <= 7u && hero->x <= 255u && hero->y <= 255u && hero->z <= 8u)
    {
        int radius = CALL_1(int, __thiscall, 0x4E42E0, hero);
        OpenArea(hero->owner_id, hero->x, hero->y, hero->z, radius, 1);
    }
}

_LHF_(L_AfterHeroAppearOnMap)
{
    H3Hero *hero = reinterpret_cast<H3Hero *>(c->esi);
    OpenArea(hero);

    return EXEC_DEFAULT;
}

void __stdcall H_Hero_CheckLevelUps(HiHook *h, H3Hero *hero)
{
    int level = hero->level;
    CALL_1(void, __thiscall, h->GetDefaultFunc(), hero);

    if (hero->level > level)
    {
        OpenArea(hero);
    }
}

void __stdcall HiHook_00476da0(HiHook *h, _BattleMgr_ *bm, int sideWinner)
{
    scoutingCheckAfterCombat = 1;
    CALL_2(void, __thiscall, h->GetDefaultFunc(), bm, sideWinner);
    scoutingCheckAfterCombat = 0;
}

int __stdcall HeroGetSSkill(HiHook *h, H3Hero *hero, int skillId, int value)
{
    const int result = CALL_3(int, __thiscall, h->GetDefaultFunc(), hero, skillId, value);
    if (result && skillId == HSS_SCOUTING)
    {
        OpenArea(hero);
    }

    return result;
}

bool __stdcall H_Hero_AddArt(HiHook *h, H3Hero *hero, _Artifact_ *art, int slot)
{
    const bool result = CALL_3(bool, __thiscall, h->GetDefaultFunc(), hero, art, slot);

    if (result && (art->id == AID_SPECULUM || art->id == AID_SPYGLASS))
    {
        OpenArea(hero);
    }
    return result;
}

_LHF_(LoHook_004ad4cc)
{
    H3Hero *hero = *reinterpret_cast<H3Hero **>(c->ebp - 0x24);
    if (hero)
    {
        OpenArea(hero);
    }

    H3Hero *oppHero = *reinterpret_cast<H3Hero **>(c->ebp - 0x28);
    if (oppHero)
    {
        OpenArea(hero);
    }

    return EXEC_DEFAULT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void ScoutingRadiusFixes(PatcherInstance *_PI)
{
    // Исправление бага с неоткрывающейся картой при получении разведки, выкупе героя из таверны и пр.

    // from external tavern + from castle
    _PI->WriteLoHook(0x4D7C4B, L_AfterHeroAppearOnMap);

    // from prison
    _PI->WriteLoHook(0x4A3D9A, L_AfterHeroAppearOnMap);

    // Обертка над H3Hero::CheckLevelUps
    _PI->WriteHiHook(0x4DA990, SPLICE_, EXTENDED_, THISCALL_, H_Hero_CheckLevelUps);

    // Обертка над H3CombatManager::SetWinner
    _PI->WriteHiHook(0x476da0, SPLICE_, EXTENDED_, THISCALL_, HiHook_00476da0);

    // on hero get sec. skill ( check on scouting )
    _PI->WriteHiHook(0x04E2540, SPLICE_, EXTENDED_, THISCALL_, HeroGetSSkill);

    // Обертка над H3Hero::EquipArtifact
    _PI->WriteHiHook(0x4E2C70, SPLICE_, EXTENDED_, THISCALL_, H_Hero_AddArt);

    // H3AdventureManager::StartBattle
    _PI->WriteLoHook(0x4ad4cc, LoHook_004ad4cc);
}
} // namespace scouting
