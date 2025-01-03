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
    static inline H3MapItemWarehouse *GetWarehouse(const H3MapItem *mapItem) noexcept;
};

class WarehousesExtender : public extender::ObjectsExtender
{

    WarehousesExtender();

    virtual ~WarehousesExtender();

  private:
    virtual void CreatePatches() override;
    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;
    virtual BOOL SetHintInH3TextBuffer(const H3MapItem *mapItem, const H3Hero *currentHero,
                                       const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept override final;
    virtual BOOL InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept override final;
    virtual BOOL InitNewWeekMapItemSetup(H3MapItem *mapItem) const noexcept override final;
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept override final;
    virtual BOOL SetAiMapItemWeight(const H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiMapItemWeight) const noexcept override final;

    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;

  private:
    //	static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
    //	static _LHF_(Shrine__AtGetName);

  public:
    static WarehousesExtender &Get();
};

} // namespace warehouses
