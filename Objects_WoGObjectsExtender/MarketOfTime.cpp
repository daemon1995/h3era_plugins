#include "MarketOfTime.h"
#include "WoGObjectsExtender.h"

namespace wog
{
namespace marketOfTime
{
namespace
{
constexpr int FIRST_PICTURE_ID = 30729;
constexpr int CHOOSE_PICTURE_OR_CANCEL = 10;
constexpr DWORD HERO_VARIABLES_ADDRESS = 0xA4AB10;
constexpr int HERO_VARIABLE_COUNT = 200;
constexpr int WARFARE_VARIABLE = 48;
using SetSelectablePictures = void(__stdcall *)(int);

std::string Text(const char *name, const char *legacyName,
                 const std::vector<std::string> &parameters = {})
{
    const std::string key = std::string("wog_objects.marketOfTime.") + name;
    bool found = false;
    EraJS::read(key, found);
    return Era::tr(found ? key.c_str() : legacyName, parameters);
}

int WarfareSkill(const H3Hero *hero) noexcept
{
    // ERA Erm.pas: TWVars = array [0..255, 1..200] of integer.
    if (hero->id < 0 || hero->id >= 256)
        return eSecondary::ARTILLERY;
    const auto *variables = static_cast<const int *>(
        Era::GetRealAddr(reinterpret_cast<void *>(HERO_VARIABLES_ADDRESS)));
    return variables ? variables[hero->id * HERO_VARIABLE_COUNT + WARFARE_VARIABLE - 1]
                     : eSecondary::ARTILLERY;
}

int ChooseSkill(const H3Hero *hero, const SkillList &skills)
{
    const HMODULE era = GetModuleHandleA("era.dll");
    const auto setSelectablePictures = era ? reinterpret_cast<SetSelectablePictures>(
        GetProcAddress(era, "_SetDialog8SelectablePicsMask")) : nullptr;
    // Earlier native dialogs support two selectable pictures. Keep all skills accessible there too.
    const int pageSize = setSelectablePictures ? PAGE_SIZE : 2;
    const std::string message = Text("choice", "wog.109.choice", {"gold", std::to_string(GOLD_COST),
                                                                 "movement", std::to_string(MOVEMENT_COST)});
    for (int first = 0; first < skills.count; first += pageSize)
    {
        H3PictureVector pictures;
        for (int i = first; i < skills.count && i < first + pageSize; ++i)
        {
            const int skill = skills.ids[i];
            pictures.Add(H3PictureCategories::SecSkill(static_cast<eSecondary>(skill),
                                                      static_cast<eSecSkillLevel>(hero->secSkill[skill])));
        }
        if (setSelectablePictures)
            setSelectablePictures((1 << pictures.Size()) - 1);
        Era::TComplexDialogOpts options{};
        options.msgType = CHOOSE_PICTURE_OR_CANCEL;
        H3Messagebox::Display(message.c_str(), pictures, options.value);
        const int choice = H3WindowManager::Get()->ClickedItemID() - FIRST_PICTURE_ID;
        if (choice >= 0 && choice < static_cast<int>(pictures.Size()))
            return skills.SkillOnPage(first, choice);
        // Like the ERM script, Cancel advances to the next group of skills.
    }
    return -1;
}
} // namespace

BOOL Handles(const H3MapItem *mapItem) noexcept
{
    return mapItem && (mapItem->objectType == WOG_OBJECT_TYPE || mapItem->objectType == ERA_OBJECT_TYPE) &&
           mapItem->objectSubtype == OBJECT_SUBTYPE;
}

BOOL Visit(H3Hero *hero, const BOOL isHuman) noexcept
{
    // The parent checks activation for legacy 63/51; native 141/51 is independent of WoG option 109.
    if (!hero || !isHuman)
        return TRUE;
    if (hero->owner < 0 || hero->owner >= 8)
        return TRUE;

    const std::string gold = std::to_string(GOLD_COST);
    H3Messagebox::Show(Text("title", "wog.109.title", {"gold", gold}).c_str());
    if (hero->movement < MOVEMENT_COST)
    {
        H3Messagebox::Show(Text("noMoves", "wog.109.noMoves").c_str());
        return TRUE;
    }
    const bool warfare = Era::GetAssocVarIntValue("wog_193_enabled") != 0;
    const SkillList skills = CollectSkills(hero->secSkill, warfare, warfare ? WarfareSkill(hero) : 0);
    if (!skills.count)
    {
        H3Messagebox::Show(Text("noSkills", "wog.109.noSkills", {"name", hero->name}).c_str());
        return TRUE;
    }
    const int skill = ChooseSkill(hero, skills);
    if (skill < 0 || !hero->secSkill[skill])
        return TRUE;
    const H3PictureCategories picture = H3PictureCategories::SecSkill(
        static_cast<eSecondary>(skill), static_cast<eSecSkillLevel>(hero->secSkill[skill]));
    if (!H3Messagebox::Choice(Text("forget", "wog.109.forget", {"gold", gold}).c_str(), picture))
        return TRUE;

    auto &resources = P_Main->players[hero->owner].playerResources;
    if (resources.gold < GOLD_COST)
    {
        H3Messagebox::Show(Text("noMoney", "wog.109.noMoney").c_str());
        return TRUE;
    }
    // Use ERA's native receiver to retain HE:S hooks (including Warfare's three linked skills).
    // No function from script 69 is invoked. UnlearnSkill alone would bypass those hooks.
    Era::ExecErmCmd(H3String::Format("HE%d:S%d/0;", hero->id, skill).String());
    if (hero->secSkill[skill] != 0)
        return TRUE;
    hero->AddResource(eResource::GOLD, -GOLD_COST);
    hero->movement -= MOVEMENT_COST;
    hero->RecalculateMovement();
    return TRUE;
}

BOOL SetHint(const BOOL isRightClick) noexcept
{
    const std::string message = isRightClick
        ? Text("hint", "wog.109.title", {"gold", std::to_string(GOLD_COST),
                                         "movement", std::to_string(MOVEMENT_COST)})
        : Text("name", "wog.109.objName");
    libc::sprintf(h3_TextBuffer, "%s", message.c_str());
    return TRUE;
}
} // namespace marketOfTime
} // namespace wog
