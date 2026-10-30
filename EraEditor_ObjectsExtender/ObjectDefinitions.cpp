#include "ObjectDefinitions.h"
#include "ObjectProperties.h"
#include "ObjectExtenders/DwellingsExtender.h"
#include "TextHandlers/ArtifactHandler.h"
#include "../headers/EraPluginsAPI/ObjectDefinitionsAPI.h"
#include <array>
#include "../headers/Era/eraModList.hpp"

namespace editorObjects
{
namespace {
std::vector<std::string> &Snapshot() { static auto *lines = new std::vector<std::string>; return *lines; }
std::array<int, 232> originalLimits;
std::array<int, OBJECT_TYPE_LIMIT> subtypeCounts{};
bool captured = false;
unsigned __stdcall DefinitionCount() { return unsigned(Snapshot().size()); }
const char *__stdcall DefinitionAt(int index) { return index >= 0 && unsigned(index) < Snapshot().size() ? Snapshot()[index].c_str() : nullptr; }
int __stdcall OriginalLimit(int type) { return captured && type >= 0 && type < 232 ? originalLimits[type] : -1; }
}

static_assert(OBJECT_TYPE_LIMIT == h3::limits::OBJECTS, "Object type limits must match the editor");

int __stdcall ObjectDefinitions::SubtypeCount(int type)
{
    return IsObjectType(type) ? subtypeCounts[type] : 0;
}

const std::vector<std::string> &ObjectDefinitions::Properties()
{
    // Prototypes are startup configuration, not language data. Discover them once.
    static const auto *properties = new std::vector<std::string>(ReadProperties());
    return *properties;
}

std::vector<std::string> ObjectDefinitions::ReadProperties()
{
    return object_config::CollectDefinitions([](const char *key) { return EraJS::read(key); },
        modList::GetEraModList(modList::CASE_TO_LOWER));
}

H3TextFile *__stdcall ObjectDefinitions::LoadObjectPrototypes(HiHook *h, LPCSTR fileName)
{
    auto *txt = CDECL_1(H3TextFile *, h->GetDefaultFunc(), fileName);
    if (!txt || _stricmp(fileName, "ZEObjts.txt") != 0)
        return txt;
    const auto &additions = Properties();
    // H3TextFile's vector is layout-compatible, but its game allocation helpers are not.
    struct NativeLines
    {
        DWORD allocator;
        LPCSTR *first;
        LPCSTR *last;
        LPCSTR *capacity;
    };
    static_assert(sizeof(NativeLines) == 16, "Editor text vector layout changed");
    auto &native = *reinterpret_cast<NativeLines *>(reinterpret_cast<BYTE *>(txt) + 0x1C);
    if (!native.first || native.last <= native.first)
        return txt;
    int count = 0;
    if (!ReadObjectCount(native.first[0], count) || size_t(count) >= size_t(native.last - native.first))
        return txt;
    std::vector<std::string> lines;
    if (!ReadObjectDefinitions(native.first, size_t(native.last - native.first), lines))
        return txt;
    if (!captured)
    {
        originalLimits.fill(-1);
        ObjectProperty property;
        for (size_t i = 1; i < lines.size(); ++i)
            if (ParseObjectProperty(lines[i], property)) originalLimits[property.type] = (std::max)(originalLimits[property.type], property.subtype);
        captured = true;
    }
    MergeObjectProperties(lines, additions);
    ArtifactHandler::AppendObjectDefinitions(lines);
    if (!additions.empty() || lines.size() != size_t(count) + 1)
    {
        auto *data = CDECL_1(LPCSTR *, 0x504A8E, lines.size() * sizeof(LPCSTR));
        if (!data)
            return txt;
        for (size_t i = 0; i < lines.size(); ++i)
            data[i] = Eramap::ToStaticStr(lines[i].c_str());
        CDECL_1(void, 0x504AB7, native.first);
        native.first = data;
        native.last = data + lines.size();
        native.capacity = native.last;
    }
    Snapshot().assign(lines.begin() + 1, lines.end());
    subtypeCounts = object_config::SubtypeCounts(Snapshot());
    DwellingsExtender::DefinitionsLoaded();
    return txt;
}

void ObjectDefinitions::Init()
{
    static const era_objects::DefinitionsApi api{sizeof(api), 1, DefinitionCount, DefinitionAt, OriginalLimit, SubtypeCount};
    era_services::Publish(era_objects::DEFINITIONS, &api);
    _PI->WriteHiHook(0x4909E2, CDECL_, LoadObjectPrototypes);
}
} // namespace editorObjects
