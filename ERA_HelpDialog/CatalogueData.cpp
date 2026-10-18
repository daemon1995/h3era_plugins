#include "CatalogueData.h"
#include "MasteryPopup.h"
#include "ObjectCard.h"

namespace helpdlg
{
namespace
{
// Native class table used by H3Hero::GetHeroClassName. Keep the binary layout
// in one place; never index it until the hero class has been checked.
#pragma pack(push, 4)
struct HeroClassData
{
    int town;
    LPCSTR name;
    int aggression;
    unsigned char primary[4];
    unsigned char earlyPrimary[4];
    unsigned char latePrimary[4];
    unsigned char secondary[28];
    unsigned char towns[9];
    char padding[3];
};
#pragma pack(pop)
static_assert(sizeof(HeroClassData) == 0x40, "Native hero class layout changed");

bool HasCreatureData(int id)
{
    if (id < 0 || id >= static_cast<int>(P_CreatureCount))
        return false;
    const auto &info = P_CreatureInformation[id];
    return helpdlg::HasCreatureData(info.nameSingular, info.defName);
}

int ArtifactTableCount() noexcept
{
    // H3ArtifactCount reads 0x44D1AA: a combination-mask bound (160 in WoG).
    // UN:A uses this table bound, which artifact extenders update separately.
    return IntAt(0x717020);
}

class ArtifactFrameProbe
{
    H3DefLoader def{NH3Dlg::Assets::ARTIFACT_DEF};
    H3Pcx16Loader canvas;

  public:
    ArtifactFrameProbe()
    {
        if (def.Get() && def->widthDEF > 0 && def->heightDEF > 0 && def->widthDEF <= 4096 && def->heightDEF <= 4096)
            canvas.Set(H3LoadedPcx16::Create(def->widthDEF, def->heightDEF));
    }
    bool HasImage(int id)
    {
        return DefFrameHasVisiblePixels(def.Get(), id, canvas.Get());
    }
};

bool ShowNativeCreatureDialog(int creatureId)
{
    if (!HasCreatureData(creatureId) || !P_WindowManager)
        return false;
    const int savedResult = P_WindowManager->resultItemID;
    // H3CreatureInfoDlg is an abstract native type. Its storage is constructed
    // and destroyed by the game, without running any C++ base constructors.

    H3CreatureInfoDlg *dlg = H3ObjectAllocator<H3CreatureInfoDlg>().allocate(1);
    if (!dlg)
        return false;

    auto *dialog = THISCALL_5(H3CreatureInfoDlg *, 0x764AD6, dlg, static_cast<eCreature>(creatureId), 0, 0,
                              static_cast<DWORD>(1)); // Include the OK button.
    if (!dialog)
        return false;
    struct DialogScope
    {
        H3CreatureInfoDlg *dialog;
        int savedResult;
        ~DialogScope()
        {
            P_WindowManager->resultItemID = savedResult;
            THISCALL_2(int, 0x5F3EC0, dialog, TRUE); // DELETE DIALOG (Dtor + Memory free)
        }
    } scope{dialog, savedResult};
    // The native title is item 203; keep the creature dialog and installed hooks.
    if (auto *name = dialog->GetText(203))
        name->SetText(ObjectTitle(P_CreatureInformation[creatureId].nameSingular, creatureId).c_str());
    dialog->SetX(std::max(0, (H3GameWidth::Get() - dialog->GetWidth()) / 2));
    dialog->SetY(std::max(0, (H3GameHeight::Get() - dialog->GetHeight()) / 2));
    THISCALL_1(int, 0x5FFA20, dialog);
    return true;
}

std::string Number(int value)
{
    return std::to_string(value);
}
std::string Resources(const H3Resources &cost)
{
    std::string text;
    for (int resource = 0; resource < 7; ++resource)
        if (cost.asArray[resource])
            text += (text.empty() ? h3_NullString : ", ") + Number(cost.asArray[resource]) + " " +
                    Safe(P_ResourceName[resource]);
    return text.empty() ? "0" : text;
}

LPCSTR DataRoot(main::eHelpPage page)
{
    switch (page)
    {
    case main::eHelpPage::CREATURES:
        return "creatures";
    case main::eHelpPage::ARTIFACTS:
        return "artifacts";
    case main::eHelpPage::HEROES:
        return "heroes";
    case main::eHelpPage::SPELLS:
        return "spells";
    case main::eHelpPage::TOWNS:
        return "towns";
    default:
        return "skills";
    }
}

LPCSTR CategoryName(main::eHelpPage page, int index)
{
    return Text(H3String::Format("help.%s.categories.%d.name", DataRoot(page), index).String());
}

Facet ReadFacet(main::eHelpPage page, int index, int optionCount)
{
    Facet facet;
    const auto root = H3String::Format("help.%s.facets.%d", DataRoot(page), index);
    facet.name = Text(H3String::Format("%s.name", root.String()).String());
    facet.options.reserve(optionCount);
    for (int option = 0; option < optionCount; ++option)
        facet.options.push_back(Text(H3String::Format("%s.options.%d", root.String(), option).String()));
    return facet;
}

void Finish(CatalogueEntry &entry)
{
    bool found = false;
    LPCSTR extra =
        EraJS::read(H3String::Format("help.%s.entries.%d.description", DataRoot(entry.page), entry.id).String(), found);
    if (found && *extra)
    {
        entry.notes = extra;
        if (!entry.compactDetails && entry.masteryRows.empty())
        {
            entry.description += "\n\n";
            entry.description += entry.notes;
        }
    }
    entry.detailsLoaded = true;
}

int SkillGroup(int skill)
{
    // IDs are the native secondary-skill enum, not translated names.
    switch (skill)
    {
    case 0:
    case 2:
    case 3:
    case 5:
        return 1;
    case 1:
    case 10:
    case 19:
    case 20:
    case 22:
    case 23:
    case 26:
    case 27:
        return 2;
    case 7:
    case 8:
    case 11:
    case 12:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 24:
    case 25:
        return 3;
    default:
        return 4;
    }
}

CatalogueEntry Creature(int id, bool details = true)
{
    const auto &info = P_CreatureInformation[id];
    CatalogueEntry entry{};
    entry.page = main::eHelpPage::CREATURES;
    entry.id = id;
    entry.name = Borrow(info.nameSingular);
    entry.def = NH3Dlg::Assets::CREATURE_LARGE;
    entry.frame = id + 2;
    const bool commander = Era::IsCommanderId && Era::IsCommanderId(id);
    entry.filter.categories.push_back(commander                         ? 12
                                      : info.siegeWeapon                ? 11
                                      : info.town >= 0 && info.town < 9 ? info.town + 1
                                                                        : 10);
    entry.filter.level = info.level;
    entry.filter.facets[0] = info.flyer ? 2 : 1;
    entry.filter.facets[1] = info.shooter ? 2 : 1;
    if (!details)
        return entry;
    const int upgrade = H3Creature::GetUpgrade(id);
    entry.summary = FormatText("help.formats.creature_summary", TownName(info.town), info.level + 1);
    entry.compactDetails = true;
    entry.stats = {{Text("help.text.attack"), Number(info.attack)},
                   {Text("help.text.defense"), Number(info.defence)},
                   {Text("help.text.damage"), Number(info.damageLow) + "-" + Number(info.damageHigh)},
                   {Text("help.text.health"), Number(info.hitPoints)},
                   {Text("help.text.speed"), Number(info.speed)}};
    entry.cardRows = {{Text("help.text.recruitment"),
                       FormatText("help.formats.recruitment", Resources(info.cost).c_str(), info.grow)}};
    std::string charges;
    if (info.numberShots > 0)
        charges = FormatText("help.formats.shots", info.numberShots);
    if (info.spellCharges > 0)
        charges +=
            (charges.empty() ? h3_NullString : " / ") + FormatText("help.formats.spell_charges", info.spellCharges);
    entry.cardRows.push_back({Text("help.text.charges"), charges});
    std::string traits;
    if (info.flyer)
        traits += Text("help.text.flying");
    if (info.shooter)
        traits += Text("help.text.ranged");
    if (info.undead)
        traits += Text("help.text.undead");
    if (info.doubleWide)
        traits += Text("help.text.two_hexes");
    if (info.doubleAttack)
        traits += Text("help.text.double_attack");
    if (info.noRetaliation)
        traits += Text("help.text.no_retaliation");
    if (info.mindImmunity)
        traits += Text("help.text.mind_immunity");
    if (info.fireImmunity)
        traits += Text("help.text.fire_immunity");
    if (info.noMeleePenalty)
        traits += Text("help.text.no_melee_penalty");
    if (info.extendedAttack)
        traits += Text("help.text.extended_attack");
    if (info.attackAllAround)
        traits += Text("help.text.attacks_adjacent_enemies");
    if (info.dragon)
        traits += Text("help.text.dragon");
    if (commander)
        traits += Text("help.text.commander");
    if (info.siegeWeapon)
        traits += Text("help.text.war_machine");
    entry.cardRows.push_back({Text("help.text.traits"), traits});
    entry.cardRows.push_back({Text("help.text.abilities"), Safe(info.description)});
    if (upgrade >= 0 && upgrade != id && upgrade < static_cast<int>(P_CreatureCount))
    {
        const auto upgradeName = Safe(P_CreatureInformation[upgrade].nameSingular);
        entry.cardRows.push_back({Text("help.text.upgrade"), upgradeName});
    }
    Finish(entry);
    return entry;
}

CatalogueEntry Hero(int id, bool details = true)
{
    const auto &info = P_HeroInfo[id];
    const auto &specialty = P_HeroSpecialty[id];
    CatalogueEntry entry{};
    entry.page = main::eHelpPage::HEROES;
    entry.id = id;
    entry.name = Borrow(info.name);
    // Empty/undefined hero slots may still contain a stale portrait pointer in extended tables.
    // Never render a portrait unless the hero record itself has a valid identity/name.
    entry.pcx = info.name && *info.name ? Borrow(info.largePortrait) : h3_NullString;
    const int heroClass = info.heroClass;
    const bool validClass = heroClass >= 0 && heroClass < 18;
    const HeroClassData *type = validClass ? reinterpret_cast<const HeroClassData *>(0x67D868) + heroClass : nullptr;
    const int town = type ? type->town : -1;
    if (town >= 0 && town < 9)
        entry.filter.categories.push_back(town + 1);
    entry.filter.facets[0] = validClass ? (heroClass % 2 + 1) : 0;
    if (!details)
        return entry;
    entry.summary = Safe(type ? type->name : Text("help.text.unknown_class")) + " / " + Safe(TownName(town));
    entry.compactDetails = true;
    entry.statsLabel = Text("help.text.starting_primary_skills");
    if (type)
        for (int skill = 0; skill < 4; ++skill)
            entry.stats.push_back({Borrow(P_PrimarySkillName[skill]), Number(type->primary[skill])});
    entry.cardRows.push_back({Text("help.text.specialty"), Safe(specialty.spFull) + "\n" + Safe(specialty.spDescr)});
    std::string startingSkills;
    std::vector<CardIcon> skillIcons;
    for (const auto &skill : info.sskills)
        if (skill.type >= 0 && skill.type < limits::SECONDARY_SKILLS && skill.level >= 1 && skill.level <= 3)
        {
            const auto name =
                Safe(H3SecondarySkillLevel::Get()[skill.level]) + " " + Safe(P_SecondarySkillInfo[skill.type].name);
            startingSkills += (startingSkills.empty() ? h3_NullString : ", ") + name;
            skillIcons.push_back({NH3Dlg::Assets::SSKILL_44, name, SecondarySkillFrame(skill.type, skill.level)});
        }
    entry.cardRows.push_back(
        {Text("help.text.starting_skills"), startingSkills.empty() ? Text("help.text.none") : startingSkills});
    entry.cardRows.back().icons = std::move(skillIcons);
    std::string startingArmy;
    std::vector<CardIcon> armyIcons;
    const int creatureCount = static_cast<int>(P_CreatureCount);
    for (int stack = 0; stack < 3; ++stack)
    {
        const int creature = info.armyType[stack];
        const auto &amount = info.creatureAmount[stack];
        if (creature >= 0 && creature < creatureCount)
        {
            const auto &creatureInfo = P_CreatureInformation[creature];
            const auto pluralName = Safe(creatureInfo.namePlural);
            const auto stackText = (pluralName.empty() ? Safe(creatureInfo.nameSingular) : pluralName) + ": " +
                                   ArmyAmount(amount.lowAmount, amount.highAmount);
            startingArmy += (startingArmy.empty() ? h3_NullString : "; ") + stackText;
            armyIcons.push_back({NH3Dlg::Assets::CREATURE_SMALL, stackText, creature + 2});
        }
    }
    entry.cardRows.push_back(
        {Text("help.text.starting_army"), startingArmy.empty() ? Text("help.text.none") : startingArmy});
    entry.cardRows.back().icons = std::move(armyIcons);
    const int spellCount = limits::TOTAL_SPELLS;

    const auto startingSpell = info.startingSpell >= 0 && info.startingSpell < spellCount
                                   ? Safe(P_Spell[info.startingSpell].name)
                                   : Text("help.text.none");
    entry.cardRows.push_back(
        {Text("help.text.spellbook"),
         info.hasSpellbook ? FormatText("help.formats.spellbook", startingSpell.c_str()) : Text("help.text.no")});
    if (info.hasSpellbook && info.startingSpell >= 0 && info.startingSpell < spellCount)
        entry.cardRows.back().icons.push_back({NH3Dlg::Assets::SPELL_SMALL, startingSpell, info.startingSpell + 1});
    entry.cardRows.push_back({Text("help.text.biography"), Safe(H3HeroDefaultBiography::Get()[id]), true});
    Finish(entry);
    return entry;
}

CatalogueEntry Spell(int id, bool details = true)
{
    const auto &info = P_Spell[id];
    CatalogueEntry entry{};
    entry.page = main::eHelpPage::SPELLS;
    entry.id = id;
    entry.name = Borrow(info.name);
    entry.def = NH3Dlg::Assets::SPELLS_DEF;
    entry.frame = id;
    entry.spellSchool = FirstSpellSchool(static_cast<int>(info.school));
    const int masks[] = {NH3Spells::NSchool::FIRE, NH3Spells::NSchool::AIR, NH3Spells::NSchool::WATER,
                         NH3Spells::NSchool::EARTH};
    for (int index = 0; index < 4; ++index)
        if (static_cast<int>(info.school) & masks[index])
            entry.filter.categories.push_back(index + 1);
    const bool creatureAbility = IsCreatureSpell(id, info.creatureSpell != FALSE);
    if (creatureAbility)
        entry.filter.categories.push_back(5);
    entry.filter.level = creatureAbility ? 0 : info.level;
    entry.filter.facets[0] = creatureAbility ? 3 : info.mapSpell ? 2 : 1;
    entry.filter.facets[1] = info.damageSpell ? 2 : 1;
    if (!details)
        return entry;
    std::string school;
    for (int index = 0; index < 4; ++index)
        if (static_cast<int>(info.school) & masks[index])
        {
            if (!school.empty())
                school += ", ";
            school += CategoryName(main::eHelpPage::SPELLS, index + 1);
        }
    entry.summary = creatureAbility ? Text("help.text.creature_ability") : FormatText("help.formats.level", info.level);
    if (!school.empty())
        entry.summary += " / " + school;
    if (info.spEffect)
        entry.summary += Text("help.text.sp_hero_spell_power");
    const LPCSTR levels[] = {Text("help.text.none"), Text("help.text.basic"), Text("help.text.advanced"),
                             Text("help.text.expert")};
    for (int level = 0; level < 4; ++level)
    {
        entry.masteryRows.push_back(
            {levels[level], Number(info.manaCost[level]), Borrow(info.description[level]),
             info.spEffect
                 ? FormatText("help.formats.spell_strength", info.baseValue[level], info.spEffect < 0 ? "-" : "+",
                              std::to_string(info.spEffect < 0 ? -static_cast<long long>(info.spEffect) : info.spEffect)
                                  .c_str())
                 : Number(info.baseValue[level])});
    }

    Finish(entry);
    return entry;
}

CatalogueEntry Artifact(int id, bool details = true)
{
    const auto &info = P_ArtifactSetup[id];
    CatalogueEntry entry{};
    entry.page = main::eHelpPage::ARTIFACTS;
    entry.id = id;
    entry.name = Borrow(info.name);
    entry.def = NH3Dlg::Assets::ARTIFACT_DEF;
    entry.frame = id;
    const int type = static_cast<int>(info.type);
    const int flags[] = {2, 4, 8, 16};
    for (int index = 0; index < 4; ++index)
        if (type & flags[index])
            entry.filter.categories.push_back(index + 1);
    if ((type & 1) || !(type & 30))
        entry.filter.categories.push_back(5);
    const int position = static_cast<int>(info.position);
    entry.filter.facets[0] = position >= 1 && position <= 14 ? position : 15;
    if (info.position == eArtifactPositions::ANY_HAND)
        entry.filter.facets[0] = 16;
    if (info.position == eArtifactPositions::LEFT_RING || info.position == eArtifactPositions::RIGHT_RING)
        entry.filter.facets[0] = 7;
    if (info.position == eArtifactPositions::MISC1 || info.position == eArtifactPositions::MISC2 ||
        info.position == eArtifactPositions::MISC3 || info.position == eArtifactPositions::MISC4 ||
        info.position == eArtifactPositions::MISC5)
        entry.filter.facets[0] = 9;
    entry.filter.facets[1] = info.partOfComboArtifactId != eCombinationArtifacts::NONE ? 2 : 1;
    if (!details)
        return entry;
    const auto positions = ReadFacet(main::eHelpPage::ARTIFACTS, 0, 17).options;

    entry.summary = FormatText("help.formats.artifact_summary", info.cost, positions[entry.filter.facets[0]]);
    entry.compactDetails = true;
    entry.cardRows.push_back({Text("help.text.effect"), Safe(info.description)});
    if (info.disabled)
        entry.cardRows.push_back({Text("help.text.availability"), Text("help.text.disabled_in_the_artifact_pool")});
    if (info.hasSpell)
        entry.cardRows.push_back({Text("help.text.magic"), Text("help.text.provides_spells")});
    if (info.partOfComboArtifactId != eCombinationArtifacts::NONE)
    {
        const int combined = FindCombinationArtifactId(H3ArtifactSetup::Get(), ArtifactTableCount(),
                                                       static_cast<int>(info.partOfComboArtifactId));
        const auto name = combined >= 0 ? Safe(P_ArtifactSetup[combined].name) : Text("help.text.unknown_combination");
        entry.cardRows.push_back({Text("help.text.combines_into"), name});
    }
    if (info.comboArtifactId != eCombinationArtifacts::NONE)
    {
        std::string components;
        for (int component = 0; component < ArtifactTableCount(); ++component)
            if (P_ArtifactSetup[component].partOfComboArtifactId == info.comboArtifactId)
            {
                const auto name = Safe(P_ArtifactSetup[component].name);
                components += (components.empty() ? h3_NullString : ", ") + name;
            }
        entry.cardRows.push_back({Text("help.text.components"), components});
    }
    Finish(entry);
    return entry;
}
CatalogueEntry Skill(int id, bool details = true)
{
    CatalogueEntry entry{};
    entry.page = main::eHelpPage::SECONDARY_SKILLS;
    entry.id = id;
    entry.name = Borrow(P_SecondarySkillInfo[id].name);
    entry.def = NH3Dlg::Assets::SSKILL_44;
    entry.frame = 3 + id * 3;
    entry.filter.categories.push_back(SkillGroup(id));
    if (!details)
        return entry;
    const LPCSTR levels[] = {Text("help.text.basic"), Text("help.text.advanced"), Text("help.text.expert")};
    for (int level = 0; level < 3; ++level)
    {
        entry.masteryRows.push_back(
            {levels[level], h3_NullString, Borrow(P_SecondarySkillInfo[id].description[level])});
    }

    Finish(entry);
    return entry;
}

CatalogueEntry Town(int town, bool details = true)
{
    CatalogueEntry entry{};
    entry.page = main::eHelpPage::TOWNS;
    entry.id = town;
    entry.name = Borrow(TownName(town));
    entry.def = NH3Dlg::Assets::TOWN_SMALL;
    entry.frame = town * 2 + 2;
    entry.filter.categories.push_back(town < 3 ? 1 : town < 6 ? 2 : 3);
    if (!details)
        return entry;
    entry.description = Text("help.text.creature_roster_base_upgraded");
    for (int level = 0; level < 7; ++level)
    {
        const int base = P_TownCreatureTypes[town].base[level];
        const int upgrade = P_TownCreatureTypes[town].upgrade[level];
        const int count = static_cast<int>(P_CreatureCount);
        entry.description += FormatText("help.formats.town_roster", level + 1,
                                        base >= 0 && base < count ? P_CreatureInformation[base].nameSingular
                                                                  : Text("help.text.unavailable"),
                                        upgrade >= 0 && upgrade < count ? P_CreatureInformation[upgrade].nameSingular
                                                                        : Text("help.text.unavailable"));
    }
    entry.description += Text("help.text.hero_classes");
    const auto *classes = reinterpret_cast<const HeroClassData *>(0x67D868);
    for (int type = 0; type < 18; ++type)
        if (classes[type].town == town)
            entry.description += Safe(classes[type].name) + "\n";

    Finish(entry);
    return entry;
}
} // namespace

LPCSTR TownName(int town)
{
    return town >= 0 && town < 9 ? P_TownNames[town] : Text("help.ui.neutral");
}

bool TryBuildObjectEntry(main::eHelpPage page, int id, CatalogueEntry &entry, bool details)
{
    if (id < 0)
        return false;
    switch (page)
    {
    case main::eHelpPage::CREATURES:
        if (!HasCreatureData(id))
            return false;
        entry = Creature(id, details);
        break;
    case main::eHelpPage::ARTIFACTS: {
        if (id >= ArtifactTableCount() || IsArtifactPlaceholder(id) ||
            !HasArtifactText(P_ArtifactSetup[id].name, P_ArtifactSetup[id].description))
            return false;
        ArtifactFrameProbe frames;
        if (!frames.HasImage(id))
            return false;
        entry = Artifact(id, details);
        break;
    }
    case main::eHelpPage::HEROES:
        if (id >= static_cast<int>(H3HeroCount::Get()))
            return false;
        entry = Hero(id, details);
        break;
    case main::eHelpPage::SPELLS:
        if (id >= limits::TOTAL_SPELLS)
            return false;
        entry = Spell(id, details);
        break;
    case main::eHelpPage::SECONDARY_SKILLS:
        if (id >= limits::SECONDARY_SKILLS)
            return false;
        entry = Skill(id, details);
        break;
    case main::eHelpPage::TOWNS:
        if (id >= 9)
            return false;
        entry = Town(id, details);
        break;
    default:
        return false;
    }
    return true;
}

Catalogue BuildCatalogue(main::eHelpPage page)
{
    Catalogue catalogue{};
    catalogue.page = page;
    catalogue.categories.reserve(13);
    catalogue.categories.push_back(Text("help.ui.all"));
    if (page == main::eHelpPage::CREATURES || page == main::eHelpPage::HEROES)
    {
        for (int town = 0; town < 9; ++town)
            catalogue.categories.emplace_back(TownName(town));
        catalogue.factionCategoryCount = static_cast<int>(catalogue.categories.size()) - 1;
    }
    if (page == main::eHelpPage::CREATURES)
    {
        catalogue.categories.resize(13);
        catalogue.firstLevel = 1;
        catalogue.levelCount = 7;
        catalogue.facets = {ReadFacet(page, 0, 3), ReadFacet(page, 1, 3)};
        const int count = static_cast<int>(P_CreatureCount);
        catalogue.entries.reserve(count);
        for (int id = 0; id < count; ++id)
            if (HasCreatureData(id))
                catalogue.entries.push_back(Creature(id, false));
    }
    else if (page == main::eHelpPage::HEROES)
    {
        catalogue.facets.clear();
        const int heroCount = static_cast<int>(H3HeroCount::Get());
        catalogue.entries.reserve(heroCount);
        for (int id = 0; id < heroCount; ++id)
            catalogue.entries.push_back(Hero(id, false));
    }
    else if (page == main::eHelpPage::ARTIFACTS)
    {
        catalogue.categories.resize(6);
        catalogue.facets = {ReadFacet(page, 0, 17), ReadFacet(page, 1, 3)};
        const int count = ArtifactTableCount();
        catalogue.entries.reserve(count);
        ArtifactFrameProbe frames;
        for (int id = 0; id < count; ++id)
            if (!IsArtifactPlaceholder(id) &&
                HasArtifactText(P_ArtifactSetup[id].name, P_ArtifactSetup[id].description) && frames.HasImage(id))
                catalogue.entries.push_back(Artifact(id, false));
    }
    else if (page == main::eHelpPage::SPELLS)
    {
        catalogue.categories.resize(6);
        catalogue.levelCount = 6;
        catalogue.facets = {ReadFacet(page, 0, 4), ReadFacet(page, 1, 3)};
        // H3SpellCount is the 70 hero spells; H3Spell's native table has
        // 81 entries including creature abilities, IDs 70..80.
        const int count = limits::TOTAL_SPELLS;
        catalogue.entries.reserve(count);
        for (int id = 0; id < count; ++id)
            catalogue.entries.push_back(Spell(id, false));
    }
    else if (page == main::eHelpPage::SECONDARY_SKILLS)
    {
        catalogue.categories.resize(5);
        catalogue.entries.reserve(limits::SECONDARY_SKILLS);
        for (int id = 0; id < limits::SECONDARY_SKILLS; ++id)
            catalogue.entries.push_back(Skill(id, false));
    }
    else if (page == main::eHelpPage::TOWNS)
    {
        catalogue.categories.resize(4);
        catalogue.entries.reserve(9);
        for (int town = 0; town < 9; ++town)
            catalogue.entries.push_back(Town(town, false));
    }
    for (size_t index = 0; index < catalogue.categories.size(); ++index)
    {
        // Native town labels follow the currently loaded game language and mods.
        if (index >= 1 && index <= static_cast<size_t>(catalogue.factionCategoryCount))
            continue;
        catalogue.categories[index] = CategoryName(page, static_cast<int>(index));
    }
    return catalogue;
}

H3DlgDef *CreateSpellSchoolUnderlay(int x, int y, int width, int height, int school)
{
    const char *name = SpellSchoolDef(school);
    if (!name)
        return nullptr;
    H3DefLoader def(name);
    if (!def.Get() || !def->groups || def->groupsCount <= 0 || !def->groups[0] || def->groups[0]->count <= 0 ||
        def->widthDEF <= 0 || def->heightDEF <= 0 || def->widthDEF > width || def->heightDEF > height)
        return nullptr;
    auto *item = H3DlgDef::Create(x, y, width, height, -1, name, 0);
    if (item)
        item->DeActivate();
    return item;
}

bool IsObjectBanned(main::eHelpPage page, int id)
{
    if (id < 0)
        return false;
    auto *game = H3Main::Get();
    // The Help window can also be opened from the main menu. Do not treat
    // unloaded scenario arrays as current bans or read beyond native tables.
    const bool hasMap = game && game->mainSetup.mapitems && game->mainSetup.mapSize > 0;
    switch (page)
    {
    case main::eHelpPage::ARTIFACTS:
        return (id < ArtifactTableCount() && P_ArtifactSetup[id].disabled) ||
               (hasMap && RuntimeBanFlag(game->artifactsAllowed, 144, id));
    case main::eHelpPage::SPELLS:
        return hasMap && RuntimeBanFlag(game->disabledSpells, 70, id);
    case main::eHelpPage::SECONDARY_SKILLS:
        return hasMap && RuntimeBanFlag(game->bannedSkills, 28, id);
    case main::eHelpPage::HEROES:
        if (hasMap && id < 156)
        {
            bool offered = false;
            for (const auto &player : game->players)
                offered |= player.tavernHeroL == id || player.tavernHeroR == id;
            return RuntimeHeroBanned(game->heroOwner[id], game->heroMayBeHiredBy[id].Get(), offered);
        }
        return false;
    default:
        return false;
    }
}

int CreatureSpellFallback(int spellId) noexcept
{
    // Native creature-ability spell IDs 70..80 have no guaranteed frames in spells.def.
    // Use the creature requested for each ability only when the spell frame is missing.
    static const int creatures[] = {
        NH3Creatures::BASILISK,      NH3Creatures::WYVERN_MONARCH, NH3Creatures::DENDROID_GUARD,
        NH3Creatures::ZOMBIE,        NH3Creatures::SCORPICORE,     NH3Creatures::GHOST_DRAGON,
        NH3Creatures::LICH,          NH3Creatures::THUNDERBIRD,    NH3Creatures::SERPENT_FLY,
        NH3Creatures::MIGHTY_GORGON, NH3Creatures::RUST_DRAGON};
    const int index = spellId - 70;
    return index >= 0 && index < static_cast<int>(sizeof(creatures) / sizeof(creatures[0])) ? creatures[index] : -1;
}

bool ShowCreatureDetails(int creatureId, bool popup)
{
    if (!HasCreatureData(creatureId))
        return false;
    if (!popup)
        return ShowNativeCreatureDialog(creatureId);

    // Reuse the exact catalogue entry and RMB rendering path of the Creatures
    // page instead of opening the native LMB creature window.
    return ShowObjectDetails(Creature(creatureId), popup);
}

bool ShowObjectDetails(const CatalogueEntry &entry, bool popup)
{
    if (entry.page == main::eHelpPage::CREATURES && !popup)
        return ShowNativeCreatureDialog(entry.id);
    if (!entry.detailsLoaded && entry.id >= 0)
    {
        CatalogueEntry details;
        return TryBuildObjectEntry(entry.page, entry.id, details) && ShowObjectDetails(details, popup);
    }
    if (!entry.masteryRows.empty())
        return ShowMasteryDialog(entry, popup);
    if (entry.compactDetails)
        return ShowCompactObjectCard(entry, popup);
    const int width = std::min(620, H3GameWidth::Get() - 20);
    const int height = std::min(560, H3GameHeight::Get() - 20);
    H3Dlg dlg(width, height, -1, -1, false, false);
    if (!AddSafeBackground(dlg))
        return false;
    int titleY = 16;
    if (*entry.pcx)
    {
        H3PcxLoader portrait(entry.pcx);
        if (portrait.Get() && portrait->width > 0 && portrait->height > 0 && portrait->width <= width - 36 &&
            portrait->height <= height - 190)
        {
            dlg.CreatePcx((width - portrait->width) / 2, 16, 10, entry.pcx);
            titleY += portrait->height + 8;
        }
    }
    else if (entry.def)
    {
        H3DefLoader def(entry.def);
        if (def.Get() && def->groups && def->groupsCount > 0 && def->groups[0] && entry.frame >= 0 &&
            entry.frame < def->groups[0]->count && def->widthDEF > 0 && def->heightDEF > 0 &&
            def->widthDEF <= width - 36 && def->heightDEF <= height - 190)
        {
            const int imageX = (width - def->widthDEF) / 2;
            if (auto *underlay =
                    CreateSpellSchoolUnderlay(imageX, 16, def->widthDEF, def->heightDEF, entry.spellSchool))
                dlg.AddItem(underlay);
            dlg.CreateDef(imageX, 16, 10, entry.def, entry.frame);
            titleY += def->heightDEF + 8;
        }
    }
    dlg.CreateText(18, titleY, width - 36, 32, ObjectTitle(entry.name, entry.id).c_str(), NH3Dlg::Text::BIG,
                   eTextColor::GOLD, -1);
    int bodyY = titleY + 38;
    if (!entry.summary.empty())
    {
        dlg.CreateText(18, bodyY, width - 36, 26, entry.summary.c_str(), NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1);
        bodyY += 30;
    }
    auto *text = H3DlgScrollableText::Create(entry.description.c_str(), 18, bodyY, width - 54,
                                             height - bodyY - (popup ? 20 : 62), NH3Dlg::Text::MEDIUM,
                                             eTextColor::REGULAR, false);
    if (text)
        dlg.AddItem(text);
    ShowDetailsDialog(dlg, popup);
    return true;
}
} // namespace helpdlg
