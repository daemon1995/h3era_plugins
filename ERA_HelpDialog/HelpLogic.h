#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <functional>
#include <string>
#include <vector>

// Game-independent logic shared by the UI. No game addresses.
namespace helpdlg
{
constexpr int kCatalogueIdSort = 1;
constexpr int kCatalogueSortSchema = 2;

inline int InitialCatalogueSort(int schema, int savedSort) noexcept
{
    return schema < kCatalogueSortSchema ? kCatalogueIdSort : savedSort;
}

inline bool HasCreatureData(const char *name, const char *combatDef) noexcept
{
    return name && *name && combatDef && *combatDef;
}

inline bool HasArtifactText(const char *name, const char *description) noexcept
{
    return name && *name && description && *description;
}

inline bool IsArtifactPlaceholder(int id) noexcept
{
    return id == 144 || id == 145;
}

// Raw frames use palette index zero for transparency. Compressed frames use
// the game's decoder; two backgrounds preserve black and white icons. The
// caller reuses one off-screen canvas for the whole DEF.
template <class Def, class Canvas> bool DefFrameHasVisiblePixels(Def *def, int id, Canvas *canvas)
{
    if (!def || !def->groups || def->groupsCount <= 0 || !def->groups[0] || !def->palette565 || !canvas ||
        !canvas->buffer || def->widthDEF <= 0 || def->heightDEF <= 0 || canvas->width < def->widthDEF ||
        canvas->height < def->heightDEF)
        return false;
    const auto *group = def->groups[0];
    if (!group->frames || id < 0 || id >= group->count)
        return false;
    const auto *frame = group->frames[id];
    if (!frame || !frame->rawData || frame->rawDataSize <= 0 || frame->frameWidth <= 0 || frame->frameHeight <= 0)
        return false;
    if (frame->compressionType == 0)
    {
        // The generic native raw blitter paints index zero too. Inspect the
        // indices directly so an entirely transparent raw frame stays empty.
        const int stride = frame->width2;
        if (stride < frame->frameWidth || frame->frameWidth > frame->rawDataSize ||
            frame->frameHeight - 1 > (frame->rawDataSize - frame->frameWidth) / stride)
            return false;
        for (int y = 0; y < frame->frameHeight; ++y)
            for (int x = 0; x < frame->frameWidth; ++x)
                if (frame->rawData[y * stride + x])
                    return true;
        return false;
    }
    const unsigned char backgrounds[] = {0, 255};
    for (auto color : backgrounds)
    {
        canvas->FillRectangle(0, 0, def->widthDEF, def->heightDEF, color, color, color);
        const auto background = canvas->GetPixel(0, 0);
        def->DrawToPcx16(0, id, canvas, 0, 0);
        for (int y = 0; y < def->heightDEF; ++y)
            for (int x = 0; x < def->widthDEF; ++x)
            {
                const auto pixel = canvas->GetPixel(x, y);
                if (pixel.r != background.r || pixel.g != background.g || pixel.b != background.b)
                    return true;
            }
    }
    return false;
}

// Combination indices and artifact IDs are separate. Resolve through the
// current setup table so modded combinations need no fixed ID offsets.
template <typename Setup> inline int FindCombinationArtifactId(const Setup *setups, int count, int combination)
{
    if (!setups || combination < 0)
        return -1;
    for (int id = 0; id < count; ++id)
        if (static_cast<int>(setups[id].comboArtifactId) == combination)
            return id;
    return -1;
}

inline bool CanRestoreTextScroll(bool visible, bool activeDialog, bool hasCanvas)
{
    return visible && activeDialog && hasCanvas;
}
template <typename Flag> inline bool RuntimeBanFlag(const Flag *flags, int count, int id)
{
    return flags && id >= 0 && id < count && flags[id] != 0;
}
inline bool RuntimeHeroBanned(int owner, unsigned hireMask, bool offeredInTavern)
{
    if (offeredInTavern || (owner >= 0 && owner < 8))
        return false;
    // 0x40 is shared by banned heroes and current tavern offers. A mask with
    // no player bits also excludes a free hero from every player's pool.
    return owner == 0x40 || !(hireMask & 0xFF);
}

inline int ParseInt(const std::string &text, int fallback = 0)
{
    if (text.empty())
        return fallback;
    errno = 0;
    char *end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);
    return end != text.c_str() && !*end && errno != ERANGE && value >= INT_MIN && value <= INT_MAX
               ? static_cast<int>(value)
               : fallback;
}

struct HotkeyFilter
{
    int context = 0;
    int firstRow = 0;
};

inline int Bound(int value, int low, int high)
{
    return std::max(low, std::min(value, std::max(low, high)));
}

inline int CycleOption(int current, int count, bool backwards)
{
    if (count <= 1)
        return 0;
    return (Bound(current, 0, count - 1) + (backwards ? count - 1 : 1)) % count;
}

inline int HotkeyContext(int context)
{
    return context == -1 || (context >= 1 && context <= 6) ? context : 6;
}

// Every draw rectangle must fit both its source tile and destination.
inline bool ForEachBackgroundTile(int width, int height, int tileWidth, int tileHeight,
                                  const std::function<void(int, int, int, int)> &draw)
{
    if (width <= 0 || height <= 0 || tileWidth <= 0 || tileHeight <= 0)
        return false;
    for (int y = 0; y < height;)
    {
        const int h = std::min(tileHeight, height - y);
        for (int x = 0; x < width;)
        {
            const int w = std::min(tileWidth, width - x);
            draw(x, y, w, h);
            x += w;
        }
        y += h;
    }
    return true;
}

inline bool IsCreatureSpell(int id, bool nativeFlag)
{
    return nativeFlag || (id >= 70 && id < 81);
}

// Native school flags / indices: Air=1, Fire=2, Water=4, Earth=8.
inline int FirstSpellSchool(int mask)
{
    for (int index = 0; index < 4; ++index)
        if (mask & (1 << index))
            return index;
    return -1;
}
inline const char *SpellSchoolDef(int index)
{
    static const char *names[] = {"SpLevA.def", "SpLevF.def", "SpLevW.def", "SpLevE.def"};
    return index >= 0 && index < 4 ? names[index] : nullptr;
}

struct CardIcon
{
    const char *def = "";
    std::string label;
    int frame = 0;
    int artifactId = -1;
};
struct CardRow
{
    const char *label = "";
    std::string value;
    bool detailsOnly = false;
    std::vector<CardIcon> icons;
    bool componentGrid = false;
    bool artifactCard = false;
};
inline std::vector<const CardRow *> VisibleCardRows(const std::vector<CardRow> &rows, bool popup)
{
    std::vector<const CardRow *> visible;
    for (const auto &row : rows)
        if ((!row.icons.empty() ||
             std::any_of(row.value.begin(), row.value.end(), [](unsigned char c) { return !std::isspace(c); })) &&
            (!popup || !row.detailsOnly))
            visible.push_back(&row);
    return visible;
}

struct MasteryRow
{
    const char *level = "";
    std::string mana;
    const char *effect = "";
    std::string strength;
};
inline int SecondarySkillFrame(int skill, int level)
{
    return skill >= 0 && skill < 28 && level >= 1 && level <= 3 ? 2 + skill * 3 + level : -1;
}
inline std::string ArmyAmount(int low, int high)
{
    return low == high ? std::to_string(low) : std::to_string(low) + "-" + std::to_string(high);
}
// Share the available height between rows, preserving short rows and shrinking
// only the extra space of long rows. Their cells can scroll when necessary.
inline std::vector<int> FitCardRows(const std::vector<int> &natural, const std::vector<int> &minimums, int available)
{
    std::vector<int> heights;
    if (natural.empty())
        return heights;
    available = std::max(0, available);
    int baseTotal = 0;
    long long extraTotal = 0;
    for (size_t index = 0; index < natural.size(); ++index)
    {
        const int requested = natural[index];
        const int minimum = index < minimums.size() ? std::max(0, minimums[index]) : 0;
        const int base = std::min(std::max(0, requested), minimum);
        heights.push_back(base);
        baseTotal += base;
        extraTotal += std::max(0, requested - base);
    }
    if (baseTotal > available)
    {
        int remaining = available, weight = baseTotal;
        for (auto &height : heights)
        {
            const int assigned = weight ? static_cast<int>(static_cast<long long>(remaining) * height / weight) : 0;
            weight -= height;
            remaining -= assigned;
            height = assigned;
        }
        return heights;
    }
    long long remaining = available - baseTotal;
    for (size_t index = 0; index < natural.size(); ++index)
    {
        const int extra = std::max(0, natural[index] - heights[index]);
        const int assigned = extraTotal <= remaining ? extra
                             : extraTotal            ? static_cast<int>(remaining * extra / extraTotal)
                                                     : 0;
        heights[index] += assigned;
        remaining -= assigned;
        extraTotal -= extra;
    }
    return heights;
}
inline std::vector<int> FitMasteryRows(const std::vector<int> &natural, int available, int minimum)
{
    const int floor =
        natural.empty() ? 0 : std::min(std::max(0, minimum), std::max(0, available) / static_cast<int>(natural.size()));
    return FitCardRows(natural, std::vector<int>(natural.size(), floor), available);
}

struct CatalogueGrid
{
    int cellWidth, cellHeight, columns, rows;
};
constexpr int kCatalogueCaptionHeight = 32;

constexpr int kPageButtonHeight = 25;
constexpr int kPageButtonGap = 2;
constexpr int kModDropdownRowHeight = kPageButtonHeight + kPageButtonGap;
struct ModDropdownLayout
{
    int x, y, width, height, rows;
};

inline ModDropdownLayout FitModDropdown(int x, int y, int buttonWidth, int buttonHeight, int count, int screenWidth,
                                        int screenHeight)
{
    ModDropdownLayout layout;
    layout.width = Bound(buttonWidth, 1, std::max(1, screenWidth));
    const int availableRows = std::max(1, (screenHeight + kPageButtonGap) / kModDropdownRowHeight);
    layout.rows = Bound(count, 1, std::min(10, availableRows));
    layout.height = layout.rows * kModDropdownRowHeight - kPageButtonGap;
    layout.x = Bound(x, 0, screenWidth - layout.width);
    const int below = y + buttonHeight + kPageButtonGap;
    const int preferredY = below + layout.height <= screenHeight ? below : y - layout.height - kPageButtonGap;
    layout.y = Bound(preferredY, 0, screenHeight - layout.height);
    return layout;
}

inline CatalogueGrid FitCatalogueGrid(int width, int height, int portraitWidth, int portraitHeight, bool compact,
                                      int itemCount = 0)
{
    CatalogueGrid grid;
    const int availableWidth = std::max(1, width - 34);
    const int nominalCellWidth = std::max(compact ? 74 : 96, portraitWidth + 12);
    grid.cellWidth = nominalCellWidth;
    // Keep captions compact enough for four catalogue rows at 800x600.
    grid.cellHeight = std::max(compact ? 72 : 88, portraitHeight + kCatalogueCaptionHeight + 6);
    grid.columns = std::max(1, availableWidth / nominalCellWidth);
    if (compact && itemCount > 0)
    {
        // Secondary skills currently have 28 entries. Prefer enough columns
        // to fit them in four rows whenever the available width allows it.
        const int targetColumns = (itemCount + 3) / 4;
        const int tightCellWidth = std::max(56, portraitWidth + 8);
        const int tightCapacity = std::max(1, availableWidth / tightCellWidth);
        const int compactColumns = std::min(targetColumns, tightCapacity);
        if (compactColumns > grid.columns)
        {
            grid.columns = compactColumns;
            grid.cellWidth = std::max(tightCellWidth, availableWidth / grid.columns);
        }
    }
    const int heightCapacity = std::min(std::max(1, height / grid.cellHeight), std::max(1, 1800 / grid.columns));
    const int rowsNeeded = itemCount > 0 ? (itemCount + grid.columns - 1) / grid.columns : heightCapacity;
    grid.rows = std::max(1, std::min(heightCapacity, rowsNeeded));
    return grid;
}

inline std::string Lower(std::string text)
{
    for (auto &character : text)
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    return text;
}

inline int CompareNames(const char *left, const char *right) noexcept
{
    while (*left || *right)
    {
        const int a = std::tolower(static_cast<unsigned char>(*left));
        const int b = std::tolower(static_cast<unsigned char>(*right));
        if (a != b)
            return a < b ? -1 : 1;
        ++left;
        ++right;
    }
    return 0;
}

inline std::string NormalizePluginPath(std::string path)
{
    path = Lower(path);
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}
inline bool IsPluginFile(const std::string &name)
{
    const auto dot = name.find_last_of('.');
    if (dot == std::string::npos)
        return false;
    const auto extension = Lower(name.substr(dot));
    return extension == ".era" || extension == ".dll" || extension == ".bin";
}
struct PluginNote
{
    std::string file, name, description;
};
inline const PluginNote *FindPluginNote(const std::vector<PluginNote> &notes, const std::string &relativePath)
{
    const auto path = NormalizePluginPath(relativePath);
    const auto slash = path.find_last_of('/');
    const auto filename = path.substr(slash == std::string::npos ? 0 : slash + 1);
    const PluginNote *fallback = nullptr;
    for (const auto &note : notes)
    {
        const auto key = NormalizePluginPath(note.file);
        if (key == path)
            return &note;
        if (!fallback && key == filename)
            fallback = &note;
    }
    return fallback;
}
inline std::string GroupPluginFiles(const std::vector<std::string> &files, const std::vector<PluginNote> &notes)
{
    std::string output;
    for (const char *folder : {"EraPlugins", "EraPlugins/AfterWog", "EraPlugins/BeforeWog"})
    {
        std::vector<std::string> group;
        for (const auto &file : files)
        {
            const auto normalized = NormalizePluginPath(file);
            const auto slash = normalized.find_last_of('/');
            if (slash != std::string::npos && normalized.substr(0, slash) == NormalizePluginPath(folder))
                group.push_back(file);
        }
        if (group.empty())
            continue;
        std::stable_sort(group.begin(), group.end(), [](const std::string &a, const std::string &b) {
            return NormalizePluginPath(a) < NormalizePluginPath(b);
        });
        output += (output.empty() ? h3_NullString : "\n\n") + std::string("{") + folder + ":}";
        for (const auto &file : group)
        {
            output += "\n- " + file.substr(file.find_last_of("/\\") + 1);
            if (const auto *note = FindPluginNote(notes, file))
            {
                if (!note->name.empty())
                    output += "\n  " + note->name;
                if (!note->description.empty())
                    output += "\n  " + note->description;
            }
        }
    }
    return output;
}

inline int SearchObjectId(const std::string &query)
{
    const auto start = query.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return -1;
    const auto end = query.find_last_not_of(" \t\r\n");
    const auto number = query.substr(start, end - start + 1);
    if (number.find_first_not_of("0123456789") != std::string::npos)
        return -1;
    return ParseInt(number, -1);
}

inline bool MatchesWords(const std::string &haystack, const std::string &query)
{
    const std::string text = Lower(haystack);
    const std::string words = Lower(query);
    size_t position = 0;
    while (position < words.size())
    {
        while (position < words.size() && std::isspace(static_cast<unsigned char>(words[position])))
            ++position;
        const size_t start = position;
        while (position < words.size() && !std::isspace(static_cast<unsigned char>(words[position])))
            ++position;
        if (position > start && text.find(words.substr(start, position - start)) == std::string::npos)
            return false;
    }
    return true;
}

inline int ParseHotkeyContext(const std::string &value)
{
    const std::string type = Lower(value);
    if (type.empty())
        return 6;
    char *end = nullptr;
    const long number = std::strtol(type.c_str(), &end, 10);
    if (end != type.c_str() && !*end && number >= -1 && number <= 6)
        return number;
    if (type == "all" || type == "any" || type == "global" || type == "everywhere")
        return -1;
    if (type == "none")
        return 0;
    if (type == "adv_map" || type == "adv_map_dlg" || type == "adventure" || type == "map")
        return 1;
    if (type == "hero" || type == "hero_dlg")
        return 2;
    if (type == "town" || type == "town_dlg" || type == "city")
        return 3;
    if (type == "combat" || type == "combat_dlg" || type == "battle")
        return 4;
    if (type == "main_menu" || type == "main_menu_dlg" || type == "menu")
        return 5;
    return 6;
}

inline std::string JsonPath(const std::string &root, const std::string &field)
{
    // Accept absolute roots returned by ArrayRoot as well as relative fields.
    if (field.compare(0, 5, "help.") == 0)
        return field;
    return field.empty() ? root : root + "." + field;
}

struct HotkeyRecord
{
    int context = 6;
    int id = -1;
    const char *keys = "", *name = "", *description = "";
};

struct HotkeyLine
{
    int context;
    const char *source = "", *keys = "", *name = "";
    int sourceId = 0, id = 0;
    const char *description = "";
};
struct HotkeyFocus
{
    int sourceId = -1, id = -1;
    size_t offset = std::string::npos;
};
inline std::string GroupHotkeys(std::vector<HotkeyLine> entries, const std::function<std::string(int)> &contextName,
                                HotkeyFocus *focus = nullptr)
{
    if (focus)
        focus->offset = std::string::npos;
    for (auto &entry : entries)
        entry.context = HotkeyContext(entry.context);
    std::stable_sort(entries.begin(), entries.end(), [](const HotkeyLine &a, const HotkeyLine &b) {
        if (a.context != b.context)
            return a.context < b.context;
        return a.sourceId != b.sourceId ? a.sourceId < b.sourceId : a.id < b.id;
    });
    std::string output;
    int previous = -2;
    for (const auto &entry : entries)
    {
        if (entry.context != previous)
        {
            if (!output.empty())
                output += "\n";
            output += "{" + contextName(entry.context) + "}\n";
            previous = entry.context;
        }
        if (focus && entry.sourceId == focus->sourceId && entry.id == focus->id)
            focus->offset = output.size();
        output += "[";
        output += entry.keys;
        output += "] ";
        output += entry.name;
        if (*entry.source)
        {
            output += " (";
            output += entry.source;
            output += ")";
        }
        if (*entry.description)
        {
            output += "\n  ";
            output += entry.description;
        }
        output += "\n";
    }
    return output;
}
using ReadJsonField = std::function<const char *(const std::string &, bool &)>;
inline std::vector<HotkeyRecord> ReadHotkeyRecords(const ReadJsonField &read)
{
    std::vector<HotkeyRecord> records;
    for (const std::string base : {std::string("hotkeys"), std::string("categories.hotkeys.content")})
    {
        int missing = 0;
        for (int index = 0; index < 4096 && missing < 32; ++index)
        {
            const std::string prefix = base + "." + std::to_string(index);
            bool found = false;
            const char *keys = read(prefix + ".keys", found);
            if (!found)
                keys = read(prefix + ".key", found);
            if (!found || !*keys)
            {
                ++missing;
                continue;
            }
            missing = 0;
            HotkeyRecord record;
            record.id = index;
            record.keys = keys;
            record.name = read(prefix + ".name", found);
            record.description = read(prefix + ".description", found);
            record.context = ParseHotkeyContext(read(prefix + ".type", found));
            records.push_back(record);
        }
        if (!records.empty())
            break;
    }
    return records;
}

struct CatalogueFilter
{
    int category = 0;
    unsigned levels = 0; // zero means all; otherwise an independent set of levels
    int facets[4] = {};
    int sort = kCatalogueIdSort; // Zero retains the explicit name-sort option.
    int firstRow = 0;
};

struct FilterRecord
{
    std::vector<int> categories;
    int level = -1;
    int facets[4] = {};
};

inline bool MatchesFilter(const FilterRecord &record, const CatalogueFilter &filter)
{
    if (filter.category &&
        std::find(record.categories.begin(), record.categories.end(), filter.category) == record.categories.end())
        return false;
    if (filter.levels && (record.level < 0 || record.level >= 32 || !(filter.levels & (1u << record.level))))
        return false;
    for (int index = 0; index < 4; ++index)
        if (filter.facets[index] && filter.facets[index] != record.facets[index])
            return false;
    return true;
}
} // namespace helpdlg
