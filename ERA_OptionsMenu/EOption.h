#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace era_options
{
using OptionId = std::int32_t;
constexpr OptionId InvalidOptionId = -1;
constexpr OptionId FirstDynamicOptionId = 1000;

enum class OptionType { Checkbox, Choice };

struct OptionDisableRule
{
    OptionId id = InvalidOptionId;
    std::string tag;
    int when = 1;
    int value = 0;
};

struct OptionRequireRule
{
    OptionId id = InvalidOptionId;
    std::string tag;
    int when = 1;
    int value = 1;
};

// Own strings independently of ERA's translation cache. Setting IDs are
// unrelated to H3Dlg control IDs and to indices in WoG's option array.
struct OptionDefinition
{
    std::string key; // Canonical lowercase ASCII after registration.
    OptionId fixedId = InvalidOptionId;
    std::string modFolder;
    std::string modName;
    std::string category; // Collapsible group inside a page.
    int categoryOrder = 0; // Equal priorities retain ascending minimum option ID.
    std::string page; // Independent page inside the owner mod; empty means General.
    std::string section; // Old category alias, used only when category is empty.
    std::string name;
    std::string hint;
    std::string popup;
    std::vector<std::string> tags;
    std::vector<OptionDisableRule> disables;
    std::vector<OptionRequireRule> requires;
    OptionType type = OptionType::Checkbox;
    std::vector<std::string> choices;
    int defaultValue = 0;
    bool enabled = true;
    bool visible = true;
    bool changeDuringGame = false; // Author-declared capability, read from JSON, never from a savegame.
    // Explicit native binding, independent of the registry ID. Values map
    // zero-based UI states to WoG's raw integers (including inverted flags).
    int wogOption = -1;
    std::vector<int> wogValues;
};

class EOption final
{
    friend class OptionRegistry;
    OptionId id;
    OptionDefinition definition;
    int value;

  public:
    EOption(OptionId id, const OptionDefinition &definition);
    OptionId GetId() const noexcept;
    const OptionDefinition &Definition() const noexcept;
    const std::string &GetKey() const noexcept;
    int GetValue() const noexcept;
    bool AcceptsValue(int candidate) const noexcept;
    bool ToWogValue(int candidate, int &raw) const noexcept;
    bool FromWogValue(int raw, int &candidate) const noexcept;
    bool IsWogValueLocked(int raw) const noexcept;
};

std::string LowerAscii(std::string value);
bool ParseInteger(const std::string &text, int &value);
bool ParseBoolean(const std::string &text, bool &value);
bool ValidateDefinition(const OptionDefinition &definition, std::string &error);
} // namespace era_options
