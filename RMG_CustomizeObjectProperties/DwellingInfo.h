#pragma once
#include "pch.h"

namespace rmgDwellings
{
LPCSTR GetName(int type, int subtype) noexcept;
BOOL AppendHint(int type, int subtype, H3String &text) noexcept;
void InitDefaultValues() noexcept;
H3RmgObjectGenerator *CreateGenerator(const RMGObjectProperties &info) noexcept;
} // namespace rmgDwellings
