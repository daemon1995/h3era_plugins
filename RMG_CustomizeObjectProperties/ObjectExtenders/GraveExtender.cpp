#include "../pch.h"
namespace grave
{
    GraveExtender::GraveExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.GraveExtender.daemon_n"))
{

    CreatePatches();
}

    GraveExtender::~GraveExtender()
{
}

BOOL GraveExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player,
                                               int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept
{
    if (auto object = H3MapItemGrave::GetFromMapItem(mapItem))
    {
        // Pattern used: 0042FFF2 (AI digs grail)
        //int x = pos.GetX(); int y = pos.GetY(); int z = pos.GetZ();

        //if (hero->x == x && hero->y == y && hero->z == z)
        //{
        //    if (hero->movement == hero->maxMovement)
        //    {
        //        // TryToDigGrail(o_AdvMgr, ai_hero->x, ai_hero->y, ai_hero->l); 0040EBF0
        //        THISCALL_4(void, 0x040EBF0, P_AdventureMgr->Get(), hero->x, hero->y, hero->z);
        //        return true;
        //    }
        //    else
        //    {
        //        hero->movement = 0;
        //        aiMapItemWeight = 0;
        //        return true;
        //    }
        //}
        //else if (*moveDistance < 400)

        if (*moveDistance < 400)
        {
            *moveDistance = 0;
            aiMapItemWeight = 10000;
        }
        else
        {
            aiMapItemWeight = 50;
        }

        return true;
    }

    return false;
}

void ShowMessage(const H3MapItem* mapItem, const bool isDigging, const int goldAmount, const int artId)
{
    const bool skipMapMessage = globalPatcher->VarValue<int>("HD.UI.AdvMgr.SkipMapMsgs");

    H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));

    if (isDigging)
    {
        objName.Append(EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.text.visited", mapItem->objectType, mapItem->objectSubtype)
            .String()));

        H3PictureCategories picOne(ePictureCategories::ARTIFACT, artId);
        H3PictureCategories picTwo(ePictureCategories::GOLD, goldAmount);
        H3Messagebox::Show(objName, picOne, picTwo);
    }
    else
    {
        objName.Append(EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.text.visit", mapItem->objectType, mapItem->objectSubtype)
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

void GraveDiggingProccess(H3MapItem* mapItem, H3Hero* hero)
{
    // get grave data
    H3MapItemGrave* grave = H3MapItemGrave::GetFromMapItem(mapItem);
    const int goldAmount = grave->goldAmount * 500;
    const int artId = grave->artifactID;

    // add resource
    THISCALL_3(void, 0x04E3870, hero, eResource::GOLD, goldAmount);
    // add artifact Hero::AddArtifact(Hero, &P2, 1, 1);
    H3Artifact artifact = H3Artifact(artId, -1);
    THISCALL_4(void, 0x04E32E0, hero, &artifact, 1, 1);
    // add debuff
    hero->moraleBonus -= 3;

    //Player_IsHuman(o_ActivePlayer)
    H3Player* activePlayer = P_ActivePlayer->Get();
    BOOL isHuman = activePlayer->is_human;
    //BOOL isHuman = activePlayer->is_human2;
    //BOOL isHuman = THISCALL_1(BOOL, 0x04BAA60, P_ActivePlayer->Get());
    if (isHuman)
    {
        ShowMessage(mapItem, 1, goldAmount, artId);
    }

    //AdvMgr_Delete_Object_OnMap(AdvMng, ItemAtCoords, PosMixed, 1); try 0
    H3Point coordinates = mapItem->GetCoordinates();
    int x = coordinates.x;
    int y = coordinates.y;
    int z = coordinates.z;
    H3Position pos = H3Position(x, y, z);
    THISCALL_4(void, 0x049DDE0, P_AdventureManager->Get(), mapItem, pos.Mixed(), 0);
    //Game::Place_Object(o_Game, vX, vY, L, HOLE, -1, -1);
    THISCALL_7(void, 0x04C9550, P_Game->Get(), x, y, z, eObject::HOLE, -1, -1);
}

BOOL GraveExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                         const BOOL isHuman) const noexcept
{
    if (auto objSetup = H3MapItemGrave::GetFromMapItem(mapItem))
    {
        if (isHuman)
        {
            ShowMessage(mapItem, 0, 0, 0);
        }
        else
        {
            hero->movement = 0;
            GraveDiggingProccess(mapItem, hero); // AI instantly digs
        }
        return true;
    }

    return false;
}

BOOL GraveExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const H3Player *activePlayer,
                                                  const BOOL isRightClick) const noexcept
{
    if (H3MapItemGrave::GetFromMapItem(mapItem))
    {
        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return false;
}

_LHF_(OnDiggingBeforePuttingAHole)
{
    int x = c->esi;
    int y = c->ebx;
    int z = IntAt(c->ebp+0x10);

    H3MapItem *mapItem = P_AdventureMgr->Get()->GetMapItem(x, y, z);

    if (mapItem->objectType == extender::ERA_OBJECT_TYPE && mapItem->objectSubtype == GRAVE_OBJECT_SUBTYPE)
    {
        H3Hero* hero = *reinterpret_cast<H3Hero**>(c->ebp - 0x10);

        GraveDiggingProccess(mapItem, hero);

        c->return_address = 0x040F080;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}

_LHF_(OnCheckPlaceIsValidToDig)
{
    H3MapItem *mapItem = reinterpret_cast<H3MapItem *>(c->ecx);

    if (mapItem->objectType == extender::ERA_OBJECT_TYPE && mapItem->objectSubtype == GRAVE_OBJECT_SUBTYPE)
    {
        c->return_address = 0x4FD40A;
        return NO_EXEC_DEFAULT;
    }
	
    return EXEC_DEFAULT;
}

void GraveExtender::CreatePatches()
{
    if (!m_isInited)
    {
        _pi->WriteLoHook(0x40EE89, OnDiggingBeforePuttingAHole);
        _pi->WriteLoHook(0x4FD3B7, OnCheckPlaceIsValidToDig);

        m_isInited = true;
    }
}

inline H3MapItemGrave* H3MapItemGrave::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::ERA_OBJECT_TYPE &&
        mapItem->objectSubtype == GRAVE_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemGrave*>(reinterpret_cast<const H3MapItemGrave*>(&mapItem->setup));
    }

    return nullptr;
}

H3RmgObjectGenerator * GraveExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{
    if (objectInfo.type == extender::ERA_OBJECT_TYPE && objectInfo.subtype == GRAVE_OBJECT_SUBTYPE)
    {
        return CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

BOOL GraveExtender::InitNewGameMapItemSetup(H3MapItem* mapItem) const noexcept
{
    if (const auto grave = H3MapItemGrave::GetFromMapItem(mapItem))
    {
        *grave = {};
        const int intMax = 8;
        const int intMin = 3;
        int range = intMax - intMin + 1;
        grave->goldAmount = rand() % range + intMin;  // rnd 3-8 included, *500 during diging
        
        //v121 = Random(0, 99);
        const int v121 = rand() % 100;
        //v122 = v121 >= 50 ? 4 : 2; //minor or treasure
        const int artLvl = v121 >= 50 ? 4 : 2;
        //v123 = map_item->SetUp & 0xFF80001F | ((GameMgr_GenRandomArt_ByLevel(o_Game, v122) & 0x3FF) << 13); 13 bit of warrior tomb is artId
        const int artId = THISCALL_2(int, 0x4C9190, P_Game->Get(), artLvl) & 0x3FF; // Restrict the artNum to 1023 (10 bit info)
        grave->artifactID = artId;

        return true;
    }
    return false;
}

GraveExtender& GraveExtender::Get()
{
    static GraveExtender _instance;
    return _instance;
}
} // namespace grave
