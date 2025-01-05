#pragma once

namespace hermitsShack
{
constexpr int HERMITS_SHACK_OBJECT_SUBTYPE = 5;

struct H3MapItemHermitsShack
{
    static int hermitsShackCounter;

    static constexpr LPCSTR ErmVariableFormat = "hermitsShack_%d_%d";

  public:
    INT32 id;

  public:
    static inline BOOL IsVisitedByHero(const H3MapItemHermitsShack *hermitsShack, const H3Hero *hero) noexcept;
    static inline H3MapItemHermitsShack *GetFromMapItem(const H3MapItem *mapItem) noexcept;
};

class HermitsShackExtender : public extender::ObjectsExtender
{

    HermitsShackExtender();

    virtual ~HermitsShackExtender();

  private:
    virtual void CreatePatches() override;
    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;

    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept override final;
    virtual BOOL InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept override final;
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept override final;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept override final;
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;

  public:
    static HermitsShackExtender &Get();
};

} // namespace hermitsShack
