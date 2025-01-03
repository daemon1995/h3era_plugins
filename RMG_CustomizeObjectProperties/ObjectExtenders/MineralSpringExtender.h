#pragma once

namespace mineralSpring
{
constexpr int MINERAL_SPRING_OBJECT_TYPE = 144;
constexpr int MINERAL_SPRING_OBJECT_SUBTYPE = 4;
constexpr int MOVE_POINTS_GIVEN = 600;
constexpr int LUCK_GIVEN = 1;

struct H3MapItemMineralSpring
{
    static constexpr LPCSTR ErmVariableFormat = "mineralSpring_%d";

  //public:
    //INT32 id;

  public:
    void Reset();
    static inline BOOL IsVisitedByHero(const H3Hero *hero) noexcept;
    static inline H3MapItemMineralSpring *GetFromMapItem(H3MapItem *mapItem) noexcept;
};

class MineralSpringExtender : public extender::ObjectsExtender
{

    MineralSpringExtender();

    virtual ~MineralSpringExtender();

  private:
    virtual void CreatePatches() override;
    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;

  private:
    static _LHF_(H3AdventureManager__ObjectVisit);
    static _LHF_(AIHero_GetObjectPosWeight);
    static _LHF_(Game__NewGameObjectIteration);
    static _LHF_(H3AdventureManager__GetDefaultObjectClickHint);
    static _LHF_(H3AdventureManager__GetDefaultObjectHoverHint);

  private:
    //	static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
    //	static _LHF_(Shrine__AtGetName);

  public:
    static MineralSpringExtender &Get();
};

}
