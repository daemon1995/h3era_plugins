#include "../pch.h"
namespace observatory
{
ObservatoryExtender::ObservatoryExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.ObservatoryExtender.daemon_n"))
{

    CreatePatches();
}

ObservatoryExtender::~ObservatoryExtender()
{
}

BOOL ObservatoryExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player,
                                             int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept
{
     if (auto observatory = H3MapItemObservatory::GetFromMapItem(mapItem))
    {
         const bool isVisitedByHero = H3MapItemObservatory::IsVisitedByHero(hero);

        if (!isVisitedByHero)
        {
            // Pattern used: return AIHero_GetObjectPosWeight_Stables(hero, moveCost); 0052AFA0

            //Stables formila:
            //halfWeekMovePtsProfit = (__int16)(o_MoveTXT_Obj_94 * (8 - o_Game->CurDayOfWeek) / 2)
            // 400 * (8 - P_Game->Get()->date) / 2
            //from 1400 to 200 (-200 per day)
            const int currDay = P_Game->Get()->date.day;
            const int weekProfit = 100 * (8 - currDay);
            int a = 5;
            if (*moveDistance < weekProfit)
            {
                *moveDistance = 0;
                aiMapItemWeight = 10000;
            }
            else
            {
                aiMapItemWeight = 50;
                *moveDistance -= weekProfit;
            }
        }

        return true;
    }

    return false;
}

BOOL H3MapItemObservatory::IsVisitedByHero(const H3Hero *hero) noexcept
{
    sprintf(h3_TextBuffer, ErmVariableFormat, hero->id);

    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

void ShowMessage(const H3MapItem *mapItem, const bool isVisitedByHero)
{
    // const bool skipMapMessage = globalPatcher->VarValue<int>("HD.UI.AdvMgr.SkipMapMsgs");

    H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));

    if (isVisitedByHero)
    {
        objName.Append(EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.text.visited", mapItem->objectType, mapItem->objectSubtype)
                .String()));

        // if (skipMapMessage)
        //{
        //     // AdvMgr_DisplayResourcesInfoPanel
        //     THISCALL_4(void, 0x415FC0, P_AdventureMgr->Get(), objName.String(), -1, -1);
        // }
        // else
        //{
        H3Messagebox::Show(objName);
        //}
    }
    else
    {
        objName.Append(EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.text.visit", mapItem->objectType, mapItem->objectSubtype)
                .String()));

        // if (skipMapMessage)
        //{
        //     // TODO: должна быть картинка
        //     THISCALL_4(void, 0x415FC0, P_AdventureMgr->Get(), objName.String(), -1, -1);
        // }
        // else
        //{
        // H3PictureCategories pics(ePictureCategories::POSITIVE_LUCK, -1);
        H3Messagebox::Show(objName); //, pics);
        //}
    }
}

BOOL ObservatoryExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                       const BOOL isHuman) const noexcept
{
    if (auto observatory = H3MapItemObservatory::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemObservatory::IsVisitedByHero(hero);

        if (!isVisitedByHero)
        {
            sprintf(h3_TextBuffer, H3MapItemObservatory::ErmVariableFormat,
                    hero->id);                          // получение имени переменной
            Era::SetAssocVarIntValue(h3_TextBuffer, 1); // отметить переменную, что объект посещен
        }

        if (isHuman)
        {
            ShowMessage(mapItem, isVisitedByHero);
        }
    }

    return false;
}

BOOL ObservatoryExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const H3Player *activePlayer,
                                                const BOOL isRightClick) const noexcept
{
    if (const auto observatory = H3MapItemObservatory::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVisitedByHero = H3MapItemObservatory::IsVisitedByHero(hero);
            sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ",
                    P_GeneralText->GetText(isVisitedByHero ? 354 : 355));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}

_LHF_(Observatory_HeroReset)
{
    int heroId = c->edx; // !!UN:C(hook)/(STRUCT_HOOK_CONTEXT_EDX)/4/?(heroId:y);
    sprintf(h3_TextBuffer, H3MapItemObservatory::ErmVariableFormat, heroId); // receiving name var
    Era::SetAssocVarIntValue(h3_TextBuffer, 0);                              // set var to 0

    return EXEC_DEFAULT;
}

_LHF_(OnCalcScoutingRadius_BeforeReturn)
{
    H3Hero *hero = reinterpret_cast<H3Hero *>(c->esi);

    if (hero)
    {
        const bool isVisitedByHero = H3MapItemObservatory::IsVisitedByHero(hero);

        if (isVisitedByHero)
        {
            c->ebx += 2; // Apply bonus from Observatory
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(OnNewWeek_StartFunc)
{
    for (size_t i = 0; i < h3::limits::HEROES; i++)
    {
        auto hero = &P_Game->heroes[i];

        if (H3MapItemObservatory::IsVisitedByHero(hero))
        {
            sprintf(h3_TextBuffer, H3MapItemObservatory::ErmVariableFormat, hero->id);
            Era::SetAssocVarIntValue(h3_TextBuffer, 0);
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(test_stables)
{
    const H3Position pos = *reinterpret_cast<H3Position*>(c->ebp + 0x8);
    int* moveDistance = reinterpret_cast<int*>(c->edi);

    return EXEC_DEFAULT;
}

void ObservatoryExtender::CreatePatches()
{
    if (!m_isInited)
    {
        _pi->WriteLoHook(0x4D89B8, Observatory_HeroReset);
        _pi->WriteLoHook(0x4E43C7, OnCalcScoutingRadius_BeforeReturn);
        //_pi->WriteLoHook(0x4C8449, OnNewWeek_StartFunc);
        _pi->WriteLoHook(0x4C8450, OnNewWeek_StartFunc);

        //_pi->WriteLoHook(0x52978A, test_stables);

        m_isInited = true;
    }
}

inline H3MapItemObservatory *H3MapItemObservatory::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_UNREACHABLE_YT_OBJECT_TYPE &&
        mapItem->objectSubtype == OBSERVATORY_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemObservatory *>(reinterpret_cast<const H3MapItemObservatory *>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator *ObservatoryExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::HOTA_UNREACHABLE_YT_OBJECT_TYPE &&
        objectInfo.subtype == OBSERVATORY_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

ObservatoryExtender &ObservatoryExtender::Get()
{
    static ObservatoryExtender _instance;
    return _instance;
}
} // namespace observatory
