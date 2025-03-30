#include "../pch.h"
namespace university

{

INT8 UniversityExtender::bannedSkillsCopy[limits::SECONDARY_SKILLS];
UniversityExtender::UniversityExtender()
    : ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.UniversityExtender.daemon_n"))
{
}

UniversityExtender::~UniversityExtender()
{
}
H3RmgObjectGenerator *UniversityExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{

    if (objectInfo.type == eObject::UNIVERSITY && objectInfo.subtype)
    {

        return extender::ObjectsExtender::CreateRMGObjectGen(objectInfo);
    }
    return nullptr;
}

void UniversityExtender::AfterLoadingObjectTxtProc(const INT16 *maxSubtypes)
{
    const int length = maxSubtypes[eObject::UNIVERSITY];

    universitiesData.resize(length);

    // std::fill(universitiesObligatorySkills.begin(), universitiesObligatorySkills.end(), MAXDWORD);

    bool readSuccess = false;
    bool patchIsRequired = false;
    for (size_t i = 0; i < length; i++)
    {
        std::unordered_set<UINT8> skillSet;

        for (size_t j = 0; j < limits::SECONDARY_SKILLS; j++)
        {

            size_t skillId = EraJS::readInt(
                H3String::Format("RMG.objectGeneration.%d.%d.skills.allowed.%d", eObject::UNIVERSITY, i, j).String(),
                readSuccess);
            const DWORD bitMask = 1 << skillId;

            if (readSuccess)
            {
                if (skillId < limits::SECONDARY_SKILLS)
                {
                    skillSet.insert(skillId);
                }
            }
            else if (skillSet.size())
            {
                int k = 0;
                do
                {
                    skillSet.insert(k++);
                } while (skillSet.size() < 4);
                DWORD skillBits = 0;
                for (auto s : skillSet)
                {
                    skillBits |= (1 << s);
                }
                universitiesData[i].allowedSkills = skillBits;
                patchIsRequired = true;
                break;
            }
        }

        // DWORD obligatorySkills = EraJS::readInt(
        //     H3String::Format("RMG.objectGeneration.%d.%d.skills.obligatory", eObject::UNIVERSITY, i).String(),
        //     readSuccess);

        // if (readSuccess)
        //{
        //     universitiesData[i].obligatorySkills = obligatorySkills;
        //     patchIsRequired = true;
        // }
    }
    if (patchIsRequired)
    {
        CreatePatches();
    }
    //    universitiesAllowedSkills.emplace_back();
}

BOOL UniversityExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero,
                                               const H3Player *activePlayer, const BOOL isRightClick) const noexcept
{
    if (mapItem->objectType == eObject::UNIVERSITY && mapItem->objectSubtype > 0)
    {

        H3String objName = RMGObjectInfo::GetObjectName(mapItem);

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {

            /*  sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ",
                      P_GeneralText->GetText(isVistedByHero ? 354 : 355));*/
            objName.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objName.String());

        return true;
    }

    return 0;
}

void __stdcall UniversityExtender::Game_SetupUniversity(HiHook *h, H3Main *game, const H3MapItem *university)
{
    bool dataChanged = false;

    const auto &universityData = Get().universitiesData[university->objectSubtype];

    if (university->objectSubtype <= UNI_OBJECT_SUBTYPE)
    {
        memcpy(bannedSkillsCopy, game->bannedSkills, sizeof(bannedSkillsCopy));
        for (size_t i = 0; i < limits::SECONDARY_SKILLS; i++)
        {
            // if (obligatorySkills & (1 << i))
            {
                //     game->bannedSkills[i] = false;
            }
            // else
            if (!(universityData.allowedSkills & (1 << i)))
            {
                game->bannedSkills[i] = true;
            }
        }
        dataChanged = true;
    }

    THISCALL_2(void, h->GetDefaultFunc(), game, university);
    // university->university
    //  game->universities[university->university.id].sSkill[0]; ;
    if (dataChanged)
    {
        memcpy(game->bannedSkills, bannedSkillsCopy, sizeof(bannedSkillsCopy));
    }
}
INT UniversityExtender::objectSubtype = -1;
_LHF_(UniversityDlg_BeforeCreate)
{
    if (auto mapItem = reinterpret_cast<H3MapItem *>(c->edi))
    {
        if (mapItem->objectType == eObject::UNIVERSITY)
        {
            UniversityExtender::objectSubtype = mapItem->objectSubtype;
        }
    }

    return EXEC_DEFAULT;
}

_LHF_(UniversityDlg_SetWidgetText)
{
    auto &objectSubtype = UniversityExtender::objectSubtype;
    if (objectSubtype != -1)
    {
        bool readSuccess = false;
        LPCSTR name = EraJS::read(
            H3String::Format("RMG.objectGeneration.%d.%d.name", eObject::UNIVERSITY, objectSubtype).String(),
            readSuccess);

        if (readSuccess)
        {
            c->edx = int(name);
        }
        objectSubtype = -1;
    }

    return EXEC_DEFAULT;
}

_LHF_(University_AtGetName)
{
    if (auto mapItem = reinterpret_cast<H3MapItem *>(c->ebx))
    {
        if (mapItem->objectType == eObject::UNIVERSITY)
        {

            bool readSuccess = false;
            LPCSTR name = EraJS::read(
                H3String::Format("RMG.objectGeneration.%d.%d.name", eObject::UNIVERSITY, mapItem->objectSubtype)
                    .String(),
                readSuccess);

            if (readSuccess)
                libc::sprintf(h3_TextBuffer, name);
        }
    }

    return EXEC_DEFAULT;
}
void UniversityExtender::CreatePatches()
{
    if (!m_isInited)
    {
        _pi->WriteHiHook(0x04C03B0, THISCALL_, Game_SetupUniversity);
        _pi->WriteLoHook(0x04AA196, UniversityDlg_BeforeCreate);
        _pi->WriteLoHook(0x05EFA36, UniversityDlg_SetWidgetText);

        //_pi->WriteLoHook(0x4C1974, Game__AtShrineOfMagicIncantationSettingSpell);
        //_pi->WriteLoHook(0x4C1A0E, Game__AtShrineOfMagicGestureSettingSpell);
        //_pi->WriteLoHook(0x04A5291, Shrine__AtVisit);
        //_pi->WriteLoHook(0x04A5459, Shrine__AtWisdomCheck);

        _pi->WriteLoHook(0x0415439, University_AtGetName);
        _pi->WriteLoHook(0x040CAE5, University_AtGetName);

        //_pi->WriteLoHook(0x40DA24, Shrine__AtGetHint);

        m_isInited = true;
    }
}

UniversityExtender &UniversityExtender::Get()
{
    // TODO: insert return statement here

    static UniversityExtender _instance;
    return _instance;
}

} // namespace university
