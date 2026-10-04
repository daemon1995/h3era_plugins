#include "EOption.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>

namespace era_options
{
namespace
{
std::string Trim(const std::string &value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? std::string() :
        value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}
}

EOption::EOption(OptionId identifier, const OptionDefinition &source)
    : id(identifier), definition(source), value(source.defaultValue)
{
}

OptionId EOption::GetId() const noexcept { return id; }
const OptionDefinition &EOption::Definition() const noexcept { return definition; }
const std::string &EOption::GetKey() const noexcept { return definition.key; }
int EOption::GetValue() const noexcept { return value; }

bool EOption::AcceptsValue(int candidate) const noexcept
{
    return candidate >= 0 && (definition.type == OptionType::Checkbox ? candidate <= 1 :
        static_cast<size_t>(candidate) < definition.choices.size());
}

bool EOption::ToWogValue(int candidate, int &raw) const noexcept
{
    if (definition.wogOption < 0 || !AcceptsValue(candidate))
        return false;
    raw = definition.wogValues.empty() ? candidate : definition.wogValues[candidate];
    return true;
}

bool EOption::FromWogValue(int raw, int &candidate) const noexcept
{
    if (definition.wogOption < 0)
        return false;
    // Native WoG checkboxes also store read-only off/on as 2/3.
    // Choice indices (including 2/3) retain their ordinary meaning.
    if (IsWogValueLocked(raw))
        raw -= 2;
    int value = raw;
    if (!definition.wogValues.empty())
    {
        const auto found = std::find(definition.wogValues.begin(), definition.wogValues.end(), raw);
        if (found == definition.wogValues.end())
            return false;
        value = static_cast<int>(found - definition.wogValues.begin());
    }
    if (!AcceptsValue(value))
        return false;
    candidate = value;
    return true;
}

bool EOption::IsWogValueLocked(int raw) const noexcept
{
    const auto &mapping = definition.wogValues;
    return definition.wogOption >= 0 && definition.type == OptionType::Checkbox &&
        (definition.wogOption < 1 || definition.wogOption > 4) &&
        (raw == 2 || raw == 3) && (mapping.empty() ||
            (mapping.size() == 2 && ((mapping[0] == 0 && mapping[1] == 1) ||
                                    (mapping[0] == 1 && mapping[1] == 0))));
}

std::string LowerAscii(std::string value)
{
    for (char &ch : value)
        if (ch >= 'A' && ch <= 'Z')
            ch = static_cast<char>(ch + ('a' - 'A'));
    return value;
}

bool ParseInteger(const std::string &text, int &value)
{
    const std::string normalized = Trim(text);
    if (normalized.empty())
        return false;
    char *end = nullptr;
    errno = 0;
    const long parsed = std::strtol(normalized.c_str(), &end, 10);
    if (errno == ERANGE || end == normalized.c_str() || *end || parsed < INT_MIN || parsed > INT_MAX)
        return false;
    value = static_cast<int>(parsed);
    return true;
}

bool ParseBoolean(const std::string &text, bool &value)
{
    const std::string normalized = LowerAscii(Trim(text));
    if (normalized == "true" || normalized == "1" || normalized == "on")
        value = true;
    else if (normalized == "false" || normalized == "0" || normalized == "off")
        value = false;
    else
        return false;
    return true;
}

bool ValidateDefinition(const OptionDefinition &definition, std::string &error)
{
    error.clear();
    if (definition.key.empty() || definition.key.size() > 255 ||
        definition.key.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-:") !=
            std::string::npos)
        error = "key must contain 1..255 ASCII letters, digits, '_', '-', '.', or ':'";
    else if (definition.fixedId < InvalidOptionId)
        error = "id must be a non-negative 32-bit integer";
    else if (definition.modFolder.empty() || definition.name.empty())
        error = "mod folder and option name are required";
    else if (definition.type != OptionType::Checkbox && definition.type != OptionType::Choice)
        error = "unsupported option type";
    else if (definition.type == OptionType::Choice &&
        (definition.choices.size() < 2 || definition.choices.size() > static_cast<size_t>(INT_MAX)))
        error = "choice options require at least two choices";
    else if (definition.type == OptionType::Checkbox && !definition.choices.empty())
        error = "checkbox options cannot have choices";
    else if (std::any_of(definition.choices.begin(), definition.choices.end(),
                         [](const std::string &choice) { return choice.empty(); }))
        error = "choice names cannot be empty";
    else if (definition.wogOption < -1 || definition.wogOption >= 1000)
        error = "wog_option must be an index in 0..999";
    else if (definition.wogOption < 0 && !definition.wogValues.empty())
        error = "wog_values requires wog_option";
    else if (!definition.wogValues.empty() &&
             definition.wogValues.size() != (definition.type == OptionType::Checkbox ? 2 : definition.choices.size()))
        error = "wog_values must map every checkbox state or choice";
    else if (!definition.wogValues.empty() && [&definition]() {
        auto sorted = definition.wogValues;
        std::sort(sorted.begin(), sorted.end());
        return std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end();
    }())
        error = "wog_values cannot contain duplicate raw values";
    else if (!EOption(InvalidOptionId, definition).AcceptsValue(definition.defaultValue))
        error = "default is outside the option's value range";
    else
    {
        for (const auto &rule : definition.disables)
            if ((rule.id >= 0) == !rule.tag.empty() || rule.id < InvalidOptionId || rule.value < 0 ||
                !EOption(InvalidOptionId, definition).AcceptsValue(rule.when))
            {
                error = "disables requires exactly one id/tag, a valid source state and a non-negative target state";
                break;
            }
        for (const auto &rule : definition.requires)
            if ((rule.id >= 0) == !rule.tag.empty() || rule.id < InvalidOptionId || rule.value < 0 ||
                rule.when == 0 || !EOption(InvalidOptionId, definition).AcceptsValue(rule.when))
            {
                error = "requires needs exactly one id/tag, a nonzero source state and a non-negative required state";
                break;
            }
    }
    return error.empty();
}
} // namespace era_options
