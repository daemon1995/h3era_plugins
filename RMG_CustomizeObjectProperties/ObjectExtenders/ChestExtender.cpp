#include "../pch.h"
namespace chests
{
// int H3MapItemGazebo::gazeboCounter = 0;

ChestsExtender::ChestsExtender() : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.ChestExtender.daemon_n"))
{

    //  CreatePatches();

    // Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");
}

ChestsExtender::~ChestsExtender()
{
}

DWORD ChestsExtender::WoGObjectHasOptionEnabled(int option) noexcept
{
    return DwordAt(WOG_OPTIONS_ARRAY + option * 4);
}

BOOL ChestsExtender::InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept
{
    if (mapItem->objectType == eObject::TREASURE_CHEST)
        return true;

    /*
    if (auto chest = ChestsExtender::GetFromMapItem(mapItem))
    {
        // gazebo->id = H3MapItemGazebo::gazeboCounter++;
        return true;
    }
    */

    return false;
}

/*
_LHF_(Game__NewGameBeforeSetObjectsInitialParameters)
{
    H3MapItemGazebo::gazeboCounter = 0;

    return EXEC_DEFAULT;
}
*/

/*
BOOL GazeboExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const H3Player *activePlayer,
                                           const BOOL isRightClick) const noexcept
{

    if (const auto gazebo = H3MapItemGazebo::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVistedByHero = H3MapItemGazebo::IsVisitedByHero(*gazebo, hero);
            sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ",
                    P_GeneralText->GetText(isVistedByHero ? 354 : 355));
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}
*/

/*
void ChestsExtender::CreatePatches()
{
    if (!m_isInited)
    {
        //// Era::RegisterHandler
        //// 004BFCBE
        //_pi->WriteLoHook(0x04BFCBE, Game__NewGameBeforeSetObjectsInitialParameters);

        m_isInited = true;
    }
}
*/

/*
inline H3MapItemGazebo *H3MapItemGazebo::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_OBJECT_TYPE && mapItem->objectSubtype == GAZEBO_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemGazebo *>(reinterpret_cast<const H3MapItemGazebo *>(&mapItem->setup));
    }

    return nullptr;
}
*/

H3RmgObjectGenerator *ChestsExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == h3::eObject::TREASURE_CHEST)
    {
        if (objectInfo.subtype >= 7 && objectInfo.subtype <= 10 && (WoGObjectHasOptionEnabled(3) == 1))
            return nullptr;

        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

ChestsExtender &ChestsExtender::Get()
{
    static ChestsExtender _instance;
    return _instance;
}
} // namespace chests
