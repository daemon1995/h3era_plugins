#include "pch.h"

namespace artifacts
{
ArtifactHints *ArtifactHints::instance = nullptr;

ArtifactHints::ArtifactHints() : IGamePatch("EraPlugin.ArtifactHints.daemon_n")
{

    CreatePatches();
}

BOOL ArtifactHints::CreateStatsString(const H3Artifact *artifact, const H3Hero *hero, H3String *result) noexcept
{
    StatBytes artifactStats(*artifact);

    H3Artifact equippedInSlot;

    if (hero)
    {
        for (size_t i = 0; i < 19; i++)
        {
            if (hero->CanPlaceArtifact(artifact->id, i) || hero->CanReplaceArtifact(artifact->id, i))
            {
                equippedInSlot = hero->bodyArtifacts[i];
                break;
            }
        }
    }

    const bool clickedArtHasStats = artifactStats;
    StatBytes equippedArtifactsStats(equippedInSlot);

    const bool equippedArtHasStats = equippedArtifactsStats;

    if (clickedArtHasStats || equippedArtHasStats)
    {
        settings.Load();
        char statsBuffer[4][64]{};
        if (equippedInSlot.Empty() || equippedInSlot == *artifact)
        {
            for (int i = 0; i < 4; ++i)
            {
                libc::sprintf(statsBuffer[i], STATS_FORMAT, artifactStats.stats[i]);
            }
        }
        else
        {

            for (int i = 0; i < 4; ++i)
            {
                if (const int statsDifference = artifactStats.stats[i] - equippedArtifactsStats.stats[i])
                {
                    LPCSTR format = statsDifference > 0 ? settings.increaseFormat : settings.decreaseFormat;
                    libc::sprintf(h3_TextBuffer, format, statsDifference);
                    libc::sprintf(statsBuffer[i], COMPARED_STATS_FORMAT, artifactStats.stats[i], h3_TextBuffer);
                }
                else
                {
                    libc::sprintf(statsBuffer[i], STATS_FORMAT, artifactStats.stats[i]);
                }
            }
        }

        libc::sprintf(h3_TextBuffer, EraJS::read(settings.textFormat), statsBuffer[0], statsBuffer[1], statsBuffer[2],
                      statsBuffer[3]);

        hintTextBuffer = h3_TextBuffer;

        return true;
    }
    return false;
}

H3String *__stdcall ArtifactHints::BuildUpArtifactDescription(HiHook *h, const H3Artifact *artifact,
                                                              H3String *resultString) noexcept
{

    auto result = THISCALL_2(H3String *, h->GetDefaultFunc(), artifact, resultString);

    if (!instance->active || artifact->Empty())
    {
        return result;
    }

    auto hero = P_DialogHero->Get();
    if (!hero)
    {
        hero = *reinterpret_cast<H3Hero **>(0x06AAAE0);
        // auto mgr = H3SwapManager::Get();
        // hero = mgr->hero[mgr->heroSelected];
    }
    const int artifactId = artifact->id;
    const int playerID = P_Game->Get()->GetPlayerID();
    sprintf(Era::z[0], PRIMARY_SKILLS_ERM_VARIABLE_FORMAT, playerID);
    const bool isEnabled = Era::GetAssocVarIntValue(Era::z[0]);

    if (isEnabled && artifactId > eArtifact::FIRST_AID_TENT)
    {
        if (instance->CreateStatsString(artifact, hero, &instance->hintTextBuffer))
        {

            if (instance->settings.addAsExtraObject)
            {
                instance->drawMultyPicDlgPatch->Apply();

                H3FontLoader fnt(NH3Dlg::Text::MEDIUM);

                const int linesNum = fnt->GetLinesCountInText(result->String(), 256);

                if (linesNum < 5)
                {
                    auto topTextLines = linesNum < 5 ? endl : doubleEndl;
                    instance->settings.placeBelowText ? result->Append(doubleEndl) : *result = endl + *result;
                }
            }
            else if (instance->settings.placeBelowText)
            {
                *result += doubleEndl + instance->hintTextBuffer;
                instance->hintTextBuffer.Erase();
            }
            else
            {
                instance->hintTextBuffer += doubleEndl + *result;
                result->Assign(instance->hintTextBuffer);
                instance->hintTextBuffer.Erase();
            }
        }
    }

    libc::sprintf(Era::z[0], PRIMARY_SKILLS_ERM_VARIABLE_FORMAT, playerID);

    if (Era::GetAssocVarIntValue(Era::z[0]))
    {
    }

    return result;
}
int __stdcall ArtifactHints::BuildMultyPicDlg(HiHook *h, H3Game *game)
{
    if (auto dlg = **reinterpret_cast<H3Dlg ***>(0x04F71C4 + 1))
    {
        const int dlgWidth = dlg->GetWidth();
        const int dlgHeight = dlg->GetHeight();

        constexpr int offset = 18;
        const int textWidth = dlgWidth - (offset << 1);
        const int placeY = instance->settings.placeBelowText ? dlgHeight - 25 - offset : offset;
        dlg->CreateText(offset, placeY, textWidth, 20, instance->hintTextBuffer.String(), instance->settings.fontName,
                        eTextColor::REGULAR, -1);
        instance->hintTextBuffer.Erase();

        h->Undo();
    }
    return THISCALL_1(int, h->GetDefaultFunc(), game);
}

ArtifactHints &ArtifactHints::Get()
{
    if (!instance)
    {
        instance = new ArtifactHints();
    }
    return *instance;
}

StatBytes::StatBytes(const H3Artifact &art)
{
    if (art.Empty())
    {
        *this = StatBytes();
    }
    else
    {
        // original stats
        auto source = &reinterpret_cast<INT8 *>(DwordAt(0x04E2E94 + 1))[art.id << 2];
        memcpy(stats, source, sizeof(stats));

        // combo parts stats
        const eCombinationArtifacts combArt = art.GetCombinationArtifactIndex();

        if (eCombinationArtifacts::NONE != combArt)
        {
            const int lastArtifactId = IntAt(0x717020);
            for (size_t i = 0; i < lastArtifactId; i++)
            {
                const auto &artPiece = P_ArtifactSetup[i];
                if (artPiece.combinationArtifactId == combArt)
                {
                    *this += StatBytes(H3Artifact(eArtifact(i)));
                }
            }
        }
    }
}
StatBytes::StatBytes()
{
    memset(stats, 0, sizeof(stats));
}
StatBytes::operator bool() const
{
    return static_cast<bool>(stats[0] | stats[1] | stats[2] | stats[3]);
}

StatBytes &StatBytes::operator+=(const StatBytes &other)
{
    for (int i = 0; i < 4; ++i)
        stats[i] += other.stats[i];
    return *this;
}
void HintsText::Load()
{
    bool readSuccess = false;
    auto txt = EraJS::read("gem_plugin.artifact_hints.primary_skills.text_format", readSuccess);
    if (readSuccess)
    {
        textFormat = txt;
    }
    txt = EraJS::read("gem_plugin.artifact_hints.primary_skills.increase_format", readSuccess);
    if (readSuccess)
    {
        increaseFormat = txt;
    }
    readSuccess = false;
    txt = EraJS::read("gem_plugin.artifact_hints.primary_skills.decrease_format", readSuccess);
    if (readSuccess)
    {
        decreaseFormat = txt;
    }

    txt = EraJS::read("gem_plugin.artifact_hints.primary_skills.external_widget_font", readSuccess);
    if (readSuccess)
    {
        fontName = txt;
    }
    placeBelowText = EraJS::readInt("gem_plugin.artifact_hints.primary_skills.place_below_text");
    addAsExtraObject = EraJS::readInt("gem_plugin.artifact_hints.primary_skills.external_widget");
}

void ArtifactHints::CreatePatches() noexcept
{
    if (!m_isInited)
    {
        WriteHiHook(0x04DB650, THISCALL_, BuildUpArtifactDescription);
        drawMultyPicDlgPatch = _pi->CreateHiHook(0x4F71BB, CALL_, EXTENDED_, THISCALL_, BuildMultyPicDlg);
        settings.Load();
        m_isInited = true;
    }
}

} // namespace artifacts
