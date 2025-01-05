#pragma once

namespace mineralSpring
{
constexpr int MINERAL_SPRING_OBJECT_SUBTYPE = 4;
constexpr int MOVE_POINTS_GIVEN = 600;
constexpr int LUCK_GIVEN = 1;

struct H3MapItemMineralSpring
{
    static constexpr LPCSTR ErmVariableFormat = "mineralSpring_%d";

    // public:
    // INT32 id;

  public:
    // void Reset();
    static inline BOOL IsVisitedByHero(const H3Hero *hero) noexcept;
    static inline H3MapItemMineralSpring *GetFromMapItem(const H3MapItem *mapItem) noexcept;
};

class MineralSpringExtender : public extender::ObjectsExtender
{

    MineralSpringExtender();

    virtual ~MineralSpringExtender();

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
    static MineralSpringExtender &Get();
};

} // namespace mineralSpring
