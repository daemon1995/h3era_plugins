// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"
// #include "framework.h"
using namespace h3;

namespace dllText
{
const char *PLUGIN_VERSION = "1.01";
const char *INSTANCE_NAME = "EraPlugin.HotA_AI_Value.jackslater";
const char *PLUGIN_AUTHOR = "jackslater";
// const char* PROJECT_NAME = "$(ProjectName)";
const char *PLUGIN_DATA = __DATE__;
} // namespace dllText
void __stdcall OnReportVersion(Era::TEvent *e)
{

    // show plugin name, version and compilation time
    sprintf(h3_TextBuffer, "{%s} v%s (%s)", PROJECT_NAME, dllText::PLUGIN_VERSION, __DATE__);
    std::string temp(h3_TextBuffer);
    Era::ReportPluginVersion(temp.c_str());
}
Patcher *globalPatcher;
PatcherInstance *_PI;

/*!
  \brief Entry point of the DLL.

  \param hModule Handle to the DLL module.
  \param ul_reason_for_call Reason for calling the function.
  \param lpReserved Reserved for future use.

  \return TRUE if the function succeeds, FALSE otherwise.

  This function is the entry point of the DLL. It handles the different
  reasons for calling the function and initializes the plugin if it
  hasn't been initialized before.
*/

/*

1. Assign settings from settings dlg with lmitizer
2. Make Dlg Scroll and Input text working;
3. Add Default settings reset;
4. Fix Resized Pictures and add new enabele button;
5. Assign native global limit arrays with Object Limitizer
6. Fix/Rewrite Loop sounds
7. Check several objects.txt merging
8. Dlg open is slow
. ???
. Remove Testing code;
. Release;




*/

_LHF_(CrTraitsTxt_AfterLoad)
{
    // AI_Value (done in RMG_CustomizeObjectProperties)
    //P_CreatureInformation[eCreature::MONK].aiValue = 582 / 5;
    //P_CreatureInformation->Get()[eCreature::MONK].aiValue = 116;
    // монах 485 -> 582
    // нага 2016 -> 1814
    // ифрит-султан 1848 -> 2343 (fight_value 1584 -> 1802)
    // Призрачный Дракон 4696 -> 4919 (fight_value 3228 -> 3346)
    // Скорпикора 1589 -> 1685
    // Красный Дракон 4702 -> 5003
    // Циклоп-Король 1443 -> 1544
    // Ящер 126 -> 151 (fight_value 115 -> 137)
    // Ящер-Воин 156 -> 209 (fight_value 130 -> 174)
    // Огненная Птица 4547 -> 4336 (fight_value 3248 -> 3097)
    // Сказочный дракон 19580 -> 30501

    // TODO: Adv.Map - low 0x6C, high 0x70
    // горгона low 12 -> 8, high 25 -> 16
    // могучая горгона low 12 -> 8, high 20 -> 12
    // змии low 10 -> 12, high 20 -> 25
    // змии-дракон low 10 -> 12, high 16 -> 20
    // василиск low 8 -> 10, high 16 -> 20
    // великий василиск low 8 -> 10, high 12 -> 16
    // Огненный Элементаль low 16 -> 10, high 25 -> 20
    // Психический Элементаль low 8 -> 5, high 16 -> 12
    // Элементаль Магии low 8 -> 5, high 12 -> 10
    // Энергетический элементаль low 12 -> 10, high 25 -> 16
    // Феникс low 4 -> 3, high 10 -> 8
    // Снайперы low 5 -> 10, high 12 -> 16


    //H3CreatureInformation* crInfoTable = *reinterpret_cast<H3CreatureInformation**>(0x06747B0);
    //crInfoTable[eCreature::MONK].aiValue = 582;
    


    return EXEC_DEFAULT;
}


void __stdcall OnAfterWog(Era::TEvent* event)
{
    //_PI->WriteLoHook(0x04EEAF2, CrTraitsTxt_AfterLoad);
    //_PI->WriteLoHook(0x04EF247, CrTraitsTxt_AfterLoad);
    P_CreatureInformation->Get()[eCreature::MONK].aiValue = 582;
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static BOOL plugin_On = 0; //!< Flag to indicate if the plugin is on

    // TODO: Perform actions based on the reason for calling

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH: {
        //!< Attach process, initialize the plugin if it hasn't been initialized before

        //! Check if the plugin is already on
        if (!plugin_On)
        {
            plugin_On = 1;

            //! Connect to the Era framework
            Era::ConnectEra(hModule, dllText::INSTANCE_NAME);
            Era::RegisterHandler(OnReportVersion, "OnReportVersion");

            //! Get the global patcher
            globalPatcher = GetPatcher();

            //! Create an instance of the plugin
            _PI = globalPatcher->CreateInstance(dllText::INSTANCE_NAME);
            //_PI->WriteLoHook(0x04EDE90, CrTraitsTxt_AfterLoad);
            //_PI->WriteLoHook(0x04EDE97, CrTraitsTxt_AfterLoad);
            //_PI->WriteLoHook(0x04EEAF2, CrTraitsTxt_AfterLoad);


            Era::RegisterHandler(OnAfterWog, "OnAfterWog");
            //_PI->WriteLoHook(0x04EF247, CrTraitsTxt_AfterLoad);
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
