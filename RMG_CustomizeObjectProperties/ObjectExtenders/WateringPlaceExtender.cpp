#include "../pch.h"
namespace wateringPlace
{
WateringPlaceExtender::WateringPlaceExtender()
    : ObjectExtender(globalPatcher->CreateInstance("EraPlugin.WateringPlaceExtender.daemon_n"))
{

    CreatePatches();
}

WateringPlaceExtender::~WateringPlaceExtender()
{
}

BOOL WateringPlaceExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player,
                                               int &aiMapItemWeight, int *moveDistance,
                                               const H3Position pos) const noexcept
{
    if (auto wateringPlace = H3MapItemWateringPlace::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemWateringPlace::IsVisitedByHero(hero);

        if (!isVisitedByHero)
        {
            // Pattern used: visit SWAN POND 00528A4C
            // v36 = *moveDistance;
            int v36 = *moveDistance;
            // currMovePts = hero->Movement;
            int currMovePts = hero->movement;
            // if ( *moveDistance > currMovePts || v36 + 200 < currMovePts )
            //     return 0;
            if (*moveDistance > currMovePts || v36 + 200 < currMovePts)
            {
                aiMapItemWeight = 0;
                return true;
            }
            // v100 = *(float*)moveDistance;
            float v100 = static_cast<float>(*moveDistance);
            // v23 = v36 < currMovePts;
            int v23 = v36 < currMovePts;
            // pos = (type_point)currMovePts;
            int v30 = currMovePts;
            // p_pos = (int*)&pos;
            int *p_v30 = (int *)&v30;
            // if (!v23)
            //     p_pos = (int*)&v100;
            if (!v23)
            {
                p_v30 = (int *)&v100;
            }
            //*moveDistance = *p_pos;
            *moveDistance = *p_v30;
            // AI_Value = Army_Get_AI_Value(&hero->army);
            const int Army_Get_AI_Value = THISCALL_1(int, 0x044A950, &hero->army);
            // pos = (type_point)(((__int16)Hero_GetSumOfPrimarySkills(hero) + 40) * AI_Value / 40);
            const int SumOfPrimarySkills = THISCALL_1(int, 0x04E5BD0, hero);
            v30 = (SumOfPrimarySkills + 40) * Army_Get_AI_Value / 40;
            //*(_QWORD*)&v8 = (__int64)((double)*(int*)&pos * (double)v3); // what is v3?
            // return v8;
            aiMapItemWeight = v30;
        }

        return true;
    }

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
    if (H3MapItemWateringPlace::GetFromMapItem(mapItem))
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
                        hero->id);                          // ��������� ����� ����������
                Era::SetAssocVarIntValue(h3_TextBuffer, 1); // �������� ����������, ��� ������ �������
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

// TODO: ����� ������������ ��� � ������� ����������� ��� �������
void __stdcall OnEveryDay(Era::TEvent *event)
{
    for (const auto heroId : P_Game->players[P_CurrentPlayerID].heroIDs)
    {
        auto hero = &P_Game->heroes[heroId];

        if (H3MapItemWateringPlace::IsVisitedByHero(hero))
        {

            // hero->RecalculateMovement();
            // THISCALL_4(void, 0x0415D40, P_AdventureManager->Get(), 1, 0, 0); // AdvMgr_UpdateInfoPanel
            // THISCALL_4(void, 0x04032E0, P_AdventureManager->dlg, -1, 1, 1); // H3AdventureMgrDlg::RedrawHeroesSlots
            sprintf(h3_TextBuffer, H3MapItemWateringPlace::ErmVariableFormat, hero->id); // ��������� ����� ����������
            Era::SetAssocVarIntValue(h3_TextBuffer, 0);                                  // �������� ����������
            hero->movement += MOVE_POINTS_GIVEN;
            // THISCALL_4(void, 0x04032E0, P_AdventureManager->dlg, -1, 1, 1); // H3AdventureMgrDlg::RedrawHeroesSlots
            P_AdventureManager->FullUpdate();
            // DlgHeroInfo_Update 004E247A
            // DlgHeroInfo_ProcessAction 004DD5CE
            // Dlg_HeroInfo_Main 004E1CBC
        }
    }
    // ��� ������ ���� ������ (� �������, ������� � �.�.)
    // for (size_t i = 0; i < h3::limits::HEROES; i++)
    //{
    //    auto hero = &P_Game->heroes[i];
    //    if (hero->id == P_CurrentPlayerID)
    //    {
    //        if (H3MapItemWateringPlace::IsVisitedByHero(hero))
    //        {
    //            //hero->maxMovement += MOVE_POINTS_GIVEN; // �� ��������
    //            //hero->movement += 456456;// MOVE_POINTS_GIVEN; // �� ��������
    //            hero->maxMovement = 213123;
    //            //hero->RecalculateMovement(); // �� ��������
    //            sprintf(h3_TextBuffer, H3MapItemWateringPlace::ErmVariableFormat, hero->id); // ��������� �����
    //            ���������� Era::SetAssocVarIntValue(h3_TextBuffer, 0); // �������� ����������
    //        }
    //    }
    //}
}

_LHF_(WateringPlace_HeroReset)
{
    int heroId = c->edx; // !!UN:C(hook)/(STRUCT_HOOK_CONTEXT_EDX)/4/?(heroId:y);
    sprintf(h3_TextBuffer, H3MapItemWateringPlace::ErmVariableFormat, heroId); // ��������� ����� ����������
    Era::SetAssocVarIntValue(h3_TextBuffer, 0);                                // �������� ����������

    return EXEC_DEFAULT;
}

void WateringPlaceExtender::CreatePatches()
{
    if (!m_isInited)
    {
        Era::RegisterHandler(OnEveryDay, "OnEveryDay");      // ������ ������ ��������� � ��������� �������� � ERA
        _pi->WriteLoHook(0x4D89B8, WateringPlace_HeroReset); // 5081528 = 4D89B8 - hero reset // �������� ���������

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
        return extender::ObjectExtenderManager::CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

WateringPlaceExtender* WateringPlaceExtender::instance = nullptr;

WateringPlaceExtender& WateringPlaceExtender::Get()
{
    if (!instance)
        instance = new WateringPlaceExtender();
    return *instance;
}
} // namespace wateringPlace
