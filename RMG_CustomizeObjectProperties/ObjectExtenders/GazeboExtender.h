#pragma once

namespace gazebo
{
constexpr int GAZEBO_OBJECT_TYPE = 144;
constexpr int GAZEBO_OBJECT_SUBTYPE = 6;
constexpr int EXP_GIVEN = 2000;
constexpr int GOLD_REQUIRED = 1000;

float GetHeroLearningPower(const H3Hero *hero); // это прототип функции

struct H3MapItemGazebo
{
    static constexpr LPCSTR ErmVariableFormat = "gazebo_%d_%d";

  public:
    INT32 id;

  public:
    void Reset();
    static inline BOOL IsVisitedByHero(const H3MapItemGazebo gazebo, const H3Hero *hero) noexcept;
    static inline H3MapItemGazebo *GetFromMapItem(H3MapItem *mapItem) noexcept;
};

class GazeboExtender : public extender::ObjectsExtender
{

    GazeboExtender();

    virtual ~GazeboExtender();

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
    static GazeboExtender &Get();
};

} // namespace gazebo
