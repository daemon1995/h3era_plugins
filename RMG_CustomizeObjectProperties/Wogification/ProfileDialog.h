#pragma once
#include "Settings.h"

namespace wogification
{
const char *ProfileButtonDef();
// Saving writes only differences from current JSON defaults. Loading replaces
// the draft only after successful parsing. Cancel leaves draft/path unchanged.
bool SelectProfileFile(bool save, Draft &draft, std::string &path);
}
