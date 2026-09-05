#include "ModInformation.h"

#include <cstdlib>
#include <cstring>

namespace
{
hkcategories::eType ParseHotkeyType(LPCSTR type)
{
    if (!type || !*type)
    {
        return hkcategories::OTHER_DLG;
    }

    char *end = nullptr;
    const long numericType = std::strtol(type, &end, 10);
    if (end != type && *end == '\0' && numericType >= hkcategories::ANY_DLG &&
        numericType <= hkcategories::OTHER_DLG)
    {
        return static_cast<hkcategories::eType>(numericType);
    }

    if (!_stricmp(type, "ALL") || !_stricmp(type, "ANY"))
        return hkcategories::ANY_DLG;
    if (!_stricmp(type, "NONE") || !_stricmp(type, "GLOBAL") || !_stricmp(type, "EVERYWHERE"))
        return hkcategories::NONE;
    if (!_stricmp(type, "ADV_MAP") || !_stricmp(type, "ADV_MAP_DLG") || !_stricmp(type, "ADVENTURE") ||
        !_stricmp(type, "MAP"))
        return hkcategories::ADV_MAP_DLG;
    if (!_stricmp(type, "HERO") || !_stricmp(type, "HERO_DLG"))
        return hkcategories::HERO_DLG;
    if (!_stricmp(type, "TOWN") || !_stricmp(type, "TOWN_DLG") || !_stricmp(type, "CITY"))
        return hkcategories::TOWN_DLG;
    if (!_stricmp(type, "COMBAT") || !_stricmp(type, "COMBAT_DLG"))
        return hkcategories::COMBAT_DLG;
    return hkcategories::OTHER_DLG;
}
}

LastActiveDlgModInfo ModInformation::lastActiveModInfo;

ModInformation::ModInformation(LPCSTR modFolderName, const UINT id)
    : hasSomeInfo(false), id(id), name(modFolderName), document(modFolderName), activeCategory(nullptr)
{

    categories.clear();
    // start parsing panelCategories
    // first parse hotkeys

    if (hotkeysCategory = CreateHotkeysCategory())
    {
        categories.emplace_back(hotkeysCategory);
    }

    // create native catigories while json is parsed
    int startIndex = 0;
    while (auto category = CreateNativeCategory(startIndex++))
    {
        categories.emplace_back(category);
    }

    if (!categories.empty())
    {
        hasSomeInfo = true;
    }
}

ModInformation::~ModInformation()
{
    for (auto &cat : categories)
    {
        if (cat)
        {
            cat->~Category();
        }
        // delete cat->content;
        //
    }
}

HotKeysCategory *ModInformation::CreateHotkeysCategory() const noexcept
{
    HotKeysCategory *result = nullptr;
    std::vector<HotKey> hotkeys;

    // Preferred format: help.<mod_folder_name>.hotkeys[]. The adapter also
    // probes help.mods.<mod_folder_name>.hotkeys[] for old files.
    const H3String hotkeysBase = document.ArrayRoot("hotkeys");
    const bool hasArrayFormat = !hotkeysBase.Empty();

    if (hasArrayFormat)
    {
        for (int hotkeyId = 0;; ++hotkeyId)
        {
            bool readSuccess = false;
            H3String itemBase(hotkeysBase);
            itemBase.Append('.');
            itemBase.Append(hotkeyId);
            H3String keyPath(itemBase);
            keyPath.Append(".keys");
            H3String keys = document.Read(keyPath.String(), readSuccess);
            if (!readSuccess || keys.Empty())
            {
                break;
            }

            H3String namePath(itemBase);
            namePath.Append(".name");
            H3String descriptionPath(itemBase);
            descriptionPath.Append(".description");
            H3String typePath(itemBase);
            typePath.Append(".type");
            H3String name = document.Read(namePath.String());
            H3String description = document.Read(descriptionPath.String());
            H3String type = document.Read(typePath.String());
            const hkcategories::eType parsedType = type.Empty() ? hkcategories::OTHER_DLG : ParseHotkeyType(type.String());
            hotkeys.emplace_back(HotKey{parsedType, keys, name, description});
        }
    }
    else
    {
        // Legacy format: help.<mod>.categories.hotkeys.content[] (or its
        // help.mods.<mod> equivalent).
        bool readSuccess = false;
        H3String hkName = document.Read("categories.hotkeys.name", readSuccess);
        if (readSuccess)
        {
            for (int hotkeyId = 0;; ++hotkeyId)
            {
                H3String itemBase("categories.hotkeys.content.");
                itemBase.Append(hotkeyId);
                H3String keyPath(itemBase);
                keyPath.Append(".key");
                H3String keys = document.Read(keyPath.String(), readSuccess);
                if (!readSuccess || keys.Empty())
                    break;
                H3String typePath(itemBase);
                typePath.Append(".type");
                H3String descriptionPath(itemBase);
                descriptionPath.Append(".description");
                const int type = document.ReadInt(typePath.String());
                H3String description = document.Read(descriptionPath.String());
                hotkeys.emplace_back(HotKey{static_cast<hkcategories::eType>(type), keys, h3_NullString, description});
            }
        }
    }

    if (!hotkeys.empty())
    {
        result = new HotKeysCategory();
        result->hotkeys = hotkeys;
        if (hasArrayFormat)
            result->name = document.Read("hotkeys.name");
        else
            result->name = document.Read("categories.hotkeys.name");
        result->content = new Content();
    }

    return result;
}

Category *ModInformation::CreateNativeCategory(const int index) const noexcept
{
    Category *result = nullptr;
    bool readSucces = false;

    H3String categoryBase("categories.");
    categoryBase.Append(index);
    H3String categoryNamePath(categoryBase);
    categoryNamePath.Append(".name");
    H3String catName = document.Read(categoryNamePath.String(), readSucces);
    if (readSucces)
    {

        if (result = new Category())
        {
            result->name = catName;

            result->content = new Content();
            H3String categoryContentPath(categoryBase);
            categoryContentPath.Append(".content");
            result->content->text = document.Read(categoryContentPath.String());

            // H3String defName =
        }
    }

    return result;
}

const Category &ModInformation::ActiveCategory() const noexcept
{
    return *activeCategory;
}

size_t ModInformation::Size() const noexcept
{
    return categories.size();
}

void ModInformation::SetVisible(const BOOL state)
{
}

void ModInformation::StoreModInfoAsActive() const noexcept
{
    // lastActiveModInfo.scrollBarPos  = this->activeCategory
    lastActiveModInfo.categoryId = m_lastActiveCategoryId;
}
Category::~Category()
{
    if (content)
    {
        delete content;
        content = nullptr;
    }
}
void Category::ShowContent() const noexcept
{
}
