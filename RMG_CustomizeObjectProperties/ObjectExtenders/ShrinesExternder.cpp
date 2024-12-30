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

_LHF_(ShrinesExternder::Game__AtShrineOfMagicIncantationSettingSpell)
{

    // set spell level generated coresponding to map item subtype (allows any level spells);
    if (const H3MapItem *shrines = reinterpret_cast<H3MapItem *>(c->esi))
    {
        int spellLevel = shrines->objectSubtype;
        spellLevel = Clamp(0, spellLevel, 4); // restrict spell levels 1-5 (0_4)
        ByteAt(0x4C1995 + 1) = spellLevel;
    }
    return EXEC_DEFAULT;
}

_LHF_(ShrinesExternder::Shrine__AtGetName)
{
    if (const H3MapItem *shrine = *reinterpret_cast<H3MapItem **>(c->ebp + 0x8))
    {
        currentShrineHint = nullptr;
        if (shrine->objectSubtype != 0)
        {
            currentShrineHint = shrine;
            bool readSucces = false;
            int strPtr = int(EraJS::read(
                H3String::Format("RMG.objectGeneration.88.%d.name", shrine->objectSubtype).String(), readSucces));

            if (readSucces)
            {
                c->edi = strPtr;
                // return after original text set
                c->return_address = 0x40D85F;
                return NO_EXEC_DEFAULT;
            }
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(ShrinesExternder::Shrine__AtGetHint)
{
    if (currentShrineHint)
    {
        // if (_shrine->objectSubtype != 0)
        {
            bool readSucces = false;
            int strPtr = int(EraJS::read(
                H3String::Format("RMG.objectGeneration.88.%d.hint", currentShrineHint->objectSubtype).String(),
                readSucces));

            if (readSucces)
            {
                c->edi = strPtr;
                // return after original text set
                c->return_address = 0x040DA2B;
                return NO_EXEC_DEFAULT;
            }
        }
    }

    return EXEC_DEFAULT;
}
void ShrinesExternder::CreatePatches()
{
    if (!m_isInited)
    {

        _pi->WriteLoHook(0x4C1974, Game__AtShrineOfMagicIncantationSettingSpell);
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
