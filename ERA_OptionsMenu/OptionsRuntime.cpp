#include "pch.h"
#include "OptionsRuntime.h"
#include "OptionsMenuApi.h"
#include "WoGOptionsBridge.h"
#include "OptionExport.h"
#include "OptionDependencies.h"
#include "ObjectBansRuntime.h"
#include "GameContext.h"

#include <set>

namespace era_options
{
namespace
{
OptionRegistry registry;
JsonLoadResult lastLoad;
bool loaded = false;
std::set<std::string> changedKeys;
std::string valuesPath;
OptionDependencies dependencies;
DependencyState dependencyState;
OptionValues requestedValues;
OptionValues mapRequestedValues;
DependencyState mapDependencyState;
bool dependenciesLoaded = false;
std::string lastSaveError;
std::string lastChangeError;

bool ReadEraJson(const std::string &key, std::string &value)
{
    const char *raw = Era::tr(key.c_str());
    if (!raw || key == raw)
        return false;
    value = raw;
    return true;
}

void RestoreSavedValue(OptionId id)
{
    const EOption *option = registry.Find(id);
    if (option->Definition().wogOption >= 0)
    {
        int value = 0;
        if (ReadWogValue(*option, value))
            registry.RestoreValue(id, value);
        return; // Native preferences, rather than a stale ERA INI copy.
    }
    char buffer[64] = {};
    GetPrivateProfileStringA("Options", option->GetKey().c_str(), "", buffer, sizeof(buffer), ValuesFilePath().c_str());
    int value = 0;
    if (ParseInteger(buffer, value))
        registry.RestoreValue(id, value);
}

void ReportIssues(const JsonLoadResult &report)
{
    for (const auto &issue : report.issues)
        OutputDebugStringA(("ERA_OptionsMenu: " + issue.path + ": " + issue.message + "\n").c_str());
}

OptionValues ImmutableNativeValues(bool currentMap = false)
{
    OptionValues result;
    for (const auto &option : registry.Options())
        if (option->Definition().wogOption >= 0)
        {
            int value = 0;
            bool locked = false;
            if (ReadWogValue(*option, value, &locked, currentMap) &&
                (locked || !option->Definition().enabled || (currentMap && !option->Definition().changeDuringGame)))
                result[option->GetId()] = value;
        }
        else if (currentMap && (!option->Definition().enabled || !option->Definition().changeDuringGame))
            result[option->GetId()] = ReadOptionValue(*option, true);
    return result;
}

bool ApplyRequestedValues(OptionValues next)
{
    auto state = dependencies.Evaluate(registry, next, ImmutableNativeValues());
    lastChangeError = state.error;
    if (!lastChangeError.empty()) return false;
    auto pending = changedKeys;
    std::vector<std::pair<const EOption *, int>> nativeChanges;
    for (const auto &option : registry.Options())
    {
        const auto id = option->GetId();
        if (option->GetValue() != state.values[id])
        {
            pending.insert(option->GetKey());
            if (option->Definition().wogOption >= 0) nativeChanges.emplace_back(option.get(), state.values[id]);
        }
        const auto previous = requestedValues.find(id), current = next.find(id);
        if (previous != requestedValues.end() && current != next.end() && previous->second != current->second)
            pending.insert(option->GetKey());
    }
    lastChangeError.clear();
    if (!ApplyWogValues(nativeChanges, lastChangeError)) return false;
    for (const auto &value : state.values) registry.RestoreValue(value.first, value.second);
    requestedValues = std::move(next);
    dependencyState = std::move(state);
    changedKeys = std::move(pending);
    return true;
}

void RebuildDependencies(bool apply = true)
{
    for (const auto &option : registry.Options()) requestedValues.emplace(option->GetId(), option->GetValue());
    for (const auto &issue : dependencies.Configure(registry)) lastLoad.issues.push_back({"dependencies", issue});
    if (!dependenciesLoaded)
    {
        const auto initial = dependencies.Evaluate(registry, requestedValues, ImmutableNativeValues());
        for (const auto id : dependencies.Targets())
            if (initial.IsBlocked(id) && registry.Find(id)->Definition().wogOption >= 0)
            {
                char buffer[64] = {};
                GetPrivateProfileStringA("DependencyValues", registry.Find(id)->GetKey().c_str(), "",
                    buffer, sizeof(buffer), ValuesFilePath().c_str());
                int value = 0;
                if (ParseInteger(buffer, value) && registry.Find(id)->AcceptsValue(value)) requestedValues[id] = value;
            }
        dependenciesLoaded = true;
    }
    if (apply)
    {
        if (!ApplyRequestedValues(requestedValues))
        {
            dependencyState = dependencies.Evaluate(registry, requestedValues, ImmutableNativeValues());
            lastLoad.issues.push_back({"dependencies", lastChangeError});
        }
    }
    else dependencyState = dependencies.Evaluate(registry, requestedValues, ImmutableNativeValues());
}
} // namespace

OptionRegistry &Registry() { return registry; }
const JsonLoadResult &LastLoadResult() { return lastLoad; }
const std::string &LastSaveError() { return lastSaveError; }
const std::string &LastChangeError() { return lastChangeError; }
void InvalidateOptionTexts() { loaded = false; }

const std::string &ValuesFilePath()
{
    if (valuesPath.empty())
    {
        std::vector<char> buffer(1024);
        DWORD length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        while (length >= buffer.size())
        {
            buffer.resize(buffer.size() * 2);
            length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        }
        const std::string executable(buffer.data(), length);
        const auto slash = executable.find_last_of("\\/");
        valuesPath = (slash == std::string::npos ? std::string() : executable.substr(0, slash + 1)) +
            "ERA_OptionsMenu.ini";
    }
    return valuesPath;
}

void EnsureOptionsLoaded(bool applyDependencies)
{
    if (loaded)
        return;
    const auto folders = modList::GetEraModList(false);
    if (folders.empty())
        return; // VFS may not be initialized yet. Retry on a later normal call.
    const size_t previousCount = registry.Options().size();
    lastLoad = LoadJsonOptions(registry, ReadEraJson, folders);
    for (size_t index = previousCount; index < registry.Options().size(); ++index)
    {
        RestoreSavedValue(registry.Options()[index]->GetId());
        requestedValues[registry.Options()[index]->GetId()] = registry.Options()[index]->GetValue();
    }
    RebuildDependencies(applyDependencies);
    loaded = true;
    ReportIssues(lastLoad);
}

RegisterResult RegisterJsonOption(const std::string &path, const std::string &modFolder)
{
    const bool currentMap = IsOnMap();
    EnsureOptionsLoaded(!currentMap);
    std::string modName = modFolder;
    ReadEraJson("era_options." + LowerAscii(modFolder) + ".name", modName);
    OptionDefinition definition;
    std::string error;
    if (!ReadJsonOption(ReadEraJson, path, modFolder, modName, definition, error))
        return {RegisterStatus::Invalid, InvalidOptionId, error};
    auto result = registry.Register(definition);
    if (result.status == RegisterStatus::Registered)
    {
        RestoreSavedValue(result.id);
        requestedValues[result.id] = registry.Find(result.id)->GetValue();
    }
    if (result.Succeeded())
    {
        RebuildDependencies(!currentMap);
        if (currentMap) BeginCurrentMapOptions();
    }
    return result;
}

std::string DefaultExportPath()
{
    const auto &values = ValuesFilePath();
    return TextToUtf8(values.substr(0, values.find_last_of("\\/") + 1) + "ERA_OptionsMenu.export.json",
                      GetACP()); // GetModuleFileNameA uses the Windows filesystem codepage.
}

bool ExportOptionsJson(const std::string &pathUtf8, std::string &error, bool allowEditing, bool currentMap)
{
    try
    {
        EnsureOptionsLoaded(allowEditing && !currentMap);
        if (!loaded)
        {
            error = "ERA language data is not loaded yet";
            return false;
        }
        if (allowEditing) RefreshWogValues(currentMap);
        const auto readValue = [currentMap](const EOption &option) {
            const int value = ReadOptionValue(option, currentMap);
            if (value < 0) throw std::runtime_error("cannot read current WoG value: " + option.GetKey());
            return value;
        };
        return WriteOptionsJson(pathUtf8, SerializeOptionsJson(registry,
            Era::GetCodePage(), readValue), error);
    }
    catch (const std::exception &exception) { error = exception.what(); return false; }
}

int ReadOptionValue(const EOption &option, bool currentMap)
{
    int value = option.GetValue();
    if (option.Definition().wogOption >= 0)
        return ReadWogValue(option, value, nullptr, currentMap) ? value : -1;
    if (currentMap)
    {
        const std::string key = "era_options.values." + option.GetKey();
        if (Era::GetAssocVarIntValue((key + ".present").c_str()))
            value = Era::GetAssocVarIntValue(key.c_str());
    }
    return value;
}

void BeginCurrentMapOptions()
{
    mapDependencyState = {};
    mapRequestedValues.clear();
    for (const auto &option : registry.Options())
        mapRequestedValues[option->GetId()] = ReadOptionValue(*option, true);
    mapDependencyState = dependencies.Evaluate(registry, mapRequestedValues, ImmutableNativeValues(true));
    for (const auto id : dependencies.Targets())
        if (mapDependencyState.IsBlocked(id))
        {
            const std::string key = "era_options.requested." + registry.Find(id)->GetKey();
            if (Era::GetAssocVarIntValue((key + ".present").c_str()))
            {
                const int value = Era::GetAssocVarIntValue(key.c_str());
                if (registry.Find(id)->AcceptsValue(value)) mapRequestedValues[id] = value;
            }
        }
    mapDependencyState = dependencies.Evaluate(registry, mapRequestedValues, ImmutableNativeValues(true));
}

void RefreshWogValues(bool currentMap)
{
    if (currentMap)
    {
        for (const auto &option : registry.Options())
        {
            const auto id = option->GetId();
            const int value = ReadOptionValue(*option, true);
            if (value >= 0 && (!mapDependencyState.IsBlocked(id) || value != mapDependencyState.values[id]))
                mapRequestedValues[id] = value;
        }
        mapDependencyState = dependencies.Evaluate(registry, mapRequestedValues, ImmutableNativeValues(true));
        return; // Opening/refreshing a map menu never applies preferences.
    }
    for (const auto &option : registry.Options())
        if (option->Definition().wogOption >= 0 && !changedKeys.count(option->GetKey()))
        {
            int value = 0;
            if (ReadWogValue(*option, value))
            {
                registry.RestoreValue(option->GetId(), value);
                if (!dependencyState.IsBlocked(option->GetId())) requestedValues[option->GetId()] = value;
            }
        }
    if (!ApplyRequestedValues(requestedValues))
        dependencyState = dependencies.Evaluate(registry, requestedValues, ImmutableNativeValues());
}

bool IsOptionEditable(const EOption &option, bool currentMap)
{
    const auto &state = currentMap ? mapDependencyState : dependencyState;
    if (!option.Definition().enabled || state.IsBlocked(option.GetId()) ||
        !AllowOptionEditing(true, currentMap, option.Definition().changeDuringGame))
        return false;
    if (option.Definition().wogOption < 0)
        return true;
    int value = 0;
    bool locked = false;
    return ReadWogValue(option, value, &locked, currentMap) && !locked;
}

std::string OptionDependencyHint(const EOption &option, bool currentMap)
{
    const auto &state = currentMap ? mapDependencyState : dependencyState;
    const auto found = state.blockers.find(option.GetId());
    if (found == state.blockers.end()) return {};
    std::string text = state.missingRequirements.count(option.GetId()) ? "Requires: " : "Disabled by: ";
    for (const auto id : found->second)
    {
        if (text.back() != ' ') text += ", ";
        if (const auto *source = registry.Find(id)) text += source->Definition().name;
    }
    return text;
}

bool ChangeOptionValue(OptionId id, int value, bool currentMap)
{
    EOption *option = registry.Find(id);
    RefreshWogValues(currentMap);
    lastChangeError.clear();
    if (!option || !IsOptionEditable(*option, currentMap) || !option->AcceptsValue(value))
        return false;
    if (currentMap)
    {
        auto next = mapRequestedValues;
        next[id] = value;
        auto state = dependencies.Evaluate(registry, next, ImmutableNativeValues(true));
        // A legacy save can contain ignored, immutable settings inconsistent
        // with new JSON rules. Only newly introduced conflicts reject a click.
        for (const auto &conflict : state.conflicts)
            if (!mapDependencyState.conflicts.count(conflict)) { lastChangeError = conflict; break; }
        if (!lastChangeError.empty()) return false;
        std::vector<std::pair<const EOption *, int>> nativeChanges;
        for (const auto &entry : registry.Options())
            if (entry->Definition().wogOption >= 0 && state.values[entry->GetId()] != ReadOptionValue(*entry, true))
                nativeChanges.emplace_back(entry.get(), state.values[entry->GetId()]);
        if (!ApplyWogValues(nativeChanges, lastChangeError, true)) return false;
        for (const auto target : dependencies.Targets())
        {
            const auto *dependent = registry.Find(target);
            if (!dependent->Definition().changeDuringGame) continue;
            const std::string key = "era_options.requested." + dependent->GetKey();
            Era::SetAssocVarIntValue(key.c_str(), next[target]);
            Era::SetAssocVarIntValue((key + ".present").c_str(), 1);
        }
        for (const auto &entry : registry.Options())
            if (entry->Definition().wogOption < 0 && state.values[entry->GetId()] != ReadOptionValue(*entry, true))
            {
                const std::string key = "era_options.values." + entry->GetKey();
                Era::SetAssocVarIntValue(key.c_str(), state.values[entry->GetId()]);
                Era::SetAssocVarIntValue((key + ".present").c_str(), 1);
            }
        mapRequestedValues = std::move(next);
        mapDependencyState = std::move(state);
        return true;
    }
    auto next = requestedValues;
    next[id] = value;
    return ApplyRequestedValues(std::move(next));
}

bool SaveOptionValues()
{
    lastSaveError.clear();
    if (!SaveObjectBanPreferences(lastSaveError)) return false;
    if (changedKeys.empty()) return true;
    bool success = true;
    bool nativeChanged = false;
    for (const auto &key : changedKeys)
        nativeChanged |= registry.Find(key)->Definition().wogOption >= 0;
    // Keep user intent separately for native targets forced off by dependencies.
    // Store it before the effective profile so a restart cannot lose that intent.
    for (const auto id : dependencies.Targets())
        if (const auto *option = registry.Find(id))
            if (option->Definition().wogOption >= 0)
            {
                const std::string value = std::to_string(requestedValues[id]);
                if (!WritePrivateProfileStringA("DependencyValues", option->GetKey().c_str(), value.c_str(), ValuesFilePath().c_str()))
                {
                    lastSaveError = "cannot save dependency values (Win32 error " + std::to_string(GetLastError()) + ")";
                    return false;
                }
            }
    const bool nativeSaved = !nativeChanged || SaveWogProfile(&lastSaveError);
    success &= nativeSaved;
    for (auto it = changedKeys.begin(); it != changedKeys.end(); )
    {
        const EOption *option = registry.Find(*it);
        if (option->Definition().wogOption >= 0)
        {
            if (nativeSaved)
                it = changedKeys.erase(it);
            else
                ++it;
            continue;
        }
        const std::string value = std::to_string(requestedValues[option->GetId()]);
        if (WritePrivateProfileStringA("Options", it->c_str(), value.c_str(), ValuesFilePath().c_str()))
            it = changedKeys.erase(it);
        else
        {
            if (lastSaveError.empty()) lastSaveError = "cannot save " + ValuesFilePath() +
                " (Win32 error " + std::to_string(GetLastError()) + ")";
            success = false;
            ++it;
        }
    }
    return success;
}
} // namespace era_options

#pragma comment(linker, "/EXPORT:EraOptions_GetId=_EraOptions_GetId@4")
#pragma comment(linker, "/EXPORT:EraOptions_RegisterJson=_EraOptions_RegisterJson@8")
#pragma comment(linker, "/EXPORT:EraOptions_GetValue=_EraOptions_GetValue@8")
#pragma comment(linker, "/EXPORT:EraOptions_SetValue=_EraOptions_SetValue@8")
#pragma comment(linker, "/EXPORT:EraOptions_Save=_EraOptions_Save@0")
#pragma comment(linker, "/EXPORT:EraOptions_ExportJson=_EraOptions_ExportJson@4")

extern "C" int32_t __stdcall EraOptions_GetId(const char *key)
{
    try
    {
        era_options::EnsureOptionsLoaded(!era_options::IsOnMap());
        return key ? era_options::Registry().GetId(key) : -1;
    }
    catch (...) { return -1; }
}

extern "C" int32_t __stdcall EraOptions_RegisterJson(const char *path, const char *modFolder)
{
    try
    {
        if (!path || !*path || !modFolder || !*modFolder)
            return -1;
        const auto result = era_options::RegisterJsonOption(path, modFolder);
        return result.Succeeded() ? result.id : static_cast<int32_t>(result.status);
    }
    catch (...) { return -1; }
}

extern "C" int32_t __stdcall EraOptions_GetValue(int32_t id, int32_t *outValue)
{
    try
    {
        const bool currentMap = era_options::IsOnMap();
        era_options::EnsureOptionsLoaded(!currentMap);
        const auto *option = era_options::Registry().Find(id);
        if (!option || !outValue)
            return 0;
        era_options::RefreshWogValues(currentMap);
        const int value = era_options::ReadOptionValue(*option, currentMap);
        if (value < 0) return 0;
        *outValue = value;
        return 1;
    }
    catch (...) { return 0; }
}

extern "C" int32_t __stdcall EraOptions_SetValue(int32_t id, int32_t value)
{
    try
    {
        const bool currentMap = era_options::IsOnMap();
        era_options::EnsureOptionsLoaded(!currentMap);
        return era_options::ChangeOptionValue(id, value, currentMap) ? 1 : 0;
    }
    catch (...) { return 0; }
}

extern "C" int32_t __stdcall EraOptions_Save()
{
    try { return era_options::IsOnMap() || era_options::SaveOptionValues() ? 1 : 0; }
    catch (...) { return 0; }
}

extern "C" int32_t __stdcall EraOptions_ExportJson(const char *pathUtf8)
{
    try
    {
        std::string error;
        return era_options::ExportOptionsJson(pathUtf8 ? pathUtf8 : era_options::DefaultExportPath(), error,
            false, era_options::IsOnMap()) ? 1 : 0;
    }
    catch (...) { return 0; }
}
