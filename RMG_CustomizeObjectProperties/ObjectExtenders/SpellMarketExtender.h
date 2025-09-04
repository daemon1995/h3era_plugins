#pragma once
namespace spellmarket
{

constexpr int SPELL_MARKET_OBJECT_SUBTYPE = 12;
constexpr int GOLD_REQUIRED = 2000;
enum eVisitError : char
{
    NONE = 0,
    NO_SPELLS,
    NO_MONEY,
    NO_WISDOM,
    NO_SPELL_BOOK,
    LEARNED,
    VISITED
};
struct SpellMarket
{
    eSpell spells[2];
};
struct H3MapItemSpellMarket
{

    static constexpr LPCSTR visitedByHeroFormat = "RMG_H3MapItemSpellMarketVisited_%d_%d";
    // static constexpr LPCSTR usedFormat = "RMG_H3MapItemSpellMarketUsed_%d_%d";
    static constexpr LPCSTR maxHeroIdFormat = "RMG_H3MapItemSpellMarketMaxHeroId_%d";
    static constexpr LPCSTR spellFormat = "RMG_H3MapItemSpellMarketSpell_%d_%d";
    static constexpr LPCSTR indexFormat = "RMG_H3MapItemSpellMarketSpellIndex";

  public:
    INT32 id;

  public:
    void Reset();
    void GenerateSpells();

  public:
    inline BOOL IsVisitedByHero(const H3Hero *hero) const noexcept;
    inline void SetVisited(const int heroId, const BOOL state) const noexcept;
    inline eSpell GetSpell(const int index) const noexcept;
    void SetSpell(const int index, const eSpell spell) const noexcept;
    void BuySpell(H3Hero *hero, const int index) const noexcept;

    eVisitError TryToVisit(H3Hero *hero) const noexcept;

  public:
    static inline H3MapItemSpellMarket *GetFromMapItem(const H3MapItem *mapItem) noexcept;
};
class SpellMarketExtender : public extender::ObjectExtender
{
    static SpellMarketExtender* instance;

  private:
    static std::vector<SpellMarket> spellMarkets;

  private:
    SpellMarketExtender();
    virtual ~SpellMarketExtender();

  protected:
    virtual void CreatePatches();

    //	virtual void GetObjectPreperties() noexcept override final;
    //	void SetRmgObjectGenData(const int objectSubtype)  noexcept;
    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept override final;
    virtual BOOL InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept override;
    virtual BOOL InitNewWeekMapItemSetup(H3MapItem *mapItem) const noexcept override;
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept override final;
    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;
    //   static BOOL IsWoGObject(const H3MapItem *mapItem) noexcept;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiMapItemWeight, int *moveDistance,
                                    const H3Position pos) const noexcept override final;

  public:
    //   static BOOL IsWoGObject(const H3RmgObjectGenerator *p_ObjGen) noexcept;

    //  static BOOL WoGObjectHasOptionEnabled(const H3RmgObjectGenerator *p_ObjGen) noexcept;
    static SpellMarketExtender &Get();
};

} // namespace spellmarket
