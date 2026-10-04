#define NOMINMAX
#include <windows.h>
#include "OptionMenuModel.h"

#include <algorithm>
#include <cwctype>
#include <sstream>
#include <tuple>

namespace era_options
{
const std::string &OptionCategory(const OptionDefinition &definition)
{
    if (!definition.category.empty()) return definition.category;
    return definition.section;
}

std::wstring NormalizeOptionSearch(const std::string &text, unsigned codePage)
{
    if (text.empty()) return {};
    if (!codePage) codePage = GetACP();
    int length = MultiByteToWideChar(codePage, 0, text.c_str(), -1, nullptr, 0);
    if (!length && codePage != GetACP())
    {
        codePage = GetACP();
        length = MultiByteToWideChar(codePage, 0, text.c_str(), -1, nullptr, 0);
    }
    if (length <= 1) return {};
    std::vector<wchar_t> buffer(length);
    MultiByteToWideChar(codePage, 0, text.c_str(), -1, buffer.data(), length);
    CharLowerBuffW(buffer.data(), length - 1);
    std::wstring result;
    bool space = false;
    for (int index = 0; index < length - 1; ++index)
        if (iswspace(buffer[index])) space = !result.empty();
        else
        {
            if (space) result.push_back(L' ');
            result.push_back(buffer[index]);
            space = false;
        }
    return result;
}

bool MatchesOptionSearch(const EOption &option, const std::wstring &query, unsigned codePage)
{
    if (query.empty()) return true;
    std::wstring number = query;
    if (number.front() == L'#') number.erase(0, 1);
    if (number.compare(0, 7, L"option ") == 0) number.erase(0, 7);
    if (!number.empty() && number.find_first_not_of(L"0123456789") == std::wstring::npos)
    {
        int id = -1;
        return ParseInteger(std::string(number.begin(), number.end()), id) && option.GetId() == id;
    }
    const auto &definition = option.Definition();
    std::string text = definition.name + " " + definition.key + " " + definition.hint + " " +
        definition.popup + " " + definition.modName + " " + definition.modFolder + " " +
        OptionCategory(definition) + " " + definition.page + " " + definition.section + " " +
        std::to_string(option.GetId());
    for (const auto &tag : definition.tags) text += " " + tag;
    for (const auto &choice : definition.choices) text += " " + choice;
    const auto normalized = NormalizeOptionSearch(text, codePage);
    std::wistringstream words(query);
    std::wstring word;
    while (words >> word)
        if (normalized.find(word) == std::wstring::npos) return false;
    return true;
}

int LastGridStart(const std::vector<int> &rowHeights, int viewportHeight)
{
    int first = static_cast<int>(rowHeights.size());
    int used = 0;
    while (first > 0 && rowHeights[first - 1] <= viewportHeight - used)
        used += rowHeights[--first];
    return std::max(0, first == static_cast<int>(rowHeights.size()) ? first - 1 : first);
}

std::vector<OptionGridRow> BuildOptionGrid(const std::vector<MenuCategory> &categories,
    const std::function<int(const OptionCell &)> &measureHeight, int viewportHeight)
{
    struct Block { std::vector<OptionCell> cells; int category; long long height = 0; };
    std::vector<Block> blocks;
    std::vector<OptionCell> titles;
    const auto height = [&](const OptionCell &cell) {
        return std::max(1, std::min(measureHeight(cell), std::max(1, viewportHeight)));
    };
    for (int categoryIndex = 0; categoryIndex < static_cast<int>(categories.size()); ++categoryIndex)
    {
        const auto &category = categories[categoryIndex];
        // A sole choice with the category's name uses the category title once.
        EOption *soleChoice = category.options.size() == 1 &&
            category.options.front()->Definition().type == OptionType::Choice &&
            OptionCategory(category.options.front()->Definition()) == category.options.front()->Definition().name ?
            category.options.front() : nullptr;
        const OptionCell title = {soleChoice, OptionCellKind::Category, -1, &category, categoryIndex};
        titles.push_back(title);
        Block block = {{title}, categoryIndex};
        if (!category.collapsed)
            for (auto *option : category.options)
        {
            if (option->Definition().type == OptionType::Choice)
            {
                if (option != soleChoice) block.cells.push_back({option, OptionCellKind::Heading});
                for (int choice = 0; choice < static_cast<int>(option->Definition().choices.size()); ++choice)
                    block.cells.push_back({option, OptionCellKind::Radio, choice});
            }
            else block.cells.push_back({option, OptionCellKind::Checkbox});
            // Attach the title to the first option, avoiding an orphan header.
            blocks.push_back(std::move(block));
            block = {{}, categoryIndex};
        }
        if (!block.cells.empty()) blocks.push_back(std::move(block));
    }
    if (blocks.empty()) return {};
    std::vector<long long> prefix(1, 0);
    for (auto &block : blocks)
    {
        for (const auto &cell : block.cells) block.height += height(cell);
        prefix.push_back(prefix.back() + block.height);
    }
    const auto wholeCategory = [&](size_t cut) {
        return cut == blocks.size() || blocks[cut - 1].category != blocks[cut].category;
    };
    const auto rightHeight = [&](size_t cut) {
        return prefix.back() - prefix[cut] + (wholeCategory(cut) ? 0 : height(titles[blocks[cut].category]));
    };
    size_t split = blocks.size();
    if (prefix.back() > viewportHeight)
    {
        bool fits = false;
        // Prefer complete categories; the last fitting split fills left first.
        for (int pass = 0; pass < 2 && !fits; ++pass)
            for (size_t cut = 1; cut < blocks.size(); ++cut)
                if ((pass || wholeCategory(cut)) && prefix[cut] <= viewportHeight && rightHeight(cut) <= viewportHeight)
                {
                    split = cut;
                    fits = true;
                }
        if (!fits)
        {
            // Overflow only after considering both columns. Balance scrollable
            // content, preferring whole categories when heights tie.
            long long best = prefix.back();
            for (size_t cut = 1; cut < blocks.size(); ++cut)
            {
                const auto largest = std::max(prefix[cut], rightHeight(cut));
                if (largest < best || (largest == best && (wholeCategory(cut) || !wholeCategory(split))))
                {
                    split = cut;
                    best = largest;
                }
            }
        }
    }
    std::vector<OptionGridRow> rows;
    // Independent starts avoid padding every right-hand row because one label
    // on the left wraps. Total height equals the taller column.
    std::map<int, OptionGridRow> starts;
    int bottom = 0;
    for (int column = 0; column < 2; ++column)
    {
        int y = 0;
        const auto append = [&](const OptionCell &cell) {
            const int cellHeight = height(cell);
            auto &row = starts[y];
            if (column) { row.right = cell; row.rightHeight = cellHeight; }
            else { row.left = cell; row.leftHeight = cellHeight; }
            y += cellHeight;
        };
        if (column && !wholeCategory(split)) append(titles[blocks[split].category]);
        for (size_t index = column ? split : 0; index < (column ? blocks.size() : split); ++index)
            for (const auto &cell : blocks[index].cells) append(cell);
        bottom = std::max(bottom, y);
    }
    for (auto current = starts.begin(); current != starts.end(); ++current)
    {
        const auto next = std::next(current);
        current->second.height = (next == starts.end() ? bottom : next->first) - current->first;
        rows.push_back(current->second);
    }
    return rows;
}

MenuDialogSize OptionsDialogSize(int screenWidth, int screenHeight, bool fullScreen)
{
    const int border = fullScreen ? 6 : 0;
    return {std::max(1, std::min(screenWidth - border, fullScreen ? screenWidth - border : 800)),
            std::max(1, std::min(screenHeight - border, fullScreen ? screenHeight - border : 600))};
}

bool DialogShadowFits(int x, int y, int width, int height, int screenWidth, int screenHeight)
{
    // Native dialog shadow extends eight pixels to the right and bottom.
    return x >= 0 && y >= 0 && width > 0 && height > 0 &&
        static_cast<int64_t>(x) + width + 8 <= screenWidth &&
        static_cast<int64_t>(y) + height + 8 <= screenHeight;
}

namespace
{
void SortCategories(std::vector<MenuCategory> &categories)
{
    std::stable_sort(categories.begin(), categories.end(), [](const MenuCategory &left, const MenuCategory &right) {
        return left.order < right.order;
    });
}
}

const std::string &OptionCellName(const OptionCell &cell)
{
    if (cell.kind == OptionCellKind::Category) return cell.category->name;
    return cell.kind == OptionCellKind::Radio ? cell.option->Definition().choices[cell.choice] :
        cell.option->Definition().name;
}

ScrollArea ScrollAreaAt(int mouseX, int mouseY, int sidebarWidth, int sidebarTop, int sidebarHeight,
                        int optionX, int optionTop, int optionWidth, int optionHeight)
{
    if (mouseX >= 18 && mouseX < sidebarWidth + 18 && mouseY >= sidebarTop && mouseY < sidebarTop + sidebarHeight)
        return ScrollArea::Mods;
    if (mouseX >= optionX && mouseX < optionX + optionWidth && mouseY >= optionTop && mouseY < optionTop + optionHeight)
        return ScrollArea::Options;
    return ScrollArea::None;
}

OptionMenuModel::OptionMenuModel(const OptionRegistry &registry, unsigned codePage) : codePage(codePage)
{
    for (const auto &option : registry.Options())
        if (option->Definition().visible) allOptions.push_back(option.get());
    std::sort(allOptions.begin(), allOptions.end(), [](const EOption *left, const EOption *right) {
        return left->GetId() < right->GetId();
    });
    // Mods and pages follow the smallest ID; category_order can override categories.
    for (auto *option : allOptions)
    {
        const auto &definition = option->Definition();
        auto mod = std::find_if(mods.begin(), mods.end(), [&definition](const MenuMod &entry) {
            return entry.folder == definition.modFolder;
        });
        if (mod == mods.end())
        {
            mods.push_back({definition.modFolder, definition.modName});
            mod = mods.end() - 1;
        }
        auto page = std::find_if(mod->pages.begin(), mod->pages.end(), [&definition](const MenuPage &entry) {
            return entry.name == definition.page;
        });
        if (page == mod->pages.end())
        {
            mod->pages.push_back({definition.page});
            page = mod->pages.end() - 1;
        }
        page->options.push_back(option);
        const auto &name = OptionCategory(definition);
        auto category = std::find_if(page->categories.begin(), page->categories.end(), [&name](const MenuCategory &entry) {
            return entry.name == name;
        });
        if (category == page->categories.end())
        {
            page->categories.push_back({name});
            category = page->categories.end() - 1;
            category->order = definition.categoryOrder;
        }
        else category->order = std::min(category->order, definition.categoryOrder);
        category->options.push_back(option);
    }
    for (auto &mod : mods)
        for (auto &page : mod.pages) SortCategories(page.categories);
    if (!mods.empty())
    {
        selectedMod = selectedPage = 0;
        mods.front().expanded = true;
    }
}

const std::vector<EOption *> &OptionMenuModel::Options() const
{
    if (IsSearching()) return searchMatches;
    if (selectedMod < 0 || selectedPage < 0) return empty;
    return mods[selectedMod].pages[selectedPage].options;
}

const std::vector<MenuCategory> &OptionMenuModel::Categories() const
{
    if (IsSearching()) return searchCategories;
    if (selectedMod < 0 || selectedPage < 0) return emptyCategories;
    return mods[selectedMod].pages[selectedPage].categories;
}

void OptionMenuModel::ClickMod(int mod)
{
    if (mod < 0 || mod >= static_cast<int>(mods.size())) return;
    mods[mod].expanded = !mods[mod].expanded;
    SelectPage(mod, 0);
}

void OptionMenuModel::SelectPage(int mod, int page)
{
    if (mod < 0 || mod >= static_cast<int>(mods.size()) || page < 0 ||
        page >= static_cast<int>(mods[mod].pages.size())) return;
    selectedMod = mod;
    selectedPage = page;
    SetQuery("");
}

void OptionMenuModel::ToggleCategory(int category)
{
    if (category < 0 || category >= static_cast<int>(Categories().size())) return;
    auto &target = IsSearching() ? searchCategories[category] : mods[selectedMod].pages[selectedPage].categories[category];
    target.collapsed = !target.collapsed;
}

void OptionMenuModel::SetQuery(const std::string &text)
{
    query = text;
    const auto normalized = NormalizeOptionSearch(text, codePage);
    if (normalized == normalizedQuery) return;
    normalizedQuery = normalized;
    searchPosition = 0;
    searchMatches.clear();
    searchCategories.clear();
    std::map<std::tuple<std::string, std::string, std::string>, size_t> groups;
    if (IsSearching())
        for (auto *option : allOptions)
            if (MatchesOptionSearch(*option, normalizedQuery, codePage))
            {
                searchMatches.push_back(option);
                const auto &definition = option->Definition();
                const auto identity = std::make_tuple(definition.modFolder, definition.page, OptionCategory(definition));
                auto group = groups.find(identity);
                if (group == groups.end())
                {
                    std::string name = definition.modName;
                    for (const auto &part : {definition.page, OptionCategory(definition)})
                        if (!part.empty()) name += " / " + part;
                    group = groups.emplace(identity, searchCategories.size()).first;
                    searchCategories.push_back({name});
                    searchCategories.back().order = definition.categoryOrder;
                }
                else searchCategories[group->second].order = std::min(searchCategories[group->second].order, definition.categoryOrder);
                searchCategories[group->second].options.push_back(option);
            }
    SortCategories(searchCategories);
}

int OptionMenuModel::ScrollPosition() const
{
    if (IsSearching()) return searchPosition;
    if (selectedMod < 0 || selectedPage < 0) return 0;
    const auto found = pagePositions.find({mods[selectedMod].folder, mods[selectedMod].pages[selectedPage].name});
    return found == pagePositions.end() ? 0 : found->second;
}

void OptionMenuModel::SetScrollPosition(int position)
{
    position = std::max(0, position);
    if (IsSearching()) searchPosition = position;
    else if (selectedMod >= 0 && selectedPage >= 0)
        pagePositions[{mods[selectedMod].folder, mods[selectedMod].pages[selectedPage].name}] = position;
}
} // namespace era_options
