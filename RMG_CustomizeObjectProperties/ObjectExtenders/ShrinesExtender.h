#pragma once
namespace shrines
{
struct ExactSpellShrineMapItem
{
};

class ShrinesExtender : public extender::ObjectExtender
{
    static ShrinesExtender *instance;

    static constexpr int LEVEL_REQUIRED = 15;
    static constexpr int SPELL_COST = 2000;
    static constexpr int SPELL_LEVEL = 4;

  private:
    static const H3MapItem *currentShrineHint;

  private:
    ShrinesExtender();
    virtual ~ShrinesExtender();

  protected:
    virtual void CreatePatches();

    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiMapItemWeight, int *moveDistance,
                                    const H3Position pos) const noexcept override final;
    virtual BOOL RMGDlg_ShowCustomObjectHint(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                             const H3String &defaltText) noexcept override final;

    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;
    //	void SetRmgObjectGenData(const int objectSubtype)  noexcept;

  private:
    static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
    static _LHF_(Game__AtShrineOfMagicGestureSettingSpell);
    static _LHF_(Shrine__AtVisit);
    static _LHF_(Shrine__AtWisdomCheck);
    static _LHF_(Shrine__AtGetName);
    static _LHF_(Shrine__AtGetHint);
    static LPCSTR GetCustomName(const H3MapItem *shrine);
    static LPCSTR GetCustomHint(const H3MapItem *shrine);

  public:
    static ShrinesExtender &Get();
};

} // namespace shrines
