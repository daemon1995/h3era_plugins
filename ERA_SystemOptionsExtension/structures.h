#pragma once
#pragma pack(push, 4)
struct ConfigStruct
{
    int enemySpeed;
    int playerSpeed;
    int musicVolume;
    int soundVolume;
    int lastMusicVolume;
    int lastSoundVolume;
    int autoSave;
    int showRoute;
    int moveReminder;
    int quickCombat;
    int videoSubtitles;
    int townOutlines;
    int animateSpellBook;
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
    int showEntireGrid;
    int combatShowCursorShadow;
    int combatShowMovementShadow;
    int combatViewArmy[7];
    char dontTryRedbook;
    char firstInstall;
    char padding[2];
    char cUniqueSystemID[4];
    int combatSpeed;
    char cCurRemoteReceive[13];
    char cRemoteReceiveDiff[13];
    char cCurRemoteSend[13];
    char cNetName[21];

    static constexpr unsigned int ADDRESS = 0x6987A8;

    inline static ConfigStruct &Get()
    {
        return *reinterpret_cast<ConfigStruct *>(ADDRESS);
    }
};
#pragma pack(pop)
