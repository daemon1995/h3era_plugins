#include "../framework.h"
#include "../../../headers/EraPluginsAPI/TextOverridesAPI.h"
#include <unordered_map>
#include <string>
#include <new>

namespace text_services
{
namespace
{
struct Binding { const char *original, *published; std::string key, alias; };
using Bindings = std::unordered_map<const char **, Binding>;
Bindings &Get() { static auto *bindings = new Bindings; return *bindings; }
const char *Lookup(const Binding &binding, bool &found)
{
    const char *value = EraJS::read(binding.key, found);
    if (!found && !binding.alias.empty()) value = EraJS::read(binding.alias, found);
    return value;
}
int __stdcall Apply(const char **slot, const char *key, const char *alias)
{
    if (!slot || !key || !*key) return -1;
    try
    {
        auto &bindings = Get();
        auto found = bindings.find(slot);
        if (found == bindings.end()) found = bindings.emplace(slot, Binding{*slot, *slot, key, alias ? alias : ""}).first;
        else
        {
            if (*slot != found->second.published) found->second.original = *slot;
            found->second.key = key; found->second.alias = alias ? alias : "";
        }
        auto &binding = found->second;
        bool exists = false;
        const char *value = Lookup(binding, exists);
        *slot = exists ? value : binding.original;
        binding.published = *slot;
        return exists ? 1 : 0;
    }
    catch (const std::bad_alloc &) { return -1; }
}
void __stdcall Forget(const void *base, std::size_t bytes)
{
    const auto address = reinterpret_cast<std::uintptr_t>(base);
    auto &bindings = Get();
    for (auto it = bindings.begin(); it != bindings.end();)
    {
        const auto slot = reinterpret_cast<std::uintptr_t>(it->first);
        if (slot >= address && slot - address < bytes) it = bindings.erase(it);
        else ++it;
    }
}
void __stdcall Reload()
{
    for (auto &entry : Get())
    {
        auto &binding = entry.second;
        if (*entry.first != binding.published) continue;
        bool found = false;
        const char *value = Lookup(binding, found);
        *entry.first = found ? value : binding.original;
        binding.published = *entry.first;
    }
}
void __stdcall OnReload(Era::TEvent *) { Reload(); }
const era_text::Api api{sizeof(api), 1, Apply, Forget, Reload};
}
void Initialize()
{
    if (era_services::Publish(era_text::SERVICE, &api)) Era::RegisterHandler(OnReload, "OnAfterReloadLanguageData");
}
}
