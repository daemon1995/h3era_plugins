#pragma once
#include "pch.h"

namespace preview
{
    class MonPreview :public IGamePatch
    {

        static MonPreview* instance;

    private:
        H3LoadedPcx* creaturePanelBackgrounds[2];

        H3Vector<H3LoadedPcx16*> resizedSpellPictures;


        virtual void CreatePatches() override;

    private:



        MonPreview();
        //~MonPreview();

        static H3CombatMonsterPanel* __stdcall H3CombatMonsterPanel_Ctor(HiHook* h, H3CombatMonsterPanel* panel,
            int x,
            int y,
            int a4,
            int dy,
            H3BaseDlg* parent,
            DWORD type);
        static _LHF_(H3CombatMonsterPanel_Prepare);
        void CreateResizedSpellEffectPictures();
        void CreatePanelBackGrounds();
    public:
       static MonPreview& Get();//(PatcherInstance* _PI)

    };



}

