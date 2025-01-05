#include "../pch.h"
namespace mineralSpring
{
MineralSpringExtender::MineralSpringExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.WateringPlaceExtender.daemon_n"))
{

    CreatePatches();
}

MineralSpringExtender::~MineralSpringExtender()
{
}

BOOL MineralSpringExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player,
                                               int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept
{
    if (auto mineralSpring = H3MapItemMineralSpring::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemMineralSpring::IsVisitedByHero(hero);

        if (!isVisitedByHero)
        {
            // Pattern used: return AIHero_GetObjectPosWeight_Oasis(hero, 64, 200, (int)moveCost); 0052A6C0
            // 64 - offset of temporary bonus, located in _H3Hero_ + 0x105
            // 200 - movePtsGiven/2

            if (*moveDistance < (MOVE_POINTS_GIVEN / 2))
            {
                *moveDistance = 0;
                aiMapItemWeight = 10000;
            }
            else
            {
                *moveDistance -= (MOVE_POINTS_GIVEN / 2);
                // Hero_GetLuckBonus 004E3930
                const int heroLuckBonus = THISCALL_4(int, 0x04E3930, hero, 0, 0, LUCK_GIVEN);
                // AI_Get_LuckChange_Mogif 004355B0
                const long double v9 = FASTCALL_2(long double, 0x04355B0, heroLuckBonus, 1);
                // Hero_GetSumOfPrimarySkills 004E5BD0
                const int SumOfPrimarySkills = THISCALL_1(int, 0x04E5BD0, hero);
                // *(_QWORD *)&aiMapItemWeight = (__int64)((double)(Army_Get_AI_Value(&hero->army) * (SumOfPrimarySkills + 40) / 40) *v9);
                // Army_Get_AI_Value 0044A950
                const int Army_Get_AI_Value = THISCALL_1(int, 0x044A950, &hero->army);
                aiMapItemWeight = (Army_Get_AI_Value * (SumOfPrimarySkills + 40) / 40) * v9;
            }
        }

        return true;
    }

    return false;
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

        //if (skipMapMessage)
        //{
        //    // TODO: должна быть картинка
        //    THISCALL_4(void, 0x415FC0, P_AdventureMgr->Get(), objName.String(), -1, -1);
        //}
        //else
        //{
            H3PictureCategories pics(ePictureCategories::POSITIVE_LUCK, -1);
            H3Messagebox::Show(objName, pics);
        //}
    }
}

BOOL MineralSpringExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                         const BOOL isHuman) const noexcept
{
    if (auto mineralSpring = H3MapItemMineralSpring::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemMineralSpring::IsVisitedByHero(hero);

        if (!isVisitedByHero)
        {
            hero->movement += MOVE_POINTS_GIVEN;
            hero->luckBonus += LUCK_GIVEN;

            // P_WindowManager->H3Redraw();

            // AdvMgr_FullUpdate(AdvMng, 1, 0);
            P_AdventureManager->FullUpdate();
           
            //THISCALL_4(void, 0x0415D40, P_AdventureManager->Get(), 1, 0, 0); // AdvMgr_UpdateInfoPanel
            //THISCALL_4(void, 0x04032E0, P_AdventureManager->dlg, -1, 1, 1); // H3AdventureMgrDlg::RedrawHeroesSlots

            sprintf(h3_TextBuffer, H3MapItemMineralSpring::ErmVariableFormat,
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

BOOL MineralSpringExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const H3Player *activePlayer,
                                                  const BOOL isRightClick) const noexcept
{
    if (const auto wateringPlace = H3MapItemMineralSpring::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVisitedByHero = H3MapItemMineralSpring::IsVisitedByHero(hero);
            sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ",
                    P_GeneralText->GetText(isVisitedByHero ? 354 : 355));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}

void __stdcall OnAfterBattleUniversal(Era::TEvent *event)
{
    for (size_t i = 0; i < 2; i++)
    {
        H3Hero *hero = P_CombatMgr->hero[i];
        if (hero)
        {
            sprintf(h3_TextBuffer, H3MapItemMineralSpring::ErmVariableFormat, hero->id); // receiving name var
            Era::SetAssocVarIntValue(h3_TextBuffer, 0);                                  // set var to 0
        }
    }
}

_LHF_(MineralSpring_HeroReset)
{
    int heroId = c->edx; // !!UN:C(hook)/(STRUCT_HOOK_CONTEXT_EDX)/4/?(heroId:y);
    sprintf(h3_TextBuffer, H3MapItemMineralSpring::ErmVariableFormat, heroId); // receiving name var
    Era::SetAssocVarIntValue(h3_TextBuffer, 0);                                // set var to 0

    return EXEC_DEFAULT;
}

void MineralSpringExtender::CreatePatches()
{
    if (!m_isInited)
    {
        Era::RegisterHandler(OnAfterBattleUniversal, "OnAfterBattleUniversal");
        _pi->WriteLoHook(0x4D89B8, MineralSpring_HeroReset);

        m_isInited = true;
    }
}

inline H3MapItemMineralSpring *H3MapItemMineralSpring::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_OBJECT_TYPE &&
        mapItem->objectSubtype == MINERAL_SPRING_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemMineralSpring *>(reinterpret_cast<const H3MapItemMineralSpring *>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator *MineralSpringExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::HOTA_OBJECT_TYPE && objectInfo.subtype == MINERAL_SPRING_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

MineralSpringExtender &MineralSpringExtender::Get()
{
    static MineralSpringExtender _instance;
    return _instance;
}
} // namespace mineralSpring
