#include "../pch.h"
namespace gazebo
{
int H3MapItemGazebo::gazeboCounter = 0;

GazeboExtender::GazeboExtender() : ObjectExtender(globalPatcher->CreateInstance("EraPlugin.GazeboExtender.daemon_n"))
{

    CreatePatches();
}

GazeboExtender::~GazeboExtender()
{
}

BOOL GazeboExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player, int &aiMapItemWeight,
                                        int *moveDistance, const H3Position pos) const noexcept
{

    if (auto gazebo = H3MapItemGazebo::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemGazebo::IsVisitedByHero(gazebo, hero);

        if (!isVisitedByHero)
        {
            if (P_ActivePlayer->playerResources.gold >= GOLD_REQUIRED)
            {
                // pseudocode addr 0052BB89 (TreeofKnowledge) and 00529833 (LearningStone)
                //*(_QWORD*)&v8 = (__int64)(hp->turnExperienceToRVRatio * 1000.0);
                const float aiExperience = EXP_GIVEN * hero->AI_experienceEffectiveness;

                // return (__int64)((double)v6 - HeroOwner->ai.resource_value[6] * 2000.0);
                aiMapItemWeight =
                    static_cast<int>(aiExperience - player->resourceImportance[eResource::GOLD] * GOLD_REQUIRED);
            }
        }

        return true;
    }

    return false;
}

BOOL H3MapItemGazebo::IsVisitedByHero(const H3MapItemGazebo *gazebo, const H3Hero *hero) noexcept
{
    sprintf(h3_TextBuffer, ErmVariableFormat, gazebo->id, hero->id);

    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

void ShowMessage(const H3MapItem *mapItem, const int playerGoldAmount, const bool isVisitedByHero)
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
    else if (playerGoldAmount < GOLD_REQUIRED)
    {
        objName.Append(EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.text.cannotVisit", mapItem->objectType, mapItem->objectSubtype)
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

BOOL AskQuestion(const H3MapItem *mapItem, const int expGiven)
{
    H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));

    objName.Append(EraJS::read(
        H3String::Format("RMG.objectGeneration.%d.%d.text.visit", mapItem->objectType, mapItem->objectSubtype)
            .String()));
    H3PictureCategories picOne(ePictureCategories::EXPERIENCE, expGiven);
    H3PictureCategories picTwo(ePictureCategories::GOLD, -GOLD_REQUIRED - 100000); // To avoid showing +1000gold/week

    return H3Messagebox::Choice(objName, picOne, picTwo);
}

BOOL GazeboExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                  const BOOL isHuman) const noexcept
{

    if (auto gazebo = H3MapItemGazebo::GetFromMapItem(mapItem))
    {

        const bool isVisitedByHero = H3MapItemGazebo::IsVisitedByHero(gazebo, hero);
        const int playerGoldBeforeVisit = P_ActivePlayer->playerResources.gold;

        if (!isVisitedByHero)
        {
            if (playerGoldBeforeVisit >= GOLD_REQUIRED)
            {
                // Calculate exp given considering the learning power
                float heroLearningPower = h3functions::GetHeroLearningPower(hero);
                const int expGiven = static_cast<int>(EXP_GIVEN * heroLearningPower);

                BOOL agreed = !isHuman;
                if (isHuman)
                {
                    agreed = AskQuestion(mapItem, expGiven);
                }
                // AI says yes by default
                if (agreed)
                {
                    THISCALL_3(void, 0x04E3870, hero, eResource::GOLD, -GOLD_REQUIRED);
                    THISCALL_4(void, 0x04E3620, hero, expGiven, 1, 1);
                    sprintf(h3_TextBuffer, H3MapItemGazebo::ErmVariableFormat, gazebo->id, hero->id);
                    Era::SetAssocVarIntValue(h3_TextBuffer, 1);
                }

                return true;
            }
        }

        if (isHuman)
        {
            ShowMessage(mapItem, playerGoldBeforeVisit, isVisitedByHero);
        }

        return true;
    }

    return false;
}

BOOL GazeboExtender::InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept
{

    if (auto gazebo = H3MapItemGazebo::GetFromMapItem(mapItem))
    {
        gazebo->id = H3MapItemGazebo::gazeboCounter++;
        return true;
    }

    return false;
}

_LHF_(Game__NewGameBeforeSetObjectsInitialParameters)
{
    H3MapItemGazebo::gazeboCounter = 0;

    return EXEC_DEFAULT;
}

BOOL GazeboExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const H3Player *activePlayer,
                                           const BOOL isRightClick) const noexcept
{

    if (const auto gazebo = H3MapItemGazebo::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVisitedByHero = H3MapItemGazebo::IsVisitedByHero(gazebo, hero);
            sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ",
                    P_GeneralText->GetText(isVisitedByHero ? 354 : 355));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}

void GazeboExtender::CreatePatches()
{
    if (!m_isInited)
    {
        // Era::RegisterHandler
        // 004BFCBE
        _pi->WriteLoHook(0x04BFCBE, Game__NewGameBeforeSetObjectsInitialParameters);

        m_isInited = true;
    }
}

inline H3MapItemGazebo *H3MapItemGazebo::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_OBJECT_TYPE && mapItem->objectSubtype == GAZEBO_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemGazebo *>(reinterpret_cast<const H3MapItemGazebo *>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator *GazeboExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::HOTA_OBJECT_TYPE && objectInfo.subtype == GAZEBO_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

GazeboExtender* GazeboExtender::instance = nullptr;

GazeboExtender& GazeboExtender::Get()
{
    if (!instance)
        instance = new GazeboExtender();
    return *instance;
}
} // namespace gazebo
