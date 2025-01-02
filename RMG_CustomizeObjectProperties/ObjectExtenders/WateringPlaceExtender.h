#pragma once

namespace wateringPlace
{
constexpr int WATERING_PLACE_OBJECT_TYPE = 144;
constexpr int WATERING_PLACE_OBJECT_SUBTYPE = 3;
constexpr int MOVE_POINTS_GIVEN = 1000;

struct H3MapItemWateringPlace
{
    static constexpr LPCSTR ErmVariableFormat = "wateringPlace_%d";

  //public:
    //INT32 id;

  public:
    void Reset();
    static inline BOOL IsVisitedByHero(const H3Hero *hero) noexcept;
    static inline H3MapItemWateringPlace *GetFromMapItem(H3MapItem *mapItem) noexcept;
};

class WateringPlaceExtender : public extender::ObjectsExtender
{

    WateringPlaceExtender();

    virtual ~WateringPlaceExtender();

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
    static WateringPlaceExtender &Get();
};

}
