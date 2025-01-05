#pragma once

namespace observatory
{
constexpr int OBSERVATORY_OBJECT_SUBTYPE = 1;
constexpr int VISION_RADIUS_GIVEN = 2;

struct H3MapItemObservatory
{
    static constexpr LPCSTR ErmVariableFormat = "observatory_%d"; //heroid

    // public:
    // INT32 id;

  public:
    // void Reset();
    static inline BOOL IsVisitedByHero(const H3Hero *hero) noexcept;
    static inline H3MapItemObservatory *GetFromMapItem(const H3MapItem *mapItem) noexcept;
};

class ObservatoryExtender : public extender::ObjectsExtender
{

    ObservatoryExtender();

    virtual ~ObservatoryExtender();

  private:
    virtual void CreatePatches() override;
    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;

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
    static ObservatoryExtender &Get();
};

} // namespace observatory
