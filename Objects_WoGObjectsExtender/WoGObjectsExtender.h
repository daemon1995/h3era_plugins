#pragma once
#include "framework.h"

#include <vector>
namespace wog
{
constexpr int WOG_OBJECT_TYPE = extender::WOG_OBJECT_TYPE;
constexpr int ERA_OBJECT_TYPE = extender::ERA_OBJECT_TYPE;
constexpr DWORD WOG_OPTIONS_ARRAY = 0x02771920;
constexpr int WOG_OPTIONS_COUNT = 1000;

class WoGObjectsExtender : public extender::ObjectExtender
{
    static WoGObjectsExtender *instance;

  private:
    static constexpr LPCSTR jsonKeyFormat = "RMG.objectGeneration.%d.%d.optionId";

  private:
    static std::vector<int> WoGObjectOptionsIds;

  private:
    WoGObjectsExtender();
    virtual ~WoGObjectsExtender() {};

  protected:
    virtual void CreatePatches() override;
    virtual void AfterLoadingObjectsTxtProc(const INT16 *maxSubtypes) noexcept override final;
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const extender::RMGObjectProperties &info,
                                                     const BOOL isPseudoGeneration) const noexcept override;
    virtual BOOL InitNewGameMapItemSetup(H3MapItem *mapItem, int typeCount, int subtypeCount) const noexcept override;

    virtual BOOL VisitMapItem(H3Hero *hero, H3MapItem *mapItem, const H3Position pos,
                             const BOOL isHuman) const noexcept override;
    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *hero, const int interactPlayerId,
                                     const BOOL isRightClick) const noexcept override;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *hero, const H3Player *player,
                                   int &aiWeight, int *moveDistance, const H3Position pos) const noexcept override;

  public:
    static BOOL IsWoGOptionEnabled(int optionId) noexcept;
    static BOOL WoGObjectHasOptionEnabled(const extender::RMGObjectProperties &info) noexcept;
    static WoGObjectsExtender &Get();

  private:
    static _LHF_(LegacyObjectVisit);
    static _LHF_(LegacyObjectHoverHint);
    static _LHF_(LegacyObjectClickHint);
};

} // namespace wog
