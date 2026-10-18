#include "ObjectText.h"
#include "../headers/Era/ObjectText.hpp"

namespace editorObjects
{
LPCSTR ObjectText::NativeTypeName(int type) noexcept
{
    if (type < 0 || type >= h3::limits::OBJECTS)
        return "";
    // Native object-kind metadata: 16-byte records, name at +4. Respect relocations.
    const DWORD table = DwordAt(0x57D648);
    if (table)
        if (LPCSTR name = *reinterpret_cast<LPCSTR *>(table + type * 16 + 4))
            return name;
    return "";
}

_LHF_(ObjectText::PaletteHint)
{
    // ESI points to a palette prototype (TObjectProps), before subtype-specific lookups.
    const int type = IntAt(c->esi + 0x1C);
    const int subtype = IntAt(c->esi + 0x20);
    std::string hint;
    if (!EraJS::ReadObjectHint(type, subtype, NativeTypeName(type), hint))
        return EXEC_DEFAULT;
    // The hook displaces MOV EAX,[ESI+1C] and the temporary string's cleanup-state assignment.
    IntAt(c->ebp - 4) = 0;
    c->esi = reinterpret_cast<int>(Eramap::ToStaticStr(hint.c_str()));
    c->return_address = 0x48CCF4;
    return NO_EXEC_DEFAULT;
}

_LHF_(ObjectText::MapHint)
{
    // EAX is TMapObject; its shared definition starts with a 12-byte resource chain.
    const DWORD definition = DwordAt(c->eax + 4);
    if (!definition)
        return EXEC_DEFAULT;
    const int type = IntAt(definition + 0x28);
    const int subtype = IntAt(definition + 0x2C);
    std::string hint;
    if (!EraJS::ReadObjectHint(type, subtype, NativeTypeName(type), hint))
        return EXEC_DEFAULT;

    // Emulate the virtual GetName result using the editor's own string implementation.
    // No game H3String allocator is valid in this process. The caller destroys this temporary.
    auto *result = reinterpret_cast<void *>(c->ebp - 0x10);
    std::memset(result, 0, 16);
    THISCALL_3(void, 0x401A6A, result, hint.c_str(), hint.size());
    c->eax = reinterpret_cast<int>(result);
    // Skip the indirect virtual call, including its result-buffer argument push.
    c->return_address = 0x46EE3C;
    return NO_EXEC_DEFAULT;
}

void ObjectText::Init()
{
    _PI->WriteLoHook(0x48CBF3, PaletteHint);
    _PI->WriteLoHook(0x46EE31, MapHint);
}
} // namespace editorObjects
