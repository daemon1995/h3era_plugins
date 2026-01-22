// dllmain.cpp : Определяет точку входа для приложения DLL.
#define _H3API_PLUGINS_
#include "framework.h"

using namespace h3;

Patcher *globalPatcher = nullptr;
PatcherInstance *_PI = nullptr;
namespace dllText
{
LPCSTR instanceName = "EraPlugin." PROJECT_NAME ".daemon_n";
}

struct NecromancySettings
{

    float skillPower[4]{0.f, 0.05f, 0.1f, 0.15f};          // halved default values for necromancy skill levels 0-3
    float artifactsPower[4]{0.025f, 0.05f, 0.075f, 0.15f}; // halved default values for artifacts boosting necromancy
    float amplifierPower{0.05f};                           // halved default value for necromancy amplifier building
    float obeliskPower{0.1f};                              // halved default value for obelisk building
    int raisedCreature[4]{NH3Creatures::SKELETON, NH3Creatures::WALKING_DEAD, NH3Creatures::WIGHT,
                                NH3Creatures::WIGHT}; // creature to raise with necromancy skill
} necroSettings;

static constexpr LPCSTR baseKey = "suft.necromancy.";


void SetFloatPointerPatch(LPCSTR key, const DWORD patchAddress, float &settingValue)
{
    bool readSuccess = false;
    const float temp = EraJS::readFloat((H3String(baseKey) + key).String(), readSuccess);
    if (readSuccess && temp >= 0.f)
    {
        settingValue = temp;
    }
    _PI->WriteDword(patchAddress, (DWORD)&settingValue);
}
void SetFloatArrayPointerPatch(LPCSTR key, const DWORD patchAddress, float *settingValue, const int size)
{

    for (int i = 0; i < size; i++)
    {
        bool readSuccess = false;
        libc::sprintf(h3_TextBuffer, "%s.%s.%i", baseKey, key, i);
        float temp = EraJS::readFloat(h3_TextBuffer, readSuccess);
        if (readSuccess && temp >= 0.f)
        {
            settingValue[i] = temp;
        }
    }

    _PI->WriteDword(patchAddress, (DWORD)settingValue);
}

void SetIntPatch(LPCSTR key, const DWORD patchAddress, int& settingValue)
{
    bool readSuccess = false;
    const int temp = EraJS::readInt((H3String(baseKey) + key).String(), readSuccess);
    if (readSuccess && temp >= 0)
    {
        settingValue = temp;
    }
    _PI->WriteDword(patchAddress, settingValue);
}

_LHF_(HooksInit)
{
    SetFloatArrayPointerPatch("skill_power", 0x4E3F56 + 3, necroSettings.skillPower, 4);
    // SetFloatPointerPatch("0", 0x4A2D34 + 1, necroSettings.skillPower[0]);
    // SetFloatPointerPatch("0", 0x4A2D34 + 1, necroSettings.skillPower[0]);
    // SetFloatPointerPatch("0", 0x4A2D34 + 1, necroSettings.skillPower[0]);
    // SetFloatPointerPatch("0", 0x4A2D34 + 1, necroSettings.skillPower[0]);

    SetFloatPointerPatch("artifacts.54", 0x4E3FE5 + 2, necroSettings.artifactsPower[0]);
    SetFloatPointerPatch("artifacts.55", 0x4E402B + 2, necroSettings.artifactsPower[1]);
    SetFloatPointerPatch("artifacts.56", 0x4E4072 + 2, necroSettings.artifactsPower[2]);
    SetFloatPointerPatch("artifacts.130", 0x4E4175 + 2, necroSettings.artifactsPower[3]);
    SetFloatPointerPatch("buildings.21", 0x4E40FE + 2, necroSettings.amplifierPower);
    SetFloatPointerPatch("buildings.26", 0x4E411D + 2, necroSettings.obeliskPower);

    SetIntPatch("creatures.0", 0x4E3F39 + 1, necroSettings.raisedCreature[0]);
    SetIntPatch("creatures.1", 0x4E3F32 + 1, necroSettings.raisedCreature[1]);
    SetIntPatch("creatures.2", 0x4E3F29 + 1, necroSettings.raisedCreature[2]);
    SetIntPatch("creatures.3", 0x4E3F1E + 1, necroSettings.raisedCreature[3]);


    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    static bool initialized = false;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (!initialized)
        {
            initialized = true;
            globalPatcher = GetPatcher();
            _PI = globalPatcher->CreateInstance(dllText::instanceName);
            Era::ConnectEra(hModule, dllText::instanceName);
            _PI->WriteLoHook(0x4EEAF2, HooksInit);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
