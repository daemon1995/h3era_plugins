#include "../pch.h"
namespace skeletonTransformer
{

SkeletonTransformerExtender::SkeletonTransformerExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.SkeletonTransformerExtender.daemon_n"))
{
    CreatePatches();
}

SkeletonTransformerExtender::~SkeletonTransformerExtender()
{
}

BOOL SkeletonTransformerExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player,
                                                     int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept
{
    // TODO: look at: AI town manager (or visiting) - transformer
    // if (auto skeletonTransformer = H3MapItemSkeletonTransformer::GetFromMapItem(mapItem))
    //{

    //    if (P_ActivePlayer->playerResources.gold >= GOLD_REQUIRED)
    //    {
    //        // адрес похожего псевдокода 0052BB89
    //        const float aiExperience = EXP_GIVEN * hero->AI_experienceEffectiveness;
    //        aiMapItemWeight =
    //            static_cast<int>(aiExperience - player->resourceImportance[eResource::GOLD] * GOLD_REQUIRED);

    //    }

    //    return true;
    //}

    return false;
}

BOOL SkeletonTransformerExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                               const BOOL isHuman) const noexcept
{

    if (auto skeletonTransformer = H3MapItemSkeletonTransformer::GetFromMapItem(mapItem))
    {
        if (isHuman)
        {
            H3Army* army = &hero->army;
            char dlgBuffer[1024]; // Memory for dlg
            H3Dlg* dlg = (H3Dlg*)dlgBuffer;
            THISCALL_2(void, 0x0565770, dlg, army);         // Dlg_SkeletTransformer_Create
            P_MouseManager->SetCursor(0, eCursor::DEFAULT); // Change cursor to default
            THISCALL_2(void, 0x05FFA20, dlg, 0);            // H3BaseDlg::ShowAndRun
            THISCALL_1(void, 0x05661E0, dlg);               // Dlg_SkeletTransformer_Destroy
            memset(Era::z[1], 0, 512);                      // Clear memory
        }

        return true;
    }

    return false;
}

BOOL SkeletonTransformerExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero,
                                                        const H3Player *activePlayer,
                                                        const BOOL isRightClick) const noexcept
{
    if (const auto gazebo = H3MapItemSkeletonTransformer::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);
        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}

void SkeletonTransformerExtender::CreatePatches()
{
    if (!m_isInited)
    {
        // Era::RegisterHandler
        // 004BFCBE
        //_pi->WriteLoHook(0x04BFCBE, Game__NewGameBeforeSetObjectsInitialParameters);

        m_isInited = true;
    }
}

inline H3MapItemSkeletonTransformer *H3MapItemSkeletonTransformer::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_OBJECT_TYPE &&
        mapItem->objectSubtype == SKELETON_TRANSFORMER_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemSkeletonTransformer *>(
            reinterpret_cast<const H3MapItemSkeletonTransformer *>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator *SkeletonTransformerExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::HOTA_OBJECT_TYPE && objectInfo.subtype == SKELETON_TRANSFORMER_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

SkeletonTransformerExtender &SkeletonTransformerExtender::Get()
{
    static SkeletonTransformerExtender _instance;
    return _instance;
}
} // namespace skeletonTransformer
