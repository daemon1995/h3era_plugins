#pragma once

namespace warehouses
{
constexpr int WAREHOUSE_OBJECT_TYPE = 142;

struct H3MapItemWarehouse
{

    unsigned resourceType : 5;
    /** @brief [05] which players have come by*/
    unsigned visited : 8;
    /** @brief [13]*/
    unsigned resourceAmount : 10;
    unsigned _u1 : 9;

  public:
    void Reset();
    static inline H3MapItemWarehouse *GetWarehouse(H3MapItem *mapItem) noexcept;
};

class WarehousesExtender : public extender::ObjectsExtender
{

    WarehousesExtender();

    virtual ~WarehousesExtender();

  private:
    virtual void CreatePatches() override;
    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;

  private:
    static _LHF_(H3AdventureManager__ObjectVisit);
    static _LHF_(AIHero_GetObjectPosWeight);
    static _LHF_(Game__NewGameObjectIteration);
    static _LHF_(Game__NewWeekObjectIteration);
    static _LHF_(H3AdventureManager__GetDefaultObjectClickHint);
    static _LHF_(H3AdventureManager__GetDefaultObjectHoverHint);

  private:
    //	static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
    //	static _LHF_(Shrine__AtGetName);

  public:
    static WarehousesExtender &Get();
};

} // namespace warehouses
