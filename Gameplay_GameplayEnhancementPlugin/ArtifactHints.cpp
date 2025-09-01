#include "pch.h"

namespace artifacts
{
ArtifactHints *ArtifactHints::instance = nullptr;

ArtifactHints::ArtifactHints() : IGamePatch("EraPlugin.ArtifactHints.daemon_n")
{

    CreatePatches();
}
BOOL ArtifactHints::CreateCombinePartsString(const H3Artifact *artifact, const H3Hero *hero, H3String *result) noexcept
{

    if (artifact && hero)
    {
        const eCombinationArtifacts combArt = artifact->GetCombinationArtifact();
        std::vector<int> combinedArtifactParts;
        if (combArt != eCombinationArtifacts::NONE)
        {
            result->Erase();
            const int lastArtifactId = IntAt(0x717020);
            bool artsFound = false;
            int combinedArtifactId = eArtifact::NONE;
            for (size_t i = 0; i < lastArtifactId; i++)
            {
                //   if (i == artifact->id)
                //      continue;
                const auto &artPiece = P_ArtifactSetup[i];
                if (artPiece.combinationArtifactId == combArt)
                {
                    combinedArtifactParts.push_back(i);
                }
                else if (!artsFound && artPiece.comboID == combArt)
                {
                    artsFound = true;
                    combinedArtifactId = i;
                }
            }

            if (!combinedArtifactParts.empty())
            {

                constexpr LPCSTR equippedColor = "{~LightGreen}";
                constexpr LPCSTR storedColor = "{~Orange}";
                constexpr LPCSTR missingColor = "{~Grey}";
                H3String piecesText;
                int equippedPeicesCount = 0;
                for (const auto comboPartId : combinedArtifactParts)
                {
                    BOOL equipped = 0;
                    int storedCount = 0;
                    for (auto &i : hero->bodyArtifacts)
                    {
                        if (i.id == comboPartId)
                        {
                            equipped = 1;
                            equippedPeicesCount += 1;
                            break;
                        }
                    }
                    for (auto &i : hero->backpackArtifacts)
                    {
                        storedCount += (i.id == comboPartId);
                    }
                    const char *textColor = equipped ? equippedColor : storedCount ? storedColor : missingColor;
                    H3String overflowText;
                    if (storedCount > 1)
                    {
                        overflowText = H3String::Format(" (%d)", storedCount);
                    }

                    libc::sprintf(h3_TextBuffer, "\n%s%s%s{~}", textColor, P_ArtifactSetup[comboPartId].name,
                                  overflowText.String());
                    piecesText += h3_TextBuffer;
                    // result->Append(endl);
                    // result->Append(h3_TextBuffer);
                }

                // create header line
                LPCSTR artNameColor = equippedPeicesCount ? equippedColor : missingColor;
                libc::sprintf(h3_TextBuffer, "\n%s%s (%d/%d):{~}\n", artNameColor,
                              P_ArtifactSetup[combinedArtifactId].name, equippedPeicesCount,
                              combinedArtifactParts.size());
                // append colored artifact name
                result->Append(h3_TextBuffer);

                // append pieces list
                result->Append(piecesText);

                return true;
            }
        }
    }

    return false;
}

void ArtifactHints::ChangeMessageBoxHeight(const int additionalHeight) noexcept
{
    IntAt(0x04F65D4 + 2) += additionalHeight;
    IntAt(0x04F662F + 1) += additionalHeight;
    messageboxHeightChange += additionalHeight;
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

void __stdcall ArtifactHints::SwapMgr_InteractArtifactSlot(HiHook *h, H3SwapManager *mgr, const int side, int slotIndex,
                                                           int a4) noexcept
{
    instance->swapSide = side;
    THISCALL_4(void, h->GetDefaultFunc(), mgr, side, slotIndex, a4);
}

H3String *__stdcall ArtifactHints::BuildUpArtifactDescription(HiHook *h, const H3Artifact *artifact,
                                                              H3String *resultString) noexcept
{

    auto result = THISCALL_2(H3String *, h->GetDefaultFunc(), artifact, resultString);

    if (!instance->active || artifact->Empty())
    {
        return result;
    }

    // try to get hero from dialog first
    auto hero = P_DialogHero->Get();
    if (!hero)
    {
        // try to get hero from any of market dialogs
        hero = *reinterpret_cast<H3Hero **>(0x06AAAE0);
        if (!hero)
        {
            // try to get hero from swap manager if artifact was just swapped
            if (auto mgr = H3SwapManager::Get())
                hero = mgr->hero[instance->swapSide];
        }
    }
    const int artifactId = artifact->id;
    const int playerID = P_Game->Get()->GetPlayerID();
    libc::sprintf(Era::z[0], COMBINATIONS_ERM_VARIABLE_FORMAT, playerID);
    const bool artHintsEnabled = Era::GetAssocVarIntValue(Era::z[0]);

    if (hero && artHintsEnabled && !instance->isUniteComboArtifactCall)
    {
        if (instance->CreateCombinePartsString(artifact, hero, &instance->hintTextBuffer))
        {
            *result += doubleEndl + (instance->hintTextBuffer);
            instance->hintTextBuffer.Erase();

            if (!instance->messageboxHeightChange)
            {
                instance->ChangeMessageBoxHeight(300);
            }
        }
    }

    libc::sprintf(Era::z[0], PRIMARY_SKILLS_ERM_VARIABLE_FORMAT, playerID);
    const bool statHintsEnabled = Era::GetAssocVarIntValue(Era::z[0]);

    if (statHintsEnabled)
    {
        if (instance->CreateStatsString(artifact, hero, &instance->hintTextBuffer))
        {

            if (instance->settings.addAsExtraObject)
            {
                instance->drawMultiPicDlgPatch->Apply();

                instance->settings.placeBelowText ? result->Append(doubleEndl) : *result = endl + *result;
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
            if (!instance->messageboxHeightChange)
            {
                instance->ChangeMessageBoxHeight(100);
            }
        }
    }

    return result;
}
int __stdcall ArtifactHints::UniteComboArtifacts(HiHook *h, const H3Hero *hero, const int artId) noexcept
{
    instance->isUniteComboArtifactCall = true;

    const DWORD result = THISCALL_2(int, h->GetDefaultFunc(), hero, artId);

    instance->isUniteComboArtifactCall = false;
    return result;
}
int __stdcall ArtifactHints::BuildMultiPicDlg(HiHook *h, H3Game *game)
{
    if (auto dlg = **reinterpret_cast<H3Dlg ***>(0x04F71C4 + 1))
    {
        if (instance->settings.addAsExtraObject)
        {

            const int dlgWidth = dlg->GetWidth();
            const int dlgHeight = dlg->GetHeight();

            constexpr int offset = 18;
            const int textWidth = dlgWidth - (offset << 1);
            const int placeY = instance->settings.placeBelowText ? dlgHeight - 25 - offset : offset;
            dlg->CreateText(offset, placeY, textWidth, 20, instance->hintTextBuffer.String(),
                            instance->settings.fontName, eTextColor::REGULAR, -1);
            instance->hintTextBuffer.Erase();
        }
        if (instance->messageboxHeightChange)
        {
            instance->ChangeMessageBoxHeight(-instance->messageboxHeightChange);
        }

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
        WriteHiHook(0x05AF920, THISCALL_, SwapMgr_InteractArtifactSlot); // dolls
        WriteHiHook(0x05AFD20, THISCALL_, SwapMgr_InteractArtifactSlot); // backpack

        WriteHiHook(0x04DB650, THISCALL_, BuildUpArtifactDescription);
        WriteHiHook(0x04D9F30, THISCALL_, UniteComboArtifacts);

        drawMultiPicDlgPatch = _pi->CreateHiHook(0x4F71BB, CALL_, EXTENDED_, THISCALL_, BuildMultiPicDlg);
        settings.Load();
        m_isInited = true;
    }
}

} // namespace artifacts
