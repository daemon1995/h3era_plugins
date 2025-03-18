#include "../pch.h"
namespace colosseumOfTheMagi
{
int H3MapItemColosseumOfTheMagi::colosseumOfTheMagiCounter = 0;

ColosseumOfTheMagiExtender::ColosseumOfTheMagiExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.ColosseumOfTheMagiExtender.daemon_n"))
{

    CreatePatches();
}

ColosseumOfTheMagiExtender::~ColosseumOfTheMagiExtender()
{
}

BOOL ColosseumOfTheMagiExtender::SetAiMapItemWeight(H3MapItem *mapItem, const H3Hero *hero, const H3Player *player,
                                                    int &aiMapItemWeight) const noexcept
{

    if (auto colosseumOfTheMagi = H3MapItemColosseumOfTheMagi::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemColosseumOfTheMagi::IsVisitedByHero(colosseumOfTheMagi, hero);

        if (!isVisitedByHero)
        {
            // Код из арены для ИИ
            int needExpoToNextLvl = h3functions::NeedExpoToNextLevel(hero->level);
            float moveDist = (float)(2 * needExpoToNextLvl);
            aiMapItemWeight = static_cast<int>(moveDist * hero->AI_experienceEffectiveness);
        }

        return true;
    }

    return false;
}

BOOL H3MapItemColosseumOfTheMagi::IsVisitedByHero(const H3MapItemColosseumOfTheMagi *colosseumOfTheMagi,
                                                  const H3Hero *hero) noexcept
{
    sprintf(h3_TextBuffer, ErmVariableFormat, colosseumOfTheMagi->id, hero->id);

    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

void ShowMessage(const H3MapItem *mapItem) // , const int playerGoldAmount, const bool isVisitedByHero)
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

H3Messagebox::ePick AskQuestionWithTwoOptions(const H3MapItem *mapItem)
{
    H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));

    objName.Append(EraJS::read(
        H3String::Format("RMG.objectGeneration.%d.%d.text.visit", mapItem->objectType, mapItem->objectSubtype)
            .String()));
    H3PictureCategories picOne(ePictureCategories::SPELL_POWER, 2);
    H3PictureCategories picTwo(ePictureCategories::KNOWLEDGE, 2);

    return H3Messagebox::Choose(objName, picOne, picTwo);
}

BOOL ColosseumOfTheMagiExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                              const BOOL isHuman) const noexcept
{

    if (auto colosseumOfTheMagi = H3MapItemColosseumOfTheMagi::GetFromMapItem(mapItem))
    {

        const bool isVisitedByHero = H3MapItemColosseumOfTheMagi::IsVisitedByHero(colosseumOfTheMagi, hero);

        if (!isVisitedByHero)
        {
            if (isHuman)
            {
                const auto advMgr = P_AdventureManager->Get();
                // void __thiscall AdvMgr_SetTimer(_AdvMgr_ *this, int timerType, signed int addedTime)
                THISCALL_3(void, 0x415CC0, advMgr, 0, -1);

                //void __thiscall AdvMgr_UpdateInfoPanel(_AdvMgr_ *this, int a2, char needRedraw, char a4)
                THISCALL_4(void, 0x0415D40, advMgr, 0, 0, 1);

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
                            hero->id);                          // получение имени переменной
                    Era::SetAssocVarIntValue(h3_TextBuffer, 1); // отметить переменную, что объект посещен
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
                        hero->id);                          // получение имени переменной
                Era::SetAssocVarIntValue(h3_TextBuffer, 1); // отметить переменную, что объект посещен
            }
            return true;
        }

        if (isHuman)
        {
            ShowMessage(mapItem); // , playerGoldBeforeVisit, isVisitedByHero);
        }

        return true;
    }

    return false;
}

BOOL ColosseumOfTheMagiExtender::InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept
{

    if (auto colosseumOfTheMagi = H3MapItemColosseumOfTheMagi::GetFromMapItem(mapItem))
    {
        colosseumOfTheMagi->id = H3MapItemColosseumOfTheMagi::colosseumOfTheMagiCounter++;
        return true;
    }

    return false;
}

_LHF_(Game__NewGameBeforeSetObjectsInitialParameters)
{
    H3MapItemColosseumOfTheMagi::colosseumOfTheMagiCounter = 0;

    return EXEC_DEFAULT;
}

BOOL ColosseumOfTheMagiExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero,
                                                       const H3Player *activePlayer,
                                                       const BOOL isRightClick) const noexcept
{

    if (const auto colosseumOfTheMagi = H3MapItemColosseumOfTheMagi::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

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

    return false;
}

void ColosseumOfTheMagiExtender::CreatePatches()
{
    if (!m_isInited)
    {
        // Era::RegisterHandler
        // 004BFCBE
        _pi->WriteLoHook(0x04BFCBE, Game__NewGameBeforeSetObjectsInitialParameters);

        m_isInited = true;
    }
}

inline H3MapItemColosseumOfTheMagi *H3MapItemColosseumOfTheMagi::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_OBJECT_TYPE &&
        mapItem->objectSubtype == COLOSSEUM_OF_THE_MAGI_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemColosseumOfTheMagi *>(
            reinterpret_cast<const H3MapItemColosseumOfTheMagi *>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator *ColosseumOfTheMagiExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::HOTA_OBJECT_TYPE && objectInfo.subtype == COLOSSEUM_OF_THE_MAGI_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

ColosseumOfTheMagiExtender &ColosseumOfTheMagiExtender::Get()
{
    static ColosseumOfTheMagiExtender _instance;
    return _instance;
}
} // namespace colosseumOfTheMagi
