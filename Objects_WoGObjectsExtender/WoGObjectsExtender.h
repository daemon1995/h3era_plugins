#pragma once
#include "framework.h"

#include <array>
namespace wog
{
constexpr int WOG_OBJECT_TYPE = eObject::PYRAMID;
constexpr DWORD WOG_OPTIONS_ARRAY = 0x02771920;

class WoGObjectsExtender : public extender::ObjectExtender
{
    static WoGObjectsExtender *instance;

  private:
    static constexpr LPCSTR jsonKeyFormat = "RMG.objectGeneration.%d.%d.optionId";

  private:
    static std::array<int, 255> WoGObjectOptionsIds;

  private:
    WoGObjectsExtender();
    virtual ~WoGObjectsExtender() {};

  protected:
    virtual void AfterLoadingObjectsTxtProc(const INT16 *maxSubtypes) noexcept override final;
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const extender::RMGObjectProperties &info,
                                                     const BOOL isPseudoGeneration) const noexcept override;

    //	virtual void GetObjectPreperties() noexcept override final;
    //	void SetRmgObjectGenData(const int objectSubtype)  noexcept;


  public:
    static BOOL WoGObjectHasOptionEnabled(const extender::RMGObjectProperties &info) noexcept;
    static WoGObjectsExtender &Get();
};

} // namespace wog
