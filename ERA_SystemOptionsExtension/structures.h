#pragma once
#pragma pack(push, 4)
struct OriginalConfig
{
    static constexpr unsigned int ADDRESS = 0x6987A8;

    int enemySpeed;
    int playerSpeed;
    int musicVolume;
    int effectsVolume;
    int lastMusicVolume;
    int lastEffectsVolume;
    int autoSave;
    int showRoute;
    int moveReminder;
    int quickCombat;
    int videoSubtitles;
    int buildingOutlines;
    int spellBookAnimation;
    int mapScrollSpeed;
    int blackoutComputer;
    int autoCreatures;
    int autoSpells;
    int autoCatapult;
    int autoBallista;
    int autoFirstAidTent;
    int preferBink;
    int mainGameShowMenu;
    int screenX;
    int screenY;
    int fullScreen;
    int showHexGrid;
    int cursorShadow;
    int movementShadow;
    int combatViewArmy[7];
    char dontTryRedbook;
    char firstInstall;
    char padding[2];
    char cUniqueSystemID[4];
    int animationSpeed;
    char cCurRemoteReceive[13];
    char cRemoteReceiveDiff[13];
    char cCurRemoteSend[13];
    char cNetName[21];

  public:
    inline static OriginalConfig &Get()
    {
        return *reinterpret_cast<OriginalConfig *>(ADDRESS);
    }
};

struct AdditionalConfig
{

    static constexpr LPCSTR sectionName = "Settings.Extra";
    static constexpr LPCSTR fileName = "heroes3.ini";
    struct SettingsEntry
    {
        LPCSTR keyName = nullptr;
        int value = 0;
        int defaultValue = 0;

      public:
        explicit operator int &() noexcept
        {
            return value;
        }
        explicit operator const int &() const noexcept
        {
            return value;
        }
    };

    SettingsEntry backgroundSound{"Sound.BackgroundLooping", 1, 1};
    SettingsEntry doubleClickSplit{"Sound.ButtonDoubleClick", 0, 0};
    SettingsEntry quickAutoResolve{"Combat.QuickAutoResolve", 0, 0};
    SettingsEntry showCreatureHealthBar{"Combat.ShowCreatureHealthBar", 0, 0};

  private:
    inline SettingsEntry *begin() noexcept
    {
        return &backgroundSound;
    }
    // SettingsEntry* end() noexcept
    //{
    //     return begin() + Count;
    // }
    inline SettingsEntry *data() noexcept
    {
        return &backgroundSound;
    }

  public:
    inline static AdditionalConfig &Get()
    {
        static AdditionalConfig instance;
        return instance;
    }
    inline static BOOL Load()
    {
        AdditionalConfig &instance = Get();
        constexpr size_t length = sizeof(AdditionalConfig) / sizeof(SettingsEntry);
        auto array = instance.data();
        for (size_t i = 0; i < length; i++)
        {
            auto &entry = array[i];
            if (Era::ReadStrFromIni(entry.keyName, sectionName, fileName, h3_TextBuffer))
            {
                entry.value = atoi(h3_TextBuffer);
            }
        }
        return 1;
    }
    static BOOL Save()
    {
        AdditionalConfig &instance = Get();
        constexpr size_t length = sizeof(AdditionalConfig) / sizeof(SettingsEntry);
        auto array = instance.data();
        for (size_t i = 0; i < length; i++)
        {
            auto &entry = array[i];
            libc::sprintf(h3_TextBuffer, "%d", entry.value);
            Era::WriteStrToIni(entry.keyName, h3_TextBuffer, sectionName, fileName);
        }
        return 1;
    }
};
#pragma pack(pop)
