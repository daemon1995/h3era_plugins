#pragma once

#include "HandlersList.h"

// Text files which are loaded as a flat one-column list and are not backed by
// a named game structure. Their public JSON form is:
// era.<txt_name>.<string_id>
class LegacyListHandler
{
    static void ReadList(LPCSTR txtName, LPCSTR *table, const size_t count)
    {
        bool readSuccess = false;
        LPCSTR readResult = nullptr;

        for (size_t i = 0; i < count; i++)
        {
            libc::sprintf(h3_TextBuffer, "era.%s.%u", txtName, static_cast<unsigned>(i));
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                table[i] = readResult;
        }
    }

    static LPCSTR *At(const uintptr_t address) noexcept
    {
        return reinterpret_cast<LPCSTR *>(address);
    }

    static void ReadTextFile(LPCSTR txtName, H3TextFile *file)
    {
        if (!file)
            return;

        bool readSuccess = false;
        LPCSTR readResult = nullptr;
        for (UINT i = 0; i < file->Size(); i++)
        {
            libc::sprintf(h3_TextBuffer, "era.%s.%u", txtName, i);
            readResult = EraJS::read(h3_TextBuffer, readSuccess);
            if (readSuccess)
                (*file)[i] = readResult;
        }
    }

  public:
    static void Init()
    {
        ReadTextFile("genrltxt", H3GeneralText::Get());
        ReadTextFile("advevent", H3AdveventText::Get());

        // Addresses and lengths are taken from TXT-loader pseudocode in
        // export/functions_json.
        ReadList("crgen1", At(0x00691548), 80);
        ReadList("crgen4", At(0x006913A0), 2);
        ReadList("xtrainfo", At(0x006912C8), 28);
        ReadList("randsign", At(0x00696CC0), 25);
        ReadList("campbttn", At(0x006A5920), 67);
        ReadList("mineevnt", At(0x006A5E84), 8);
        ReadList("terrname", At(0x006A5EE8), 10);
        ReadList("objnames", At(0x006A7A50), 232);
        ReadList("restypes", At(0x006A5EC8), 8);
        ReadList("minename", At(0x006A7534), 8);
        ReadList("plcolors", At(0x006A7E60), 8);
        ReadList("priskill", At(0x006A53F4), 4);
        ReadList("skilllev", At(0x006A75D4), 3);
        ReadList("herobios", At(0x006A673C), 163);
        ReadList("castinfo", At(0x006A5C8C), 7);
        ReadList("tvrninfo", At(0x006A5EA4), 8);
        ReadList("hallinfo", At(0x006A748C), 10);
        ReadList("tcommand", At(0x006A5DE8), 35);
        ReadList("overview", At(0x006A7F24), 16);
        ReadList("heroscrn", At(0x006A8078), 33);
        ReadList("tentcolr", At(0x006A7584), 8);
        ReadList("credits", At(0x006A7768), 2);
        ReadList("randtvrn", At(0x00696DE8), 256);
        ReadList("vcdesc", At(0x0069FBC8), 14);
        ReadList("lcdesc", At(0x0069FC70), 4);
        ReadList("turndur", At(0x0069FB9C), 11);
        ReadList("jktext", At(0x0068C764), 73);
    }
};
