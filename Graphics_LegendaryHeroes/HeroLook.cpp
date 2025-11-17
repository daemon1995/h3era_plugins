#include "pch.h"

UINT16 HeroLook::ornament;
bool HeroLook::interfaceMod;
bool HeroLook::forceOverride;
bool HeroLook::needRedraw = false;
bool HeroLook::changeButtons = false;

H3String HeroLook::pcxNames[4];

constexpr UINT16 CAMPAIGN_FACTION_ID = 99;
HeroLook *HeroLook::heroLook = nullptr;
int HeroLook::heroCount = h3::limits::HEROES;
char HeroLook::portraitNameBuffer[13] = {};

// [HEROES_MAX_AMOUNT] ;

// UINT16 HeroLook::ornament;
// constexpr const char* HeroLook::pcxPngNames = {""};

constexpr const char *NHD_EMPTY_PCX = "nhd_look.pcx";

void HeroLook::CorrectPcxNames(const UINT heroId)
{

    HeroLook &m_heroWgt = heroLook[heroId];
    pcxNames[0] = H3String::Format(defaultPcxPngNames[0], m_heroWgt.background);
    pcxNames[1] = H3String::Format(defaultPcxPngNames[1], m_heroWgt.faction, m_heroWgt.portraitIndex);
    pcxNames[2] = H3String::Format(defaultPcxPngNames[2], m_heroWgt.ornament);
    pcxNames[3] = H3String::Format(defaultPcxPngNames[3], m_heroWgt.ornament);

    // reset picture if doesn't exist
    for (size_t i = 0; i < 4; i++)
        if (!Era::PcxPngExists(pcxNames[i].String()))
            pcxNames[i] = NHD_EMPTY_PCX;
}

void HeroLook::SaveIniSettings(const HeroLook &m_heroWgt, const UINT heroId)
{
    constexpr int SIZE = 4;

    const UINT16 values[SIZE] = {m_heroWgt.faction, m_heroWgt.portraitIndex, m_heroWgt.background, m_heroWgt.original};
    constexpr const char *keys[SIZE] = {"faction", "portraitIndex", "background", "originalPortrait"};

    for (size_t i = 0; i < SIZE; i++)
    {
        Era::WriteStrToIni(keys[i], std::to_string(values[i]).c_str(), std::to_string(heroId).c_str(), iniPath);
    }
    Era::WriteStrToIni("ornament", std::to_string(m_heroWgt.ornament).c_str(), "main", iniPath);
    Era::SaveIni(iniPath);
}

void HeroLook::ResetSettings()
{
    constexpr int SIZE = 3;                                                        // 4;
    constexpr const char *keys[SIZE] = {"faction", "portraitIndex", "background"}; // , "originalPortrait"};

    heroCount = P_HeroCount;
    heroLook = new HeroLook[heroCount];
    libc::sprintf(h3_TextBuffer, "%d", 0); // set default buffer

    if (Era::ReadStrFromIni("ornament", "main", iniPath, h3_TextBuffer))
        ornament = atoi(h3_TextBuffer);
    else
        ornament = 4;
    for (size_t i = 0; i < 4; i++)
    {
        pcxNames[i] = NHD_EMPTY_PCX;
    }

    // sprintf(h3_TextBuffer, "%d", 0); // set default buffer

    bool forceOverride = false;

    if (Era::ReadStrFromIni("originalPortraitsOverride", "main", iniPath, h3_TextBuffer))
        forceOverride = atoi(h3_TextBuffer);
    int index = 0;
    int campaignInex = 0;
    for (int i = 0; i < heroCount; i++)
    {
        if (i && P_HeroInfo[i].heroClass / 2 != P_HeroInfo[i - 1].heroClass / 2) // set next class type
            index = 0;

        heroLook[i].native.small = P_HeroInfo[i].smallPortrait;
        heroLook[i].native.large = P_HeroInfo[i].largePortrait;

        heroLook[i].portraitIndex = index++;
        heroLook[i].background = P_HeroInfo[i].heroClass / 2;
        heroLook[i].faction = P_HeroInfo[i].heroClass / 2;
        heroLook[i].original = false;

        if (Era::ReadStrFromIni("originalPortrait", std::to_string(i).c_str(), iniPath, h3_TextBuffer))
            heroLook[i].original = atoi(h3_TextBuffer);

        if (P_HeroInfo[i].campaignHero || i == eHero::SIR_MULLICH)
        {
            heroLook[i].portraitIndex = campaignInex++;
            --index;
            heroLook[i].faction = CAMPAIGN_FACTION_ID;
        }

        if (!forceOverride && !heroLook[i].original)
        {
            AssignPngPortrait(i);

            UINT16 *values[SIZE] = {&heroLook[i].faction, &heroLook[i].portraitIndex,
                                    &heroLook[i].background}; // , &heroLook[i].original};

            for (size_t j = 0; j < SIZE; j++)
                if (Era::ReadStrFromIni(keys[j], std::to_string(i).c_str(), iniPath, h3_TextBuffer))
                    *values[j] = libc::atoi(h3_TextBuffer);
        }
    }
}
void HeroLook::AssignPngPortrait(const UINT heroId)
{

    libc::sprintf(portraitNameBuffer, "nhl%d_%d.pcx", heroLook[heroId].faction, heroLook[heroId].portraitIndex);
    if (Era::PcxPngExists(portraitNameBuffer))
        libc::sprintf(const_cast<char *>(P_HeroInfo[heroId].largePortrait), "%s", portraitNameBuffer);

    libc::sprintf(portraitNameBuffer, "nhs%d_%d.pcx", heroLook[heroId].faction, heroLook[heroId].portraitIndex);
    if (Era::PcxPngExists(portraitNameBuffer))
        libc::sprintf(const_cast<char *>(P_HeroInfo[heroId].smallPortrait), "%s", portraitNameBuffer);
}
DllExport void SetNymDependecies(bool newInterMod)
{
    HeroLook::interfaceMod = newInterMod;
}

DllExport void GetNativePortraitName(const UINT heroId, char *large, char *small)
{

    if (heroId < HeroLook::heroCount)
    {
        libc::sprintf(large, HeroLook::heroLook[heroId].native.large.String());
        libc::sprintf(small, HeroLook::heroLook[heroId].native.small.String());
    }

    return;
}
DllExport void GetPortaitStatus(int *heroesArray)
{

    for (size_t i = 0; i < HeroLook::heroCount; i++)
    {
        heroesArray[i] = HeroLook::heroLook[i].original;
    }
    return;
}

DllExport void GetCurrentPortraitName(const UINT heroId, char *large, char *small)
{

    if (heroId < HeroLook::heroCount)
    {
        libc::sprintf(large, P_HeroInfo[heroId].largePortrait);
        libc::sprintf(small, P_HeroInfo[heroId].smallPortrait);
    }

    return;
}

DllExport void GetModPortraitName(const UINT heroId, char *large, char *small)
{

    if (heroId >= 0 && heroId < HeroLook::heroCount)
    {
        auto &h = HeroLook::heroLook[heroId];

        libc::sprintf(large, "nhl%d_%d.pcx", h.faction, h.portraitIndex);
        libc::sprintf(small, "nhs%d_%d.pcx", h.faction, h.portraitIndex);
    }

    return;
}

DllExport void RedrawHeroScreen(bool needRedraw)
{
    HeroLook::needRedraw = needRedraw;
}

DllExport void SetHeroLook(const UINT heroId, const int faction, const int portraitIndex, const int background,
                           const int ornament, const bool original, bool save = 0)
{

    if (heroId < HeroLook::heroCount)
    {
        auto &h = HeroLook::heroLook[heroId];
        h.faction = faction;
        h.portraitIndex = portraitIndex;
        h.background = background;
        h.ornament = ornament;
        h.original = original;
        if (save)
        {
            h.SaveIniSettings(h, heroId);
        }
    }

    return;
}

DllExport void GetHeroLook(const UINT heroId, int *faction, int *portraitIndex, int *background, int *ornament,
                           bool *original)
{

    if (heroId < HeroLook::heroCount)
    {
        auto &h = HeroLook::heroLook[heroId];
        if (faction)
            *faction = h.faction;
        if (portraitIndex)
            *portraitIndex = h.portraitIndex;
        if (background)
            *background = h.background;
        if (ornament)
            *ornament = h.ornament;
        if (original)
            *original = h.original;
    }

    return;
}

DllExport int SetPortraitName(const UINT heroId, LPCSTR large, LPCSTR small)
{
    bool result = heroId < HeroLook::heroCount;

    if (result)
    {
        auto buffer = HeroLook::portraitNameBuffer;
        libc::sprintf(h3_TextBuffer, large);
        libc::sprintf(buffer, P_HeroInfo[heroId].largePortrait);

        libc::sprintf(h3_TextBuffer, small);
        libc::sprintf(buffer, P_HeroInfo[heroId].smallPortrait);
    }

    return result;
}

void HeroLook::LoadIniSettings(HeroLook *heroes)
{
    return;
    constexpr int SIZE = 4;

    // auto m_heroWgt = heroes[0];
    constexpr const char *keys[SIZE] = {"faction", "portraitIndex", "background", "originalPortrait"};

    libc::sprintf(h3_TextBuffer, "%d", 0); // set default buffer

    if (Era::ReadStrFromIni("ornament", "main", iniPath, h3_TextBuffer))
        ornament = atoi(h3_TextBuffer);

    for (size_t i = 0; i < HeroLook::heroCount; i++)
    {
        UINT16 *values[SIZE] = {&heroLook[i].faction, &heroLook[i].portraitIndex, &heroLook[i].background,
                                &heroLook[i].original};

        for (size_t j = 0; j < SIZE; j++)
            if (Era::ReadStrFromIni(keys[j], std::to_string(i).c_str(), iniPath, h3_TextBuffer))
                *values[j] = atoi(h3_TextBuffer);

        if (heroLook[i].original == false)
        {
            // const char* text = H3String::Format("nhl%d_%d.pcx", *values[0], *values[1]).String();
            //	sprintf(const_cast<char*>(P_HeroInfo[i].largePortrait), "nhl%d_%d.pcx", *values[0], *values[1]);
            // sprintf(P_HeroInfo[i].smallPortrait, "nhs%d_%d.pcx", *values[0], *values[1]);
            //_tcs
            // strcpy(P_HeroInfo[i].smallPortrait)
            //	P_HeroInfo[i].smallPortrait = "nhs1_1.pcx";// H3String::Format("nhl%d_%d.pcx", *values[0],
            //*values[1]).String();

            //	SetPortraitName(i, H3String::Format("nhl%d_%d.pcx", *values[0], *values[1]).String(),
            // H3String::Format("nhl%d_%d.pcx", *values[0], *values[1]).String());
        }
    }
}
