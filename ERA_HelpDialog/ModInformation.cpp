#include "framework.h"

ModInformation::ModInformation(LPCSTR modFolderName, const UINT id)
    : name(modFolderName), hasSomeInfo(false), id(id), activeCategory(nullptr)
{

    json = jsonBase;
    json.Append(modFolderName);

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
    // H3String jsonKey = H3String::Format("%s.panelCategories.hotkeys",
    // json.String());

    bool readSucces = false;
    std::vector<HotKey> hotkeys;
    LPCSTR hkName = EraJS::read(H3String::Format("%s.categories.hotkeys.name", json.String()).String(), readSucces);
    if (readSucces)
    {
        int hotkeyId = 0;
        while (true)
        {
            H3String key = EraJS::read(
                H3String::Format("%s.categories.hotkeys.content.%d.key", json.String(), hotkeyId).String(), readSucces);
            if (!readSucces || key.Empty())
            {
                break;
            }

            const int type = EraJS::readInt(
                H3String::Format("%s.categories.hotkeys.content.%d.type", json.String(), hotkeyId).String());
            H3String description = EraJS::read(
                H3String::Format("%s.categories.hotkeys.content.%d.description", json.String(), hotkeyId).String());

            hotkeys.emplace_back(HotKey{hkcategories::eType(type), key, description});
            hotkeyId++;
        }
    }
    // if added at least one hotkey
    if (hotkeys.size())
    {

        result = new HotKeysCategory();
        result->hotkeys = hotkeys;
        result->name = hkName;
        result->content = new Content();
    }

    return result;
}

Category *ModInformation::CreateNativeCategory(const int index) const noexcept
{
    Category *result = nullptr;
    bool readSucces = false;

    LPCSTR catName = EraJS::read(H3String::Format("%s.categories.%d.name", json.String(), index).String(), readSucces);
    if (readSucces)
    {

        if (result = new Category())
        {
            result->name = catName;

            result->content = new Content();
            result->content->text =
                EraJS::read(H3String::Format("%s.categories.%d.content", json.String(), index).String());

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