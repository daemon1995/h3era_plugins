/////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// ИСПРАВЛЕНИЕ БАГОВ HD MOD /////////////////////////////////////////
#include <algorithm>

namespace HdMod
{

void __stdcall RandomMap_WriteHeader_AssignName(HiHook *h, _HStr_ *this_, LPCSTR mapName, int len)
{
    CALL_3(_HStr_ *, __thiscall, h->GetDefaultFunc(), this_, mapName, len);

    std::string str(this_->c_str);
    str.erase(std::remove(str.begin(), str.end(), '\x12'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\x1'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\x2'), str.end());

    this_->Set((char *)str.c_str());
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void HdModFixes(Patcher *_P, PatcherInstance *_PI)
{
    // исправление поломки имени случайной карты при генерации со включённым тывиком "HD.Misc.RenameRandMap" в
    // настройках HD Mod
    const int renameRandMap = _P->VarGetValue<bool>("HD.Misc.RenameRandMap", FALSE);
    if (renameRandMap)
    {
        _PI->WriteHiHook(0x54A264, CALL_, EXTENDED_, THISCALL_, RandomMap_WriteHeader_AssignName);

        // H3API solution
        /**

        void __stdcall HiHook_Test3(HiHook* h, H3String* this_, LPCSTR mapName, int len)
        {
            THISCALL_3(char, h->GetDefaultFunc(), this_, mapName, len);
            this_->Remove("\x12");
            this_->Remove("\x1");
            this_->Remove("\x2");
        }

        _PI->WriteHiHook(0x54A264, CALL_, EXTENDED_, THISCALL_, HiHook_Test3);
        */
    }
}
} // namespace HdMod
