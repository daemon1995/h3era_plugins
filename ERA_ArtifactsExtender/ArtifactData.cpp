#include "pch.h"
#include <cstdarg>
#include "../headers/Era/JsonOverridesSchema.hpp"

namespace artifacts
{
void Log(const char *format, ...)
{
    char buffer[2048];
    va_list args;
    va_start(args, format);
    _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
    va_end(args);
    OutputDebugStringA(buffer);
    CreateDirectoryA("Debug", nullptr);
    CreateDirectoryA("Debug\\Era", nullptr);
    if (FILE *file = fopen("Debug\\Era\\ERA_ArtifactsExtender.log", "a"))
    {
        fprintf(file, "%s\n", buffer);
        fclose(file);
    }
}

namespace
{
bool ReadInt(const std::string &prefix, const char *field, int &value)
{
    const std::string key = prefix + "." + field;
    bool found = false;
    const char *text = EraJS::read(key, found);
    if (!found) return false;
    if (EraJS::ParseNumber(text, value)) return true;
    Log("Invalid integer: %s = %s", key.c_str(), text);
    return false;
}
void ReadText(LPCSTR &target, const std::string &prefix, const char *field)
{
    bool found = false;
    const char *value = EraJS::read(prefix + "." + field, found);
    if (found) target = value;
}
template <class T, class Predicate>
void ReadChecked(T &target, const std::string &key, const char *field, Predicate valid)
{
    int value;
    if (!ReadInt(key, field, value)) return;
    if (valid(value)) target = static_cast<T>(value);
    else Log("Rejected value: %s.%s = %d", key.c_str(), field, value);
}
} // namespace

bool ArtifactData::HasDefinition(const std::string &key)
{
    const char *fields[] = {"id", "name", "description", "cost", "position", "type", "disabled", "hasSpell",
                            "event", "primary.attack", "primary.defence", "primary.spellPower", "primary.knowledge",
                            "ai.value", "image", "mapDef"};
    for (auto field : fields)
        if (!EraJS::isEmpty((key + "." + field).c_str())) return true;
    return false;
}

void ArtifactData::ReloadText(H3ArtifactSetup &setup, LPCSTR &event, bool name, bool description, int id) const
{
    const std::string keys[] = {legacyKey, "era.artifacts." + std::to_string(id)};
    for (const auto &key : keys)
    {
        if (key.empty()) continue;
        if (name) ReadText(setup.name, key, "name");
        if (description) ReadText(setup.description, key, "description");
        ReadText(event, key, "event");
    }
}

void ArtifactData::Apply(H3ArtifactSetup &setup, signed char *primary, LPCSTR &event, int id)
{
    const std::string keys[] = {legacyKey, "era.artifacts." + std::to_string(id)};
    for (const auto &key : keys)
    {
        if (key.empty()) continue;
        ReadText(setup.name, key, "name");
        ReadText(setup.description, key, "description");
        ReadText(event, key, "event");
        bool found = false;
        const char *imageName = EraJS::read(key + ".image", found);
        if (found) image = imageName;
        const char *groundDef = EraJS::read(key + ".mapDef", found);
        if (found) mapDef = groundDef;
        ReadChecked(imageFrame, key, "imageFrame", [](int v) { return v >= 0 && v < CAPACITY; });
        ReadChecked(setup.cost, key, "cost", [&](int v) { return EraJS::ValidArtifactNumber(EraJS::ArtifactSchema::cost, v, CAPACITY, 12); });
        // Indices into LockSlotToArtSlots, not worn-slot numbers or masks.
        ReadChecked(setup.position, key, "position", [&](int v) { return EraJS::ValidArtifactNumber(EraJS::ArtifactSchema::position, v, CAPACITY, 12); });
        ReadChecked(setup.type, key, "type", [&](int v) { return EraJS::ValidArtifactNumber(EraJS::ArtifactSchema::type, v, CAPACITY, 12); });
        ReadChecked(setup.disabled, key, "disabled", [&](int v) { return EraJS::ValidArtifactNumber(EraJS::ArtifactSchema::disabled, v, CAPACITY, 12); });
        ReadChecked(setup.hasSpell, key, "hasSpell", [&](int v) { return EraJS::ValidArtifactNumber(EraJS::ArtifactSchema::hasSpell, v, CAPACITY, 12); });
        const char *fields[] = {"primary.attack", "primary.defence", "primary.spellPower", "primary.knowledge"};
        for (int skill = 0; skill < 4; ++skill)
        {
            int value;
            if (ReadInt(key, fields[skill], value))
            {
                if (value >= -128 && value <= 127)
                {
                    primary[skill] = static_cast<signed char>(value);
                    primaryChanged[skill] = true;
                }
                else Log("Primary bonus outside signed-byte range: %s.%s", key.c_str(), fields[skill]);
            }
        }
        int value;
        if (ReadInt(key, "ai.value", value) && value >= 0 && value <= 10000000)
        {
            aiValue = value;
            hasAiValue = true;
        }
        // Numeric links alone do not update the combination table or its masks.
        for (auto field : {"comboArtifactId", "partOfComboArtifactId"})
            if (ReadInt(key, field, value) && value != -1)
                Log("Combination JSON is deferred; ignored %s.%s", key.c_str(), field);
    }
}
} // namespace artifacts
