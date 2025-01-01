#include "../pch.h"
namespace gazebo
{
GazeboExtender::GazeboExtender() : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.GazeboExtender.daemon_n"))
{

    CreatePatches();
}

GazeboExtender::~GazeboExtender()
{
}

// Устанавливает ценность объекта для ИИ
_LHF_(GazeboExtender::AIHero_GetObjectPosWeight)
{
    if (H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->esi))
    {
        if (auto gazebo = H3MapItemGazebo::GetFromMapItem(mapItem))
        {
            const H3Hero *hero = reinterpret_cast<H3Hero *>(c->ebx);
            const bool isVistedByHero = H3MapItemGazebo::IsVisitedByHero(*gazebo, hero);

            if (!isVistedByHero)
            {
                if (P_ActivePlayer->playerResources.gold >= GOLD_REQUIRED)
                {
                    // адрес похожего псевдокода 0052BB89
                    const __int64 aiExperience = EXP_GIVEN * hero->AI_experienceEffectiveness;
                    const H3Player *player = *reinterpret_cast<H3Player **>(c->ebp - 0x4);
                    const int aiResWeight =
                        (__int64)((double)aiExperience - player->resourceImportance[eResource::GOLD] * GOLD_REQUIRED);

                    if (aiResWeight)
                    {
                        c->eax = aiResWeight;
                        c->return_address = 0x05285A1;
                        return NO_EXEC_DEFAULT;
                    }
                }
            }
        }
    }

    return EXEC_DEFAULT;
}

BOOL H3MapItemGazebo::IsVisitedByHero(const H3MapItemGazebo gazebo, const H3Hero *hero) noexcept
{
    sprintf(h3_TextBuffer, ErmVariableFormat, gazebo.id, hero->id);

    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

void ShowMessage(const H3MapItem *mapItem, const int playerGoldAmount, const bool isVistedByHero)
{
    const bool skipMapMessage = globalPatcher->VarValue<int>("HD.UI.AdvMgr.SkipMapMsgs");

    H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));

    if (skipMapMessage)
    {
        if (isVistedByHero)
        {
            objName.Append(EraJS::read(
                H3String::Format("RMG.objectGeneration.%d.%d.text.visited", mapItem->objectType, mapItem->objectSubtype)
                    .String()));
        }
        else if (playerGoldAmount < GOLD_REQUIRED)
        {
            objName.Append(EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.text.cannotVisit",
                                                        mapItem->objectType, mapItem->objectSubtype)
                                           .String()));
        }
        THISCALL_4(void, 0x415FC0, P_AdventureMgr->Get(), objName.String(), -1, -1);
    }

    else
    {
        if (isVistedByHero)
        {
            objName.Append(EraJS::read(
                H3String::Format("RMG.objectGeneration.%d.%d.text.visited", mapItem->objectType, mapItem->objectSubtype)
                    .String()));
        }
        else if (playerGoldAmount < GOLD_REQUIRED)
        {
            objName.Append(EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.text.cannotVisit",
                                                        mapItem->objectType, mapItem->objectSubtype)
                                           .String()));
        }
        H3Messagebox::Show(objName);
    }
}

_LHF_(GazeboExtender::H3AdventureManager__ObjectVisit)
{
    if (auto mapItem = reinterpret_cast<H3MapItem *>(c->edi))
    {
        if (auto gazebo = H3MapItemGazebo::GetFromMapItem(mapItem))
        {
            const H3Hero *hero = *reinterpret_cast<H3Hero **>(c->ebp + 0x8);
            const char isHuman = CharAt(c->ebp + 0x14);
            const bool isVisitedByHero = H3MapItemGazebo::IsVisitedByHero(*gazebo, hero);
            const int playerGoldBeforeVisit = P_ActivePlayer->playerResources.gold;

            if (!isVisitedByHero)
            {
                if (playerGoldBeforeVisit >= GOLD_REQUIRED)
                {
                    // Calculate exp given considering the learning power
                    float heroLearningPower = GetHeroLearningPower(hero);
                    const int expGiven = EXP_GIVEN * heroLearningPower;

                    BOOL agreed = !isHuman;
                    if (isHuman)
                    {
                        H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));
                        objName.Append(EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.text.visit",
                                                                    mapItem->objectType, mapItem->objectSubtype)
                                                       .String()));
                        H3PictureCategories pics(ePictureCategories::EXPERIENCE, expGiven);
                        agreed = H3Messagebox::Choice(objName, pics);
                    }
                    if (agreed)
                    {
                        THISCALL_3(void, 0x04E3870, hero, eResource::GOLD, -GOLD_REQUIRED);
                        THISCALL_4(void, 0x04E3620, hero, expGiven, 1, 1);
                        sprintf(h3_TextBuffer, H3MapItemGazebo::ErmVariableFormat, gazebo->id,
                                hero->id);                          // получение имени переменной
                        Era::SetAssocVarIntValue(h3_TextBuffer, 1); // отметить переменную, что объект посещен
                    }
                    return EXEC_DEFAULT;
                }
            }

            if (isHuman)
            {
                ShowMessage(mapItem, playerGoldBeforeVisit, isVisitedByHero);
            }
        }
    }

    return EXEC_DEFAULT;
}

float GetHeroLearningPower(const H3Hero *hero)
{
    return THISCALL_1(float, 0x04E4AB0, hero);
}

int gazeboCounter = 0;
_LHF_(GazeboExtender::Game__NewGameObjectIteration)
{
    auto mapItem = reinterpret_cast<H3MapItem *>(c->esi);
    if (const auto gazebo = H3MapItemGazebo::GetFromMapItem(mapItem))
    {
        gazebo->id = gazeboCounter++;
    }

    return EXEC_DEFAULT;
}

_LHF_(Game__NewGameBeforeSetObjectsInitialParameters)
{
    gazeboCounter = 0;

    return EXEC_DEFAULT;
}

BOOL ShowHint(LoHook *h, HookContext *c, LPCSTR delimiter)
{
    H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->ebx);

    if (const auto gazebo = H3MapItemGazebo::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVistedByHero = H3MapItemGazebo::IsVisitedByHero(*gazebo, hero);
            sprintf(h3_TextBuffer, "%s%s", delimiter, P_GeneralText->GetText(isVistedByHero ? 354 : 355));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        c->edi = (int)h3_TextBuffer;
        c->return_address = h->GetAddress() + 7;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

_LHF_(GazeboExtender::H3AdventureManager__GetDefaultObjectClickHint)
{
    return ShowHint(h, c, "\n\n");
}
_LHF_(GazeboExtender::H3AdventureManager__GetDefaultObjectHoverHint)
{
    return ShowHint(h, c, " ");
}

void GazeboExtender::CreatePatches()
{
    if (!m_isInited)
    {
        // Era::RegisterHandler
        // 004BFCBE
        _pi->WriteLoHook(0x04BFCBE, Game__NewGameBeforeSetObjectsInitialParameters);
        _pi->WriteLoHook(0x04C0A5F, Game__NewGameObjectIteration);
        //_pi->WriteLoHook(0x04C8847, Game__NewWeekObjectIteration);

        _PI->WriteLoHook(0x4A819C, H3AdventureManager__ObjectVisit);
        _PI->WriteLoHook(0x528559, AIHero_GetObjectPosWeight);

        _PI->WriteLoHook(0x40D052, H3AdventureManager__GetDefaultObjectHoverHint); // mouse over hint
        _PI->WriteLoHook(0x415999, H3AdventureManager__GetDefaultObjectClickHint); // rmc hint

        //_pi->WriteLoHook(0x4C1974, Game__AtShrineOfMagicIncantationSettingSpell);
        //_pi->WriteLoHook(0x40D858, Shrine__AtGetName);

        m_isInited = true;
    }
}

inline H3MapItemGazebo *H3MapItemGazebo::GetFromMapItem(H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == GAZEBO_OBJECT_TYPE && mapItem->objectSubtype == GAZEBO_OBJECT_SUBTYPE)
    {
        return reinterpret_cast<H3MapItemGazebo *>(&mapItem->setup);
    }

    return nullptr;
}

GazeboExtender &GazeboExtender::Get()
{
    static GazeboExtender _instance;
    return _instance;
}
} // namespace gazebo
