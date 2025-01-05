#pragma once

namespace grave
{
constexpr int GRAVE_OBJECT_SUBTYPE = 1;

struct H3MapItemGrave
{
  //static constexpr LPCSTR ErmVariableFormat = "grave_%d";

  public:
      /** @brief [00] 0..1023 artifact ID*/
      unsigned  artifactID : 10;
      /** @brief [10] 0..1023 goldAmount*/
      unsigned goldAmount : 10;
      /** @brief [20]*/
      unsigned _u1 : 12;

  public:
    //void Reset();
    //static inline BOOL IsVisitedByHero(const H3Hero *hero) noexcept;
    static inline H3MapItemGrave*GetFromMapItem(const H3MapItem *mapItem) noexcept;
};

class GraveExtender : public extender::ObjectsExtender
{

    GraveExtender();

    virtual ~GraveExtender();

  private:
    virtual void CreatePatches() override;
    //	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
    //	virtual void GetObjectPreperties() noexcept override final;

    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept override final;
    virtual BOOL InitNewGameMapItemSetup(H3MapItem* mapItem) const noexcept override final;
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept override final;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiMapItemWeight, int* moveDistance, const H3Position pos) const noexcept override final;
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;

  private:
    //	static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
    //	static _LHF_(Shrine__AtGetName);

  public:
    static GraveExtender&Get();
};

} // namespace grave
