#pragma once
#include "ObjectBans.h"

namespace era_options
{
const ObjectBanRules &ObjectBanPreferences();
ObjectBanRules CurrentMapObjectBans();
void ReloadObjectBanPreferences();
void SetObjectBan(BanObjectKind kind, int id, bool banned);
void SetObjectBanSource(int source, bool enabled);
bool SaveObjectBanPreferences(std::string &error);
const std::string &ObjectBanLoadError();
std::string ObjectBanPreferencesPath();
std::vector<BanObjectInfo> NativeBanObjects(BanObjectKind kind);
void RegisterObjectBanEvents();
bool SkipBannedConfluxSpell(int spellId);
} // namespace era_options
