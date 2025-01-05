#include "../pch.h"
namespace templeOfLoyalty
{
TempleOfLoyaltyExtender::TempleOfLoyaltyExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.TempleOfLoyaltyExtender.daemon_n"))
{

    CreatePatches();
}

TempleOfLoyaltyExtender::~TempleOfLoyaltyExtender()
{
}

BOOL TempleOfLoyaltyExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player,
                                                 int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept
{
     if (auto templeOfLoyalty = H3MapItemTempleOfLoyalty::GetFromMapItem(mapItem))
    {
         const bool isVisitedByHero = H3MapItemTempleOfLoyalty::IsVisitedByHero(hero);

        if (!isVisitedByHero)
        {
            // Pattern used: visit TEMPLE 0052979C AIHero_GetObjectPosWeight_Temple

            //v88 = hero->TempMod;
            //if ((v88 & 0x100) != 0 || (v88 & 0x4000000) != 0 || *moveCost > hero->Movement)
            //return 0;
            if (*moveDistance > hero->movement)
            {
                aiMapItemWeight = 0;
                return true;
            }
           
            const H3Army* army = &hero->army;
            int armyFractionsCount = THISCALL_2(int, 0x44A880, army, 0);

            if (armyFractionsCount == 1)
            {
                aiMapItemWeight = 0;
                return true;
            }
            //return AIHero_GetObjectPosWeight_Temple(hero, 2);
            aiMapItemWeight = THISCALL_2(int, 0x5281D0, hero, 2);
        }

        return true;
    }

    return false;
}

BOOL H3MapItemTempleOfLoyalty::IsVisitedByHero(const H3Hero *hero) noexcept
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
            // AdvMgr_DisplayResourcesInfoPanel
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
            THISCALL_4(void, 0x415FC0, P_AdventureMgr->Get(), objName.String(), -1, -1);
        }
        else
        {
            H3Messagebox::Show(objName);
        }
    }
}

BOOL TempleOfLoyaltyExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                           const BOOL isHuman) const noexcept
{
    if (auto templeOfLoyalty = H3MapItemTempleOfLoyalty::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemTempleOfLoyalty::IsVisitedByHero(hero);

        if (!isVisitedByHero)
        {
            sprintf(h3_TextBuffer, H3MapItemTempleOfLoyalty::ErmVariableFormat,
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

BOOL TempleOfLoyaltyExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero,
                                                    const H3Player *activePlayer,
                                                    const BOOL isRightClick) const noexcept
{

    if (const auto templeOfLoyalty = H3MapItemTempleOfLoyalty::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVisitedByHero = H3MapItemTempleOfLoyalty::IsVisitedByHero(hero);
            sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ",
                    P_GeneralText->GetText(isVisitedByHero ? 354 : 355));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}

void __stdcall OnEveryDay(Era::TEvent *event)
{
    for (const auto heroId : P_Game->players[P_CurrentPlayerID].heroIDs)
    {
        auto hero = &P_Game->heroes[heroId];

        if (H3MapItemTempleOfLoyalty::IsVisitedByHero(hero))
        {
            sprintf(h3_TextBuffer, H3MapItemTempleOfLoyalty::ErmVariableFormat, hero->id); // получение имени переменной
            Era::SetAssocVarIntValue(h3_TextBuffer, 0);                                    // обнулить переменную
        }
    }
}

_LHF_(TempleOfLoyalty_HeroReset)
{
    int heroId = c->edx; // !!UN:C(hook)/(STRUCT_HOOK_CONTEXT_EDX)/4/?(heroId:y);
    sprintf(h3_TextBuffer, H3MapItemTempleOfLoyalty::ErmVariableFormat, heroId); // получение имени переменной
    Era::SetAssocVarIntValue(h3_TextBuffer, 0);                                  // обнулить переменную

    return EXEC_DEFAULT;
}

_LHF_(TempleOfLoyalty_SetArmyFractionsCountDesc)
{
    const H3Hero *hero = *reinterpret_cast<H3Hero **>(c->ebp + 0x14);

    if (hero)
    {
        const bool isVisitedByHero = H3MapItemTempleOfLoyalty::IsVisitedByHero(hero);

        if (isVisitedByHero)
        {
            c->edi = 1; // set fractionsCount=1 (all creatures are from same town
        }

        return EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

_LHF_(TempleOfLoyalty_SetArmyFractionsCountValue)
{
    const H3Hero* hero = *reinterpret_cast<H3Hero**>(c->ebp + 0x8);

    if (hero)
    {
        const bool isVisitedByHero = H3MapItemTempleOfLoyalty::IsVisitedByHero(hero);

        if (isVisitedByHero)
        {
            c->edi = 1; // set fractionsCount=1 (all creatures are from same town
        }

        return EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

void TempleOfLoyaltyExtender::CreatePatches()
{
    if (!m_isInited)
    {
        Era::RegisterHandler(OnEveryDay, "OnEveryDay");
        _pi->WriteLoHook(0x4D89B8, TempleOfLoyalty_HeroReset); // 5081528 = 4D89B8 - hero reset // Обнуляем посещение
        _pi->WriteLoHook(0x44B85B, TempleOfLoyalty_SetArmyFractionsCountDesc);
        _pi->WriteLoHook(0x44AB77, TempleOfLoyalty_SetArmyFractionsCountValue);
        
        m_isInited = true;
    }
}

inline H3MapItemTempleOfLoyalty *H3MapItemTempleOfLoyalty::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_OBJECT_TYPE &&
        mapItem->objectSubtype == TEMPLE_OF_LOYALTY_OBJECT_SUBTYPE)
    {
        // return reinterpret_cast<H3MapItemWateringPlace *>(&mapItem->setup);
        return const_cast<H3MapItemTempleOfLoyalty *>(
            reinterpret_cast<const H3MapItemTempleOfLoyalty *>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator *TempleOfLoyaltyExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::HOTA_OBJECT_TYPE && objectInfo.subtype == TEMPLE_OF_LOYALTY_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

TempleOfLoyaltyExtender &TempleOfLoyaltyExtender::Get()
{
    static TempleOfLoyaltyExtender _instance;
    return _instance;
}
} // namespace templeOfLoyalty
