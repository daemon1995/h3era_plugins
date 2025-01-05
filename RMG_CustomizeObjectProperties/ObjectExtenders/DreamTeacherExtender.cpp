#include "../pch.h"
namespace dreamTeacher
{
int H3MapItemDreamTeacher::dreamTeacherCounter = 0;

DreamTeacherExtender::DreamTeacherExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.DreamTeacherExtender.daemon_n"))
{

    CreatePatches();
}

DreamTeacherExtender::~DreamTeacherExtender()
{
}

BOOL DreamTeacherExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player,
                                              int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept
{
    // If coords are needed (moveDist, itemCoords) // type_point! ebp + 0x8 in arguments
    // const H3Point itemCoords = mapItem->GetCoordinates();
    // itemCoords.x, itemCoords.y, itemCoords.z

    if (auto dreamTeacher = H3MapItemDreamTeacher::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemDreamTeacher::IsVisitedByHero(dreamTeacher, hero);

        if (!isVisitedByHero)
        {
            // get visited obj counter
            int visitedObjCount = H3MapItemDreamTeacher::GetVisitedObjectsCountByHero(hero);

            // pseudocode addr 0052BB89 (TreeofKnowledge) and 00529833 (LearningStone)
            
            // calculate exp given considering the learning power
            float heroLearningPower = h3functions::GetHeroLearningPower(hero);
            int expGiven = static_cast<int>((EXP_GIVEN + (ADDITIONAL_EXP_GIVEN_FOR_EACH_VISIT * visitedObjCount)) * heroLearningPower);
            
            // pseudocode addr 0052BB89
            //*(_QWORD *)&v8 = (__int64)(hero->turnExperienceToRVRatio * 1000.0);
            aiMapItemWeight = static_cast<int>(expGiven * hero->AI_experienceEffectiveness);
        }

        return true;
    }

    return false;
}

BOOL H3MapItemDreamTeacher::IsVisitedByHero(const H3MapItemDreamTeacher * dreamTeacher, const H3Hero *hero) noexcept
{
    sprintf(h3_TextBuffer, ErmVariableFormat, dreamTeacher->id, hero->id);

    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

int H3MapItemDreamTeacher::GetVisitedObjectsCountByHero(const H3Hero* hero) noexcept
{
    sprintf(h3_TextBuffer, VisitedObjectsCount, hero->id);

    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

void ShowMessage(const H3MapItem *mapItem, int expGiven, const bool isVisitedByHero)
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

        // if (skipMapMessage)
        //{
        //     // TODO: должна быть картинка
        //     THISCALL_4(void, 0x415FC0, P_AdventureMgr->Get(), objName.String(), -1, -1);
        // }
        // else
        //{
        H3PictureCategories pics(ePictureCategories::EXPERIENCE, expGiven);
        H3Messagebox::Show(objName, pics);
        //}
    }
}

BOOL DreamTeacherExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                        const BOOL isHuman) const noexcept
{

    if (auto dreamTeacher = H3MapItemDreamTeacher::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemDreamTeacher::IsVisitedByHero(dreamTeacher, hero);

        if (isVisitedByHero)
        {
            if (isHuman)
            {
                ShowMessage(mapItem, 0, isVisitedByHero);
            }
        }
        else
        {
            // get visited obj counter
            int visitedObjCount = H3MapItemDreamTeacher::GetVisitedObjectsCountByHero(hero);
            
            // calculate exp given considering the learning power
            float heroLearningPower = h3functions::GetHeroLearningPower(hero);
            int expGiven = static_cast<int>((EXP_GIVEN + (ADDITIONAL_EXP_GIVEN_FOR_EACH_VISIT * visitedObjCount)) * heroLearningPower);

            if (isHuman)
            {
                ShowMessage(mapItem, expGiven, isVisitedByHero);
            }
            
            // give exp
            THISCALL_4(void, 0x04E3620, hero, expGiven, 1, 1);

            // Hero::CheckLevelUps
            THISCALL_1(void, 0x04DA990, hero);

            // set obj as visited
            sprintf(h3_TextBuffer, H3MapItemDreamTeacher::ErmVariableFormat, dreamTeacher->id,
                hero->id);
            Era::SetAssocVarIntValue(h3_TextBuffer, 1);

            // increase visited obj counter
            visitedObjCount += 1;
            sprintf(h3_TextBuffer, H3MapItemDreamTeacher::VisitedObjectsCount, hero->id);
            Era::SetAssocVarIntValue(h3_TextBuffer, visitedObjCount);
        }

        return true;
    }

    return false;
}

BOOL DreamTeacherExtender::InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept
{

    if (auto dreamTeacher = H3MapItemDreamTeacher::GetFromMapItem(mapItem))
    {
        dreamTeacher->id = H3MapItemDreamTeacher::dreamTeacherCounter++;
        return true;
    }

    return false;
}

_LHF_(Game__NewGameBeforeSetObjectsInitialParameters)
{
    H3MapItemDreamTeacher::dreamTeacherCounter = 0;

    return EXEC_DEFAULT;
}

BOOL DreamTeacherExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const H3Player *activePlayer,
                                                 const BOOL isRightClick) const noexcept
{

    if (const auto dreamTeacher = H3MapItemDreamTeacher::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVisitedByHero = H3MapItemDreamTeacher::IsVisitedByHero(dreamTeacher, hero);
            sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ",
                    P_GeneralText->GetText(isVisitedByHero ? 354 : 355));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}

void DreamTeacherExtender::CreatePatches()
{
    if (!m_isInited)
    {
        // Era::RegisterHandler
        // 004BFCBE
        _pi->WriteLoHook(0x04BFCBE, Game__NewGameBeforeSetObjectsInitialParameters);

        m_isInited = true;
    }
}

inline H3MapItemDreamTeacher* H3MapItemDreamTeacher::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::ERA_OBJECT_TYPE &&
        mapItem->objectSubtype == DREAM_TEACHER_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemDreamTeacher *>(reinterpret_cast<const H3MapItemDreamTeacher*>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator * DreamTeacherExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::ERA_OBJECT_TYPE && objectInfo.subtype == DREAM_TEACHER_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

DreamTeacherExtender& DreamTeacherExtender::Get()
{
    static DreamTeacherExtender _instance;
    return _instance;
}
} // namespace dreamTeacher
