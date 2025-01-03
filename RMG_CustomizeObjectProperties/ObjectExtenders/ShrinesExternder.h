#pragma once
namespace shrines
{

class ShrinesExternder : public extender::ObjectsExtender
{
  private:
    static const H3MapItem *currentShrineHint;

  private:
    ShrinesExternder();
    virtual ~ShrinesExternder();

  protected:
    virtual void CreatePatches() override;

    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;

    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;
    //	void SetRmgObjectGenData(const int objectSubtype)  noexcept;

  private:
    static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
    static _LHF_(Shrine__AtGetName);
    static _LHF_(Shrine__AtGetHint);

  public:
    static ShrinesExternder &Get();
};

} // namespace shrines
