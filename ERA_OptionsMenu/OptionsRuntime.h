#pragma once

#include "OptionJson.h"

namespace era_options
{
OptionRegistry &Registry();
void EnsureOptionsLoaded(bool applyDependencies = true);
void InvalidateOptionTexts();
void RefreshWogValues(bool currentMap = false);
void BeginCurrentMapOptions();
int ReadOptionValue(const EOption &option, bool currentMap = false);
const JsonLoadResult &LastLoadResult();
RegisterResult RegisterJsonOption(const std::string &path, const std::string &modFolder);
bool ChangeOptionValue(OptionId id, int value, bool currentMap = false);
bool IsOptionEditable(const EOption &option, bool currentMap = false);
std::string OptionDependencyHint(const EOption &option, bool currentMap = false);
const std::string &LastChangeError();
bool SaveOptionValues();
const std::string &LastSaveError();
std::string DefaultExportPath();
bool ExportOptionsJson(const std::string &pathUtf8, std::string &error, bool allowEditing = true, bool currentMap = false);
const std::string &ValuesFilePath();
} // namespace era_options
