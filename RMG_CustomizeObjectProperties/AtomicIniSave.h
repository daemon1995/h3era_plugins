#pragma once

#include <functional>

namespace rmgsettings
{
// The writer receives a separate, staged INI path. The destination and its ERA
// cache remain unchanged if any write, save, flush, or replacement fails.
bool SaveIniAtomically(const char *path, const std::function<bool(const char *)> &writer,
                       bool preserveExisting = false);
} // namespace rmgsettings
