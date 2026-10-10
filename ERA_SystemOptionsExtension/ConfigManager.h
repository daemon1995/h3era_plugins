#pragma once
#include "SettingsSupport.h"
#include <array>
#include <unordered_map>
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
    struct ConfigEntry;

    enum class EOptionChangeSource : int
    {
        InitialLoad,
        Dialog,
        ExternalApi,
    };

    static constexpr LPCSTR sectionName = "Settings.Extra";
    static constexpr LPCSTR fileName = sysopts::SETTINGS_FILE;
    static std::unordered_map<std::string, ConfigEntry *> optionsMap;

    struct ConfigEntry
    {
        using ApplyCallback = void (*)(const ConfigEntry &, EOptionChangeSource);

        LPCSTR keyName = nullptr;
        int value = 0;
        int defaultValue = 0;
        int maxValue = 1;
        ApplyCallback applyCallback = nullptr;

      public:
        explicit operator int &() noexcept
        {
            return value;
        }
        explicit operator const int &() const noexcept
        {
            return value;
        }

      public:
        void Apply(const EOptionChangeSource source)
        {
            if (applyCallback)
                applyCallback(*this, source);
        }
        BOOL SetValue(const int newValue, const EOptionChangeSource source)
        {
            const int clampedValue = Clamp(0, newValue, maxValue);
            if (value == clampedValue)
                return FALSE;
            if (!CanChangeOption(*this, source))
                return FALSE;

            value = clampedValue;
            Apply(source);
            if (source != EOptionChangeSource::InitialLoad)
                MarkDirty();
            return TRUE;
        }
    };
    ConfigEntry alternativeButtonClick{"Sound.AlternativeButtonClick", 0, 0};
    ConfigEntry backgroundSound{"Sound.BackgroundLooping", 1, 1};
    ConfigEntry quickAutoResolve{"Combat.QuickAutoResolve", 0, 0};
    ConfigEntry battleQueue{"Combat.BattleQueue", 1, 1};
    ConfigEntry quickCombatType{"Combat.QuickCombatType", 0, 0, 3};
    ConfigEntry showCreatureHealthBar{"Combat.ShowCreatureHealthBar", 1, 1};
    ConfigEntry smoothMapScroll{"AdvMap.SmoothMapScroll", 1, 1};
    ConfigEntry battleSave{"Combat.SaveBeforeBattle", 1, 1};

  private:
    void BindCallbacks() noexcept;

    std::array<ConfigEntry *, 8> Entries() noexcept
    {
        return {{&alternativeButtonClick, &backgroundSound, &quickAutoResolve, &battleQueue, &quickCombatType,
                 &showCreatureHealthBar, &smoothMapScroll, &battleSave}};
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
    static BOOL SaveIfDirty(const BOOL reportError = FALSE);
    static void MarkDirty() noexcept;
    static UINT Revision() noexcept;
    static int ReadNativeValue(const int *valuePtr) noexcept;
    static BOOL SetNativeValue(int *valuePtr, int value, BOOL reportError = FALSE) noexcept;
    static BOOL CanChangeOption(const ConfigEntry &entry, EOptionChangeSource source) noexcept;
    static BOOL IsInCombat() noexcept;
};
#pragma pack(pop)
