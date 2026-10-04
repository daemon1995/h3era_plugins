#include "ArtifactHints.h"
#include "ArtifactComparison.h"
#include "ModuleSupport.h"

namespace artifacts
{
ArtifactHints *ArtifactHints::instance = nullptr;

ArtifactHints::ArtifactHints() : IGamePatch("EraPlugin.ArtifactHints.daemon_n")
{

    CreatePatches();
}
BOOL ArtifactHints::CreateCombinePartsString(const H3Artifact *artifact, const H3Hero *hero, H3String *result) noexcept
{

    const int artifactCount = IntAt(0x717020);
    if (artifact && hero && result && artifact->id >= 0 && artifact->id < artifactCount)
    {
        const eCombinationArtifacts combArt = artifact->GetCombinationArtifact();
        std::vector<int> combinedArtifactParts;
        if (combArt != eCombinationArtifacts::NONE)
        {
            result->Erase();
            const UINT lastArtifactId = artifactCount;
            bool artsFound = false;
            int combinedArtifactId = eArtifact::NONE;
            for (size_t i = 0; i < lastArtifactId; i++)
            {
                const auto &artPiece = P_ArtifactSetup[i];
                if (artPiece.partOfComboArtifactId == combArt)
                {
                    combinedArtifactParts.push_back(i);
                }
                else if (!artsFound && artPiece.comboArtifactId == combArt)
                {
                    artsFound = true;
                    combinedArtifactId = i;
                }
            }

            if (artsFound && !combinedArtifactParts.empty())
            {

                constexpr LPCSTR equippedColor = "{~LightGreen}";
                constexpr LPCSTR storedColor = "{~Orange}";
                constexpr LPCSTR missingColor = "{~Grey}";
                H3String piecesText;
                int equippedPiecesCount = 0;
                for (const auto comboPartId : combinedArtifactParts)
                {
                    BOOL equipped = 0;
                    int storedCount = 0;
                    for (auto &i : hero->bodyArtifacts)
                    {
                        if (i.id == comboPartId)
                        {
                            equipped = 1;
                            equippedPiecesCount += 1;
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

                    piecesText += H3String::Format("\n%s%s%s{~}", textColor,
                                                   P_ArtifactSetup[comboPartId].name, overflowText.String());
                }

                // create header line
                LPCSTR artNameColor = equippedPiecesCount ? equippedColor : missingColor;
                result->Append(H3String::Format("\n%s%s (%d/%d):{~}\n", artNameColor,
                                                P_ArtifactSetup[combinedArtifactId].name, equippedPiecesCount,
                                                static_cast<int>(combinedArtifactParts.size())));

                // append pieces list
                result->Append(piecesText);

                return true;
            }
        }
    }

    return false;
}

class ArtifactHints::HeightScope
{
    int first;
    int second;
    bool changed;
  public:
    explicit HeightScope(int additional)
        : first(IntAt(0x04F65D4 + 2)), second(IntAt(0x04F662F + 1)), changed(additional != 0)
    {
        if (changed)
        {
            IntAt(0x04F65D4 + 2) = first + additional;
            IntAt(0x04F662F + 1) = second + additional;
        }
    }
    void Restore()
    {
        if (changed)
        {
            IntAt(0x04F65D4 + 2) = first;
            IntAt(0x04F662F + 1) = second;
            changed = false;
        }
    }
    ~HeightScope() { Restore(); }
};

BOOL ArtifactHints::CreateStatsString(const H3Artifact *artifact, const H3Hero *hero, const int slot,
                                     H3String *result) noexcept
{
    if (!artifact || !result || artifact->id < 0 || artifact->id >= IntAt(0x717020))
        return FALSE;

    StatBytes artifactStats(*artifact);
    H3Artifact equippedInSlot;
    const int comparisonSlot = ComparisonSlot(*artifact, hero, slot);
    if (comparisonSlot >= 0)
        equippedInSlot = hero->bodyArtifacts[comparisonSlot];

    const bool clickedArtHasStats = artifactStats;
    StatBytes equippedArtifactsStats(equippedInSlot);

    const bool equippedArtHasStats = equippedArtifactsStats;

    if (clickedArtHasStats || equippedArtHasStats)
    {
        // Settings are loaded once by BuildUpArtifactDescription, before either extra block.
        H3String statsText[4];
        if (equippedInSlot.Empty() || equippedInSlot == *artifact)
        {
            for (int i = 0; i < 4; ++i)
            {
                statsText[i] = H3String::Format(STATS_FORMAT, artifactStats.stats[i]);
            }
        }
        else
        {

            for (int i = 0; i < 4; ++i)
            {
                if (const int statsDifference = artifactStats.stats[i] - equippedArtifactsStats.stats[i])
                {
                    LPCSTR format = statsDifference > 0 ? settings.increaseFormat : settings.decreaseFormat;
                    const H3String difference = H3String::Format(format, statsDifference);
                    statsText[i] = H3String::Format(COMPARED_STATS_FORMAT, artifactStats.stats[i],
                                                    difference.String());
                }
                else
                {
                    statsText[i] = H3String::Format(STATS_FORMAT, artifactStats.stats[i]);
                }
            }
        }

        result->Assign(H3String::Format(settings.textFormat, statsText[0].String(), statsText[1].String(),
                                        statsText[2].String(), statsText[3].String()));

        return true;
    }
    return false;
}

void __stdcall ArtifactHints::SwapMgr_InteractArtifactSlot(HiHook *h, H3SwapManager *mgr, const int side,
                                                            int slotIndex, int a4) noexcept
{
    DescriptionContext selected{mgr && side >= 0 && side < 2 ? mgr->hero[side] : nullptr,
                                h->GetAddress() == 0x5AF920 ? slotIndex : -1};
    gem::ScopedValue<DescriptionContext *> scope(instance->context, &selected);
    THISCALL_4(void, h->GetDefaultFunc(), mgr, side, slotIndex, a4);
}

H3String *__stdcall ArtifactHints::BuildUpArtifactDescription(HiHook *h, const H3Artifact *artifact,
                                                              H3String *resultString) noexcept
{
    auto result = THISCALL_2(H3String *, h->GetDefaultFunc(), artifact, resultString);
    instance->pendingMessage = Message();
    if (!instance->active || !artifact || artifact->Empty() || !result)
        return result;

    const H3Hero *hero = nullptr;
    int slot = -1;
    if (instance->context)
    {
        hero = instance->context->hero;
        slot = instance->context->slot;
    }
    else
    {
        hero = P_DialogHero->Get();
        if (!hero)
            hero = *reinterpret_cast<H3Hero **>(0x06AAAE0);
    }
    const int player = P_Game->Get()->GetPlayerID();
    if (player < 0 || player >= 8)
        return result;
    instance->settings.Load();
    Message message;
    H3String extra;
    const H3String comboKey = H3String::Format(COMBINATIONS_ERM_VARIABLE_FORMAT, player);
    if (hero && Era::GetAssocVarIntValue(comboKey.String()) && !instance->isUniteComboArtifactCall &&
        instance->CreateCombinePartsString(artifact, hero, &extra))
    {
        *result += doubleEndl + extra;
        message.additionalHeight = 300;
    }
    extra.Erase();
    const H3String statsKey = H3String::Format(PRIMARY_SKILLS_ERM_VARIABLE_FORMAT, player);
    if (instance->settings.enabled && Era::GetAssocVarIntValue(statsKey.String()) && instance->CreateStatsString(artifact, hero, slot, &extra))
    {
        message.text = instance->settings;
        if (message.text.addAsExtraObject)
        {
            message.widget = extra;
            if (message.text.placeBelowText)
                result->Append(doubleEndl);
            else
                *result = endl + *result;
        }
        else if (message.text.placeBelowText)
            *result += doubleEndl + extra;
        else
            result->Assign(extra + doubleEndl + *result);
        if (!message.additionalHeight)
            message.additionalHeight = 100;
    }
    if (message.additionalHeight)
    {
        message.description = *result;
        instance->pendingMessage = message;
    }
    return result;
}

int __stdcall ArtifactHints::UniteComboArtifacts(HiHook *h, const H3Hero *hero, const int artId) noexcept
{
    gem::ScopedValue<BOOL> scope(instance->isUniteComboArtifactCall, TRUE);
    return THISCALL_2(int, h->GetDefaultFunc(), hero, artId);
}

void __stdcall ArtifactHints::ShowMessageBox(HiHook *h, LPCSTR text, int type, int x, int y, int pic1, int value1,
                                            int pic2, int value2, int pic3, int value3, int pic4, int value4)
{
    Message message;
    if (text && !instance->pendingMessage.description.Empty() &&
        !libc::strcmp(text, instance->pendingMessage.description.String()))
        message = instance->pendingMessage;
    // A description is consumed once. An unrelated window discards stale pending data.
    instance->pendingMessage = Message();
    HeightScope height(message.additionalHeight);
    gem::ScopedValue<HeightScope *> heightContext(instance->currentHeight, &height);
    gem::ScopedValue<Message *> messageContext(instance->currentMessage, &message);
    FASTCALL_12(void, h->GetDefaultFunc(), text, type, x, y, pic1, value1, pic2, value2, pic3, value3, pic4, value4);
}

int __stdcall ArtifactHints::BuildMultiPicDlg(HiHook *h, H3Game *game)
{
    auto message = instance->currentMessage;
    auto dlg = **reinterpret_cast<H3Dlg ***>(0x04F71C4 + 1);
    if (dlg && message && !message->widget.Empty())
    {
        constexpr int offset = 18;
        const int y = message->text.placeBelowText ? dlg->GetHeight() - 25 - offset : offset;
        dlg->CreateText(offset, y, dlg->GetWidth() - 2 * offset, 20, message->widget.String(),
                        message->text.fontName, eTextColor::REGULAR, -1);
        message->widget.Erase();
    }
    // Layout is complete; restore before modal callbacks can create another window.
    if (instance->currentHeight)
        instance->currentHeight->Restore();
    return THISCALL_1(int, h->GetDefaultFunc(), game);
}

int __stdcall ArtifactHints::ShowDescription(const ArtifactDescriptionApi::Request *request)
{
    if (!request || request->size < sizeof(*request) || request->version != ArtifactDescriptionApi::VERSION ||
        !request->hero || !request->artifact || request->slot < -1 || request->slot >= 19 || !instance->active || !instance->m_isEnabled)
        return 0;
    auto artifact = static_cast<const H3Artifact *>(request->artifact);
    if (artifact->Empty() || artifact->id >= IntAt(0x717020))
        return 0;
    DescriptionContext selected{static_cast<const H3Hero *>(request->hero), request->slot};
    gem::ScopedValue<DescriptionContext *> context(instance->context, &selected);
    gem::ScopedValue<Message> pending(instance->pendingMessage, Message());
    H3String description;
    THISCALL_2(H3String *, 0x4DB650, artifact, &description);
    const H3PictureCategories picture = artifact->GetId() == eArtifact::SPELL_SCROLL
        ? H3PictureCategories::Spell(artifact->ScrollSpell()) : H3PictureCategories::Artifact(artifact->GetId());
    H3Messagebox::RMB(description.String(), picture);
    return 1;
}

ArtifactHints &ArtifactHints::Get()
{
    if (!instance)
    {
        instance = new ArtifactHints();
    }
    return *instance;
}

namespace
{
void CollectStats(int id, INT (&stats)[4], std::vector<int> &ancestors)
{
    const int count = IntAt(0x717020);
    if (id < 0 || id >= count || std::find(ancestors.begin(), ancestors.end(), id) != ancestors.end())
        return;
    auto source = &reinterpret_cast<INT8 *>(DwordAt(0x04E2E94 + 1))[id * 4];
    for (int i = 0; i < 4; ++i)
        stats[i] += source[i];
    const auto combo = P_ArtifactSetup[id].comboArtifactId;
    if (combo != eCombinationArtifacts::NONE)
    {
        ancestors.push_back(id);
        for (int part = 0; part < count; ++part)
            if (P_ArtifactSetup[part].partOfComboArtifactId == combo)
                CollectStats(part, stats, ancestors);
        ancestors.pop_back();
    }
}
}

StatBytes::StatBytes(const H3Artifact &art) : StatBytes()
{
    std::vector<int> ancestors;
    if (!art.Empty())
        CollectStats(art.id, stats, ancestors);
}
StatBytes::StatBytes()
{
    libc::memset(stats, 0, sizeof(stats));
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
    *this = HintsText();
    enabled = gem::ModuleEnabled("gem_plugin.artifact_hints.primary_skills.enabled");
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
    placeBelowText = gem::ModuleEnabled("gem_plugin.artifact_hints.primary_skills.place_below_text", false);
    addAsExtraObject = gem::ModuleEnabled("gem_plugin.artifact_hints.primary_skills.external_widget", false);
}

void ArtifactHints::CreatePatches() noexcept
{
    if (!m_isInited)
    {
        WriteHiHook(0x05AF920, THISCALL_, SwapMgr_InteractArtifactSlot); // dolls
        WriteHiHook(0x05AFD20, THISCALL_, SwapMgr_InteractArtifactSlot); // backpack

        WriteHiHook(0x04DB650, THISCALL_, BuildUpArtifactDescription);
        WriteHiHook(0x04D9F30, THISCALL_, UniteComboArtifacts);

        WriteHiHook(0x4F6C00, FASTCALL_, ShowMessageBox);
        _pi->WriteHiHook(0x4F71BB, CALL_, EXTENDED_, THISCALL_, BuildMultiPicDlg);
        static const ArtifactDescriptionApi::Api api{ArtifactDescriptionApi::MAGIC, sizeof(ArtifactDescriptionApi::Api),
                                                     ArtifactDescriptionApi::VERSION, ShowDescription};
        globalPatcher->VarValue<const ArtifactDescriptionApi::Api *>(ArtifactDescriptionApi::VARIABLE) = &api;
        settings.Load();
        m_isInited = true;
        m_isEnabled = true;
    }
}

} // namespace artifacts
