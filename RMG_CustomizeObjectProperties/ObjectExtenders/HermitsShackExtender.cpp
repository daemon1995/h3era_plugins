#include "../pch.h"
namespace hermitsShack
{
int H3MapItemHermitsShack::hermitsShackCounter = 0;

HermitsShackExtender::HermitsShackExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.HermitsShackExtender.daemon_n"))
{

    CreatePatches();
}

HermitsShackExtender::~HermitsShackExtender()
{
}

BOOL HermitsShackExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player,
                                              int &aiMapItemWeight, int *moveDistance,
                                              const H3Position pos) const noexcept
{
    if (auto hermitsShack = H3MapItemHermitsShack::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemHermitsShack::IsVisitedByHero(hermitsShack, hero);

        if (!isVisitedByHero)
        {
            // Pattern used: return AIHero_GetObjectPosWeight_WitchHut(hero, mapItem); 0052BDE0

            const int heroOwner = hero->owner;
            bool heroHasBasicOrAdvancedSkill = FALSE;
            int i = 0;
            while (i < 28 && !heroHasBasicOrAdvancedSkill)
            {
                if (hero->secSkill[i] < 3 && hero->secSkill[i] > 0)
                {
                    heroHasBasicOrAdvancedSkill = TRUE;
                    break;
                }
                i++;
            }
            if (!heroHasBasicOrAdvancedSkill)
            {
                return false;
            }

            // 00528567
            // LABEL_76:
            //*(float*)&moveDist = (float)NeedExpoToNextLevel(hp->Level);
            //*(_QWORD*)&v8 = (__int64)(*(float*)&moveDist * hp->turnExperienceToRVRatio);
            //*(_QWORD*)&v8 = (__int64)(*(float*)&moveDist * hero->AI_experienceEffectiveness);
            // return v8;

            aiMapItemWeight =
                static_cast<float>(h3functions::NeedExpoToNextLevel(hero->level)) * hero->AI_experienceEffectiveness;

            return true;
        }
    }

    return false;
}

BOOL H3MapItemHermitsShack::IsVisitedByHero(const H3MapItemHermitsShack *hermitsShack, const H3Hero *hero) noexcept
{
    sprintf(h3_TextBuffer, ErmVariableFormat, hermitsShack->id, hero->id);

    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

void ShowMessage(const H3MapItem *mapItem, const int skillId, const int skillLvl, const bool isVisitedByHero)
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
    else if (skillId == -1)
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
    else
    {
        objName.Append(EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.text.visit", mapItem->objectType, mapItem->objectSubtype)
                .String()));
        int skillPic = skillId * 3 + skillLvl + 3; // В оригинале +2, но нужен скилл на лвл выше

        // if (skipMapMessage)
        //{
        //     // TODO: должна быть картинка
        //     THISCALL_4(void, 0x415FC0, P_AdventureMgr->Get(), objName.String(), -1, -1);
        // }
        // else
        //{
        H3PictureCategories pics(ePictureCategories::SECONDARY_SKILL, skillPic);
        H3Messagebox::Show(objName, pics);
        //}
    }
}

BOOL HermitsShackExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                        const BOOL isHuman) const noexcept
{

    if (auto hermitsShack = H3MapItemHermitsShack::GetFromMapItem(mapItem))
    {
        const bool isVisitedByHero = H3MapItemHermitsShack::IsVisitedByHero(hermitsShack, hero);

        // TODO: сейчас апается скилл с минимальным ID, подходящим по условию. нужен rand()
        bool heroHasBasicOrAdvancedSkill = FALSE;
        int skillId = -1;
        int skillLvl = 0;

        // Check basic or advanced secondary skills
        int i = 0;
        while (i < 28 && !heroHasBasicOrAdvancedSkill)
        {
            if (hero->secSkill[i] < 3 && hero->secSkill[i] > 0)
            {
                heroHasBasicOrAdvancedSkill = TRUE;
                skillId = i;
                skillLvl = hero->secSkill[i];
            }
            i++;
        }

        if (!isVisitedByHero)
        {
            if (skillId != -1)
            {
                // TODO: H3Random()
                // BOOL ski = rand()

                // Апаем скилл
                hero->secSkill[skillId] = skillLvl + 1;
                // Меняем переменную
                sprintf(h3_TextBuffer, H3MapItemHermitsShack::ErmVariableFormat, hermitsShack->id,
                        hero->id);                          // получение имени переменной
                Era::SetAssocVarIntValue(h3_TextBuffer, 1); // отметить переменную, что объект посещен
            }
        }

        if (isHuman)
        {
            ShowMessage(mapItem, skillId, skillLvl, isVisitedByHero);
        }
        return true;
    }

    return false;
}

BOOL HermitsShackExtender::InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept
{

    if (auto hermitsShack = H3MapItemHermitsShack::GetFromMapItem(mapItem))
    {
        hermitsShack->id = H3MapItemHermitsShack::hermitsShackCounter++;
        return true;
    }

    return false;
}

_LHF_(Game__NewGameBeforeSetObjectsInitialParameters)
{
    H3MapItemHermitsShack::hermitsShackCounter = 0;

    return EXEC_DEFAULT;
}

BOOL HermitsShackExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const H3Player *activePlayer,
                                                 const BOOL isRightClick) const noexcept
{

    if (const auto hermitsShack = H3MapItemHermitsShack::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVisitedByHero = H3MapItemHermitsShack::IsVisitedByHero(hermitsShack, hero);
            sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ",
                    P_GeneralText->GetText(isVisitedByHero ? 354 : 355));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}

void HermitsShackExtender::CreatePatches()
{
    if (!m_isInited)
    {
        // Era::RegisterHandler
        // 004BFCBE
        _pi->WriteLoHook(0x04BFCBE, Game__NewGameBeforeSetObjectsInitialParameters);

        m_isInited = true;
    }
}

inline H3MapItemHermitsShack *H3MapItemHermitsShack::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_OBJECT_TYPE &&
        mapItem->objectSubtype == HERMITS_SHACK_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemHermitsShack *>(reinterpret_cast<const H3MapItemHermitsShack *>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator *HermitsShackExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::HOTA_OBJECT_TYPE && objectInfo.subtype == HERMITS_SHACK_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

HermitsShackExtender &HermitsShackExtender::Get()
{
    static HermitsShackExtender _instance;
    return _instance;
}
} // namespace hermitsShack
