#pragma once

namespace ancientLamp
{
constexpr int ANCIENT_LAMP_OBJECT_SUBTYPE = 0;
constexpr int ANCIENT_LAMP_CREATURE_TYPE = 37;
constexpr int ANCIENT_LAMP_MIN_CREATURE_COUNT = 4;
constexpr int ANCIENT_LAMP_MAX_CREATURE_COUNT = 6;


struct H3MapItemAncientLamp
{
    //static int ancientLampCounter;

    //static constexpr LPCSTR ErmVariableFormat = "ancientLamp_%d_%d"; // ancienLampId, playerId

  public:
    unsigned _u1 : 5;
    /** @brief [05] which players have come by*/
    unsigned visited : 8;
    /** @brief [13]*/
    unsigned creatureType : 10;
    /** @brief [23]*/
    unsigned creatureCount : 9;

  public:
    void Reset();
    //static inline BOOL IsVisitedByPlayer(const H3MapItemAncientLamp ancientLamp, const H3Hero* hero) noexcept;
    static inline H3MapItemAncientLamp *GetFromMapItem(const H3MapItem *mapItem) noexcept;
};

class AncientLampExtender : public extender::ObjectsExtender
{

    AncientLampExtender();

    virtual ~AncientLampExtender();

  private:
    virtual void CreatePatches() override;
    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;

    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept override final;
    virtual BOOL InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept override final;
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept override final;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept override final;
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;

  private:
    //	static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
    //	static _LHF_(Shrine__AtGetName);

  public:
    static AncientLampExtender&Get();
};

} // namespace ancientLamp
