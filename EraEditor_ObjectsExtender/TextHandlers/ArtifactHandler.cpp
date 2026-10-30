#include "ArtifactHandler.h"
#include "TextOverrides.h"
#include "../ObjectProperties.h"
#include <array>
#include <sstream>

namespace
{
constexpr int CONFIG_LIMIT = 1024;
struct Definition { bool declared = false; };
struct Configuration
{
    std::array<Definition, CONFIG_LIMIT> entries;
    int count = 0;
    Configuration()
    {
        const char *fields[] = {"id", "name", "description", "cost", "position", "type", "disabled", "hasSpell",
            "event", "primary.attack", "primary.defence", "primary.spellPower", "primary.knowledge", "ai.value", "image", "mapDef"};
        for (int id = 0; id < CONFIG_LIMIT; ++id)
            for (const char *field : fields)
                if (!EraJS::isEmpty(("era.artifacts." + std::to_string(id) + "." + field).c_str()))
                {
                    entries[id].declared = true;
                    count = (std::max)(count, id + 1);
                    break;
                }
    }
};
const Configuration &Config() { static const auto *config = new Configuration; return *config; }

DWORD __stdcall ReadEditorIni(HiHook *h, LPCSTR section, LPCSTR key, LPCSTR fallback, LPSTR value, DWORD size, LPCSTR file)
{
    const DWORD result = STDCALL_6(DWORD, h->GetDefaultFunc(), section, key, fallback, value, size, file);
    constexpr char suffix[] = "\\MapEdPatch\\Options.ini";
    if (!section || !key || !file || !value || size < 2 || _stricmp(section, "General") ||
        _stricmp(key, "Number of Artifacts") || std::strlen(file) < sizeof(suffix) - 1 ||
        _stricmp(file + std::strlen(file) - sizeof(suffix) + 1, suffix)) return result;
    int original = 0;
    if (!EraJS::ParseNumber(value, original) || original < 146 || original > CONFIG_LIMIT) return result;
    // Keep the configured map bit-mask storage span. Growing it is a separate migration.
    const int capacity = (original + 31) / 32 * 32;
    const int count = (std::min)((std::max)(original, Config().count), capacity);
    if (count == original) return result;
    const std::string text = std::to_string(count);
    const DWORD length = (std::min)(DWORD(text.size()), size - 1);
    std::memcpy(value, text.data(), length);
    value[length] = 0;
    return length;
}

bool ResourceExists(const std::string &name)
{
    const auto loose = [](const std::string &path) {
        const DWORD flags = GetFileAttributesA(path.c_str());
        return flags != INVALID_FILE_ATTRIBUTES && !(flags & FILE_ATTRIBUTE_DIRECTORY);
    };
    if (loose(name) || loose("Data\\" + name)) return true;
    // GrayFace's TEra.FindFileLod uses this relocated editor LOD table (100 records).
    const DWORD table = DwordAt(0x4DACE6);
    for (int index = 99; index >= 0; --index)
    {
        const DWORD lod = table + index * 400;
        if (DwordAt(lod) && THISCALL_2(BOOL8, 0x4E48F0, lod + 4, name.c_str())) return true;
    }
    return false;
}
}

static H3TextTable *__stdcall LoadArtTraitsFile(HiHook *h, LPCSTR fileName)
{
    auto &state = ArtifactHandler::Handler();
    editorObjects::TextOverrides::Forget(state.table, size_t(state.objectsCount) * sizeof(H3ArtifactSetup));
    state = {};
    auto *txt = CDECL_1(H3TextTable *, h->GetDefaultFunc(), fileName);
    if (!txt) return nullptr;
    const DWORD rows = txt->CountRows();
    DWORD expectedRows = DwordAt(0x40365E + 1) / sizeof(LPCSTR);
    // The native do/while reads row 2 even when its end operand equals 8.
    // Leave short resources to the native minimum-row check; keep its ownership.
    if (rows < 3 || expectedRows < 3) return txt;
    const auto *table = *reinterpret_cast<H3ArtifactSetup **>(0x4036EC + 1);
    // Raising only the TXT loop limit does not grow the original 144-slot array.
    if (reinterpret_cast<DWORD>(table) == 0x59A2E0) expectedRows = (std::min)(expectedRows, DWORD(146));
    state.rowCount = rows;
    state.objectsCount = (std::min)(rows, expectedRows) - 2;
    state.loadedObjectsCount = state.objectsCount;
    // TXT end operands may have been clamped by a previous reload. The bitset
    // range still records the allocated table size published by GrayFace.
    state.expectedObjectsCount = DwordAt(0x403B16);
    if (reinterpret_cast<DWORD>(table) == 0x59A2E0)
        state.expectedObjectsCount = (std::min)(state.expectedObjectsCount, DWORD(144));
    if (state.expectedObjectsCount > CONFIG_LIMIT)
        state.expectedObjectsCount = expectedRows - 2;
    const DWORD bytes = (std::min)(rows, expectedRows) * sizeof(LPCSTR);
    if (DwordAt(0x40365E + 1) > bytes) _PI->WriteDword(0x40365E + 1, bytes);
    if (DwordAt(0x40369D + 2) > bytes) _PI->WriteDword(0x40369D + 2, bytes);
    if (DwordAt(0x40382E + 3) > bytes) _PI->WriteDword(0x40382E + 3, bytes);
    return txt;
}

static bool __stdcall LoadAllArtifactTxtFiles(HiHook *h)
{
    const bool loaded = CDECL_0(bool, h->GetDefaultFunc());
    if (loaded)
    {
        auto *table = *reinterpret_cast<H3ArtifactSetup **>(0x4036EC + 1);
        auto &state = ArtifactHandler::Handler();
        state.table = table;
        state.objectsCount = (std::max)(state.objectsCount, (std::min)(DWORD(Config().count), state.expectedObjectsCount));
        if (DWORD(Config().count) > state.expectedObjectsCount && Eramap::WriteLog)
            Eramap::WriteLog("Artifact handler", "Editor capacity",
                "Some artifact IDs exceed the configured map-mask span and were not added. Mask expansion is not installed.");
        const auto count = state.objectsCount;
        for (DWORD i = 0; table && i < count; ++i)
        {
            if (i >= state.loadedObjectsCount)
            {
                table[i] = H3ArtifactSetup{};
                table[i].disabled = !Config().entries[i].declared;
                if (Config().entries[i].declared)
                    table[i].name = Eramap::ToStaticStr(("Artifact #" + std::to_string(i)).c_str());
            }
            char key[80];
            std::snprintf(key, sizeof(key), EraJS::ArtifactSchema::name, i);
            editorObjects::TextOverrides::Apply(table[i].name, key);
            std::snprintf(key, sizeof(key), EraJS::ArtifactSchema::description, i);
            editorObjects::TextOverrides::Apply(table[i].description, key);
            EraJS::ReadArtifact(table[i], static_cast<int>(i), static_cast<int>(count));
            if (i >= state.loadedObjectsCount)
            {
                table[i].comboArtifactId = eCombinationArtifacts::NONE;
                table[i].partOfComboArtifactId = eCombinationArtifacts::NONE;
            }
        }
    }
    return loaded;
}
int ArtifactHandler::GetArtifactsNumber() noexcept { return artifactsHandler.objectsCount; }
void ArtifactHandler::Init()
{
    // Runs before GrayFace's LoadOptions/MakeArtList: let it own all relocations
    // and its private ArtList used by the object palette and properties window.
    _PI->WriteHiHook(reinterpret_cast<DWORD>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetPrivateProfileStringA")), STDCALL_, ReadEditorIni);
    _PI->WriteHiHook(0x45C72C, CDECL_, LoadAllArtifactTxtFiles);
    _PI->WriteHiHook(0x40362E, CDECL_, LoadArtTraitsFile);
}

void ArtifactHandler::AppendObjectDefinitions(std::vector<std::string> &lines)
{
    using namespace editorObjects;
    std::array<bool, CONFIG_LIMIT> present{};
    std::string prototype;
    ObjectProperty property;
    for (size_t i = 1; i < lines.size(); ++i)
        if (ParseObjectProperty(lines[i], property) && property.type == 5)
        {
            if (property.subtype < CONFIG_LIMIT) present[property.subtype] = true;
            if (prototype.empty() || property.subtype == 7) prototype = lines[i];
        }
    if (prototype.empty()) return;
    std::array<std::string, 9> fields;
    std::istringstream input(prototype);
    for (auto &field : fields) input >> field;
    const std::string fallback = fields[0];
    const auto &state = Handler();
    for (DWORD id = state.loadedObjectsCount; id < state.objectsCount && id < CONFIG_LIMIT; ++id)
    {
        if (!Config().entries[id].declared || present[id]) continue;
        bool found = false;
        const char *name = EraJS::read("era.artifacts." + std::to_string(id) + ".mapDef", found);
        std::string def = found ? name : "";
        if (def.empty())
        {
            char name[16];
            std::snprintf(name, sizeof(name), "AVA%04u.def", id);
            def = ResourceExists(name) ? name : fallback;
        }
        else if (!ResourceExists(def)) def = fallback;
        fields[0] = def;
        fields[6] = std::to_string(id);
        std::string line;
        for (const auto &field : fields) { if (!line.empty()) line += ' '; line += field; }
        lines.push_back(line);
    }
    lines[0] = std::to_string(lines.size() - 1);
}
