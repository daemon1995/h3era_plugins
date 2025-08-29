#pragma once

class WgtSettings
{
    // const std::string jsonKy;
    WgtSettings();

  public:
    POINT startPos;
    POINT arrows[2];
    int width = 0;
    int height = 0;

    struct Backpack
    {
        POINT pos;
        int altitude = 0;
        int interval = 0;

    } backpack;

    int bgId = 0;

    static void loadArtSettings() noexcept;
    // void loadSettings(std::string jsonKey);

    // virtual void loadAdvSettings(std::string jsonKey);
    static POINT artSlotPositions[19];
    static POINT sacrifaceArtSlotPositions[19];

    struct ButtonsAlignment
    {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        int totalWidth = 0;
        H3Vector<std::string> buttonsToAline;
    };
    static ButtonsAlignment buttonsAlignment;
    WgtSettings(const std::string &jsonKey);
};

class HeroWgt : public WgtSettings
{
  public:
    struct Bttn
    {
        POINT pos;
        int id = 0;
        bool isOn = false;
        char *defName = nullptr;

    } npcBttn;

    struct MgrBttn : public Bttn
    {
        POINT altPos;
        char *hint = nullptr;
    } mgrBttn;

    HeroWgt(const std::string &jsonKey);

    // void loadAdvSettings(std::string jsonKey) override ;
};

class MeetWgt : public WgtSettings
{
  public:
    struct Bg
    {
        POINT pos = {0, 0};
        int width = 0;
        int height = 0;
    } bg;
    MeetWgt(const std::string &jsonKey);

    // void loadAdvSettings(std::string jsonKey) override;
};
