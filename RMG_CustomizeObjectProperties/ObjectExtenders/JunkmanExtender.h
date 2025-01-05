#pragma once

namespace junkman
{
constexpr int JUNKMAN_OBJECT_SUBTYPE = 7;

struct H3MapItemJunkman
{
    //static constexpr LPCSTR ErmVariableFormat = "gazebo_%d_%d";

  //public:
    //INT32 id;

  public:
    static inline H3MapItemJunkman*GetFromMapItem(const H3MapItem *mapItem) noexcept;
};

class JunkmanExtender : public extender::ObjectsExtender
{

    JunkmanExtender();

    virtual ~JunkmanExtender();

  private:
    virtual void CreatePatches() override;
    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;

    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept override final;
    //virtual BOOL InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept override final;
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept override final;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept override final;
    virtual H3RmgObjectGenerator* CreateRMGObjectGen(const RMGObjectInfo& objectInfo) const noexcept override final;

  private:
    //	static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
    //	static _LHF_(Shrine__AtGetName);

  public:
    static JunkmanExtender &Get();
};

} // namespace skeletonTransformer
