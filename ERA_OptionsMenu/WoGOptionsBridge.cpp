#include "pch.h"
#include "WoGOptionsBridge.h"
#include "OptionsRuntime.h"
#include "WoGProfileStore.h"
#include "ObjectBans.h"
#include <array>

namespace era_options
{
namespace
{
constexpr size_t OptionCount = WogOptionCount;

int *NativeOptions(bool globals)
{
    // ERA Erm.pas: TWoGOptions = [current, global][0..999], relocated
    // from 0x2771920. Menu preferences use globals; live edits use current.
    auto *base = static_cast<int *>(Era::GetRealAddr(reinterpret_cast<void *>(0x2771920)));
    if (!base)
        return nullptr;
    auto *global = base + (globals ? OptionCount : 0);
    MEMORY_BASIC_INFORMATION region = {};
    if (!VirtualQuery(global, &region, sizeof(region)) || region.State != MEM_COMMIT ||
        (region.Protect & (PAGE_GUARD | PAGE_NOACCESS)) ||
        !(region.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
        return nullptr;
    const auto end = reinterpret_cast<ULONG_PTR>(region.BaseAddress) + region.RegionSize;
    return reinterpret_cast<ULONG_PTR>(global) + OptionCount * sizeof(int) <= end ? global : nullptr;
}

int *GlobalOptions() { return NativeOptions(true); }

std::string GameDirectory()
{
    const auto &path = ValuesFilePath();
    return path.substr(0, path.find_last_of("\\/") + 1);
}

}

bool RetireLegacyObjectBansForNewMap()
{
    auto *current = NativeOptions(false);
    if (!current) return false;
    // Stop the old scripts from re-applying retired selectors. Keep map-author
    // locks (2/3) and all unrelated mechanics, including combination assembly.
    for (const auto &entry : LegacyObjectBans())
        if (current[entry.option] == 0 || current[entry.option] == 1) current[entry.option] = 0;
    for (const auto id : LegacyBanSources())
        if (current[id] == 0 || current[id] == 1) current[id] = 0;
    return true;
}

bool ReadWogRawValue(int index, int &value, bool currentMap)
{
    if (index < 0 || index >= static_cast<int>(OptionCount)) return false;
    const auto *values = NativeOptions(!currentMap);
    if (!values) return false;
    value = values[index];
    return true;
}

bool ReadWogValue(const EOption &option, int &value, bool *locked, bool currentMap)
{
    int raw = 0;
    if (!ReadWogRawValue(option.Definition().wogOption, raw, currentMap) || !option.FromWogValue(raw, value)) return false;
    if (locked) *locked = option.IsWogValueLocked(raw);
    return true;
}

bool ApplyWogValues(const std::vector<std::pair<const EOption *, int>> &values, std::string &error, bool currentMap)
{
    if (values.empty()) return true;
    auto *global = NativeOptions(!currentMap);
    if (!global) { error = "cannot access WoG options"; return false; }
    std::vector<std::pair<int, int>> changes;
    for (const auto &entry : values)
    {
        const auto *option = entry.first;
        const int index = option->Definition().wogOption;
        int raw = 0;
        if (index < 0 || index >= static_cast<int>(OptionCount) ||
            (currentMap && !option->Definition().changeDuringGame) ||
            option->IsWogValueLocked(global[index]) || !option->ToWogValue(entry.second, raw))
        { error = "cannot change WoG preference: " + option->GetKey(); return false; }
        changes.emplace_back(index, raw);
    }
    // All targets and allocations are checked before changing any slot.
    for (const auto &entry : changes) global[entry.first] = entry.second;
    return true;
}

bool SaveWogProfile(std::string *error)
{
    auto *global = GlobalOptions();
    if (!global)
    {
        if (error) *error = "cannot read global WoG preferences";
        return false;
    }
    std::array<std::int32_t, WogOptionCount> snapshot;
    std::copy(global, global + WogOptionCount, snapshot.begin());
    return SaveWogProfileFiles(GameDirectory(), snapshot.data(), error);
}
} // namespace era_options
