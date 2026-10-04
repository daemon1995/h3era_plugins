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

std::string Number(int value)
{
    return std::to_string(value);
}
std::string Resources(const H3Resources &cost)
{
    std::string text;
    for (int resource = 0; resource < 7; ++resource)
        if (cost.asArray[resource])
            text += (text.empty() ? "" : ", ") + Number(cost.asArray[resource]) + " " + Safe(P_ResourceName[resource]);
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

void Finish(CatalogueEntry &entry)
{
    bool found = false;
    LPCSTR extra =
        EraJS::read(H3String::Format("help.%s.entries.%d.description", DataRoot(entry.page), entry.id).String(), found);
    if (found && extra && *extra)
    {
        entry.notes = Safe(extra);
        entry.description += "\n\n" + entry.notes;
    }
    entry.filter.searchable = entry.name;
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

CatalogueEntry Creature(int id)
{
    const auto &info = P_CreatureInformation[id];
    CatalogueEntry entry{};
    entry.page = main::eHelpPage::CREATURES;
    entry.id = id;
    entry.name = Safe(info.nameSingular);
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
    entry.filter.facets[2] = info.undead ? 2 : info.alive ? 1 : 3;
    int upgrade = H3Creature::GetUpgrade(id);
    bool upgraded = false;
    // The town roster is authoritative for town upgrades; modded creatures
    // without a standard upgrade relation remain in the independent group.
    if (info.town >= 0 && info.town < 9)
        for (int level = 0; level < 7; ++level)
            upgraded |= P_TownCreatureTypes[info.town].upgrade[level] == id;
    entry.filter.facets[3] = upgraded ? 2 : upgrade >= 0 && upgrade != id ? 1 : 3;
    entry.summary = Safe(TownName(info.town)) + " / Level " + Number(info.level + 1);
    entry.compactDetails = true;
    entry.stats = {{"Attack", Number(info.attack)},
                   {"Defense", Number(info.defence)},
                   {"Damage", Number(info.damageLow) + "-" + Number(info.damageHigh)},
                   {"Health", Number(info.hitPoints)},
                   {"Speed", Number(info.speed)}};
    entry.cardRows = {{"Recruitment", Resources(info.cost) + " / Weekly growth: " + Number(info.grow)}};
    std::string charges;
    if (info.numberShots > 0)
        charges = "Shots: " + Number(info.numberShots);
    if (info.spellCharges > 0)
        charges += (charges.empty() ? "" : " / ") + std::string("Spell charges: ") + Number(info.spellCharges);
    entry.cardRows.push_back({"Charges", charges});
    entry.description =
        entry.summary + "\n\n" + Safe(info.description) +
        "\n\n"
        "Attack: " +
        Number(info.attack) + "   Defense: " + Number(info.defence) + "\nDamage: " + Number(info.damageLow) + " - " +
        Number(info.damageHigh) + "\nHealth: " + Number(info.hitPoints) + "   Speed: " + Number(info.speed) +
        "\nGrowth: " + Number(info.grow) + "\nShots: " + Number(info.numberShots) +
        "   Spell charges: " + Number(info.spellCharges) + "\nCost: " + Resources(info.cost) + "\n\nTraits: ";
    std::string traits;
    if (info.flyer)
        traits += "Flying. ";
    if (info.shooter)
        traits += "Ranged. ";
    if (info.undead)
        traits += "Undead. ";
    if (info.doubleWide)
        traits += "Two hexes. ";
    if (info.doubleAttack)
        traits += "Double attack. ";
    if (info.noRetaliation)
        traits += "No retaliation. ";
    if (info.mindImmunity)
        traits += "Mind immunity. ";
    if (info.fireImmunity)
        traits += "Fire immunity. ";
    if (info.noMeleePenalty)
        traits += "No melee penalty. ";
    if (info.extendedAttack)
        traits += "Extended attack. ";
    if (info.attackAllAround)
        traits += "Attacks adjacent enemies. ";
    if (info.dragon)
        traits += "Dragon. ";
    if (commander)
        traits += "Commander. ";
    if (info.siegeWeapon)
        traits += "War machine. ";
    entry.description += traits;
    entry.cardRows.push_back({"Traits", traits});
    entry.cardRows.push_back({"Abilities", Safe(info.description)});
    if (upgrade >= 0 && upgrade != id && upgrade < static_cast<int>(P_CreatureCount))
    {
        const auto upgradeName = Safe(P_CreatureInformation[upgrade].nameSingular);
        entry.description += "\nUpgrade: " + upgradeName;
        entry.cardRows.push_back({"Upgrade", upgradeName});
    }
    Finish(entry);
    return entry;
}

CatalogueEntry Hero(int id)
{
    const auto &info = P_HeroInfo[id];
    const auto &specialty = P_HeroSpecialty[id];
    CatalogueEntry entry{};
    entry.page = main::eHelpPage::HEROES;
    entry.id = id;
    entry.name = Safe(info.name);
    entry.pcx = Safe(info.largePortrait);
    const int heroClass = info.heroClass;
    const bool validClass = heroClass >= 0 && heroClass < 18;
    const HeroClassData *type = validClass ? reinterpret_cast<const HeroClassData *>(0x67D868) + heroClass : nullptr;
    const int town = type ? type->town : -1;
    if (town >= 0 && town < 9)
        entry.filter.categories.push_back(town + 1);
    entry.filter.facets[0] = validClass ? (heroClass % 2 + 1) : 0;
    entry.filter.facets[1] = info.campaignHero || id >= limits::HEROES ? 2 : 1;
    entry.filter.facets[2] = info.hasSpellbook ? 2 : 1;
    entry.filter.facets[3] = info.isFemale ? 2 : 1;
    entry.summary = Safe(type ? type->name : "Unknown class") + " / " + Safe(TownName(town));
    entry.compactDetails = true;
    entry.statsLabel = "Starting primary skills";
    if (type)
        for (int skill = 0; skill < 4; ++skill)
            entry.stats.push_back({Safe(P_PrimarySkillName[skill]), Number(type->primary[skill])});
    entry.cardRows.push_back({"Specialty", Safe(specialty.spFull) + "\n" + Safe(specialty.spDescr)});
    entry.description = entry.summary + "\n\n" + Safe(H3HeroDefaultBiography::Get()[id]) +
                        "\n\nSpecialty: " + Safe(specialty.spFull) + "\n" + Safe(specialty.spDescr);
    if (type)
    {
        entry.description += "\n\nStarting primary skills:\n";
        for (int skill = 0; skill < 4; ++skill)
            entry.description += Safe(P_PrimarySkillName[skill]) + ": " + Number(type->primary[skill]) + "\n";
    }
    std::string startingSkills;
    std::vector<CardIcon> skillIcons;
    entry.description += "\nStarting secondary skills:\n";
    for (const auto &skill : info.sskills)
        if (skill.type >= 0 && skill.type < limits::SECONDARY_SKILLS && skill.level >= 1 && skill.level <= 3)
        {
            const auto name =
                Safe(H3SecondarySkillLevel::Get()[skill.level]) + " " + Safe(P_SecondarySkillInfo[skill.type].name);
            entry.description += name + "\n";
            startingSkills += (startingSkills.empty() ? "" : ", ") + name;
            skillIcons.push_back({NH3Dlg::Assets::SSKILL_44, name, SecondarySkillFrame(skill.type, skill.level)});
        }
    entry.cardRows.push_back({"Starting skills", startingSkills.empty() ? "None" : startingSkills});
    entry.cardRows.back().icons = std::move(skillIcons);
    entry.description += "\nStarting army:\n";
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
            entry.description += stackText + "\n";
            startingArmy += (startingArmy.empty() ? "" : "; ") + stackText;
            armyIcons.push_back({NH3Dlg::Assets::CREATURE_SMALL, stackText, creature + 2});
        }
    }
    entry.cardRows.push_back({"Starting army", startingArmy.empty() ? "None" : startingArmy});
    entry.cardRows.back().icons = std::move(armyIcons);
    entry.description += "\nSpellbook: " + std::string(info.hasSpellbook ? "Yes" : "No");
    const int spellCount = limits::TOTAL_SPELLS;
    entry.description +=
        "\nStarting spell: " +
        (info.startingSpell >= 0 && info.startingSpell < spellCount ? Safe(P_Spell[info.startingSpell].name) : "None");

    const auto startingSpell =
        info.startingSpell >= 0 && info.startingSpell < spellCount ? Safe(P_Spell[info.startingSpell].name) : "None";
    entry.cardRows.push_back({"Spellbook", info.hasSpellbook ? "Yes / " + startingSpell : "No"});
    if (info.hasSpellbook && info.startingSpell >= 0 && info.startingSpell < spellCount)
        entry.cardRows.back().icons.push_back({NH3Dlg::Assets::SPELL_SMALL, startingSpell, info.startingSpell + 1});
    entry.cardRows.push_back({"Biography", Safe(H3HeroDefaultBiography::Get()[id]), true});
    Finish(entry);
    return entry;
}

CatalogueEntry Spell(int id)
{
    const auto &info = P_Spell[id];
    CatalogueEntry entry{};
    entry.page = main::eHelpPage::SPELLS;
    entry.id = id;
    entry.name = Safe(info.name);
    entry.def = NH3Dlg::Assets::SPELLS_DEF;
    entry.frame = id;
    entry.spellSchool = FirstSpellSchool(static_cast<int>(info.school));
    const int masks[] = {NH3Spells::NSchool::FIRE, NH3Spells::NSchool::AIR, NH3Spells::NSchool::WATER,
                         NH3Spells::NSchool::EARTH};
    const LPCSTR schools[] = {"Fire", "Air", "Water", "Earth"};
    std::string school;
    for (int index = 0; index < 4; ++index)
        if (static_cast<int>(info.school) & masks[index])
        {
            entry.filter.categories.push_back(index + 1);
            school += (school.empty() ? "" : ", ") + std::string(schools[index]);
        }
    const bool creatureAbility = IsCreatureSpell(id, info.creatureSpell != FALSE);
    if (creatureAbility)
        entry.filter.categories.push_back(5);
    entry.filter.level = creatureAbility ? 0 : info.level;
    entry.filter.facets[0] = creatureAbility ? 3 : info.mapSpell ? 2 : 1;
    entry.filter.facets[1] = info.damageSpell ? 2 : 1;
    entry.filter.facets[2] = info.expertMassVersion ? 2 : 1;
    entry.summary = creatureAbility ? "Creature ability" : "Level " + Number(info.level);
    if (!school.empty())
        entry.summary += " / " + school;
    if (info.spEffect)
        entry.summary += "\nSP = hero Spell Power";
    entry.description = entry.summary + "\nSpell power coefficient: " + Number(info.spEffect) + "\n";
    const LPCSTR levels[] = {"None", "Basic", "Advanced", "Expert"};
    for (int level = 0; level < 4; ++level)
    {
        entry.masteryRows.push_back({levels[level], Number(info.manaCost[level]), Safe(info.description[level]),
                                     SpellStrength(info.baseValue[level], info.spEffect)});
        entry.description += "\n" + std::string(levels[level]) + " / Mana: " + Number(info.manaCost[level]) +
                             " / Base effect: " + Number(info.baseValue[level]) + "\n" + Safe(info.description[level]) +
                             "\n";
    }

    Finish(entry);
    return entry;
}

CatalogueEntry Artifact(int id)
{
    const auto &info = P_ArtifactSetup[id];
    CatalogueEntry entry{};
    entry.page = main::eHelpPage::ARTIFACTS;
    entry.id = id;
    entry.name = Safe(info.name);
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
    entry.filter.facets[1] = info.disabled ? 2 : 1;
    entry.filter.facets[2] = info.partOfComboArtifactId != eCombinationArtifacts::NONE ? 2 : 1;
    entry.filter.facets[3] = info.hasSpell ? 2 : 1;
    const LPCSTR positions[] = {"Any",       "Head",       "Shoulders",      "Neck",     "Right hand",
                                "Left hand", "Torso",      "Ring",           "Feet",     "Miscellaneous",
                                "Ballista",  "Ammo cart",  "First aid tent", "Catapult", "Spellbook",
                                "Other",     "Either hand"};
    entry.summary = "Cost: " + Number(info.cost) + " gold / " + positions[entry.filter.facets[0]];
    entry.compactDetails = true;
    entry.cardRows.push_back({"Effect", Safe(info.description)});
    if (info.disabled)
        entry.cardRows.push_back({"Availability", "Disabled in the artifact pool"});
    if (info.hasSpell)
        entry.cardRows.push_back({"Magic", "Provides spells"});
    entry.description = Safe(info.description) + "\n\n" + entry.summary +
                        "\nAvailable in the artifact table: " + (info.disabled ? "No" : "Yes") +
                        "\nProvides spells: " + (info.hasSpell ? "Yes" : "No");
    if (info.partOfComboArtifactId != eCombinationArtifacts::NONE)
    {
        const int combined = FindCombinationArtifactId(H3ArtifactSetup::Get(), H3ArtifactCount::Get(),
                                                       static_cast<int>(info.partOfComboArtifactId));
        const auto name = combined >= 0 ? Safe(P_ArtifactSetup[combined].name) : "Unknown combination";
        entry.description += "\nCombination component of: " + name;
        entry.cardRows.push_back({"Combines into", name});
    }
    if (info.comboArtifactId != eCombinationArtifacts::NONE)
    {
        entry.description += "\nAssembled combination artifact.\nComponents:\n";
        std::string components;
        for (int component = 0; component < H3ArtifactCount::Get(); ++component)
            if (P_ArtifactSetup[component].partOfComboArtifactId == info.comboArtifactId)
            {
                const auto name = Safe(P_ArtifactSetup[component].name);
                entry.description += name + "\n";
                components += (components.empty() ? "" : ", ") + name;
            }
        entry.cardRows.push_back({"Components", components});
    }
    Finish(entry);
    return entry;
}
CatalogueEntry Skill(int id)
{
    CatalogueEntry entry{};
    entry.page = main::eHelpPage::SECONDARY_SKILLS;
    entry.id = id;
    entry.name = Safe(P_SecondarySkillInfo[id].name);
    entry.def = NH3Dlg::Assets::SSKILL_44;
    entry.frame = 3 + id * 3;
    entry.filter.categories.push_back(SkillGroup(id));
    const LPCSTR levels[] = {"Basic", "Advanced", "Expert"};
    for (int level = 0; level < 3; ++level)
    {
        entry.masteryRows.push_back({levels[level], "", Safe(P_SecondarySkillInfo[id].description[level])});
        entry.description +=
            std::string(levels[level]) + "\n" + Safe(P_SecondarySkillInfo[id].description[level]) + "\n\n";
    }

    Finish(entry);
    return entry;
}

CatalogueEntry Town(int town)
{
    CatalogueEntry entry{};
    entry.page = main::eHelpPage::TOWNS;
    entry.id = town;
    entry.name = Safe(TownName(town));
    entry.def = NH3Dlg::Assets::TOWN_SMALL;
    entry.frame = town * 2 + 2;
    entry.filter.categories.push_back(town < 3 ? 1 : town < 6 ? 2 : 3);
    entry.description = "Creature roster (base / upgraded):\n\n";
    for (int level = 0; level < 7; ++level)
    {
        const int base = P_TownCreatureTypes[town].base[level];
        const int upgrade = P_TownCreatureTypes[town].upgrade[level];
        const int count = static_cast<int>(P_CreatureCount);
        entry.description +=
            "Level " + Number(level + 1) + ": " +
            (base >= 0 && base < count ? Safe(P_CreatureInformation[base].nameSingular) : "Unavailable") + " / " +
            (upgrade >= 0 && upgrade < count ? Safe(P_CreatureInformation[upgrade].nameSingular) : "Unavailable") +
            "\n";
    }
    entry.description += "\nHero classes:\n";
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
    return town >= 0 && town < 9 ? P_TownNames[town] : Text("help.ui.neutral", "Neutral / independent");
}

bool TryBuildObjectEntry(main::eHelpPage page, int id, CatalogueEntry &entry)
{
    if (id < 0)
        return false;
    switch (page)
    {
    case main::eHelpPage::CREATURES:
        if (id >= static_cast<int>(P_CreatureCount))
            return false;
        entry = Creature(id);
        break;
    case main::eHelpPage::ARTIFACTS:
        if (id >= H3ArtifactCount::Get())
            return false;
        entry = Artifact(id);
        break;
    case main::eHelpPage::HEROES:
        if (id >= limits::TOTAL_HEROES)
            return false;
        entry = Hero(id);
        break;
    case main::eHelpPage::SPELLS:
        if (id >= limits::TOTAL_SPELLS)
            return false;
        entry = Spell(id);
        break;
    case main::eHelpPage::SECONDARY_SKILLS:
        if (id >= limits::SECONDARY_SKILLS)
            return false;
        entry = Skill(id);
        break;
    case main::eHelpPage::TOWNS:
        if (id >= 9)
            return false;
        entry = Town(id);
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
    catalogue.categories.push_back(Text("help.ui.all", "All"));
    if (page == main::eHelpPage::CREATURES || page == main::eHelpPage::HEROES)
        for (int town = 0; town < 9; ++town)
            catalogue.categories.emplace_back(TownName(town));
    if (page == main::eHelpPage::CREATURES)
    {
        catalogue.categories.insert(catalogue.categories.end(), {"Neutral", "War machines", "Commanders"});
        catalogue.firstLevel = 1;
        catalogue.levelCount = 7;
        catalogue.facets = {{"Movement", {"Any", "Ground", "Flying"}},
                            {"Attack", {"Any", "Melee", "Ranged"}},
                            {"Life", {"Any", "Living", "Undead", "Other"}},
                            {"Upgrade", {"Any", "Base", "Upgraded", "Independent"}}};
        for (int id = 0; id < static_cast<int>(P_CreatureCount); ++id)
            catalogue.entries.push_back(Creature(id));
    }
    else if (page == main::eHelpPage::HEROES)
    {
        catalogue.facets = {{"Class", {"Any", "Might", "Magic"}},
                            {"Availability", {"Any", "Regular", "Campaign"}},
                            {"Spellbook", {"Any", "No", "Yes"}},
                            {"Gender", {"Any", "Male", "Female"}}};
        for (int id = 0; id < limits::TOTAL_HEROES; ++id)
            catalogue.entries.push_back(Hero(id));
    }
    else if (page == main::eHelpPage::ARTIFACTS)
    {
        catalogue.categories.insert(catalogue.categories.end(),
                                    {"Treasure", "Minor", "Major", "Relic", "Special / other"});
        catalogue.facets = {
            {"Position",
             {"Any", "Head", "Shoulders", "Neck", "Right hand", "Left hand", "Torso", "Ring", "Feet", "Miscellaneous",
              "Ballista", "Ammo cart", "First aid tent", "Catapult", "Spellbook", "Other", "Either hand"}},
            {"Availability", {"Any", "Enabled", "Disabled"}},
            {"Combination", {"Any", "Not a component", "Component"}},
            {"Spells", {"Any", "No", "Yes"}}};
        for (int id = 0; id < H3ArtifactCount::Get(); ++id)
            catalogue.entries.push_back(Artifact(id));
    }
    else if (page == main::eHelpPage::SPELLS)
    {
        catalogue.categories.insert(catalogue.categories.end(),
                                    {"Fire", "Air", "Water", "Earth", "Creature abilities"});
        catalogue.levelCount = 6;
        catalogue.facets = {{"Use", {"Any", "Combat", "Adventure", "Creature ability"}},
                            {"Damage", {"Any", "No", "Yes"}},
                            {"Expert mass", {"Any", "No", "Yes"}}};
        // H3SpellCount is the 70 hero spells; H3Spell's native table has
        // 81 entries including creature abilities, IDs 70..80.
        const int count = limits::TOTAL_SPELLS;
        for (int id = 0; id < count; ++id)
            catalogue.entries.push_back(Spell(id));
    }
    else if (page == main::eHelpPage::SECONDARY_SKILLS)
    {
        catalogue.categories.insert(catalogue.categories.end(),
                                    {"Adventure", "Combat", "Magic", "Economy / leadership"});
        for (int id = 0; id < limits::SECONDARY_SKILLS; ++id)
            catalogue.entries.push_back(Skill(id));
    }
    else if (page == main::eHelpPage::TOWNS)
    {
        catalogue.categories.insert(catalogue.categories.end(), {"Good", "Evil", "Neutral alignment"});
        for (int town = 0; town < 9; ++town)
            catalogue.entries.push_back(Town(town));
    }
    for (size_t index = 0; index < catalogue.categories.size(); ++index)
    {
        // Native town labels follow the currently loaded game language and mods.
        if ((page == main::eHelpPage::CREATURES || page == main::eHelpPage::HEROES) && index >= 1 && index <= 9)
            continue;
        catalogue.categories[index] =
            Text(H3String::Format("help.%s.categories.%d.name", DataRoot(page), static_cast<int>(index)).String(),
                 catalogue.categories[index].c_str());
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
        return (id < H3ArtifactCount::Get() && P_ArtifactSetup[id].disabled) ||
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

bool ShowObjectDetails(const CatalogueEntry &entry, bool popup)
{
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
    if (!entry.pcx.empty())
    {
        H3PcxLoader portrait(entry.pcx.c_str());
        if (portrait.Get() && portrait->width > 0 && portrait->height > 0 && portrait->width <= width - 36 &&
            portrait->height <= height - 190)
        {
            dlg.CreatePcx((width - portrait->width) / 2, 16, 10, entry.pcx.c_str());
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
    dlg.CreateText(18, titleY, width - 36, 32, entry.name.c_str(), NH3Dlg::Text::BIG, eTextColor::GOLD, -1);
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
