// dllmain.cpp : Определяет точку входа для приложения DLL.
// #define _H3API_PLUGINS_
#define _H3API_PLUGINS_
#define ERA_MODLIST
#define _WOG_
#include "TestDlg.h"
#include "framework.h"
// #include "..\..\headers\H3API_RK\single_header\H3API.hpp"

using namespace h3;

Patcher *globalPatcher;
PatcherInstance *_PI;
namespace dllText
{
LPCSTR instanceName = "EraPlugin.Testing.daemon_n";
}
namespace db
{
// void echo(int a)
//{
//	Era::y[1] = a;

//	Era::ExecErmCmd("IF:L^%y1^;");
//}

// void echo(const char* a)
//{
//	sprintf(Era::z[1], a);
//	Era::ExecErmCmd("IF:L^%z1^;");
// }
// void echo(float a)
//{
//	Era::e[1] = a;
//	Era::ExecErmCmd("IF:L^%e1^;");
// }

// void dump(int a)
//{
//	Era::y[1] = a;
//	Era::ExecErmCmd("IF:M^%y1^;");
// }
// void dump(const char* a)
//{
//	sprintf(Era::z[1], a);
//	Era::ExecErmCmd("IF:M^%z1^;");
// }

} // namespace db

// void debug(int a);

// TestDlg *p_dlg = nullptr;
// int dlgWidth, dlgHeight;
//
// int defDtorCounter = 0;
// int itemDtorCounter = 0;
// bool dtorCalled = false;
//_LHF_(DlgDef_Dtor)
//{
//     if (dtorCalled)
//         defDtorCounter += 1;
//     return EXEC_DEFAULT;
// }
//
//_LHF_(DlgItem_Dtor)
//{
//     if (dtorCalled)
//         itemDtorCounter += 1;
//     return EXEC_DEFAULT;
// }
//
// H3LoadedPcx16 *drawBuffer = nullptr;
//_Pcx16_* __thiscall Pcx16_Draw_ToPcx16(
//	_Pcx16_* pcx,
//	int srcX,
//	int srcY,
//	int width,
//	int height,
//	int buffer,
//	int dstX,
//	int dstY,
//	int maxX,
//	int maxY,
//	int scanline,
//	int TransparentColor)

//_LHF_(TownScreen_EndOfRedraw)
//{
//    // TestDlg dlg(500, 500, -1, -1);
//    // dlg.CreateOKButton();
//    // dlg.Start();
//
//    P_WindowManager->screenPcx16->DrawToPcx16(0, 0, 1, drawBuffer);
//
//    // drawBuffer->DrawToPcx16(0, 0, 1, P_WindowManager->screenPcx16, 25, 25);
//
//    P_WindowManager->screenPcx16->buffer;
//    THISCALL_12(VOID, 0x44DF80, drawBuffer, 0, 0, dlgWidth, dlgHeight, P_WindowManager->screenPcx16->buffer, 200, 115,
//                500, 500, P_WindowManager->screenPcx16->scanlineSize,
//                1); // , x, y, dest->width, dest->height, dest->scanlineSize,
//                    // transparent);
//
//    return EXEC_DEFAULT;
//}
_LHF_(DlgTown_AfterCreate)
{
    return EXEC_DEFAULT;
}

_LHF_(MainWindow_F1)
{
    c->return_address = 0x4F877D;
    return NO_EXEC_DEFAULT;

    return EXEC_DEFAULT;
}

constexpr int BANK_SUBTYPE = 22;   // Creature Bank subtype
constexpr int BANK_MAX_LEVEL = 20; // Creature Bank subtype
struct HookData
{
    H3Hero *hero = nullptr;
    BOOL offerReplay = false;
    INT replayIndex = 0;
    Patch *blockUserMessage = nullptr;
} data;
LPCSTR replayFormat = "pol_creature_bank_replays_counter_%d";

BOOL SetBankGuard(H3CreatureBank &bank, const int bankPower)
{
    // const auto &baseState= P_CreatureBankSetup->Get()[BANK_SUBTYPE].states[0];

    if (bankPower > BANK_MAX_LEVEL)
    {
        return false;
    }
    auto &baseArmy = bank.guardians;
    const int baseArmyValue = baseArmy.GetArmyValue();

    eCreature monFromTheBank = eCreature::UNDEFINED;
    int monsPerSlot = 0;
    for (auto &i : baseArmy)
    {
        if (i.Type() != eCreature::UNDEFINED)
        {
            monFromTheBank = eCreature(i.Type());
            monsPerSlot = i.Count();
            break;
        }
    }
    if (monFromTheBank != eCreature::UNDEFINED && monsPerSlot > 0)
    {
        const float bankPowerFactor = 1.0f + bankPower * 0.3f;

        const int armySlots = Clamp(1, bankPower / 3 + 4, 7);

        const float valuePerSlot =
            bankPowerFactor * bankPowerFactor * P_CreatureInformation[monFromTheBank].aiValue * monsPerSlot;

        const int minMonLevel = 0; // Clamp(0, H3Random::Rand(), 6);
        const int maxMonLevel = 6; // Clamp(0, P_CreatureInformation[monFromTheBank].level, 6);

        const eCreature randomMonByLevel = THISCALL_3(eCreature, 0x4C8F80, P_Game->Get(), minMonLevel, maxMonLevel);
        const int monValue = P_CreatureInformation[randomMonByLevel].aiValue;
        const int newMonNum = Clamp(1, static_cast<int>(valuePerSlot / monValue), INT32_MAX);
        for (size_t i = 0; i < armySlots; i++)
        {
            baseArmy.type[i] = randomMonByLevel;
            baseArmy.count[i] = newMonNum;
        }
        // give resources to the bank
        for (size_t i = 0; i < 7; i++)
        {
            auto &res = bank.resources[i];
            if (H3Random::Rand(0, 99) < 20 + bankPower * 4)
            {
                res += i < eResource::GOLD ? H3Random::Rand(1, 10) : H3Random::Rand(1000, 5000);
                res *= bankPowerFactor; // Gold is always present, other resources are random
            }
            if (res > 0)
                res *= bankPowerFactor;
        }
        // give artifacts to the bank
        if (const int artsToAdd = H3Random::Rand(0, bankPower / 2))
        {

            for (size_t i = 0; i < artsToAdd; i++)
            {
                if (H3Random::Rand(0, 99) <
                    20 + bankPower * 4) // 20% chance to have an artifact with adjusted probability
                {
                    const int artType = Clamp(eArtifactType::TREASURE,
                                              static_cast<eArtifact>(H3Random::Rand(eArtifactType::TREASURE,
                                                                                    eArtifactType::MAJOR + bankPower)),
                                              eArtifactType::ALL) &
                                        ~eArtifactType::SPECIAL;
                    const int artId = P_Game->GetRandomArtifactOfLevel(artType);
                    if (artId > 0)
                    {
                        bank.artifacts.Add(artId);
                    }
                }
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}
void __stdcall AdvMgr_CrBank_Visit(HiHook *hook, H3AdventureManager *advMgr, H3Hero *hero, H3MapItem *map_item,
                                   int *XYZ, int isPlayer)
{

    data.hero = hero;

    THISCALL_5(int, hook->GetDefaultFunc(), advMgr, hero, map_item, XYZ, isPlayer);
    auto patch = _PI->WriteJmp(0x04A1352, 0x04A1486);
    //  int bankPower = 1;
    data.blockUserMessage->Apply();

    while (data.offerReplay)
    {
        const int bankId = map_item->creatureBank.id;
        // libc::sprintf(h3_TextBuffer, replayFormat, bankId);
        // const int replayCounter = Era::GetAssocVarIntValue(h3_TextBuffer) + 1;
        // Era::SetAssocVarIntValue(h3_TextBuffer, replayCounter);

        data.offerReplay = false;
        auto &cbank = P_Game->creatureBanks[bankId];
        map_item->creatureBank.taken = false;
        cbank.artifacts.RemoveAll();

        // call the original function to reset the bank
        FASTCALL_2(void, 0x047A6C0, &cbank, BANK_SUBTYPE);
        if (SetBankGuard(cbank, ++data.replayIndex))
        {
            THISCALL_5(int, hook->GetOriginalFunc(), advMgr, hero, map_item, XYZ, isPlayer);
        }

        // visit the bank again
    }
    data.hero = nullptr;
    data.replayIndex = 0;
    data.blockUserMessage->Undo();
}

void __stdcall CrBank_BattleMgr_And_Reward(HiHook *hook, H3AdventureManager *advMgr, H3Hero *hero, H3MapItem *map_item,
                                           LPCSTR text, int *XYZ, int isPlayer)
{

    THISCALL_6(int, hook->GetDefaultFunc(), advMgr, hero, map_item, text, XYZ, isPlayer);
    if (isPlayer && map_item->objectSubtype == BANK_SUBTYPE && hero && hero == data.hero && hero->owner != -1)
    {
        if (data.replayIndex <= BANK_MAX_LEVEL)
        {
            data.offerReplay = H3Messagebox::Choice(EraJS::read("pol.text.question"));
        }
        else
        {
            H3Messagebox::Show(EraJS::read("pol.text.completed"));
        }
    }
}
//_LHF_(AICombat_CheckECX)
//{
//    // if ECX == 0, skip the check and return true
//    if (c->ecx < 0)
//    {
//		H3Messagebox("pol_AiCombat: ECX < 0 detected, skipping check and returning true.");
//
//    }
//    if (c->eax<-1 || c->eax > 999)
//    {
//        H3Messagebox("pol_AiCombat: EAX < 0 detected, skipping check and returning true.");
//
//    }
//    return EXEC_DEFAULT;
//}
#include <cstdio>
#include <initializer_list>
#include <type_traits>

template <typename... Ints> void Debug(Ints... values) noexcept
{
    char buffer[1024];

    size_t pos = 0;

    (void)std::initializer_list<int>{
        ([&] { pos += std::snprintf(buffer + pos, sizeof(buffer) - pos, "%d ", int(values)); }(), 0)...};

    if (pos > 0)
        buffer[pos - 1] = '\0';
    libc::sprintf(Era::z[1], "%s", buffer);
    Era::ExecErmCmd("IF:L^%z1^");
}
_ERH_(OnGameEnter)
{
    return;
    TestDlg dlg(500, 500);
    dlg.Start();
}

_LHF_(WoG_CrExpo_Recalc)
{

    Debug(1);
    c->return_address = 0x07184D5;

    return NO_EXEC_DEFAULT;
}

_LHF_(MapTeamOpen)
{

    c->DL() = static_cast<UINT8>(1 << IntAt(c->ebp + 0x14));
    c->return_address = 0x049CE18;

    return NO_EXEC_DEFAULT;
}

struct CrExpo
{
    INT32 experience;
    INT32 number;
    struct Fl
    {
        unsigned __int32 Act : 1;
        unsigned __int32 Type : 4;
        unsigned __int32 MType : 8;
        unsigned __int32 mHasArt : 1;
        unsigned __int32 mArt : 2;
        unsigned __int32 mCopyArt : 2;
        unsigned __int32 mSubArt : 4;
        unsigned __int32 _un : 10;
    } flags;
    union MonExpUnion {
        DWORD UniData;
        DWORD MixPos;
        struct _Hero
        {
            __int16 Id;
            __int16 Slot;
        } hero;
        //_Map Map;
        //_Town Town;
        //_Mine Mine;
        //_Horn Horn;
    } place;
};
_LHF_(WoG_SetNewExp_AtExpPush)
{

    // c->DL() = static_cast<UINT8>(1 << IntAt(c->ebp + 0x14));

    const int sourceExpoOffset = IntAt(c->ebp - 0x4);
    const int dstExpoOffset = IntAt(c->ebp - 0x10);
    const int sourceNumber = IntAt(c->ebp + 0x20);
    const int dstNumber = IntAt(c->ebp + 0x24);

    auto *expTable = reinterpret_cast<CrExpo *>(0x0860550);
    // auto& sourceExpo = expTable[sourceExpoOffset];
    // auto& dstExpo = expTable[dstExpoOffset];

    INT64 totalNewExp = static_cast<INT64>(expTable[sourceExpoOffset].experience) * sourceNumber;
    totalNewExp += static_cast<INT64>(expTable[dstExpoOffset].experience) * dstNumber;
    totalNewExp /= (sourceNumber + dstNumber);
    Debug(totalNewExp);
    c->eax = static_cast<int>(totalNewExp);

    c->return_address = 0x0719BEA;
    return NO_EXEC_DEFAULT;
}

_LHF_(WoG_CreatureSplitDlg_AtSourceExp)
{
    auto &srcExpo = ValueAt<CrExpo>(0x28602AC);
    auto &dstExpo = ValueAt<CrExpo>(0x2860294);

    const int srcNum = IntAt(c->ebp - 0x10);

    const int baseNum = IntAt(0x282A77C);
    INT64 totalNewExp = (static_cast<INT64>((srcNum - baseNum)) * srcExpo.experience +
                         static_cast<INT64>(baseNum) * dstExpo.experience) /
                        srcNum;
    IntAt(c->ebp - 0x20) = static_cast<int>(totalNewExp);
    c->return_address = 0x07662D9;
    return NO_EXEC_DEFAULT;
}
_LHF_(WoG_CreatureSplitDlg_AtDestExp)
{
    auto &srcExpo = ValueAt<CrExpo>(0x28602AC);
    auto &dstExpo = ValueAt<CrExpo>(0x2860294);

    const int baseNum = IntAt(0x282A35C);

    const int dstNum = IntAt(c->ebp - 0x68);

    INT64 totalNewExp = (static_cast<INT64>((dstNum - baseNum)) * dstExpo.experience +
                         static_cast<INT64>(baseNum) * srcExpo.experience) /
                        dstNum;
    IntAt(c->ebp - 0x3C) = static_cast<int>(totalNewExp);
    c->return_address = 0x0766341;
    return NO_EXEC_DEFAULT;
}
struct ArmySlotExperience
{
    int number;
    int experience;
} armySlots[14];

_LHF_(WoG_InTowmArmyMerge_GuardIterator)
{
    const int slotId = c->eax;

    armySlots[slotId].experience = IntAt(c->ebp - 0x10C);
    armySlots[slotId].number = ValueAt<H3Army>(0x2846BF0).count[slotId];
    c->ecx = 1 << slotId;

    return EXEC_DEFAULT;
}
_LHF_(WoG_InTowmArmyMerge_VisitorIterator)
{
    const int slotId = c->ecx + 7;

    armySlots[slotId].experience = IntAt(c->ebp - 0x10C);
    armySlots[slotId].number = ValueAt<H3Army>(0x2846C28).count[slotId - 7];
    c->edx = 1 << slotId;

    return EXEC_DEFAULT;
}
_LHF_(WoG_InTowmArmyMerge_ExperienceCreatorIterator)
{

    // put final experience
    const int slotId = IntAt(c->ebp - 0x18);

    INT64 resultExp = 0;
    INT totalCreatures = IntAt(c->ebp - 0x14);

    H3Army &expArmy = ValueAt<H3Army>(c->ebp - 0xC8);

    const int slotsMerged = expArmy.count[slotId];
    for (size_t i = 0; i < 14; i++)
    {
        if (const int slotBit = slotsMerged & (1 << i))
        {
            resultExp += static_cast<INT64>(armySlots[i].experience) * armySlots[i].number;
        }
    }

    c->eax = totalCreatures ? static_cast<int>(resultExp / totalCreatures) : 0;

    c->return_address = 0x0759A2E;
    return NO_EXEC_DEFAULT;
}
enum eExpType
{
    CE_HERO = 0x1,
    CE_MAP = 0x2,
    CE_TOWN = 0x3,
    CE_MINE = 0x4,
    CE_HORN = 0x5,
};

_LHF_(HooksInit)
{
    // _PI->WriteLoHook(0x049CDF6, MapTeamOpen);

    Era::RegisterHandler(OnGameEnter, "OnGameEnter");

    if (0)
    {
        _PI->WriteLoHook(0x07184A5, WoG_CrExpo_Recalc);
        _PI->WriteLoHook(0x0719BE3, WoG_SetNewExp_AtExpPush);
        _PI->WriteLoHook(0x07662B2, WoG_CreatureSplitDlg_AtSourceExp);
        _PI->WriteLoHook(0x076631A, WoG_CreatureSplitDlg_AtDestExp);

        _PI->WriteLoHook(0x0759749, WoG_InTowmArmyMerge_GuardIterator);
        _PI->WriteLoHook(0x0759870, WoG_InTowmArmyMerge_VisitorIterator);

        _PI->WriteLoHook(0x0759A24, WoG_InTowmArmyMerge_ExperienceCreatorIterator);

        constexpr auto srcExpPatch = "8DB0 50058600 8B06 F76D 20 F77E 04 8906 8B4D 20 894E 04";
        /*
        lea     esi, [WOG__CrExpo__Table + eax]   ; esi = &Table[src]
        mov     eax, [esi]                        ; eax = Expo
        imul    dword ptr [ebp+source_mon_number] ; edx:eax = Expo * src (64-bit)
        idiv    dword ptr [esi+4]                 ; eax = (Expo*src)/Num

        mov     [esi], eax                        ; Table[src].Expo = eax

        mov     ecx, [ebp+source_mon_number]
        mov     [esi+4], ecx                     ; Table[src].Num = src
        */

        _PI->WriteHexPatch(0x0719993, srcExpPatch); // patch experience
        _PI->WriteCodePatch(0x07199A9, "%n", 28);   // nop extra code

        constexpr auto dstExpPatch = "8DB1 50058600 8B06 F76D 24 F77E 04 8906 8B4D 24 894E 04";
        /*
        lea     esi, [WOG__CrExpo__Table + ecx]   ; esi = &Table[dst]
        mov     eax, [esi]                        ; eax = Expo
        imul    dword ptr [ebp+dst_mon_number] ; edx:eax = Expo * dst (64-bit)
        idiv    dword ptr [esi+4]                 ; eax = (Expo*dst)/Num

        mov     [esi], eax                        ; Table[dst].Expo = eax

        mov     ecx, [ebp+dst_mon_number]
        mov     [esi+4], ecx                     ; Table[dst].Num = dst
        */

        _PI->WriteHexPatch(0x0719A55, dstExpPatch); // patch experience
        _PI->WriteCodePatch(0x0719A6B, "%n", 28);   // nop extra code
    }

    // _PI->WriteLoHook(0x04242B1, AICombat_CheckECX);
    // auto &vec = modList::GetEraModList();
    // for (auto &i : vec)
    //{
    //     std::string modNameP = i + '/' + std::to_string(i.length());
    //     MessageBoxA(0, modNameP.c_str(), "", MB_OK);
    // }
    ////function IsSse42Supported : boolean; assembler;
    // int aVar = 23;
    //__asm
    //{
    //     push ebx
    //     mov eax, 1
    //     cpuid
    //     shr ecx, 20
    //     and ecx, 1
    //     mov eax, ecx
    //     mov aVar, eax
    //     pop ebx
    //  //   end;
    // }
    // MessageBoxA(0, Era::IntToStr(aVar).c_str(), "caption", MB_OK);

    return EXEC_DEFAULT;
    data.blockUserMessage = _PI->WriteJmp(0x04A1352, 0x04A1486);
    data.blockUserMessage->Undo();

    _PI->WriteHiHook(0x04A894E, THISCALL_, AdvMgr_CrBank_Visit);

    _PI->WriteHiHook(0x04A1497, THISCALL_, CrBank_BattleMgr_And_Reward);

    return EXEC_DEFAULT;
    ////_PI->WriteLoHook(0x4FBD71, gem_Dlg_MainMenu_Create);
    ////_PI->WriteLoHook(0x4EA8B5, DlgDef_Dtor);
    ////
    ////_PI->WriteLoHook(0x5FE9F9, DlgItem_Dtor);
    ////_PI->WriteLoHook(0x5D5926, TownScreen_EndOfRedraw);
    ////_PI->WriteLoHook(0x5C681E, DlgTown_AfterCreate);

    // dlgWidth = H3GameWidth::Get() - 100;
    // dlgHeight = IntAt(0x5C38EF + 1);

    // IntAt(0x5C38F6 + 1) = dlgWidth;
    //// IntAt(0x5C38EF + 1) = dlgHeight;

    ////_PI->WriteDword(0x5C38F6 +1, )

    //// drawBuffer = H3LoadedPcx16::Create(P_WindowManager->screenPcx16->width,
    //// P_WindowManager->screenPcx16->width); drawBuffer =
    //// H3LoadedPcx16::Create(dlgWidth, dlgHeight);

    ////_PI->WriteLoHook(0x4F8767, MainWindow_F1);

    //_PI->WriteWord(0x4F870B, 0x9090);

    // return EXEC_DEFAULT;
}

// static _LHF_(NewScenarioDlg_Create);
//
// void __stdcall NewScenarioDlg_Create(HiHook *hook, H3SelectScenarioDialog *dlg, H3Msg *msg)
//{
//     THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);
//
//     H3DlgCaptionButton *bttn = dlg->GetCaptionButton(4444);
//     if (bttn)
//     {
//         bttn->AddHotkey(h3::eVKey::H3VK_W);
//     }
//     bttn = dlg->CreateCaptionButton(bttn->GetX(), bttn->GetY() + 45, bttn->GetWidth(), bttn->GetHeight(), 4500,
//                                     bttn->GetDef()->GetName(), "ERA options", h3::NH3Dlg::Text::SMALL, 0);
//     if (bttn)
//     {
//         bttn->SetClickFrame(1);
//         bttn->AddHotkey(h3::eVKey::H3VK_E);
//     }
// }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        globalPatcher = GetPatcher();
        _PI = globalPatcher->CreateInstance(dllText::instanceName);
        Era::ConnectEra(hModule, dllText::instanceName);

        _PI->WriteLoHook(0x4EEAF2, HooksInit);

        // if (ver.era()      )
        {
        }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
