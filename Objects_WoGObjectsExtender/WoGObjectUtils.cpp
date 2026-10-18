#include "WoGObjectUtils.h"
#include "WoGObjectsExtender.h"
#include <algorithm>
#include <map>

namespace wog { namespace util {
namespace {
constexpr int FIRST_PICTURE_ID = 30729;
constexpr int CHOOSE_PICTURE_OR_CANCEL = 10;
using SetSelectablePictures = void(__stdcall *)(int);

int ChoosePicturesImpl(const char *text, const H3PictureVector &pictures)
{
    if (!pictures.Size())
        return -1;
    const HMODULE era = GetModuleHandleA("era.dll");
    const auto setSelectablePictures = era ? reinterpret_cast<SetSelectablePictures>(
        GetProcAddress(era, "_SetDialog8SelectablePicsMask")) : nullptr;
    if (setSelectablePictures)
        setSelectablePictures((1 << pictures.Size()) - 1);
    Era::TComplexDialogOpts options{};
    options.msgType = CHOOSE_PICTURE_OR_CANCEL;
    H3Messagebox::Display(text, pictures, options.value);
    const int choice = H3WindowManager::Get()->ClickedItemID() - FIRST_PICTURE_ID;
    return choice >= 0 && choice < static_cast<int>(pictures.Size()) ? choice : -1;
}
}

std::string Tr(const std::string &suffix, const std::vector<std::string> &parameters)
{
    const std::string key = "wog_objects.native." + suffix;
    bool found = false;
    EraJS::read(key, found);
    return found ? Era::tr(key.c_str(), parameters) : suffix;
}
std::string ObjectName(const int subtype)
{
    if (const char *name = extender::GetObjectName(ERA_OBJECT_TYPE, subtype))
        if (*name)
            return name;
    const std::string key = H3String::Format("RMG.objectGeneration.%d.%d.name", ERA_OBJECT_TYPE, subtype).String();
    bool found = false;
    EraJS::read(key, found);
    if (found)
        return Era::tr(key.c_str());
    return H3String::Format("WoG object %d", subtype).String();
}
void SetHintText(const std::string &text) noexcept { libc::sprintf(h3_TextBuffer, "%s", text.c_str()); }

int ChoosePictures(const char *text, const H3PictureVector &pictures)
{
    return ChoosePicturesImpl(text, pictures);
}

int Random(const int minValue, const int maxValue) noexcept
{
    return maxValue <= minValue ? minValue : H3Random::MultiplayerRandom(minValue, maxValue);
}

bool ValidCreature(const int id) noexcept { return id >= 0 && id < H3CreatureCount::Get(); }
bool ValidArtifact(const int id) noexcept { return id >= 0 && id < H3ArtifactCount::Get(); }
bool ArtifactEnabled(const int id) noexcept
{
    return ValidArtifact(id) && !H3ArtifactSetup::Get()[id].disabled;
}
int CreatureGoldCost(const int id) noexcept { return ValidCreature(id) ? (std::max)(0, P_CreatureInformation[id].cost.gold) : 0; }
long long ArmyAiValue(const H3Army &army) noexcept
{
    long long result = 0;
    for (int i = 0; i < 7; ++i)
        if (ValidCreature(army.type[i]) && army.count[i] > 0)
            result += static_cast<long long>((std::max)(0, P_CreatureInformation[army.type[i]].aiValue)) * army.count[i];
    return result;
}

bool CanAddArmy(const H3Army &army, const std::vector<std::pair<int, int>> &stacks) noexcept
{
    std::map<int, int> unique;
    for (const auto &s : stacks)
        if (ValidCreature(s.first) && s.second > 0)
            unique[s.first] += s.second;
    int freeSlots = 0;
    for (int i = 0; i < 7; ++i)
        if (army.type[i] < 0 || army.count[i] <= 0)
            ++freeSlots;
    for (const auto &s : unique)
    {
        bool exists = false;
        for (int i = 0; i < 7; ++i)
            if (army.type[i] == s.first && army.count[i] > 0)
                exists = true;
        if (!exists && --freeSlots < 0)
            return false;
    }
    return true;
}
bool AddArmy(H3Army &army, const std::vector<std::pair<int, int>> &stacks) noexcept
{
    if (!CanAddArmy(army, stacks))
        return false;
    for (const auto &s : stacks)
    {
        if (!ValidCreature(s.first) || s.second <= 0)
            continue;
        int slot = -1;
        for (int i = 0; i < 7; ++i)
            if (army.type[i] == s.first && army.count[i] > 0) { slot = i; break; }
        if (slot < 0)
            for (int i = 0; i < 7; ++i)
                if (army.type[i] < 0 || army.count[i] <= 0) { slot = i; break; }
        if (slot < 0)
            return false;
        if (army.type[slot] == s.first)
            army.count[slot] += s.second;
        else
        {
            army.type[slot] = s.first;
            army.count[slot] = s.second;
        }
    }
    return true;
}

int CountBackpackArtifact(const H3Hero *hero, const int artifactId, const int subtype) noexcept
{
    if (!hero)
        return 0;
    int count = 0;
    for (int i = 0; i < hero->backpackCount && i < 64; ++i)
        if (hero->backpackArtifacts[i].id == artifactId && (subtype < 0 || hero->backpackArtifacts[i].subtype == subtype))
            ++count;
    return count;
}
bool RemoveBackpackArtifact(H3Hero *hero, const int artifactId, const int subtype) noexcept
{
    if (!hero)
        return false;
    for (int i = 0; i < hero->backpackCount && i < 64; ++i)
    {
        if (hero->backpackArtifacts[i].id == artifactId && (subtype < 0 || hero->backpackArtifacts[i].subtype == subtype))
        {
            hero->RemoveBackpackArtifact(i);
            return true;
        }
    }
    return false;
}

int ChoosePrimary(const char *text, const int excluded)
{
    H3PictureVector pics;
    int ids[4] = {-1, -1, -1, -1};
    int n = 0;
    for (int i = 0; i < 4; ++i)
        if (i != excluded)
        {
            ids[n++] = i;
            pics.Add(H3PictureCategories::PrimarySkill(static_cast<ePrimary>(i), 1));
        }
    const int index = ChoosePicturesImpl(text, pics);
    return index >= 0 && index < n ? ids[index] : -1;
}
int ChooseArmySlot(const H3Hero *hero, const char *text, bool)
{
    if (!hero)
        return -1;
    H3PictureVector pics;
    int slots[7];
    int n = 0;
    for (int i = 0; i < 7; ++i)
        if (ValidCreature(hero->army.type[i]) && hero->army.count[i] > 0)
        {
            slots[n++] = i;
            pics.Add(H3PictureCategories::Creature(static_cast<eCreature>(hero->army.type[i]), hero->army.count[i]));
        }
    const int index = ChoosePicturesImpl(text, pics);
    return index >= 0 && index < n ? slots[index] : -1;
}
int ChooseTown(const char *text, const int preferred)
{
    (void)preferred;
    // Selectable native message boxes support eight pictures. Page 1 has towns 0..7;
    // page 2 has Conflux plus Neutral. Return -1 for Neutral and -2 for cancel.
    {
        H3PictureVector pics;
        for (int t = 0; t < 8; ++t)
            pics.Add(H3PictureCategories::Creature(static_cast<eCreature>(NativeTownCreature(t, 0, false)), 1));
        const int index = ChoosePicturesImpl(text, pics);
        if (index >= 0)
            return index;
        if (!H3Messagebox::Choice(Tr("common.moreTowns").c_str()))
            return -2;
    }
    H3PictureVector pics;
    pics.Add(H3PictureCategories::Creature(static_cast<eCreature>(NativeTownCreature(8, 0, false)), 1));
    // Faerie Dragon is only a visual marker for the Neutral destination; no fixed creature is implied.
    pics.Add(H3PictureCategories::Creature(eCreature::FAERIE_DRAGON, 1));
    const int index = ChoosePicturesImpl(text, pics);
    if (index == 0) return 8;
    if (index == 1) return -1;
    return -2;
}

int NativeTownCreature(const int town, const int level, const bool upgraded) noexcept
{
    if (town < 0 || town > 8 || level < 0 || level > 6)
        return -1;
    const int id = town * 14 + level * 2 + (upgraded ? 1 : 0);
    return ValidCreature(id) ? id : -1;
}
int FirstCreatureAtOrBelowFightValue(const int town, const int level, const int maxFightValue) noexcept
{
    int best = -1;
    for (int i = 0; i < H3CreatureCount::Get(); ++i)
    {
        const auto &c = P_CreatureInformation[i];
        if (c.town == town && c.level == level && c.fightValue <= maxFightValue)
        {
            if (best < 0 || c.fightValue > P_CreatureInformation[best].fightValue)
                best = i;
        }
    }
    return best;
}
bool IsLegacyEnabled(const H3MapItem *item, const int optionId) noexcept
{
    return !item || item->objectType != WOG_OBJECT_TYPE || WoGObjectsExtender::IsWoGOptionEnabled(optionId);
}

}} // namespace wog::util
