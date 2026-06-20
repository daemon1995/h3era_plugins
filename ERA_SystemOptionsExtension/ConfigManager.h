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
    int videoQuality;
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
        int maxValue = 1;

      public:
        explicit operator int &() noexcept
        {
            return *reinterpret_cast<int *>(&value);
        }
        explicit operator const int &() const noexcept
        {
            return *reinterpret_cast<const int *>(&value);
        }
    };

    SettingsEntry backgroundSound{"Sound.BackgroundLooping", 1, 1};
    SettingsEntry buttonSoundSplit{"Sound.ButtonSoundSplit", 0, 0};
    SettingsEntry quickAutoResolve{"Combat.QuickAutoResolve", 0, 0};
    SettingsEntry battleQueue{"Combat.BattleQueue", 0, 0};
    SettingsEntry quickCombatType{"Combat.QuickCombatType", 0, 0, 3};
    SettingsEntry showCreatureHealthBar{"Combat.ShowCreatureHealthBar", 1, 1};
    SettingsEntry smoothMapScroll{"AdvMap.SmoothScroll", 1, 1};

  private:
    inline SettingsEntry *data() noexcept
    {
        return &backgroundSound;
    }

  protected:
    void InitialApply();

  public:
    inline static AdditionalConfig &Get()
    {
        static AdditionalConfig instance;
        return instance;
    }
    static BOOL Load();
    static BOOL Save();
};
#pragma pack(pop)
