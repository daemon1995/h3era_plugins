#pragma once
#include "pch.h"

class WgtSettings
{
    // const std::string jsonKy;
    WgtSettings();

  public:
    POINT startPos;
    POINT arrows[2];
    int width;
    int height;

    struct Backpack
    {
        POINT pos;
        int altitude;
        int interval;

    } backpack;

    int bgId;

    static void loadArtSettings() noexcept;
    // void loadSettings(std::string jsonKey);

    // virtual void loadAdvSettings(std::string jsonKey);
    static POINT artSlotPositions[19];
    static POINT sacrifaceArtSlotPositions[19];

    struct ButtonsAlignment
    {
        int x;
        int y;
        int width;
        int height;
        int totalWidth;
        H3Vector<std::string> buttonsToAline;

    } static buttonsAlignment;
    WgtSettings(const std::string &jsonKey);
};

class HeroWgt : public WgtSettings
{
  public:
    struct Bttn
    {
        POINT pos;
        int id;
        bool isOn;
        char *defName;

    } npcBttn;

    struct MgrBttn : public Bttn
    {
        POINT altPos;
        char *hint;
    } mgrBttn;

    HeroWgt(const std::string &jsonKey);

    // void loadAdvSettings(std::string jsonKey) override ;
};

class MeetWgt : public WgtSettings
{
  public:
    struct Bg
    {
        POINT pos;
        int width;
        int height;
    } bg;
    MeetWgt(const std::string &jsonKey);

    // void loadAdvSettings(std::string jsonKey) override;
};
