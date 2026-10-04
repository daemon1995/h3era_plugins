#include "OptionJson.h"

#include <climits>
#include <set>

namespace era_options
{
namespace
{
bool ReadBool(const JsonReader &read, const std::string &path, bool &value, std::string &error)
{
    std::string raw;
    if (!read(path, raw) || ParseBoolean(raw, value))
        return true;
    error = path + ": expected true/false, on/off, or 1/0";
    return false;
}

bool ReadArray(const JsonReader &read, const std::string &path, std::vector<std::string> &values,
               std::string &error)
{
    for (int index = 0; ; ++index)
    {
        std::string value;
        if (!read(path + "." + std::to_string(index), value))
            return true;
        if (value.empty())
        {
            error = path + ": array entries cannot be empty";
            return false;
        }
        values.push_back(value);
        if (index == INT_MAX)
        {
            error = path + ": too many entries";
            return false;
        }
    }
}

bool HasOptionFields(const JsonReader &read, const std::string &path)
{
    std::string ignored;
    for (const char *field : {"key", "id", "name", "type", "default", "hint", "popup", "enabled", "visible",
                              "category", "category_order", "change_during_game", "page", "section", "tags.0", "choices.0", "wog_option", "wog_values.0",
                              "disables.0.id", "disables.0.tag", "disables.0.when", "disables.0.value",
                              "requires.0.id", "requires.0.tag", "requires.0.when", "requires.0.value"})
        if (read(path + "." + field, ignored))
            return true;
    return false;
}
} // namespace

bool ReadJsonOption(const JsonReader &read, const std::string &path, const std::string &modFolder,
                    const std::string &modName, OptionDefinition &definition, std::string &error)
{
    definition = OptionDefinition();
    definition.modFolder = LowerAscii(modFolder);
    definition.modName = modName;
    error.clear();
    if (!read(path + ".key", definition.key) || definition.key.empty())
    {
        error = "key is required";
        return false;
    }
    if (!read(path + ".name", definition.name) || definition.name.empty())
    {
        error = "name is required";
        return false;
    }
    read(path + ".hint", definition.hint);
    read(path + ".popup", definition.popup);
    read(path + ".category", definition.category);
    read(path + ".page", definition.page);
    read(path + ".section", definition.section);

    std::string raw;
    if (read(path + ".category_order", raw) && !ParseInteger(raw, definition.categoryOrder))
    {
        error = "category_order must be a 32-bit integer";
        return false;
    }
    if (read(path + ".id", raw) && (!ParseInteger(raw, definition.fixedId) || definition.fixedId < 0))
    {
        error = "id must be a non-negative 32-bit integer";
        return false;
    }
    if (read(path + ".type", raw))
    {
        const std::string type = LowerAscii(raw);
        if (type == "choice")
            definition.type = OptionType::Choice;
        else if (type != "checkbox")
        {
            error = "type must be checkbox or choice";
            return false;
        }
    }
    if (read(path + ".wog_option", raw) &&
        (!ParseInteger(raw, definition.wogOption) || definition.wogOption < 0 || definition.wogOption >= 1000))
    {
        error = "wog_option must be an index in 0..999";
        return false;
    }
    std::vector<std::string> rawValues;
    if (!ReadArray(read, path + ".wog_values", rawValues, error))
        return false;
    for (const auto &value : rawValues)
    {
        int number = 0;
        if (!ParseInteger(value, number))
        {
            error = "wog_values must contain 32-bit integers";
            return false;
        }
        definition.wogValues.push_back(number);
    }
    if (!ReadBool(read, path + ".enabled", definition.enabled, error) ||
        !ReadBool(read, path + ".visible", definition.visible, error) ||
        !ReadBool(read, path + ".change_during_game", definition.changeDuringGame, error) ||
        !ReadArray(read, path + ".tags", definition.tags, error) ||
        !ReadArray(read, path + ".choices", definition.choices, error))
        return false;

    if (read(path + ".default", raw))
    {
        bool checked = false;
        if (definition.type == OptionType::Checkbox && ParseBoolean(raw, checked))
            definition.defaultValue = checked ? 1 : 0;
        else if (!ParseInteger(raw, definition.defaultValue))
        {
            error = "default must be a checkbox state or a zero-based choice index";
            return false;
        }
    }
    for (const bool required : {false, true})
    for (int index = 0; ; ++index)
    {
        const std::string rulePath = path + (required ? ".requires." : ".disables.") + std::to_string(index);
        OptionDisableRule rule;
        rule.value = required ? 1 : 0;
        std::string id, when, value;
        const bool hasId = read(rulePath + ".id", id);
        const bool hasTag = read(rulePath + ".tag", rule.tag);
        const bool hasWhen = read(rulePath + ".when", when);
        const bool hasValue = read(rulePath + ".value", value);
        if (!hasId && !hasTag && !hasWhen && !hasValue) break;
        if ((hasId && (!ParseInteger(id, rule.id) || rule.id < 0)) ||
            (hasWhen && !ParseInteger(when, rule.when)) ||
            (hasValue && !ParseInteger(value, rule.value)))
        {
            error = rulePath + ": id/when/value must be valid non-negative integers";
            return false;
        }
        if (required) definition.requires.push_back({rule.id, rule.tag, rule.when, rule.value});
        else definition.disables.push_back(std::move(rule));
        if (index == INT_MAX) { error = "too many dependency rules"; return false; }
    }
    return ValidateDefinition(definition, error);
}

JsonLoadResult LoadJsonOptions(OptionRegistry &registry, const JsonReader &read,
                               const std::vector<std::string> &modFolders)
{
    JsonLoadResult report;
    std::vector<OptionDefinition> definitions;
    std::vector<std::string> paths;
    std::vector<std::string> folders = modFolders;
    std::set<std::string> loadedFolders;
    for (size_t folderIndex = 0; folderIndex < folders.size(); ++folderIndex)
    {
        // An active mod can host descriptions for other displayed mod roots.
        // Copy the folder because include_mods may grow the vector below.
        const std::string folder = LowerAscii(folders[folderIndex]);
        if (!loadedFolders.insert(folder).second)
            continue;
        const std::string root = "era_options." + folder;
        std::vector<std::string> included;
        std::string includeError;
        if (!ReadArray(read, root + ".include_mods", included, includeError))
            report.issues.push_back({root + ".include_mods", includeError});
        folders.insert(folders.end(), included.begin(), included.end());
        std::string modName = folder;
        read(root + ".name", modName);
        if (modName.empty())
            modName = folder;
        const auto collect = [&](const std::string &collectionRoot) {
            // Optional count supports holes and diagnoses records with no key.
            // Contiguous arrays do not need an explicit count.
            int count = -1;
            std::string raw;
            if (read(collectionRoot + ".option_count", raw) && (!ParseInteger(raw, count) || count < 0))
            {
                report.issues.push_back({collectionRoot + ".option_count", "expected a non-negative integer"});
                return;
            }
            for (int index = 0; count < 0 || index < count; ++index)
            {
                const std::string path = collectionRoot + ".options." + std::to_string(index);
                if (count < 0 && !HasOptionFields(read, path))
                    break;
                OptionDefinition definition;
                std::string error;
                if (ReadJsonOption(read, path, folder, modName, definition, error))
                {
                    definitions.push_back(std::move(definition));
                    paths.push_back(path);
                }
                else
                    report.issues.push_back({path, error});
                if (index == INT_MAX)
                {
                    report.issues.push_back({collectionRoot, "too many option records"});
                    break;
                }
            }
        };
        collect(root);
        // Named collections let independent Lang files coexist in one mod.
        // All definitions still enter the same batch, reserving static IDs first.
        std::vector<std::string> names;
        std::string error;
        if (!ReadArray(read, root + ".collection_names", names, error))
            report.issues.push_back({root + ".collection_names", error});
        std::set<std::string> visited;
        for (size_t index = 0; index < names.size(); ++index)
        {
            const auto &name = names[index];
            const std::string path = root + ".collection_names." + std::to_string(index);
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") !=
                std::string::npos)
            {
                report.issues.push_back({path, "collection name must contain ASCII letters, digits, '_' or '-'"});
                continue;
            }
            if (!visited.insert(name).second)
            {
                report.issues.push_back({path, "duplicate collection name"});
                continue;
            }
            const std::string collectionRoot = root + ".collections." + name;
            std::string ignored;
            if (!read(collectionRoot + ".option_count", ignored) &&
                !HasOptionFields(read, collectionRoot + ".options.0"))
            {
                report.issues.push_back({collectionRoot, "listed collection has no option_count or options"});
                continue;
            }
            collect(collectionRoot);
        }
    }
    const auto results = registry.RegisterBatch(definitions);
    for (size_t index = 0; index < results.size(); ++index)
    {
        if (results[index].status == RegisterStatus::Registered)
            ++report.registered;
        else if (results[index].status == RegisterStatus::AlreadyRegistered)
            ++report.alreadyRegistered;
        else
            report.issues.push_back({paths[index], definitions[index].key + ": " + results[index].message});
    }
    return report;
}
} // namespace era_options
