#pragma once
#include "pch.h"
namespace switcher
{
    constexpr int COUNT_TYPES = 10;

    class SwitcherText :public IPluginText
    {

        struct
        {


        }settings;
        eVKey hotKey;


        char* ButtonHints[COUNT_TYPES];
        char* ButtonRmc[COUNT_TYPES];
        char* AutoBattleQuestion;



    protected:
        virtual void Load()  noexcept override;

    public:
        SwitcherText();
    };

    class ActionSwitcher :
        public IGamePatch
    {




    private:
        ActionSwitcher();

    protected:

        virtual void CreatePatches() noexcept final;

    private:

        SwitcherText text;


    public:

        static ActionSwitcher& Get();// (PatcherInstance* _PI);


    };
}


