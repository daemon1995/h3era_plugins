#pragma once

// Include after framework.h. Never publish readiness for a partially installed group.
namespace erm_functions
{
inline void Log(const char *component, const char *message) noexcept
{
    Era::WriteLog("ERA_RuntimeServices", component, message);
}
template <std::size_t N> bool CheckHooks(Patch *const (&hooks)[N], const char *component) noexcept
{
    for (auto *hook : hooks)
        if (!hook || !hook->IsApplied())
        {
            for (auto *installed : hooks)
                if (installed) installed->Destroy();
            Log(component, "Hook installation failed; the entire group was removed.");
            return false;
        }
    return true;
}
}
