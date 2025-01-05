#include "../pch.h"
namespace trailblazer
{
    TrailblazerSettings TrailblazerExtender::trailblazerSettings;

    TrailblazerExtender::TrailblazerExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.TrailblazerExtender.daemon_n"))
{

    CreatePatches();
}

    TrailblazerExtender::~TrailblazerExtender()
{
}

BOOL TrailblazerExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player,
                                             int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept
{
     if (auto trailblazer = H3MapItemTrailblazer::GetFromMapItem(mapItem))
    {
         const bool isVisitedByHero = H3MapItemTrailblazer::IsVisitedByHero(hero);

         if (!isVisitedByHero)
         {
             // Pattern used: return AIHero_GetObjectPosWeight_Stables(hero, moveCost); 0052AFA0

             //Stables formila:
             //halfWeekMovePtsProfit = (__int16)(o_MoveTXT_Obj_94 * (8 - o_Game->CurDayOfWeek) / 2)
             // 400 * (8 - P_Game->Get()->date) / 2
             //from 1400 to 200 (-200 per day)
             const int currDay = P_Game->Get()->date.day;
             const int weekProfit = 250 * (8 - currDay);
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

BOOL H3MapItemTrailblazer::IsVisitedByHero(const H3Hero *hero) noexcept
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

BOOL TrailblazerExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                       const BOOL isHuman) const noexcept
{
    if (auto trailblazer = H3MapItemTrailblazer::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemTrailblazer::IsVisitedByHero(hero);

        if (!isVisitedByHero)
        {
            sprintf(h3_TextBuffer, H3MapItemTrailblazer::ErmVariableFormat,
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

BOOL TrailblazerExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const H3Player *activePlayer,
                                                const BOOL isRightClick) const noexcept
{
    if (const auto trailblazer = H3MapItemTrailblazer::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVisitedByHero = H3MapItemTrailblazer::IsVisitedByHero(hero);
            sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ",
                    P_GeneralText->GetText(isVisitedByHero ? 354 : 355));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}

_LHF_(Trailblazer_HeroReset)
{
    int heroId = c->edx; // !!UN:C(hook)/(STRUCT_HOOK_CONTEXT_EDX)/4/?(heroId:y);
    sprintf(h3_TextBuffer, H3MapItemTrailblazer::ErmVariableFormat, heroId); // receiving name var
    Era::SetAssocVarIntValue(h3_TextBuffer, 0);                              // set var to 0

    return EXEC_DEFAULT;
}

_LHF_(OnNewWeek_StartFunc)
{
    for (size_t i = 0; i < h3::limits::HEROES; i++)
    {
        auto hero = &P_Game->heroes[i];

        if (H3MapItemTrailblazer::IsVisitedByHero(hero))
        {
            sprintf(h3_TextBuffer, H3MapItemTrailblazer::ErmVariableFormat, hero->id);
            Era::SetAssocVarIntValue(h3_TextBuffer, 0);
        }
    }

    return EXEC_DEFAULT;
}

int __stdcall Get_Step_Cost(
    HiHook* h,
    H3MapItem* mapItem,
    char direction,
    signed int movementLeft,
    int pathfindSkill,
    int road,
    int flyPower,
    int waterWalkPower,
    int nativeLand,
    int nomadsCount
)
{
    INT8 targetLandType = mapItem->land;

    if (H3Hero* hero = P_ActivePlayer->GetActiveHero())
    {
        if (targetLandType == TrailblazerExtender::trailblazerSettings.trailblzerTerrainType && !road)
        {
            const bool isVisitedByHero = H3MapItemTrailblazer::IsVisitedByHero(hero);
            if (isVisitedByHero)
            {
                int stepTrailCost = 75;
                if ((direction & 1) != 0)
                {
                    stepTrailCost = static_cast <int>(static_cast <float>(stepTrailCost) * 1.4142135);
                }

                return stepTrailCost;
            }
        }
    }

    return FASTCALL_9(int, h->GetDefaultFunc(), mapItem, direction, movementLeft, pathfindSkill, road, flyPower, waterWalkPower, nativeLand, nomadsCount);
}

void TrailblazerExtender::CreatePatches()
{
    if (!m_isInited)
    {
        bool readSuccess = false;
        trailblazerSettings.trailblzerTerrainType = eTerrain(EraJS::readInt("jsNO.settings.trailblazerTerrainType", readSuccess));
        trailblazerSettings.trailblazerEnabled = readSuccess;
        
        if (trailblazerSettings.trailblazerEnabled)
        {
            _pi->WriteLoHook(0x4D89B8, Trailblazer_HeroReset);
            _pi->WriteLoHook(0x4C8450, OnNewWeek_StartFunc);
            _pi->WriteHiHook(0x4B14A0, FASTCALL_, Get_Step_Cost);
        }

        m_isInited = true;
    }
}

inline H3MapItemTrailblazer* H3MapItemTrailblazer::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_OBJECT_TYPE &&
        mapItem->objectSubtype == TRAILBLAZER_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemTrailblazer*>(reinterpret_cast<const H3MapItemTrailblazer*>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator * TrailblazerExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::HOTA_OBJECT_TYPE &&
        objectInfo.subtype == TRAILBLAZER_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

TrailblazerExtender& TrailblazerExtender::Get()
{
    static TrailblazerExtender _instance;
    return _instance;
}
} // namespace trailblazer
