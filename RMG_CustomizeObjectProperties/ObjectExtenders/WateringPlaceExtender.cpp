#include "../pch.h"
namespace wateringPlace
{
WateringPlaceExtender::WateringPlaceExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.WateringPlaceExtender.daemon_n"))
{

    CreatePatches();
}

WateringPlaceExtender::~WateringPlaceExtender()
{
}

BOOL WateringPlaceExtender::SetAiMapItemWeight(H3MapItem *mapItem, const H3Hero *hero, const H3Player *player,
                                               int &aiMapItemWeight) const noexcept
{
    // TODO: use SWAN POND???? 00528567

    // if (H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->esi))
    //{
    //     if (auto wateringPlace = H3MapItemWateringPlace::GetFromMapItem(mapItem))
    //     {
    //         const H3Hero *hero = reinterpret_cast<H3Hero *>(c->ebx);
    //         const bool isVisitedByHero = H3MapItemWateringPlace::IsVisitedByHero(*wateringPlace, hero);

    //        if (!isVisitedByHero)
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

    return false;
}

BOOL H3MapItemWateringPlace::IsVisitedByHero(const H3Hero *hero) noexcept
{
    sprintf(h3_TextBuffer, ErmVariableFormat, hero->id);

    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

void ShowMessage(const H3MapItem *mapItem)
{
    const bool skipMapMessage = globalPatcher->VarValue<int>("HD.UI.AdvMgr.SkipMapMsgs");

    H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));

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

BOOL AskQuestion(const H3MapItem *mapItem)
{
    H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));

    objName.Append(EraJS::read(
        H3String::Format("RMG.objectGeneration.%d.%d.text.visit", mapItem->objectType, mapItem->objectSubtype)
            .String()));

    return H3Messagebox::Choice(objName);
}

BOOL WateringPlaceExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                         const BOOL isHuman) const noexcept
{
    if (auto wateringPlace = H3MapItemWateringPlace::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemWateringPlace::IsVisitedByHero(hero);

        if (!isVisitedByHero)
        {
            BOOL agreed = !isHuman;
            if (isHuman)
            {
                agreed = AskQuestion(mapItem);
            }
            if (agreed)
            {
                hero->movement = 0;
                hero->maxMovement += MOVE_POINTS_GIVEN;
                hero->RecalculateMovement();
                sprintf(h3_TextBuffer, H3MapItemWateringPlace::ErmVariableFormat,
                        hero->id);                          // получение имени переменной
                Era::SetAssocVarIntValue(h3_TextBuffer, 1); // отметить переменную, что объект посещен
            }

            return true;
        }

        if (isHuman)
        {
            ShowMessage(mapItem);
        }
    }

    return false;
}

BOOL WateringPlaceExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const H3Player *activePlayer,
                                                  const BOOL isRightClick) const noexcept
{

    if (const auto wateringPlace = H3MapItemWateringPlace::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVisitedByHero = H3MapItemWateringPlace::IsVisitedByHero(hero);
            sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ",
                    P_GeneralText->GetText(isVisitedByHero ? 354 : 355));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}

// TODO: Нужно использовать хук в функции перерасчета мув поинтов
void __stdcall OnEveryDay(Era::TEvent *event)
{
    for (const auto heroId : P_Game->players[P_CurrentPlayerID].heroIDs)
    {
        auto hero = &P_Game->heroes[heroId];

        if (H3MapItemWateringPlace::IsVisitedByHero(hero))
        {
            hero->movement += MOVE_POINTS_GIVEN;
            // hero->RecalculateMovement();
            THISCALL_4(void, 0x04032E0, P_AdventureManager->dlg, -1, 1, 1); // H3AdventureMgrDlg::RedrawHeroesSlots
            sprintf(h3_TextBuffer, H3MapItemWateringPlace::ErmVariableFormat, hero->id); // получение имени переменной
            Era::SetAssocVarIntValue(h3_TextBuffer, 0);                                  // обнулить переменную
        }
    }
    // Для вообще всех героев (в тюрьмах, таверне и т.д.)
    // for (size_t i = 0; i < h3::limits::HEROES; i++)
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
    //            sprintf(h3_TextBuffer, H3MapItemWateringPlace::ErmVariableFormat, hero->id); // получение имени
    //            переменной Era::SetAssocVarIntValue(h3_TextBuffer, 0); // обнулить переменную
    //        }
    //    }
    //}
}

_LHF_(WateringPlace_HeroReset)
{
    int heroId = c->edx; // !!UN:C(hook)/(STRUCT_HOOK_CONTEXT_EDX)/4/?(heroId:y);
    sprintf(h3_TextBuffer, H3MapItemWateringPlace::ErmVariableFormat, heroId); // получение имени переменной
    Era::SetAssocVarIntValue(h3_TextBuffer, 0);                                // обнулить переменную

    return EXEC_DEFAULT;
}

void WateringPlaceExtender::CreatePatches()
{
    if (!m_isInited)
    {
        Era::RegisterHandler(OnEveryDay, "OnEveryDay");      // строка должна совпадать с неймингом триггера в ERA
        _pi->WriteLoHook(0x4D89B8, WateringPlace_HeroReset); // 5081528 = 4D89B8 - hero reset // Обнуляем посещение

        m_isInited = true;
    }
}

inline H3MapItemWateringPlace *H3MapItemWateringPlace::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_OBJECT_TYPE &&
        mapItem->objectSubtype == WATERING_PLACE_OBJECT_SUBTYPE)
    {
        // return reinterpret_cast<H3MapItemWateringPlace *>(&mapItem->setup);
        return const_cast<H3MapItemWateringPlace *>(reinterpret_cast<const H3MapItemWateringPlace *>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator *WateringPlaceExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::HOTA_OBJECT_TYPE && objectInfo.subtype == WATERING_PLACE_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

WateringPlaceExtender &WateringPlaceExtender::Get()
{
    static WateringPlaceExtender _instance;
    return _instance;
}
} // namespace wateringPlace
