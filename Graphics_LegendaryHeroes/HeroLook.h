#pragma once

struct HeroLook
{
    UINT16 faction;
    UINT16 portraitIndex;
    UINT16 background;
    UINT16 original;

    struct
    {
        H3String large;
        H3String small;
    } native;

    static UINT16 ornament;
    static bool interfaceMod;
    static bool forceOverride;
    static HeroLook heroLook[156];

    static bool needRedraw;
    static bool changeButtons;

    static constexpr const char* iniPath = "Runtime\\legend_heroes.ini";
    static constexpr const char* defaultPcxPngNames[4] = { "nhBG_%d.pcx", "nhv%d_%d.pcx", "nhArt_%d.pcx", "nhBP_%d.pcx" };

    static H3String pcxNames[4];

    static void CorrectPcxNames(const UINT heroId);
    static void LoadIniSettings(HeroLook* heroes);
    static void SaveIniSettings(const HeroLook& m_heroWgt, const UINT heroId = -1);
    static void ResetSettings();

    // static void GetNativePortraitName(const UINT heroId, char large[12], char small[12]);
};