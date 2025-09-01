#include "../pch.h"
namespace spellmarket

{

SpellMarketExtender::SpellMarketExtender()
    : ObjectExtender(globalPatcher->CreateInstance("EraPlugin.SpellMarketExtender.daemon_n"))
{

    CreatePatches();
}

SpellMarketExtender::~SpellMarketExtender()
{
}

void SpellMarketExtender::CreatePatches()
{
    if (!m_isInited)
    {

        //_pi->WriteLoHook(0x4C1974, Game__AtShrineOfMagicIncantationSettingSpell);
        //_pi->WriteLoHook(0x40D858, Shrine__AtGetName);
        //_pi->WriteLoHook(0x40DA24, Shrine__AtGetHint);

        m_isInited = true;
    }
}
BOOL SpellMarketExtender::SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *activePlayer,
                                             int &aiMapItemWeight, int *moveDistance,
                                             const H3Position pos) const noexcept
{

    if (auto spellMarket = H3MapItemSpellMarket::GetFromMapItem(mapItem))
    {
        if (spellMarket->TryToVisit(const_cast<H3Hero *>(hero)) == eVisitError::NONE)
        {
            for (size_t i = 0; i < 2; i++)
            {
                const eSpell spellId = spellMarket->GetSpell(i);
                if (spellId != eSpell::NONE)
                {
                    aiMapItemWeight += h3functions::GetAIHeroSpellValue(hero, spellId);
                }
            }
            if (aiMapItemWeight > 0)
            {
                aiMapItemWeight -= activePlayer->resourceImportance[eResource::GOLD] * GOLD_REQUIRED;
            }
        }
        return true;
    }
    return false;
}

void ShowMessage(const H3Hero *hero, const H3MapItem *mapItem, const BOOL skipMapMessage, const eVisitError visitError)
{

    LPCSTR jsonStr = nullptr;
    switch (visitError)
    {
    case eVisitError::NO_SPELLS:
        jsonStr = "noSpells";
        break;
    case eVisitError::NO_MONEY:
        jsonStr = "noMoney";
        break;
    case eVisitError::NO_WISDOM:
        jsonStr = "noWisdom";
        break;
    case eVisitError::NO_SPELL_BOOK:
        jsonStr = "noSpellBook";
        break;
    case eVisitError::LEARNED:
        jsonStr = "learned";
        break;
    case eVisitError::VISITED:
        jsonStr = "visited";
        break;
    default:
        break;
    }
    if (jsonStr)
    {
        H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));
        libc::sprintf(h3_TextBuffer, "RMG.objectGeneration.%d.%d.text.%s", mapItem->objectType, mapItem->objectSubtype,
                      jsonStr);
        objName.Append("\n\n").Append(EraJS::read(h3_TextBuffer));
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

BOOL SpellMarketExtender::SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero,
                                                const H3Player *activePlayer, const BOOL isRightClick) const noexcept
{
    if (auto spellMarket = H3MapItemSpellMarket::GetFromMapItem(mapItem))
    {

        H3String objHint = RMGObjectInfo::GetObjectName(mapItem);

        if (!Era::GetAssocVarIntValue("GameplayEnhancementsPlugin_AdventureMapHints_AtHint"))
        {
            objHint.Append(isRightClick ? "\n" : " ");
            libc::sprintf(h3_TextBuffer, "RMG.objectGeneration.%d.%d.text.hint", mapItem->objectType,
                          mapItem->objectSubtype);
            objHint.Append(EraJS::read(h3_TextBuffer));
        }

        if (const H3Hero *hero = P_ActivePlayer->GetActiveHero())
        {
            const bool isVistedByHero = spellMarket->IsVisitedByHero(hero);
            sprintf(h3_TextBuffer, "%s%s", isRightClick ? "\n\n" : " ",
                    P_GeneralText->GetText(isVistedByHero ? 354 : 355));
            objHint.Append(h3_TextBuffer);
        }

        sprintf(h3_TextBuffer, "%s", objHint.String());
        return 1;
    }
    return 0;
}

BOOL SpellMarketExtender::InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept
{
    if (auto spellMarket = H3MapItemSpellMarket::GetFromMapItem(mapItem))
    {
        const int objectId = Era::GetAssocVarIntValue(H3MapItemSpellMarket::indexFormat);
        spellMarket->id = objectId;
        spellMarket->GenerateSpells();
        Era::SetAssocVarIntValue(H3MapItemSpellMarket::indexFormat, objectId + 1);
        return 1;
    }
    return 0;
}

BOOL SpellMarketExtender::InitNewWeekMapItemSetup(H3MapItem *mapItem) const noexcept
{
    if (auto spellMarket = H3MapItemSpellMarket::GetFromMapItem(mapItem))
    {
        spellMarket->Reset();
        spellMarket->GenerateSpells();
        return 1;
    }
    return 0;
}

BOOL SpellMarketExtender::VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                                       const BOOL isHuman) const noexcept
{

    if (auto spellMarket = H3MapItemSpellMarket::GetFromMapItem(mapItem))
    {

        //        eVisitError visitError = spellMarket->TryToVisit(hero);

        if (eVisitError visitError = spellMarket->TryToVisit(hero))
        {
            if (isHuman)
            {
                ShowMessage(hero, mapItem, skipMapMessageByHdMod, visitError);
            }
        }
        else
        {
            eSpell spells[2] = {spellMarket->GetSpell(0), spellMarket->GetSpell(1)};

            int selectedSpellSlot = -1;
            if (!isHuman)
            {
                int selectedValue = 0;
                for (size_t i = 0; i < 2; i++)
                {
                    if (spells[i] != eSpell::NONE)
                    {
                        const int spellValue = h3functions::GetAIHeroSpellValue(hero, spells[i]);
                        if (spellValue > selectedValue)
                        {
                            selectedSpellSlot = i;
                        }
                    }
                }
            }
            else
            {

                H3String objName = H3String::Format("{%s}", RMGObjectInfo::GetObjectName(mapItem));
                objName.Append("\n\n").Append(EraJS::read(H3String::Format("RMG.objectGeneration.%d.%d.text.visit",
                                                                           mapItem->objectType, mapItem->objectSubtype)
                                                              .String()));
                struct DlgData
                {
                    int type;
                    int subtype;
                } *dlgData = reinterpret_cast<DlgData *>(0x2734978);

                // prepare dialog IF:N by cleaning it
                for (size_t i = 0; i < 9; i++)
                {
                    dlgData[i].type = -1;
                    dlgData[i].subtype = 0;
                }

                std::vector<std::pair<eSpell, int>> displaySpells;

                for (size_t i = 0; i < 2; i++)
                {
                    const eSpell spellId = spells[i];
                    if (spellId != eSpell::NONE && !hero->learnedSpells[spellId] &&
                        P_Spell[spellId].level <= hero->secSkill[eSecondary::WISDOM] + 2)
                    {
                        displaySpells.emplace_back(std::make_pair(spellId, i));
                    }
                }
                const size_t displaySpellsCount = displaySpells.size();

                if (displaySpellsCount)
                {
                    for (size_t i = 0; i < displaySpellsCount; i++)
                    {
                        dlgData[i].type = ePictureCategories::SPELL;
                        dlgData[i].subtype = displaySpells[i].first;
                    }

                    const int savedY = Era::y[1];
                    libc::sprintf(Era::z[1], "%s", objName.String());
                    Era::ExecErmCmd("IF:N10/z1/?y1");

                    if (Era::y[1] >= 0)
                    {
                        selectedSpellSlot = displaySpells[Era::y[1]].second;
                    }
                    Era::y[1] = savedY;
                }
                else
                {
                    ShowMessage(hero, mapItem, skipMapMessageByHdMod, eVisitError::LEARNED);
                }
            }
            if (selectedSpellSlot != -1)
            {
                spellMarket->BuySpell(hero, selectedSpellSlot);
            }
        }
        return true;
    }
    return false;
}
H3RmgObjectGenerator *SpellMarketExtender::CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept
{

    if (objectInfo.type == extender::HOTA_OBJECT_TYPE && objectInfo.subtype == SPELL_MARKET_OBJECT_SUBTYPE)
    {
        return ObjectExtender::CreateDefaultH3RmgObjectGenerator(objectInfo);
    }
    return nullptr;
}

SpellMarketExtender *SpellMarketExtender::instance = nullptr;

SpellMarketExtender &SpellMarketExtender::Get()
{
    if (!instance)
        instance = new SpellMarketExtender();
    return *instance;
}

void H3MapItemSpellMarket::Reset()
{
    for (size_t i = 0; i < 2; i++)
    {
        SetSpell(i, eSpell::NONE);
    }
    libc::sprintf(h3_TextBuffer, maxHeroIdFormat, id);
    const int maxHeroId = Era::GetAssocVarIntValue(h3_TextBuffer);
    Era::SetAssocVarIntValue(h3_TextBuffer, 0);

    for (size_t i = 0; i < maxHeroId; i++)
    {
        SetVisited(i, false);
    }
}

void H3MapItemSpellMarket::GenerateSpells()
{

    SetSpell(0, eSpell::NONE);
    SetSpell(1, eSpell::NONE);

    std::vector<eSpell> spells;
    spells.reserve(15);
    int spellId = eSpell::SUMMON_BOAT;
    do
    {
        const auto &spell = P_Spell[spellId];
        if (spell.flags && spell.level == 4 && !P_Game->disabledSpells[spellId])
        {
            spells.emplace_back(eSpell(spellId));
        }

    } while (++spellId < limits::SPELLS);
    if (const int size = spells.size())
    {

        std::vector<eSpell> shuffledSpells(spells);

        if (size > 2)
        {
            DWORD h3Seed = DwordAt(0x067FBE4);
            H3Random::SetRandomSeed(h3Seed);
            //  H3Random::Random(size);

            volatile int newSize = size;
            for (size_t i = 0; i < size; i++)
            {
                const int index = H3Random::Random(newSize-- - 1);

                std::swap(spells[index], spells[newSize]);
                shuffledSpells[i] = spells[newSize];
                spells.pop_back();
            }
        }
        // std::shuffle(spells.begin(), spells.end(), H3Random::ThreadLocalSingleton::Get());
        SetSpell(0, shuffledSpells[0]);
        SetSpell(1, shuffledSpells[1]);
    }
    else
    {
        for (size_t i = 0; i < size; i++)
        {
            SetSpell(i, spells[i]);
        }
    }
}

inline void H3MapItemSpellMarket::SetVisited(const int heroId, const BOOL state) const noexcept
{
    libc::sprintf(h3_TextBuffer, visitedByHeroFormat, id, heroId);
    Era::SetAssocVarIntValue(h3_TextBuffer, state);

    if (state)
    {
        libc::sprintf(h3_TextBuffer, maxHeroIdFormat, id, heroId);

        if (Era::GetAssocVarIntValue(h3_TextBuffer) < heroId + 1)
        {
            Era::SetAssocVarIntValue(h3_TextBuffer, heroId + 1);
        }
    }
}

inline BOOL H3MapItemSpellMarket::IsVisitedByHero(const H3Hero *hero) const noexcept
{
    libc::sprintf(h3_TextBuffer, visitedByHeroFormat, id, hero->id);
    return Era::GetAssocVarIntValue(h3_TextBuffer);
}

inline eSpell H3MapItemSpellMarket::GetSpell(const int index) const noexcept
{
    libc::sprintf(h3_TextBuffer, spellFormat, id, index);
    return eSpell(Era::GetAssocVarIntValue(h3_TextBuffer));
}

inline void H3MapItemSpellMarket::SetSpell(const int index, const eSpell spell) const noexcept
{
    libc::sprintf(h3_TextBuffer, spellFormat, id, index);
    Era::SetAssocVarIntValue(h3_TextBuffer, spell);
}

void H3MapItemSpellMarket::BuySpell(H3Hero *hero, const int index) const noexcept
{
    // decrease resource
    THISCALL_3(void, 0x04E3870, hero, eResource::GOLD, -GOLD_REQUIRED);

    h3functions::HeroLearnSpell(hero, GetSpell(index));

    SetVisited(hero->id, true);
    // swap spells to show second spell later
    if (index == 0)
    {
        SetSpell(0, GetSpell(1));
        SetSpell(1, eSpell::NONE);
    }
}

eVisitError H3MapItemSpellMarket::TryToVisit(H3Hero *hero) const noexcept
{
    if (IsVisitedByHero(hero))
        return eVisitError::VISITED;

    bool cantLearn = true, noSpells = true, noWisdom = true;

    for (size_t i = 0; i < 2; i++)
    {
        const eSpell spellId = GetSpell(i);
        if (spellId == eSpell::NONE)
            continue;

        noSpells = false;
        if (hero->learnedSpells[spellId])
            continue;
        cantLearn = false;
        if (P_Spell[spellId].level > hero->secSkill[eSecondary::WISDOM] + 2)
            continue;
        noWisdom = false;
    }

    if (noSpells)
        return eVisitError::NO_SPELLS;

    if (!hero->WearsArtifact(eArtifact::SPELLBOOK))
        return eVisitError::NO_SPELL_BOOK;

    if (cantLearn)
        return eVisitError::LEARNED;

    if (noWisdom)
        return eVisitError::NO_WISDOM;

    if (P_Game->players[hero->owner].playerResources[eResource::GOLD] < GOLD_REQUIRED)
        return eVisitError::NO_MONEY;

    return eVisitError::NONE;
}

inline H3MapItemSpellMarket *H3MapItemSpellMarket::GetFromMapItem(const H3MapItem *mapItem) noexcept
{
    if (mapItem && mapItem->objectType == extender::HOTA_OBJECT_TYPE &&
        mapItem->objectSubtype == SPELL_MARKET_OBJECT_SUBTYPE)
    {
        return const_cast<H3MapItemSpellMarket *>(reinterpret_cast<const H3MapItemSpellMarket *>(&mapItem->setup));
    }
    return nullptr;
}

} // namespace spellmarket
