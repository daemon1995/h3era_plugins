#pragma once
namespace wog
{
constexpr int WOG_OBJECT_TYPE = eObject::PYRAMID;
constexpr DWORD WOG_OPTIONS_ARRAY = 0x02771920;

class WoGObjectsExtender : public extender::ObjectsExtender
{
    static WoGObjectsExtender* instance;

  private:
    static constexpr LPCSTR jsonKeyFormat = "RMG.objectGeneration.%d.%d.optionId";

  private:
    static std::array<int, extender::limits::COMMON> WoGObjectOptionsIds;

  private:
    WoGObjectsExtender();
    virtual ~WoGObjectsExtender();

  protected:
    virtual void CreatePatches() override;

    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;
    virtual void AfterLoadingObjectTxtProc(const INT16 *maxSubtypes) noexcept override final;
    //	virtual void GetObjectPreperties() noexcept override final;
    //	void SetRmgObjectGenData(const int objectSubtype)  noexcept;

  private:
    static BOOL IsWoGObject(const H3MapItem *mapItem) noexcept;

  public:
    static BOOL IsWoGObject(const H3RmgObjectGenerator *p_ObjGen) noexcept;

    static BOOL WoGObjectHasOptionEnabled(const H3RmgObjectGenerator *p_ObjGen) noexcept;
    static WoGObjectsExtender &Get();
};

} // namespace wog
