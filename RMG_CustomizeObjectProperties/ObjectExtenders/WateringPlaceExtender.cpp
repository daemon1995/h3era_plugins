#include "../pch.h"
namespace wateringPlace
{
WateringPlaceExtender::WateringPlaceExtender() : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.WateringPlaceExtender.daemon_n"))
{

    CreatePatches();
}

WateringPlaceExtender::~WateringPlaceExtender()
{
}

_LHF_(WateringPlaceExtender::AIHero_GetObjectPosWeight)
{
    // TODO: use SWAN POND???? 00528567

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

BOOL H3MapItemWateringPlace::IsVisitedByHero(const H3Hero *hero) noexcept
{
    sprintf(h3_TextBuffer, ErmVariableFormat, hero->id);

    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

void ShowMessage(const H3MapItem *mapItem, const bool isVistedByHero)
{
    const bool skipMapMessage = globalPatcher->VarValue<int>("HD.UI.AdvMgr.SkipMapMsgs");

    H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));

    if (skipMapMessage)
    {
        if (isVistedByHero)
        {
            objName.Append(EraJS::read(
                H3String::Format("RMG.objectGeneration.%d.%d.text.visited", mapItem->objectType, mapItem->objectSubtype)
                    .String()));
        }
        THISCALL_4(void, 0x415FC0, P_AdventureMgr->Get(), objName.String(), -1, -1);
    }

    else
    {
        if (isVistedByHero)
        {
            objName.Append(EraJS::read(
                H3String::Format("RMG.objectGeneration.%d.%d.text.visited", mapItem->objectType, mapItem->objectSubtype)
                    .String()));
        }
        H3Messagebox::Show(objName);
    }
}

_LHF_(WateringPlaceExtender::H3AdventureManager__ObjectVisit)
{
    if (auto mapItem = reinterpret_cast<H3MapItem *>(c->edi))
    {
        if (auto wateringPlace = H3MapItemWateringPlace::GetFromMapItem(mapItem))
        {
            H3Hero* hero = *reinterpret_cast<H3Hero**>(c->ebp + 0x8);
            const char isHuman = CharAt(c->ebp + 0x14);
            const bool isVisitedByHero = H3MapItemWateringPlace::IsVisitedByHero(hero);

            if (!isVisitedByHero)
            {
                BOOL agreed = !isHuman;
                if (isHuman)
                {
                    H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));
                    objName.Append(EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.text.visit",
                                                                mapItem->objectType, mapItem->objectSubtype)
                                                    .String()));
                    agreed = H3Messagebox::Choice(objName);
                }
                if (agreed)
                {
                    hero->movement = 0;
                    hero->maxMovement += MOVE_POINTS_GIVEN;
                    hero->RecalculateMovement();
                    sprintf(h3_TextBuffer, H3MapItemWateringPlace::ErmVariableFormat, hero->id); // получение имени переменной
                    Era::SetAssocVarIntValue(h3_TextBuffer, 1); // отметить переменную, что объект посещен
                }
                return EXEC_DEFAULT;
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

    if (const auto wateringPlace = H3MapItemWateringPlace::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVistedByHero = H3MapItemWateringPlace::IsVisitedByHero(hero);
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

_LHF_(WateringPlaceExtender::H3AdventureManager__GetDefaultObjectClickHint)
{
    return ShowHint(h, c, "\n\n");
}
_LHF_(WateringPlaceExtender::H3AdventureManager__GetDefaultObjectHoverHint)
{
    return ShowHint(h, c, " ");
}

void __stdcall OnEveryDay(Era::TEvent* event)
{
    for (const auto heroId : P_Game->players[P_CurrentPlayerID].heroIDs)
    {
        auto hero = &P_Game->heroes[heroId];

        if (H3MapItemWateringPlace::IsVisitedByHero(hero))
        {
            hero->movement += MOVE_POINTS_GIVEN;
            //hero->RecalculateMovement();
            THISCALL_4(void, 0x04032E0, P_AdventureManager->dlg, -1, 1, 1); // update screen
            sprintf(h3_TextBuffer, H3MapItemWateringPlace::ErmVariableFormat, hero->id); // получение имени переменной
            Era::SetAssocVarIntValue(h3_TextBuffer, 0); // обнулить переменную
        }
    }
    // Для вообще всех героев (в тюрьмах, таверне и т.д.)
    //for (size_t i = 0; i < h3::limits::HEROES; i++)
    //{
    //    auto hero = &P_Game->heroes[i];
    //    if (hero->id == P_CurrentPlayerID)
    //    {
    //        if (H3MapItemWateringPlace::IsVisitedByHero(hero))
    //        {
    //            //hero->maxMovement += MOVE_POINTS_GIVEN; // не работает
    //            //hero->movement += 456456;// MOVE_POINTS_GIVEN; // не работает
    //            hero->maxMovement = 213123;
    //            //hero->RecalculateMovement(); // не работает
    //            sprintf(h3_TextBuffer, H3MapItemWateringPlace::ErmVariableFormat, hero->id); // получение имени переменной
    //            Era::SetAssocVarIntValue(h3_TextBuffer, 0); // обнулить переменную
    //        }
    //    }
    //}
}

_LHF_(WateringPlace_HeroReset)
{
    int heroId = c->edx;  // !!UN:C(hook)/(STRUCT_HOOK_CONTEXT_EDX)/4/?(heroId:y);
    sprintf(h3_TextBuffer, H3MapItemWateringPlace::ErmVariableFormat, heroId); // получение имени переменной
    Era::SetAssocVarIntValue(h3_TextBuffer, 0); // обнулить переменную

    return EXEC_DEFAULT;
}

void WateringPlaceExtender::CreatePatches()
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

        Era::RegisterHandler(OnEveryDay, "OnEveryDay"); // строка должна совпадать с неймингом триггера в ERA
        _pi->WriteLoHook(0x4D89B8, WateringPlace_HeroReset); //5081528 = 4D89B8 - hero reset // Обнуляем посещение

        m_isInited = true;
    }
}

inline H3MapItemWateringPlace *H3MapItemWateringPlace::GetFromMapItem(H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == WATERING_PLACE_OBJECT_TYPE && mapItem->objectSubtype == WATERING_PLACE_OBJECT_SUBTYPE)
    {
        return reinterpret_cast<H3MapItemWateringPlace*>(&mapItem->setup);
    }

    return nullptr;
}

WateringPlaceExtender & WateringPlaceExtender::Get()
{
    static WateringPlaceExtender _instance;
    return _instance;
}
}
