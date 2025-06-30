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

BOOL ShrinesExternder::RMGDlg_ShowCustomObjectHint(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                                   const H3String &defaultHint) noexcept
{
    if (info.type >= eObject::SHRINE_OF_MAGIC_INCANTATION && info.type <= eObject::SHRINE_OF_MAGIC_THOUGHT)
    {

        if (info.type == eObject::SHRINE_OF_MAGIC_GESTURE && info.subtype > 0)
        {
            H3String additionalHint = defaultHint + "\n";
            // additionalHint += info.GetRmgTypeDescription();
            // additionalHint.Append("\n\n");

            libc::sprintf(h3_TextBuffer, "{~>SpellScr.def:0:%d block}", info.subtype - 1);
            additionalHint.Append(h3_TextBuffer);
            H3Messagebox::RMB(additionalHint.String());
        }
        else
        {
            H3Messagebox::RMB(defaultHint.String());
        }
        return true;
    }
    return 0;
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
BOOL ShrinesExternder::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *activePlayer,
                                          int &aiResWeight, int *moveDistance, const H3Position pos) const noexcept
{

    if (mapItem->objectType == eObject::SHRINE_OF_MAGIC_GESTURE && mapItem->objectSubtype > 0 &&
        hero->level < LEVEL_REQUIRED)
    {
        aiResWeight = 0;
        return true;
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
        if (const int objectSubtype = shrines->objectSubtype)
        {
            shrines->magicShrine.spell = Clamp(0, objectSubtype - 1, limits::SPELLS - 1);
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
            bool readSucces = false;
            LPCSTR visitText = EraJS::read(
                H3String::Format("RMG.objectGeneration.%d.%d.text.visit", shrine->objectType, shrine->objectSubtype)
                    .String(),
                readSucces);
            if (readSucces)
            {
                *reinterpret_cast<LPCSTR *>(c->ebp + 0x10) = visitText;
            }
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
                H3String::Format("RMG.objectGeneration.%d.%d.text.level", shrine->objectType, shrine->objectSubtype)
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
        result = RMGObjectInfo::GetObjectName(shrine);
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
            auto strPtr = EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.text.hint",
                                                       currentShrineHint->objectType, currentShrineHint->objectSubtype)
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

ShrinesExternder *ShrinesExternder::instance = nullptr;

ShrinesExternder &ShrinesExternder::Get()
{
    if (!instance)
        instance = new ShrinesExternder();
    return *instance;
}

} // namespace shrines
