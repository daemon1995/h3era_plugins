#include "ObjectDefinitions.h"
#include "ObjectProperties.h"
#include "../headers/Era/eraModList.hpp"

namespace editorObjects
{
static_assert(OBJECT_TYPE_LIMIT == h3::limits::OBJECTS, "Object type limits must match the editor");

const std::vector<std::string> &ObjectDefinitions::Properties()
{
    // Prototypes are startup configuration, not language data. Discover them once.
    static const auto *properties = new std::vector<std::string>(ReadProperties());
    return *properties;
}

std::vector<std::string> ObjectDefinitions::ReadProperties()
{
    std::vector<std::string> result;
    char key[512];
    bool found = false;
    ObjectProperty property;
    // Keep the same mod priority and flat/nested formats as the game plugin.
    for (const auto &mod : modList::GetEraModList(modList::CASE_TO_LOWER))
    {
        for (int i = 0; ; ++i)
        {
            std::snprintf(key, sizeof(key), "RMG.%s.properties.%d", mod.c_str(), i);
            LPCSTR line = EraJS::read(key, found);
            if (!found)
                break;
            if (ParseObjectProperty(line, property))
                result.emplace_back(line);
        }
        for (int group = 0; ; ++group)
        {
            int i = 0;
            for (;; ++i)
            {
                std::snprintf(key, sizeof(key), "RMG.%s.properties.%d.%d", mod.c_str(), group, i);
                LPCSTR line = EraJS::read(key, found);
                if (!found)
                    break;
                if (ParseObjectProperty(line, property))
                    result.emplace_back(line);
            }
            if (!i)
                break;
        }
    }
    for (int type = 0; type < OBJECT_TYPE_LIMIT; ++type)
        for (int subtype = 0; subtype < dwellings::MIN_CAPACITY; ++subtype)
            for (int i = 0; ; ++i)
            {
                std::snprintf(key, sizeof(key), "RMG.objectGeneration.%d.%d.properties.%d", type, subtype, i);
                LPCSTR line = EraJS::read(key, found);
                if (!found)
                    break;
                if (ParseObjectProperty(line, property) && property.type == type && property.subtype == subtype)
                    result.emplace_back(line);
            }
    return result;
}

H3TextFile *__stdcall ObjectDefinitions::LoadObjectPrototypes(HiHook *h, LPCSTR fileName)
{
    auto *txt = CDECL_1(H3TextFile *, h->GetDefaultFunc(), fileName);
    if (!txt || _stricmp(fileName, "ZEObjts.txt") != 0)
        return txt;
    const auto &additions = Properties();
    if (additions.empty())
        return txt;
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
    std::vector<std::string> lines(native.first, native.first + count + 1);
    MergeObjectProperties(lines, additions);
    auto *data = CDECL_1(LPCSTR *, 0x504A8E, lines.size() * sizeof(LPCSTR));
    if (!data)
        return txt;
    for (size_t i = 0; i < lines.size(); ++i)
        data[i] = Eramap::ToStaticStr(lines[i].c_str());
    CDECL_1(void, 0x504AB7, native.first);
    native.first = data;
    native.last = data + lines.size();
    native.capacity = native.last;
    return txt;
}

void ObjectDefinitions::Init()
{
    _PI->WriteHiHook(0x4909E2, CDECL_, LoadObjectPrototypes);
}
} // namespace editorObjects
