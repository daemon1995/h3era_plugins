#pragma once
#include <cstdint>

// Include after header.h/H3API. Patcher variables are process-local and shared by
// all DLLs; neither a plugin filename nor an export lookup is part of discovery.
namespace era_services
{
struct Header { std::uint32_t structSize, version; };
template <class Api> const Api *Find(const char *key, unsigned version = 1)
{
    if (!key || !*key) return nullptr;
    auto *patcher = GetPatcher();
    const auto *api = patcher ? patcher->VarGetValue<const Api *>(key, nullptr) : nullptr;
    return api && api->version == version && api->structSize >= sizeof(Api) ? api : nullptr;
}
template <class Api> bool Publish(const char *key, const Api *api)
{
    auto *patcher = GetPatcher();
    if (!patcher || !key || !*key || !api || api->structSize < sizeof(Api) || !api->version) return false;
    auto *variable = patcher->VarFind(key);
    if (variable && variable->GetValue() && variable->GetValue() != reinterpret_cast<std::uintptr_t>(api)) return false;
    if (!variable) variable = patcher->VarInit(key, reinterpret_cast<std::uintptr_t>(api));
    if (!variable) return false;
    *variable->GetPValue() = reinterpret_cast<std::uintptr_t>(api);
    return true;
}
constexpr const char *ARTIFACT_OWNER = "ERA.Services.ArtifactOwner.v1";
constexpr const char *DWELLING_OWNER = "ERA.Services.DwellingOwner.v1";
constexpr const char *BANK_OWNER = "ERA.Services.BankOwner.v1";
// Presence means that the domain extender owns its native table and field application.
struct Ownership { std::uint32_t structSize = sizeof(Ownership), version = 1; };
}
