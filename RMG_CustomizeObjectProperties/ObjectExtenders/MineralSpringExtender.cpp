#include "../pch.h"
namespace mineralSpring
{
MineralSpringExtender::MineralSpringExtender() : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.WateringPlaceExtender.daemon_n"))
{

    CreatePatches();
}

MineralSpringExtender::~MineralSpringExtender()
{
}

_LHF_(MineralSpringExtender::AIHero_GetObjectPosWeight)
{
    // TODO: use OASIS???? 00528567; FOUNTAIN_OF_FORTUNE 00528567

    //if (H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->esi))
    //{
    //    if (auto wateringPlace = H3MapItemWateringPlace::GetFromMapItem(mapItem))
    //    {
    //        const H3Hero *hero = reinterpret_cast<H3Hero *>(c->ebx);
    //        const bool isVistedByHero = H3MapItemWateringPlace::IsVisitedByHero(*wateringPlace, hero);

    //        if (!isVistedByHero)
    //        {
    //            if (P_ActivePlayer->playerResources.gold >= GOLD_REQUIRED)
    //            {
    //                // адрес похожего псевдокода 0052BB89
    //                const __int64 aiExperience = EXP_GIVEN * hero->AI_experienceEffectiveness;
    //                const H3Player *player = *reinterpret_cast<H3Player **>(c->ebp - 0x4);
    //                const int aiResWeight =
    //                    (__int64)((double)aiExperience - player->resourceImportance[eResource::GOLD] * GOLD_REQUIRED);

    //                if (aiResWeight)
    //                {
    //                    c->eax = aiResWeight;
    //                    c->return_address = 0x05285A1;
    //                    return NO_EXEC_DEFAULT;
    //                }
    //            }
    //        }
    //    }
    //}

    return EXEC_DEFAULT;
}

BOOL H3MapItemMineralSpring::IsVisitedByHero(const H3Hero *hero) noexcept
{
    sprintf(h3_TextBuffer, ErmVariableFormat, hero->id);

    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

void ShowMessage(const H3MapItem *mapItem, const bool isVisitedByHero)
{
    const bool skipMapMessage = globalPatcher->VarValue<int>("HD.UI.AdvMgr.SkipMapMsgs");

    H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));

    if (isVisitedByHero)
    {
        objName.Append(EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.text.visited", mapItem->objectType, mapItem->objectSubtype)
            .String()));

        if (skipMapMessage)
        {
            THISCALL_4(void, 0x415FC0, P_AdventureMgr->Get(), objName.String(), -1, -1);
        }
        else
        {
            H3Messagebox::Show(objName);
        }
    }
    else
    {
        objName.Append(EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.text.visit", mapItem->objectType, mapItem->objectSubtype)
            .String()));

        if (skipMapMessage)
        {
            // TODO: нужно отображение удачи
            // Hero_LuckFountain_Visit
            // 004A20F0
            THISCALL_4(void, 0x415FC0, P_AdventureMgr->Get(), objName.String(), -1, -1);
        }
        else
        {
            H3Messagebox::Show(objName);
        }
    }
}

_LHF_(MineralSpringExtender::H3AdventureManager__ObjectVisit)
{
    if (auto mapItem = reinterpret_cast<H3MapItem *>(c->edi))
    {
        if (auto mineralSpring = H3MapItemMineralSpring::GetFromMapItem(mapItem))
        {
            H3Hero* hero = *reinterpret_cast<H3Hero**>(c->ebp + 0x8);
            const char isHuman = CharAt(c->ebp + 0x14);
            const bool isVisitedByHero = H3MapItemMineralSpring::IsVisitedByHero(hero);

            if (!isVisitedByHero)
            {
                hero->movement += MOVE_POINTS_GIVEN;
                hero->luckBonus += LUCK_GIVEN;
                THISCALL_4(void, 0x04032E0, P_AdventureManager->dlg, -1, 1, 1); // update screen
                sprintf(h3_TextBuffer, H3MapItemMineralSpring::ErmVariableFormat, hero->id); // получение имени переменной
                Era::SetAssocVarIntValue(h3_TextBuffer, 1); // отметить переменную, что объект посещен
                
                //return EXEC_DEFAULT;
            }

            if (isHuman)
            {
                ShowMessage(mapItem, isVisitedByHero);
            }
        }
    }

    return EXEC_DEFAULT;
}

BOOL ShowHint(LoHook *h, HookContext *c, LPCSTR delimiter)
{
    H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->ebx);

    if (const auto wateringPlace = H3MapItemMineralSpring::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVistedByHero = H3MapItemMineralSpring::IsVisitedByHero(hero);
            sprintf(h3_TextBuffer, "%s%s", delimiter, P_GeneralText->GetText(isVistedByHero ? 354 : 355));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        c->edi = (int)h3_TextBuffer;
        c->return_address = h->GetAddress() + 7;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

_LHF_(MineralSpringExtender::H3AdventureManager__GetDefaultObjectClickHint)
{
    return ShowHint(h, c, "\n\n");
}
_LHF_(MineralSpringExtender::H3AdventureManager__GetDefaultObjectHoverHint)
{
    return ShowHint(h, c, " ");
}

void __stdcall OnAfterBattleUniversal(Era::TEvent* event)
{
    for (size_t i = 0; i < 2; i++)
    {
        H3Hero * hero = P_CombatMgr->hero[i];
        if (hero)
        {
            sprintf(h3_TextBuffer, H3MapItemMineralSpring::ErmVariableFormat, hero->id); // получение имени переменной
            Era::SetAssocVarIntValue(h3_TextBuffer, 0); // обнулить переменную
        }
    }
}

_LHF_(MineralSpring_HeroReset)
{
    int heroId = c->edx;  // !!UN:C(hook)/(STRUCT_HOOK_CONTEXT_EDX)/4/?(heroId:y);
    sprintf(h3_TextBuffer, H3MapItemMineralSpring::ErmVariableFormat, heroId); // получение имени переменной
    Era::SetAssocVarIntValue(h3_TextBuffer, 0); // обнулить переменную

    return EXEC_DEFAULT;
}

void MineralSpringExtender::CreatePatches()
{
    if (!m_isInited)
    {
        // Era::RegisterHandler
        // 004BFCBE
        //_pi->WriteLoHook(0x04BFCBE, Game__NewGameBeforeSetObjectsInitialParameters);
        //_pi->WriteLoHook(0x04C0A5F, Game__NewGameObjectIteration);
        //_pi->WriteLoHook(0x04C8847, Game__NewWeekObjectIteration);

        _PI->WriteLoHook(0x4A819C, H3AdventureManager__ObjectVisit);
        _PI->WriteLoHook(0x528559, AIHero_GetObjectPosWeight);

        _PI->WriteLoHook(0x40D052, H3AdventureManager__GetDefaultObjectHoverHint); // mouse over hint
        _PI->WriteLoHook(0x415999, H3AdventureManager__GetDefaultObjectClickHint); // rmc hint

        //_pi->WriteLoHook(0x4C1974, Game__AtShrineOfMagicIncantationSettingSpell);
        //_pi->WriteLoHook(0x40D858, Shrine__AtGetName);

        Era::RegisterHandler(OnAfterBattleUniversal, "OnAfterBattleUniversal"); // строка должна совпадать с неймингом триггера в ERA
        _pi->WriteLoHook(0x4D89B8, MineralSpring_HeroReset); //5081528 = 4D89B8 - hero reset // Обнуляем посещение

        m_isInited = true;
    }
}

inline H3MapItemMineralSpring *H3MapItemMineralSpring::GetFromMapItem(H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == MINERAL_SPRING_OBJECT_TYPE && mapItem->objectSubtype == MINERAL_SPRING_OBJECT_SUBTYPE)
    {
        return reinterpret_cast<H3MapItemMineralSpring*>(&mapItem->setup);
    }

    return nullptr;
}

MineralSpringExtender & MineralSpringExtender::Get()
{
    static MineralSpringExtender _instance;
    return _instance;
}
}
