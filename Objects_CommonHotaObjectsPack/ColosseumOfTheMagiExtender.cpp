#include "ColosseumOfTheMagiExtender.h"

namespace colosseumOfTheMagi
{
BOOL H3MapItemColosseumOfTheMagi::IsVisitedByHero(const H3MapItemColosseumOfTheMagi *colosseumOfTheMagi,
                                                  const H3Hero *hero) noexcept
{
    sprintf(h3_TextBuffer, ErmVariableFormat, colosseumOfTheMagi->id, hero->id);

    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

ColosseumOfTheMagiExtender *ColosseumOfTheMagiExtender::instance = nullptr;

ColosseumOfTheMagiExtender &ColosseumOfTheMagiExtender::Get()
{
    if (!instance)
        instance = new ColosseumOfTheMagiExtender();
    return *instance;
}

ColosseumOfTheMagiExtender::ColosseumOfTheMagiExtender() : ObjectExtender(_PI)
{
    CreatePatches();
}
void ColosseumOfTheMagiExtender::AfterLoadingObjectsTxtProc(const INT16 *maxSubtypes)
{
    using namespace extender;
    AddUniqueObjectInfo(UniqueObjectInfo{HOTA_OBJECT_TYPE, 2, 100});
}

BOOL ColosseumOfTheMagiExtender::InitNewGameMapItemSetup(H3MapItem *mapItem, int typeCount,
                                                         int subtypeCount) const noexcept
{
    extender::GetFromMapItem<H3MapItemColosseumOfTheMagi>(mapItem)->id = subtypeCount;
    return true;
}

H3Messagebox::ePick AskQuestionWithTwoOptions(const H3MapItem *mapItem)
{
    H3String objName = H3String::Format("{%s}", extender::GetObjectName(mapItem));

    objName.Append(EraJS::read(
        H3String::Format("RMG.objectGeneration.%d.%d.text.visit", mapItem->objectType, mapItem->objectSubtype)
            .String()));
    H3PictureCategories picOne(ePictureCategories::SPELL_POWER, 2);
    H3PictureCategories picTwo(ePictureCategories::KNOWLEDGE, 2);

    return H3Messagebox::Choose(objName, picOne, picTwo);
}

void ShowMessage(const H3MapItem *mapItem) // , const int playerGoldAmount, const bool isVisitedByHero)
{
    const bool skipMapMessage = globalPatcher->VarValue<int>("HD.UI.AdvMgr.SkipMapMsgs");

    H3String objName = H3String::Format("{%s}", extender::GetObjectName(mapItem));

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

BOOL ColosseumOfTheMagiExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                              const BOOL isHuman) const noexcept
{

    auto colosseumOfTheMagi = extender::GetFromMapItem<H3MapItemColosseumOfTheMagi>(mapItem);

    const bool isVisitedByHero = H3MapItemColosseumOfTheMagi::IsVisitedByHero(colosseumOfTheMagi, hero);

    if (!isVisitedByHero)
    {
        if (isHuman)
        {
            const auto advMgr = P_AdventureManager->Get();
            // void __thiscall AdvMgr_SetTimer(_AdvMgr_ *this, int timerType, signed int addedTime)
            THISCALL_3(void, 0x415CC0, advMgr, 0, -1);

            // void __thiscall AdvMgr_UpdateInfoPanel(_AdvMgr_ *this, int a2, char needRedraw, char a4)
            THISCALL_4(void, 0x0415D40, advMgr, 0, 1, 1);

            H3Messagebox::ePick choice = AskQuestionWithTwoOptions(mapItem);
            if (choice != H3Messagebox::ePick::NONE)
            {
                if (choice == H3Messagebox::ePick::LEFT)
                {
                    hero->primarySkill[3] += 2;
                }
                else if (choice == H3Messagebox::ePick::RIGHT)
                {
                    hero->primarySkill[2] += 2;
                }
                sprintf(h3_TextBuffer, H3MapItemColosseumOfTheMagi::ErmVariableFormat, colosseumOfTheMagi->id,
                        hero->id);                          // ��������� ����� ����������
                Era::SetAssocVarIntValue(h3_TextBuffer, 1); // �������� ����������, ��� ������ �������
            }
        }
        // for AI
        else
        {
            if (hero->primarySkill[3] >= hero->primarySkill[2])
            {
                hero->primarySkill[2] += 2;
            }
            else
            {
                hero->primarySkill[3] += 2;
            }
            sprintf(h3_TextBuffer, H3MapItemColosseumOfTheMagi::ErmVariableFormat, colosseumOfTheMagi->id,
                    hero->id);                          // ��������� ����� ����������
            Era::SetAssocVarIntValue(h3_TextBuffer, 1); // �������� ����������, ��� ������ �������
        }
        return true;
    }

    if (isHuman)
    {
        ShowMessage(mapItem); // , playerGoldBeforeVisit, isVisitedByHero);
    }

    return true;
}
BOOL ColosseumOfTheMagiExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero,
                                                       const int interactPlayerId,
                                                       const BOOL isRightClick) const noexcept
{

    const auto colosseumOfTheMagi = extender::GetFromMapItem<H3MapItemColosseumOfTheMagi>(mapItem);
    H3String objName = extender::GetObjectName(mapItem);

    if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
    {
        const bool isVisitedByHero = H3MapItemColosseumOfTheMagi::IsVisitedByHero(colosseumOfTheMagi, hero);
        sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ",
                P_GeneralText->GetText(isVisitedByHero ? 354 : 355));
        objName.Append(h3_TextBuffer);
    }

    sprintf(h3_TextBuffer, "%s", objName.String());

    return true;
}

BOOL ColosseumOfTheMagiExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *activePlayer,
                                                    int &aiMapItemWeight, int *moveDistance,
                                                    const H3Position pos) const noexcept
{
    auto colosseumOfTheMagi = extender::GetFromMapItem<H3MapItemColosseumOfTheMagi>(mapItem);

    const bool isVisitedByHero = H3MapItemColosseumOfTheMagi::IsVisitedByHero(colosseumOfTheMagi, hero);

    if (!isVisitedByHero)
    {
        // ��� �� ����� ��� ��
        int needExpoToNextLvl = FASTCALL_1(int, 0x04DA690, hero->level);
        float moveDist = (float)(2 * needExpoToNextLvl);
        aiMapItemWeight = static_cast<int>(moveDist * hero->AI_experienceEffectiveness);
    }

    return true;
}

} // namespace colosseumOfTheMagi
