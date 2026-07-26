#include "GazeboExtender.h"

namespace gazebo
{
int H3MapItemGazebo::gazeboCounter = 0;

GazeboExtender::GazeboExtender() : ObjectExtender(_PI)
{

    CreatePatches();
    objectType = extender::HOTA_OBJECT_TYPE;
    objectSubtypes += GAZEBO_OBJECT_SUBTYPE;
}

BOOL GazeboExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player, int &aiMapItemWeight,
                                        int *moveDistance, const H3Position pos) const noexcept
{

    auto gazebo = extender::GetFromMapItem<H3MapItemGazebo>(mapItem);
    const bool isVisitedByHero = H3MapItemGazebo::IsVisitedByHero(gazebo, hero);
    if (!isVisitedByHero && P_ActivePlayer->playerResources.gold >= GOLD_REQUIRED)
    {
        const float aiExperience = EXP_GIVEN * hero->AI_experienceEffectiveness;
        aiMapItemWeight =
            static_cast<int>(aiExperience - player->aIPlayer.resourceImportance[eResource::GOLD] * GOLD_REQUIRED);
    }
    return true;
}

BOOL H3MapItemGazebo::IsVisitedByHero(const H3MapItemGazebo *gazebo, const H3Hero *hero) noexcept
{
    sprintf(h3_TextBuffer, ErmVariableFormat, gazebo->id, hero->id);
    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

void ShowMessage(const H3MapItem *mapItem, const int playerGoldAmount, const bool isVisitedByHero)
{
    const bool skipMapMessage = globalPatcher->VarValue<int>("HD.UI.AdvMgr.SkipMapMsgs");

    H3String objName = H3String::Format("{%s}", extender::GetObjectName(mapItem));

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
    H3String objName = H3String::Format("{%s}", extender::GetObjectName(mapItem));

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
    auto gazebo = extender::GetFromMapItem<H3MapItemGazebo>(mapItem);
    const bool isVisitedByHero = H3MapItemGazebo::IsVisitedByHero(gazebo, hero);
    const int playerGoldBeforeVisit = P_ActivePlayer->playerResources.gold;

    if (!isVisitedByHero && playerGoldBeforeVisit >= GOLD_REQUIRED)
    {
        // Calculate exp given considering the learning power
        float heroLearningPower = hero->GetLearningPower();
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
    }
    else if (isHuman)
    {
        ShowMessage(mapItem, playerGoldBeforeVisit, isVisitedByHero);
    }

    return true;
}

BOOL GazeboExtender::InitNewGameMapItemSetup(H3MapItem *mapItem, int typeCount, int subtypeCount) const noexcept
{

    auto gazebo = extender::GetFromMapItem<H3MapItemGazebo>(mapItem);
    gazebo->id = subtypeCount;
    return true;
}

_LHF_(Game__NewGameBeforeSetObjectsInitialParameters)
{
    H3MapItemGazebo::gazeboCounter = 0;
    return EXEC_DEFAULT;
}

BOOL GazeboExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const int interactPlayerId,
                                           const BOOL isRightClick) const noexcept
{

    const auto gazebo = extender::GetFromMapItem<H3MapItemGazebo>(mapItem);
    H3String objName = extender::GetObjectName(mapItem);

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

GazeboExtender *GazeboExtender::instance = nullptr;

GazeboExtender &GazeboExtender::Get()
{
    if (!instance)
        instance = new GazeboExtender();
    return *instance;
}
} // namespace gazebo
