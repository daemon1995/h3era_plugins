#pragma once
#include "../pch.h"
#include "../ObjectsExtender.h"

namespace chests
{
// constexpr int GAZEBO_OBJECT_SUBTYPE = 6;

// float GetHeroLearningPower(const H3Hero *hero); // ��� �������� �������

class ChestsExtender : public extender::ObjectsExtender
{
    static ChestsExtender* instance;
    static constexpr DWORD WOG_OPTIONS_ARRAY = 0x02771920;
    static DWORD WoGObjectHasOptionEnabled(int option) noexcept;
    ChestsExtender();

    virtual ~ChestsExtender();

  private:
    /*   
    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept override final;
    */
    virtual BOOL InitNewGameMapItemSetup(H3MapItem *mapItem) const noexcept override final;
    /*
    virtual BOOL VisitMapItem(H3Hero *currentHero, H3MapItem *mapItem, const H3Position pos,
                              const BOOL isHuman) const noexcept override final;
    virtual BOOL SetAiMapItemWeight(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                    int &aiMapItemWeight) const noexcept override final;
    */
    virtual H3RmgObjectGenerator* CreateRMGObjectGen(const RMGObjectInfo& objectInfo) const noexcept override final;

  private:
    //	static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
    //	static _LHF_(Shrine__AtGetName);

  public:
    static ChestsExtender &Get();
};

} // namespace chests
