#include "../../include/Plugin.h"
namespace gbf
{
void InstallRenameMap(PatcherInstance *);
namespace
{
// Applied late, after WoG initialization (matching the old plugin).
_LHF_(AfterWogInit)
{
    if (patcher)
    {
        // Fix WoG Altar of Sacrifice RMB text pointing to invalid memory.
        patcher->UndoAllAt(0x5641A4);
        // See legacy/src/HdModFixes.cpp for HD.RenameRandMap patch.
        if (patcher->VarGetValue<bool>("HD.Misc.RenameRandMap", false))
        {
            // HoMM3 H3String and the legacy _HStr_ differ in ownership details;
            // the rewrite below uses H3API's safe Assign instead of old Set.
            // Hook is installed exactly once by this initialization hook.
            InstallRenameMap(instance);
        }
    }
    return EXEC_DEFAULT;
}
void __stdcall RenameMap(HiHook *hook, h3::H3String *name, const char *text, int len)
{
    THISCALL_3(void, hook->GetDefaultFunc(), name, text, len);
    if (!name || !name->String())
        return;
    std::string value(name->String());
    value.erase(std::remove_if(value.begin(), value.end(),
                               [](char ch) { return ch == '\x12' || ch == '\x01' || ch == '\x02'; }),
                value.end());
    name->Assign(value.c_str());
}
} // namespace
void InstallRenameMap(PatcherInstance *pi)
{
    pi->WriteHiHook(0x54A264, CALL_, EXTENDED_, THISCALL_, RenameMap);
}
void InstallRuntimePatches(PatcherInstance *pi)
{
    pi->WriteLoHook(0x4EE1B4, AfterWogInit);
}
} // namespace gbf
