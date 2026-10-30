#include "TextOverrides.h"
#include <unordered_map>

namespace editorObjects
{
namespace
{
struct Binding
{
    LPCSTR original, published;
    std::string key;
};
using Bindings = std::unordered_map<LPCSTR *, Binding>;
Bindings &GetBindings()
{
    static auto *bindings = new Bindings;
    return *bindings;
}
void Update(LPCSTR &slot, Binding &binding)
{
    bool found = false;
    LPCSTR text = EraJS::read(binding.key, found);
    slot = found ? text : binding.original;
    binding.published = slot;
}
void __stdcall Reload(Eramap::TEvent *)
{
    for (auto &entry : GetBindings())
        if (*entry.first == entry.second.published)
            Update(*entry.first, entry.second);
}
}

void TextOverrides::Init()
{
    Eramap::RegisterHandler(Reload, "OnAfterReloadLanguageData");
}
void TextOverrides::Apply(LPCSTR &slot, LPCSTR key)
{
    auto &bindings = GetBindings();
    auto entry = bindings.find(&slot);
    if (entry == bindings.end())
        entry = bindings.emplace(&slot, Binding{slot, slot, key}).first;
    else
    {
        if (slot != entry->second.published)
            entry->second.original = slot;
        entry->second.key = key;
    }
    Update(slot, entry->second);
}
void TextOverrides::Forget(const void *base, size_t bytes)
{
    const auto begin = reinterpret_cast<uintptr_t>(base);
    auto &bindings = GetBindings();
    for (auto it = bindings.begin(); it != bindings.end();)
    {
        const auto slot = reinterpret_cast<uintptr_t>(it->first);
        if (slot >= begin && slot - begin < bytes)
            it = bindings.erase(it);
        else
            ++it;
    }
}
}
