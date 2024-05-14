#pragma once
#include "pch.h"
using namespace h3;
class LegendHeroes :
    public IGamePatch
{
    WgtSettings *m_marketWgt =nullptr;
    WgtSettings *altarWgt = nullptr;
    MeetWgt* m_swapWgt[2];// = { nullptr,nullptr };

    HeroWgt *m_heroWgt = nullptr;

    static H3LoadedPcx16* m_drawBuffer[2];

    HeroLook *m_heroLooks;
    int lastHeroLookSet = -1;

    static LegendHeroes* instance;

    static _LHF_(SwapDlg_BeforeShow);

  //  static int __stdcall H3HeroDlgUodate(HiHook*h, H3BaseDlg* dlg);
    static _LHF_(HeroDlg_BeforeArtifactsPlacement);
    static _LHF_(HeroDlg_AfterCreate);
    static void __stdcall OnPreHeroScreen(Era::TEvent* event);


    static _LHF_(MarketDlg_BeforeArtifactsPlacement);
    static _LHF_(MarketDlg_AfterArtifactsPlacement);

    static _LHF_(SacrifaceDlg_BeforeArtifactsPlacement);




   // static void InitHeroLevelUpDialog(PatcherInstance* _pi)  noexcept;

        
    static void BaseDlg_AddBackGroundPcx(H3Dlg* dlg, WgtSettings& wgt, int heroId, bool initiate = false);
    void AlignButtons(H3BaseDlg* dlg);
    static int __stdcall DlgHero_Proc(HiHook* hook, H3Dlg* dlg, H3Msg* msg);
    
    static void __stdcall OnAfterErmInstructions(Era::TEvent* event);
  //  static void __stdcall OnGameLeave(Era::TEvent* event);

    //static void __stdcall OnSavegameRead(Era::TEvent* event);
   // static void __stdcall OnSavegameWrite(Era::TEvent* event);
    void  CreatePatches() noexcept override;

    LegendHeroes(PatcherInstance* pi);

public:
    static void Init(PatcherInstance* pi);
};

