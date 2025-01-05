#include "../pch.h"
namespace junkman
{

JunkmanExtender::JunkmanExtender() : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.JunkmanExtender.daemon_n"))
{
    CreatePatches();
}

JunkmanExtender::~JunkmanExtender()
{
}

BOOL JunkmanExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player,
                                        int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept
{
    //if (H3MapItemJunkman::GetFromMapItem(mapItem))
    //{
    //    // 005288DE blackMarket AIcalc
    //    // if ( Hero_Art_GetCount_InBackPack(hero, 1) == 64 )
    //    //     return 0;
    //    if (THISCALL_2(int, 0x4D9330, hero, 0) == 0) // count without war machines
    //    {
    //        aiMapItemWeight = 0;
    //        return true;
    //    }


    //    //if (P_ActivePlayer->playerResources.gold >= GOLD_REQUIRED)
    //    //{
    //    //    // адрес похожего псевдокода 0052BB89
    //    //    const float aiExperience = EXP_GIVEN * hero->AI_experienceEffectiveness;
    //    //    aiMapItemWeight =
    //    //        static_cast<int>(aiExperience - player->resourceImportance[eResource::GOLD] * GOLD_REQUIRED);

    //    //}

    //    return true;
    //}

    return false;
}

BOOL JunkmanExtender::VisitMapItem(H3Hero* hero, H3MapItem* mapItem, const H3Position pos,
    const BOOL isHuman) const noexcept
{
    if (auto junkman = H3MapItemJunkman::GetFromMapItem(mapItem))
    {
        if (isHuman)
        {
            // patch object name
            H3String str = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem)).String();
            LPCSTR objName = str.String();
            const DWORD prevStr = DwordAt(0x05EC3CE + 2); // store object name
            DwordAt(0x05EC3CE + 2) = (int)&objName; // patch object name

            // jump button building in dlg build (check in debugger)
            auto patch = _pi->WriteHexPatch(0x05E5F7C, "E9 FA000000"); // if jump is less than 128 byte, use EB

            // Town_ArtMarket dlg after init
            IntAt(0x06AAAE0) = (int)hero;
            IntAt(0x06AAB0C) = 3;  // marker type - art marker sell (3)
            IntAt(0x06AAB00) = 7;  // markets amount owned by player
            IntAt(0x06AAADC) = (int)&P_Game->artifactMerchant;
            CDECL_0(void, 0x05EA530);

            // restore global vars
            //IntAt(0x06AAB0C) = 0; // marker type
            IntAt(0x06AAB00) = 0; // markets amount owned by player
            DwordAt(0x05EC3CE + 2) = prevStr; // restore object name

            // cancel patch
            patch->Destroy();
        }

        // for AI need to write
        // 005248E0 AI_Visit_BlackMarket

        return true;
    }

    return false;
}

BOOL JunkmanExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const H3Player *activePlayer,
                                           const BOOL isRightClick) const noexcept
{
    if (const auto gazebo = H3MapItemJunkman::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);
        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}

void JunkmanExtender::CreatePatches()
{
    if (!m_isInited)
    {
        // globalVar = 0 // common market
        // globalVar = 1 // transfer resources to another player
        // globalVar = 2 // art marker buy
        // globalVar = 3 // art marker sell
        // globalVar = 4 // mercenary guild
        
        // TPMrkBu1.def - button mod 1 - dialog mod 3
        // TPMrkBu2.def - button mod 2 - dialog mod 3
        // TPMrkBu3.def - button mod 3
        // TPMrkBu4.def - button mod 4
        // TPMrkBu5.def - button mod 0 - dialog mod 3
        // TPMrkBu6.def - ??????
        
        m_isInited = true;
    }
}

inline H3MapItemJunkman* H3MapItemJunkman::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_OBJECT_TYPE && mapItem->objectSubtype == JUNKMAN_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemJunkman*>(reinterpret_cast<const H3MapItemJunkman*>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator * JunkmanExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::HOTA_OBJECT_TYPE && objectInfo.subtype == JUNKMAN_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

JunkmanExtender& JunkmanExtender::Get()
{
    static JunkmanExtender _instance;
    return _instance;
}
} // namespace junkman
