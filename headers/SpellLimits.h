#pragma once

#include "header.h"
#include <algorithm>

namespace spellLimits
{
inline int GetSpellCount() noexcept
{
    // Read at use time: other plugins may patch the limit after DLL loading.
    return std::max<int>(h3::limits::TOTAL_SPELLS, *reinterpret_cast<const unsigned char *>(0x04C170D));
}
} // namespace spellLimits
