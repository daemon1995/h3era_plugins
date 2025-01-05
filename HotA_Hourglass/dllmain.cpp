// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"

Patcher *globalPatcher;
PatcherInstance *_PI;

constexpr int ART_SHAMANS_PUPPET = 184;
constexpr int ART_DEMONS_HORSESHOE = 185;
constexpr int ART_RUNES_OF_IMMINENCY = 187;
constexpr int ART_PENDANT_OF_DESPAIR = 252;
constexpr int ART_CLOAK_OF_DEATHS_SHADOW = 282;


enum eSpecialTerrain : __int32
{
    NOT_SPECIAL = 0xFFFFFFFF,
    COAST = 0x0,
    MAGICAL_PLAINS = 0x1,
    CURSED_GROUND = 0x2,
    HOLY_GROUND = 0x3,
    DEVILS_FOG = 0x4,
    CLOVER_FIELD = 0x5,
    TRANSPARENT_PONDS = 0x6,
    FIRE_FIELDS = 0x7,
    ROCKY_LAND = 0x8,
    MAGIC_CLOUDS = 0x9,
};


void __stdcall BattleStack_SetLuck(HiHook *h, H3CombatCreature *stack, H3Hero *hero, H3Army *army, H3Town *town,
                                   H3Hero *oppHero, H3Army *oppArmy,
                                   int specialGround // -1 by default
)
{
    int luckLevel; // esi
    bool isHourglass = false;

    luckLevel = 0;

    if (specialGround == eSpecialTerrain::CURSED_GROUND)
    {
        goto LABEL_18;
    }

    if (army)
        // Luck_Level = Battle_Hero_get_Luck_Level(ownHero, town, oppHero, oppArmy, 0, 0);
        luckLevel = STDCALL_6(int, 0x44AFA0, hero, town, oppHero, oppArmy, 0, 0);

    if (stack->activeSpellDuration[eSpell::FORTUNE])
        luckLevel += stack->fortuneEffect;

    if (specialGround == eSpecialTerrain::CLOVER_FIELD)
    {
        switch (stack->info.town)
        {
        // for "neutral" races
        case 6:
        case 7:
        case 8:
            luckLevel += 2;
            break;
        default:
            break;
        }
    }

    // nullify all positive luck effects
    if (hero && hero->WearsArtifact(eArtifact::HOURGLASS_OF_THE_EVIL_HOUR) //(ownHero && Hero::WearsArtifactId(ownHero,
                                                                           //HOURGLASS_OF_THE_EVIL_HOUR)
        || oppHero && oppHero->WearsArtifact(
                          eArtifact::HOURGLASS_OF_THE_EVIL_HOUR)) // oppHero && Hero::WearsArtifactId(oppHero,
                                                                  // HOURGLASS_OF_THE_EVIL_HOUR))
    {
        luckLevel = 0;
        isHourglass = true;
    }

    if (stack->type != eCreature::HALFLING) // halflings are immune to misfortune and artifacts
    {
        // misfortune
        if (stack->activeSpellDuration[eSpell::MISFORTUNE])
            luckLevel -= stack->misfortuneEffect;

        // artifacts (emerald has lohook somewhere in this function, so it need to be coded again)
        if (oppHero)
        {
            if (oppHero->WearsArtifact(ART_SHAMANS_PUPPET))
                luckLevel -= 2;

            if (oppHero->WearsArtifact(ART_DEMONS_HORSESHOE))
                luckLevel -= 1;

            if (oppHero->WearsArtifact(ART_RUNES_OF_IMMINENCY))
                luckLevel -= 1;

            if (oppHero->WearsArtifact(ART_PENDANT_OF_DESPAIR))
                luckLevel -= 3;

            if (oppHero->WearsArtifact(ART_CLOAK_OF_DEATHS_SHADOW))
                luckLevel -= 2;
        }
    }

    if (stack->type != eCreature::HALFLING || luckLevel >= 1)
    LABEL_18:
        stack->luck = luckLevel;
    else // halfling
    {
        if (!isHourglass)
        {
            stack->luck = 1;
        }
        else
        {
            stack->luck = luckLevel;
        }
    }
}

_LHF_(Hero_GetLuckBonus_BeforeArtHoursGlassCheck)
{
    c->return_address = 0x4E39E8;
    return NO_EXEC_DEFAULT;
}

//H3String *__stdcall Creature_Setup_LuckModif_List(HiHook *h, H3String *basic_string, int monType, int monLuck,
//                                                  H3Hero *hero, H3Town *_town, H3Hero *oppHero, H3Army *oppArmy,
//                                                  int specialGround // -1 by default
//)
//{
//    H3String *v8;             // esi
//    const char *v9;           // ebx
//    H3Town *town;             // edi
//    H3String *v12;            // eax
//    eCreature v13;            // edx
//    int v14;                  // eax
//    H3Army *v15;              // ecx
//    int v16;                  // eax
//    H3Army *v17;              // ecx
//    H3String *v18;            // eax
//    const char *BuildingName; // eax
//    H3String *v20;            // eax
//    H3String *v21;            // eax
//    H3String *v22;            // eax
//    char v23;                 // al
//    char v24;                 // al
//    H3String ZprPo;           // [esp+Ch] [ebp-30h] BYREF
//    H3String FMp;             // [esp+1Ch] [ebp-20h] BYREF
//    int v28;                  // [esp+2Ch] [ebp-10h]
//    int v29;                  // [esp+38h] [ebp-4h]
//    int heroLuck;             // [esp+50h] [ebp+14h]
//
//    v28 = 0;
//    
//    // cursed ground
//
//    if (specialGround == eSpecialTerrain::CURSED_GROUND)
//    {
//        //v8 = basic_string;
//        // v9 = dword_006A53EC;
//        //v9 = reinterpret_cast<const char *>(0x6A53EC);
//        //basic_string->_allocator = HIBYTE(monLuck);
//        // std::string::string(basic_string, 0);
//        //THISCALL_2(void, 0x404130, basic_string, 0);
//        // H3String::Assign(basic_string, v9, strlen((const char*)v9));
//        //THISCALL_3(H3String *, 0x404180, basic_string, v9, strlen((const char *)v9));
//        H3String basic_string(reinterpret_cast<LPCSTR>(0x6A53EC));
//    }
//
//    // hourglass
//
//    else if (hero && hero->WearsArtifact(eArtifact::HOURGLASS_OF_THE_EVIL_HOUR) ||
//             oppHero && oppHero->WearsArtifact(eArtifact::HOURGLASS_OF_THE_EVIL_HOUR))
//    {
//        v8 = basic_string;
//        // vsprintf(basic_string, dword_006A53F0, o_ArtSetup[HOURGLASS_OF_THE_EVIL_HOUR].Name); // 0050C7F0
//        vsprintf(reinterpret_cast<char *>(basic_string), reinterpret_cast<const char *>(0x6A53F0),
//                 P_ArtifactSetup->Get()[eArtifact::HOURGLASS_OF_THE_EVIL_HOUR].name);
//    }
//
//    // normal case
//
//    else
//    {
//        town = _town;
//        // heroLuck = Battle_Hero_get_Luck_Level(ownerHero, _town, hostileHero, oppArmy, 0, 0);
//        heroLuck = STDCALL_6(int, 0x44AFA0, hero, _town, oppHero, oppArmy, 0, 0);
//        FMp._allocator = HIBYTE(monLuck);
//        // std::string::string(&FMp, 0);
//        THISCALL_2(void, 0x404130, &FMp, 0);
//        v29 = 0;
//
//        // rampart grail
//
//        if (hero)
//        {
//            // v12 = Sub_004DCD30_RampartGrailIncreaseLuck(ownerHero, &ZprPo);
//            v12 = THISCALL_2(H3String *, 0x4DCD30, hero, &ZprPo);
//            LOBYTE(v29) = 1;
//            // std::string::_M_append_dispatch(&FMp, v12, 0, MinusOne); // MinusOne (0063A60C)
//            THISCALL_4(int, 0x404810, &FMp, v12, 0, -1);
//            LOBYTE(v29) = 0;
//            // std::string::string(&ZprPo, 1);
//            THISCALL_2(void, 0x404130, &ZprPo, 1);
//        }
//
//        // clover field for neutral races
//
//        if (specialGround == eSpecialTerrain::CLOVER_FIELD &&
//            (P_Game->Get()->mapKind || monType != eCreature::AIR_ELEMENTAL && monType != eCreature::EARTH_ELEMENTAL && monType != eCreature::FIRE_ELEMENTAL && monType != eCreature::WATER_ELEMENTAL))
//        {
//            switch (P_CreatureInformation->Get()[monType].town)
//            {
//            case 6:
//            case 7:
//            case 8:
//                monLuck -= 2; // to avoid bonus viewing as "spells +2"
//                // H3Str_Append_String(&FMp, (const void*)dword_006A53F4, strlen((const char*)dword_006A53F4));
//                THISCALL_3(H3String *, 0x41B2A0, &FMp, reinterpret_cast<const void *>(0x6A53F4),
//                           strlen(reinterpret_cast<const char *>(0x6A53F4)));
//                town = _town;
//                break;
//            default:
//                break;
//            }
//        }
//
//        // devils and archdevils
//
//        if (oppArmy)
//        {
//            v13 = eCreature::UNDEFINED;
//            v14 = 0;
//            v15 = oppArmy;
//            while (v15->type[0] != eCreature::DEVIL)
//            {
//                ++v14;
//                v15 = (H3Army *)((char *)v15 + 4);
//                if (v14 >= 7)
//                    goto LABEL_24;
//            }
//            v13 = eCreature::DEVIL;
//
//        LABEL_24:
//            v16 = 0;
//            v17 = oppArmy;
//            do
//            {
//                if (v17->type[0] == eCreature::ARCH_DEVIL)
//                {
//                    v13 = eCreature::ARCH_DEVIL;
//                    goto LABEL_28;
//                }
//                ++v16;
//                v17 = (H3Army *)((char *)v17 + 4);
//            } while (v16 < 7);
//            if (v13 == -1)
//                goto LABEL_30;
//      
//        LABEL_28:
//            // v18 = vsprintf(&ZprPo, dword_006A58A8, o_pCreatureInfo[v13].name_plural);
//            v18 = sprintf(&ZprPo, reinterpret_cast<char *>(0x6A58A8), P_CreatureInformation->Get()[v13].namePlural);
//            LOBYTE(v29) = 2;
//            // std::string::operator+=(&FMp, v18, 0, MinusOne);
//            THISCALL_4(H3String *, 0x41B1B0, &FMp, v18, 0, -1);
//            LOBYTE(v29) = 0;
//            // std::string::string(&ZprPo, 1);
//            THISCALL_2(void, 0x404130, &ZprPo, 1);
//        }
//
//    LABEL_30:
//
//        // rampart - fountain of luck
//
//        if (town &&
//            town->type == 1
//            //&& __PAIR64__((unsigned int)Modif2Mask2 & town->BuiltBonus1, (unsigned int)Modif2Mask & town->BuiltBonus))
//            && __PAIR64__((unsigned int)DwordAt(0x66CE44) & town->built2 + 0x4, // Here just TownIsBuildingBuilt
//                          (unsigned int)DwordAt(0x66CE40) & town->built2)) // 0x15C
//        {
//            // BuildingName = Town_GetBuildingName(1, 21);
//            BuildingName = FASTCALL_2(const char *, 0x460CC0, 1, 21);
//            v20 = vsprintf(&ZprPo, "\n%s +2", BuildingName);  // offset aS2 00660B40
//            LOBYTE(v29) = 3;
//            // std::string::operator+=(&FMp, v20, 0, MinusOne);
//            THISCALL_4(H3String *, 0x41B1B0, &FMp, v20, 0, -1);
//            LOBYTE(v29) = 0;
//            // std::string::string(&ZprPo, 1);
//            THISCALL_2(void, 0x404130, &ZprPo, 1);
//        }
//
//        // halfling
//
//        if (monType == eCreature::HALFLING && heroLuck < 1)
//        {
//            v21 = vsprintf(&ZprPo, "%s are always lucky", // offset aSAreAlwaysLuck 00660B50
//                           P_CreatureInformation->Get()[eCreature::HALFLING].namePlural); // полурослики
//            LOBYTE(v29) = 4;
//            // std::string::operator+=(&FMp, v21, 0, MinusOne);
//            THISCALL_4(H3String *, 0x41B1B0, &FMp, v21, 0, -1);
//            LOBYTE(v29) = 0;
//            // std::string::string(&ZprPo, 1);
//            THISCALL_2(void, 0x404130, &ZprPo, 1);
//            heroLuck = 1;
//        }
//
//        // spells (delta between monLuck and heroLuck)
//
//        if (monLuck != heroLuck)
//        {
//            // v22 = vsprintf(&ZprPo, dword_006A58B4, monLuck - heroLuck);
//            v22 = vsprintf(&ZprPo, reinterpret_cast<char *>(0x6A58B4), monLuck - heroLuck);
//            LOBYTE(v29) = 5;
//            // std::string::operator+=(&FMp, v22, 0, MinusOne);
//            THISCALL_4(H3String *, 0x41B1B0, &FMp, v22, 0, -1);
//            LOBYTE(v29) = 0;
//            if (ZprPo.m_string)
//            {
//                v23 = *(ZprPo.m_string - 1);
//                if (!v23 || v23 == -1)
//                    // MemFree(ZprPo.m_string - 1);
//                    CDECL_1(void, 0x60B0F0, ZprPo.m_string - 1);
//                else
//                    *(ZprPo.m_string - 1) = v23 - 1;
//            }
//        }
//
//        v8 = basic_string;
//        basic_string->_allocator = FMp._allocator;
//        basic_string->m_string = 0;
//        basic_string->m_length = 0;
//        basic_string->m_capacity = 0;
//        // std::string::_M_append_dispatch(basic_string, &FMp, 0, MinusOne);
//        THISCALL_4(int, 0x404810, basic_string, &FMp, 0, -1);
//
//        if (FMp.m_string)
//        {
//            v24 = *(FMp.m_string - 1);
//            if (!v24 || v24 == -1)
//                // MemFree(FMp.m_string - 1);
//                CDECL_1(void, 0x60B0F0, FMp.m_string - 1);
//            else
//                *(FMp.m_string - 1) = v24 - 1;
//        }
//    }
//
//    return v8;
//}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static BOOL plugin_On = 0;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        //!< Attach process, initialize the plugin if it hasn't been initialized before

        //! Check if the plugin is already on
        if (!plugin_On)
        {
            plugin_On = 1;

            //! Get the global patcher
            globalPatcher = GetPatcher();

            //! Create an instance of the plugin
            _PI = globalPatcher->CreateInstance("js_HotA_Hourglass");

            // подсчет удачи стека отряда
            _PI->WriteHiHook(0x43DC40, THISCALL_, BattleStack_SetLuck);

            // скип чека часов в расчете геройской удачи
            _PI->WriteLoHook(0x4E395C, Hero_GetLuckBonus_BeforeArtHoursGlassCheck);

            // подсчет удачи стека отряда (чек героя и опп героя) ВНЕ БИТВЫ
            _PI->WriteHexPatch(0x44AFC6, "9090");
            _PI->WriteHexPatch(0x44AFDA, "EB");

            // подсчет удачи стека отряда (чек героя и опп героя) В БИТВЕ (не нужно, т.к. уже написан хайхук выше BattleStack_SetLuck)
            //_PI->WriteHexPatch(0x43DC66, "909090909090");
            //_PI->WriteHexPatch(0x43DC7E, "9090");

            // desc luck bonuses in dlg battlestack
            //_PI->WriteHiHook(0x44BE90, STDCALL_, Creature_Setup_LuckModif_List);

            // скип описания часов в dlg стека (будет не нужно после реализации хайхука выше Creature_Setup_LuckModif_List)
            _PI->WriteHexPatch(0x44BF02, "9090");
            _PI->WriteHexPatch(0x44BF16, "EB");
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
