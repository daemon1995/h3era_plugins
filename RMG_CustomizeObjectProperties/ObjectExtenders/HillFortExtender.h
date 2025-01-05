#pragma once
namespace hillFort
{

class HillFortExtender : public extender::ObjectsExtender
{
  private:
    static H3MapItem *currentHillFort;

  private:
    HillFortExtender();
    virtual ~HillFortExtender();

  protected:
    virtual void CreatePatches() override;

    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;

    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;
    //	void SetRmgObjectGenData(const int objectSubtype)  noexcept;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept override final;

  private:
    static _LHF_(HillFort_AIMapWeight);
    // static _LHF_(Shrine__AtGetName);
    // static _LHF_(Shrine__AtGetHint);

  public:
    static HillFortExtender &Get();
};

} // namespace hillFort
