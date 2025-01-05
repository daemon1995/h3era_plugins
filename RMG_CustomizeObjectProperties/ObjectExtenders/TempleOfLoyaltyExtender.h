#pragma once

namespace templeOfLoyalty
{
constexpr int TEMPLE_OF_LOYALTY_OBJECT_SUBTYPE = 0;

struct H3MapItemTempleOfLoyalty
{
    static constexpr LPCSTR ErmVariableFormat = "templeOfLoyalty_%d";

    // public:
    // INT32 id;

  public:
    static inline BOOL IsVisitedByHero(const H3Hero *hero) noexcept;
    static inline H3MapItemTempleOfLoyalty *GetFromMapItem(const H3MapItem *mapItem) noexcept;
};

class TempleOfLoyaltyExtender : public extender::ObjectsExtender
{

    TempleOfLoyaltyExtender();

    virtual ~TempleOfLoyaltyExtender();

  private:
    virtual void CreatePatches() override;
    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;

    // static _LHF_(H3AdventureManager__ObjectVisit);
    // static _LHF_(AIHero_GetObjectPosWeight);
    // static _LHF_(Game__NewGameObjectIteration);
    // static _LHF_(H3AdventureManager__GetDefaultObjectClickHint);
    // static _LHF_(H3AdventureManager__GetDefaultObjectHoverHint);

    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept override final;
    // virtual BOOL InitNewGameMapItemSetup(H3MapItem* mapItem) const noexcept override final;
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept override final;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept override final;
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;

  private:
    //	static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
    //	static _LHF_(Shrine__AtGetName);

  public:
    static TempleOfLoyaltyExtender &Get();
};

} // namespace templeOfLoyalty
