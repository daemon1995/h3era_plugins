#include "../SettingsSupport.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace
{
struct IniReader
{
    std::map<std::string, std::string> entries;
    std::vector<std::string> reads;

    std::string Address(const char *key, const char *section, const char *file) const
    {
        return std::string(file) + ':' + section + ':' + key;
    }
    bool Read(const char *key, const char *section, const char *file, char *buffer)
    {
        const auto address = Address(key, section, file);
        reads.push_back(address);
        const auto found = entries.find(address);
        if (found == entries.end())
            return false;
        std::memcpy(buffer, found->second.c_str(), found->second.size() + 1);
        return true;
    }
};
}

int main()
{
    using namespace sysopts;
    int integer = 42;
    assert(ParseInteger(" \t-2147483648 \r\n", integer) && integer == (-2147483647 - 1));
    assert(ParseInteger("2147483647", integer) && integer == 2147483647);
    for (const auto bad : {"", " ", "1x", "2147483648", "-2147483649", "1.5", "0x20", "1 2"})
    {
        integer = 42;
        assert(!ParseInteger(bad, integer) && integer == 42);
    }
    assert(!ParseInteger(nullptr, integer));
    float floating = 0.5f;
    assert(ParseFloat(" 1 \n", floating, 0.0f, 1.0f) && floating == 1.0f);
    assert(ParseFloat("-18", floating, -18.0f, 18.0f) && floating == -18.0f);
    for (const auto bad : {"nan", "inf", "-inf", "1e999", "1e-999", "0.5x", "-0.01", "1.01", "", " "})
    {
        floating = 0.5f;
        assert(!ParseFloat(bad, floating, 0.0f, 1.0f) && floating == 0.5f);
    }
    assert(!ParseFloat(nullptr, floating, 0.0f, 1.0f));

    for (const int count : {11, 101})
    {
        assert(NormalizedToTick(0.0f, count) == 0);
        assert(NormalizedToTick(1.0f, count) == count - 1);
        assert(TickToNormalized(0, count) == 0.0f);
        assert(TickToNormalized(count - 1, count) == 1.0f);
        assert(NormalizedToTick(-2.0f, count) == 0);
        assert(NormalizedToTick(2.0f, count) == count - 1);
        assert(TickToNormalized(-5, count) == 0.0f);
        assert(TickToNormalized(count + 5, count) == 1.0f);
        for (int tick = 0; tick < count; ++tick)
            assert(NormalizedToTick(TickToNormalized(tick, count), count) == tick);
    }
    assert(NormalizedToTick(0.975f, 101) == 98);
    assert(NormalizedToTick(std::numeric_limits<float>::quiet_NaN(), 101) == 0);
    assert(NormalizedToTick(std::numeric_limits<float>::infinity(), 101) == 0);
    assert(NormalizedToTick(1.0f, 1) == 0);
    assert(TickToNormalized(1, 0) == 0.0f);

    IniReader ini;
    const auto ownKey = ini.Address("held", "CombatHints", SETTINGS_FILE);
    const auto oldKey = ini.Address("held", "CombatHints", LEGACY_HEALTH_FILE);
    ini.entries[oldKey] = "1";
    char buffer[64]{};
    auto read = [&ini](const char *key, const char *section, const char *file, char *result) {
        return ini.Read(key, section, file, result);
    };
    assert(ReadMigrated(read, "held", "CombatHints", true, "held", "CombatHints", LEGACY_HEALTH_FILE, buffer));
    assert(std::strcmp(buffer, "1") == 0 && ini.reads.size() == 2);
    ini.entries[ownKey] = "0";
    ini.reads.clear();
    assert(ReadMigrated(read, "held", "CombatHints", true, "held", "CombatHints", LEGACY_HEALTH_FILE, buffer));
    assert(std::strcmp(buffer, "0") == 0 && ini.reads.size() == 1);
    // Invalid own values also stop migration: legacy settings cannot silently replace them.
    ini.entries[ownKey] = "bad";
    assert(ReadMigrated(read, "held", "CombatHints", true, "held", "CombatHints", LEGACY_HEALTH_FILE, buffer));
    integer = 0;
    assert(!ParseInteger(buffer, integer) && integer == 0);
    ini.entries.erase(ownKey);
    ini.reads.clear();
    assert(!ReadMigrated(read, "held", "CombatHints", false, "held", "CombatHints", LEGACY_HEALTH_FILE, buffer));
    assert(ini.reads.size() == 1 && ini.reads.front() == ownKey);

    SaveState state;
    assert(!state.IsDirty() && state.Revision() == 0);
    state.MarkDirty();
    assert(state.IsDirty() && state.Revision() == 1);
    state.Saved(false);
    assert(state.IsDirty() && state.Revision() == 1);
    state.Saved(true);
    assert(!state.IsDirty() && state.Revision() == 1);
    state.MarkDirty();
    assert(state.IsDirty() && state.Revision() == 2);
    state.Saved(false);
    state.Saved(false);
    assert(state.IsDirty());

    for (const bool combat : {false, true})
        for (const bool rebuild : {false, true})
            assert(CanChangeCombatOption(combat, rebuild) == !(combat && rebuild));
    assert(!CanChangeVolume(true, false, 0, 9));
    assert(!CanChangeVolume(false, false, 0, 9));
    assert(CanChangeVolume(false, false, 0, 0));
    assert(CanChangeVolume(true, true, 0, 9));
    assert(CanChangeVolume(true, false, 9, 0));
    std::puts("SettingsSupport tests passed.");
}
