#include "../pch.h"
namespace ancientLamp
{

AncientLampExtender::AncientLampExtender() : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.AncientLampExtender.daemon_n"))
{
    CreatePatches();
}

AncientLampExtender::~AncientLampExtender()
{
}

BOOL AncientLampExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player,
                                        int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept
{
    if (auto ancientLamp = H3MapItemAncientLamp::GetFromMapItem(mapItem))
    {
        // AIHero_GetBuyingStacksValue_RefugeeCamp 0052ABF0
        // return AIHero_GetBuyingStacksValue_RefugeeCamp(hero, mapItem->SubType, mapItem->SetUp);
        aiMapItemWeight = FASTCALL_3(int, 0x052ABF0, hero, ancientLamp->creatureType, ancientLamp->creatureCount);

        return true;
    }

    return false;
}

BOOL AskQuestion(const H3MapItem *mapItem)
{
    const bool skipMapMessage = globalPatcher->VarValue<int>("HD.UI.AdvMgr.SkipMapMsgs");

    if (skipMapMessage)
    {
        return true;
    }
    else
    {
        H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));

        objName.Append(EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.text.visit", mapItem->objectType, mapItem->objectSubtype)
            .String()));

        return H3Messagebox::Choice(objName);
    }
}

BOOL AncientLampExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                  const BOOL isHuman) const noexcept
{

    if (auto ancientLamp = H3MapItemAncientLamp::GetFromMapItem(mapItem))
    {
        // set object as visited
        THISCALL_2(void, 0x4FC620, mapItem, hero->owner);

        BOOL agreed; // = !isHuman;
        if (isHuman)
        {
            agreed = AskQuestion(mapItem);

            if (agreed)
            {
                INT16 creatureCount = ancientLamp->creatureCount;
                H3RecruitMgr mgr(hero->army, false, ancientLamp->creatureType, creatureCount);
                mgr.Run();
                ancientLamp->creatureCount = creatureCount;
            }
        } 
        else
        {
            // sub_005282F0(hero, SubType, &isAI);
            INT16 creatureCount = ancientLamp->creatureCount;
            FASTCALL_3(void, 0x05282F0, hero, ancientLamp->creatureType, &creatureCount);
			ancientLamp->creatureCount = creatureCount;
        }
        // Delete the object if creatureCount == 0
        if (ancientLamp->creatureCount == 0)
        {
            // Delete the object
            THISCALL_4(void, 0x049DDE0, P_AdventureManager->Get(), mapItem, pos.Mixed(), isHuman);
        }

        return true;
    }

    return false;
}

BOOL AncientLampExtender::InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept
{

    if (const auto ancientLamp = H3MapItemAncientLamp::GetFromMapItem(mapItem))
    {
        *ancientLamp = {};
        ancientLamp->creatureType = ANCIENT_LAMP_CREATURE_TYPE;
        ancientLamp->creatureCount = rand() % (ANCIENT_LAMP_MAX_CREATURE_COUNT - ANCIENT_LAMP_MIN_CREATURE_COUNT + 1) + ANCIENT_LAMP_MIN_CREATURE_COUNT; // from 4 to 6
        ancientLamp->Reset();

        return true;
    }

    return false;
}

BOOL AncientLampExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const H3Player *activePlayer,
                                           const BOOL isRightClick) const noexcept
{

    if (const auto ancientLamp = H3MapItemAncientLamp::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            // Object scouting - use only for objects with "visited" property starting from 5 bit offset
            const bool isVisitedByPlayer = THISCALL_2(bool, 0x529B70, mapItem, H3HumanID::Get());

            if (isVisitedByPlayer)
            {
                // Read string from json
                H3String hint = EraJS::read(
                    H3String::Format("RMG.objectGeneration.%d.%d.text.hint", mapItem->objectType, mapItem->objectSubtype)
                    .String());

                // Paste into hint one string (plural creature name to %s) and one integer (creature amount to %d)
                // Paste into buffer 2 strings: delimeter and hint
                sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ", H3String::Format(hint.String(), P_CreatureInformation[ANCIENT_LAMP_CREATURE_TYPE].namePlural, ancientLamp->creatureCount).String());

                // Append buffer string to object name
                objName.Append(h3_TextBuffer);
            }
        }

        sprintf(h3_TextBuffer, "%s", objName.String());


        return true;
    }

    return false;
}

void AncientLampExtender::CreatePatches()
{
    if (!m_isInited)
    {
        m_isInited = true;
    }
}

inline H3MapItemAncientLamp* H3MapItemAncientLamp::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_PICKUPABLE_OBJECT_TYPE && mapItem->objectSubtype == ANCIENT_LAMP_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemAncientLamp*>(reinterpret_cast<const H3MapItemAncientLamp*>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator * AncientLampExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::HOTA_PICKUPABLE_OBJECT_TYPE && objectInfo.subtype == ANCIENT_LAMP_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

void H3MapItemAncientLamp::Reset()
{
    visited = 0;
}

AncientLampExtender& AncientLampExtender::Get()
{
    static AncientLampExtender _instance;
    return _instance;
}
} // namespace ancientLamp
