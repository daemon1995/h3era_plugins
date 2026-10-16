#pragma once

#include <cstring>

namespace SpellDescriptions
{
namespace Preview
{
constexpr char FLAG[] = "era.opt.spells.preview";
constexpr char MR1_SETTING[] = "interface_spells_description.settings.enableMr1Preview";

// ERA versions expose JSON booleans as either literal text or 0/1.
// Missing and malformed values keep the default: MR0 only.
inline bool Mr1Enabled(const char *value) noexcept
{
    return value && (std::strcmp(value, "true") == 0 || std::strcmp(value, "1") == 0);
}
} // namespace Preview
} // namespace SpellDescriptions
