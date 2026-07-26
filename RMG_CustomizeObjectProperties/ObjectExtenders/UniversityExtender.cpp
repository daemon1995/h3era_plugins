#include "UniversityExtender.h"

namespace university
{

INT8 UniversityExtender::bannedSkillsCopy[limits::SECONDARY_SKILLS];
UniversityExtender::UniversityExtender()
    : ObjectExtender(globalPatcher->CreateInstance("EraPlugin.UniversityExtender.daemon_n"))
{
    objectType = eObject::UNIVERSITY;
}

void UniversityExtender::AfterLoadingObjectsTxtProc(const INT16 *maxSubtypes)
{
    // don't do anything if there are no added subtypes for this object type
    const int addedSubtypes = maxSubtypes[eObject::UNIVERSITY];
    if (addedSubtypes == 0)
    {
        objectType = eObject::NO_OBJ;
        return;
    }

    const int length = addedSubtypes + 1;

    objectSubtypes.Resize(addedSubtypes);
    for (size_t i = 0; i < addedSubtypes; i++)
        objectSubtypes[i] = i + 1;

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

                while (skillSet.size() < 4)
                {
                    skillSet.insert(k++);
                }

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
                                               const int interactPlayerId, const BOOL isRightClick) const noexcept
{
    if (mapItem->objectType == eObject::UNIVERSITY && mapItem->objectSubtype > 0)
    {

        H3String objName = extender::GetObjectName(mapItem);

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

BOOL UniversityExtender::RMGDlg_ShowCustomObjectHint(const H3ObjectAttributes &attributes,
                                                     H3String &defaultHint) noexcept
{

    H3String additionalHint =
        EraJS::read(H3String::Format("RMG.objectGeneration.%d.text.rmg", attributes.type).String());

    volatile int drawnSkills = 0;
    if (attributes.subtype < universitiesData.size())
    {
        for (size_t i = 0; i < limits::SECONDARY_SKILLS; i++)
        {
            if (universitiesData[attributes.subtype].allowedSkills & (1 << i))
            {
                if (drawnSkills++ % 7 == 0)
                {
                    additionalHint.Append("\n\n");
                }
                libc::sprintf(h3_TextBuffer, "{~>SECSK32.def:0:%d}", i * 3 + 3);
                additionalHint.Append(h3_TextBuffer);
            }
        }
    }
    if (additionalHint.Empty())
        return true;

    defaultHint += "\n" + additionalHint;
    return true;
}

void __stdcall UniversityExtender::Game_SetupUniversity(HiHook *h, H3Main *game, const H3MapItem *university)
{

    const auto &universityData = Get().universitiesData[university->objectSubtype];

    bool dataChanged = universityData.allowedSkills != -1;
    if (dataChanged)
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

        c->edx = int(extender::GetObjectName(eObject::UNIVERSITY, objectSubtype));
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
            LPCSTR name = extender::GetObjectName(mapItem);
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

        _pi->WriteLoHook(0x0415439, University_AtGetName);
        _pi->WriteLoHook(0x040CAE5, University_AtGetName);

        m_isInited = true;
    }
}

UniversityExtender *UniversityExtender::instance = nullptr;

UniversityExtender &UniversityExtender::Get()
{
    if (!instance)
        instance = new UniversityExtender();
    return *instance;
}

} // namespace university
