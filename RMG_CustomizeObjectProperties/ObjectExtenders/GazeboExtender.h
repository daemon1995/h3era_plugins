#pragma once

namespace gazebo
{
constexpr int GAZEBO_OBJECT_SUBTYPE = 6;
constexpr float EXP_GIVEN = 2000;
constexpr int GOLD_REQUIRED = 1000;

struct H3MapItemGazebo
{
    static int gazeboCounter;

    static constexpr LPCSTR ErmVariableFormat = "gazebo_%d_%d";

  public:
    INT32 id;

  public:
    static inline BOOL IsVisitedByHero(const H3MapItemGazebo *gazebo, const H3Hero *hero) noexcept;
    static inline H3MapItemGazebo *GetFromMapItem(const H3MapItem *mapItem) noexcept;
};

class GazeboExtender : public extender::ObjectExtender
{
    static GazeboExtender* instance;

    GazeboExtender();

    virtual ~GazeboExtender();

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
                                    int &aiMapItemWeight, int *moveDistance,
                                    const H3Position pos) const noexcept override final;
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;

  private:
    //	static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
    //	static _LHF_(Shrine__AtGetName);

  public:
    static GazeboExtender &Get();
};

} // namespace gazebo
