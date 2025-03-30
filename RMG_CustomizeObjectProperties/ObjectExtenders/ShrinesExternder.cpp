#include "../pch.h"
namespace shrines

{

const H3MapItem *ShrinesExternder::currentShrineHint = nullptr;
ShrinesExternder::ShrinesExternder()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.ShrinesExtender.daemon_n"))
{

    CreatePatches();
}

ShrinesExternder::~ShrinesExternder()
{
}
H3RmgObjectGenerator *ShrinesExternder::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{

    if (objectInfo.type == eObject::SHRINE_OF_MAGIC_INCANTATION ||
        objectInfo.type == eObject::SHRINE_OF_MAGIC_GESTURE || objectInfo.type == eObject::SHRINE_OF_MAGIC_THOUGHT)
    {

        H3RmgObjectGenerator *objGen = H3ObjectAllocator<H3RmgObjectGenerator>().allocate(1);
        objGen = THISCALL_3(H3RmgObjectGenerator *, 0x534EC0, objGen, objectInfo.type, objectInfo.value);
        objGen->subtype = objectInfo.subtype;
        return objGen;
    }
    return nullptr;
}
BOOL ShrinesExternder::SetAiMapItemWeight(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                          int &aiResWeight) const noexcept
{
    if (mapItem->objectSubtype > 0)
    {
        if (mapItem->objectType == eObject::SHRINE_OF_MAGIC_GESTURE && currentHero->level < LEVEL_REQUIRED)
        {
            aiResWeight = 0;
            return true;
        }
        else if (mapItem->objectType == eObject::SHRINE_OF_MAGIC_THOUGHT)
        {
            // aiResWeight = 0;

            //  activePlayer->resourceImportance[eResource::GOLD] * SPELL_COST;
            // return true;
        }
    }

    return false;
}

_LHF_(ShrinesExternder::Game__AtShrineOfMagicIncantationSettingSpell)
{

    // set spell level generated coresponding to map item subtype (allows any level spells);
    if (const H3MapItem *shrines = reinterpret_cast<H3MapItem *>(c->esi))
    {
        ByteAt(0x4C1995 + 1) = Clamp(0, shrines->objectSubtype, 4); // restrict spell levels 1-5 (0_4)
    }
    return EXEC_DEFAULT;
}

_LHF_(ShrinesExternder::Game__AtShrineOfMagicGestureSettingSpell)
{

    // object subtype is spell id
    if (H3MapItem *shrines = reinterpret_cast<H3MapItem *>(c->esi))
    {
        //   shrines->objectSubtype = eSpell::TOWN_PORTAL + 1;
        if (const int objectSubtype = shrines->objectSubtype)
        {
            shrines->magicShrine.spell = objectSubtype - 1;
        }
    }
    return EXEC_DEFAULT;
}
LPCSTR GetVisitText(const H3MapItem *shrine)
{
    return 0;
}
_LHF_(ShrinesExternder::Shrine__AtVisit)
{

    if (H3MapItem *shrine = *reinterpret_cast<H3MapItem **>(c->ebp + 0xC))
    {
        currentShrineHint = shrine;
        if (shrine->objectSubtype != 0)
        {
            *reinterpret_cast<LPCSTR *>(c->ebp + 0x10) = EraJS::read(
                H3String::Format("RMG.objectGeneration.%d.%d.visit", shrine->objectType, shrine->objectSubtype)
                    .String());
        }
    }
    return EXEC_DEFAULT;
}
_LHF_(ShrinesExternder::Shrine__AtWisdomCheck)
{

    const H3MapItem *shrine = currentShrineHint;
    const H3Hero *hero = reinterpret_cast<H3Hero *>(c->edi);

    if (shrine && hero && shrine->objectType == eObject::SHRINE_OF_MAGIC_GESTURE && shrine->objectSubtype > 0 &&
        hero->level < LEVEL_REQUIRED)
    {
        // if human
        if (IntAt(c->ebp + 0x18))
        {
            c->ecx |= 0xFFFFFFFF;
            c->esi = int(EraJS::read(
                H3String::Format("RMG.objectGeneration.%d.%d.level", shrine->objectType, shrine->objectSubtype)
                    .String()));
            c->return_address = 0x04A549A;
        }
        else
        {
            c->return_address = 0x04A551C;
        }
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}
LPCSTR ShrinesExternder::GetCustomName(const H3MapItem *shrine)
{
    LPCSTR result = nullptr;

    if (shrine->objectSubtype != 0)
    {
        currentShrineHint = shrine;
        bool readSucces = false;
        const auto strPtr = EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.name", shrine->objectType, shrine->objectSubtype).String(),
            readSucces);

        // if (readSucces)
        {
            result = strPtr;
        }
    }
    return result;
}
_LHF_(ShrinesExternder::Shrine__AtGetName)
{
    if (H3MapItem *shrine = *reinterpret_cast<H3MapItem **>(c->ebp + 0x8))
    {

        currentShrineHint = nullptr;

        if (auto name = GetCustomName(shrine))
        {
            c->edi = int(name);
            // return after original text set
            c->return_address = 0x40D85F;
            return NO_EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;
}

LPCSTR ShrinesExternder::GetCustomHint(const H3MapItem *shrine)
{
    LPCSTR result = nullptr;

    if (shrine->objectSubtype != 0)
    {
        bool readSucces = false;
        if (shrine->objectType == eObject::SHRINE_OF_MAGIC_INCANTATION ||
            shrine->objectType == eObject::SHRINE_OF_MAGIC_GESTURE)
        {
            auto strPtr = EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.hint", currentShrineHint->objectType,
                                                       currentShrineHint->objectSubtype)
                                          .String(),
                                      readSucces);
            // if (readSucces)
            {
                result = strPtr;
            }
        }
        // else if (shrine->objectType == eObject::SHRINE_OF_MAGIC_GESTURE)

        else if (shrine->objectType == eObject::SHRINE_OF_MAGIC_THOUGHT)
        {
            // result = "Shrine of Magic Thought";
        }
    }

    return result;
}

_LHF_(ShrinesExternder::Shrine__AtGetHint)
{
    if (currentShrineHint)
    {

        if (auto hint = GetCustomHint(currentShrineHint))
        {
            c->edi = int(hint);
            // return after original text set
            c->return_address = 0x040DA2B;
            return NO_EXEC_DEFAULT;
        }
    }

    return EXEC_DEFAULT;
}
void ShrinesExternder::CreatePatches()
{
    if (!m_isInited)
    {

        _pi->WriteLoHook(0x4C1974, Game__AtShrineOfMagicIncantationSettingSpell);
        _pi->WriteLoHook(0x4C1A0E, Game__AtShrineOfMagicGestureSettingSpell);
        _pi->WriteLoHook(0x04A5291, Shrine__AtVisit);
        _pi->WriteLoHook(0x04A5459, Shrine__AtWisdomCheck);

        _pi->WriteLoHook(0x40D858, Shrine__AtGetName);
        _pi->WriteLoHook(0x40DA24, Shrine__AtGetHint);

        m_isInited = true;
    }
}

ShrinesExternder &ShrinesExternder::Get()
{
    // TODO: insert return statement here

    static ShrinesExternder _instance;
    return _instance;
}

} // namespace shrines
