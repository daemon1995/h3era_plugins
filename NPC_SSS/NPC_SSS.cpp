//#include "Header.h"
#include "creature_info_dlg.cpp"


using namespace h3;
using namespace Era;


//struct _DlgNPC_;


_Npc_* GetNpc(int hero_id) { return ((_Npc_*)(0x28620C0 + 296 * hero_id)); }

std::vector<INT8> npcSSVec(0);

int GetWoGOptionsStatus(int option_id) { return DwordAt(0x2771920 + (option_id * 4)); }
char* Get_ITxt(int StrNum, int ColNum) { return CDECL_3(char*, 0x77710B, StrNum, ColNum, 0x2860724); }
int GetTxtStringIdBySkillId(int skillId)
{
    int strId = 0;
    switch (skillId)
    {
    case 0: case 1: case 2: case 3: case 4:
        strId = skillId+1;
        break;

    case 5: case 6: case 7: case 8:
        strId = skillId +3;
        break;

    case 9: case 10: case 11:
        strId = skillId + 6;
        break;

    case 12: case 13:
        strId = skillId + 10;
        break;

    case 14:
        strId = 29;

        break;

    default:
        break;
    }

    return strId;
}
void SetNewSecSkillsFrames(H3BaseDlg* dlg, int increment = 0)
{
    int newFrameId = 0, currentFrameId;
    int ssNumber = npcSSVec.back();
   // txt->Load
   // bool isEnhNPC = GetWoGOptionsStatus(51); // if enh npc is enabled

    for (int i = 0; i < 6; i++)
    {
       // if ((int)o_dlgNPC->SpecBonus[i])
        
        H3DlgDef* SSkillDef = dlg->GetDef(60 + i);

        currentFrameId = SSkillDef->GetFrame();

        if (increment)
        {
            auto res = std::find_if(std::begin(npcSSVec), std::end(npcSSVec), [currentFrameId](int a) {return a == currentFrameId; });

            if (res != npcSSVec.end())
                newFrameId = *(res + increment);
        }
        else  // if not switching
            newFrameId = npcSSVec.at(i); // repeat current skills. but with correct order

        int realSkillId = newFrameId % 2 ? (newFrameId + 1) / 2 - 1 : newFrameId / 2 - 1;
        int strId = GetTxtStringIdBySkillId(realSkillId);
       
        o_dlgNPC->SpecBonusHints[i] = newFrameId % 2 ? Get_ITxt(28 + strId, 1) : Get_ITxt(28 + strId, 2); // new hint depends on learned or not
        o_dlgNPC->SpecBonusPopUpText[i] = Get_ITxt(64 + strId, 1); // Setting New PopUp Msg
        o_dlgNPC->SpecBonus[i] = (char*)newFrameId; // setting new pic for PopUp Msg
        SSkillDef->SetFrame(newFrameId);
    }


    //Enable/ Disable Bttns depending on begin/end array reached

    if ((int)o_dlgNPC->SpecBonus[0] == npcSSVec[0]) 
        dlg->GetCustomButton(NPC_DLG_LEFT_BTTN_ID)->Disable();
    else
        dlg->GetCustomButton(NPC_DLG_LEFT_BTTN_ID)->Enable();

    if ((int)o_dlgNPC->SpecBonus[5] == npcSSVec.back())
        dlg->GetCustomButton(NPC_DLG_RIGHT_BTTN_ID)->Disable();
    else
        dlg->GetCustomButton(NPC_DLG_RIGHT_BTTN_ID)->Enable();


    return;
}

bool NPC_CalcSkillMayBe(_Npc_* npc, int ind)
{
    bool canSkillBeLearned = false;
    int firstAbil = 0, secondAbil = 0;
    switch (ind)
    {
    case 0: case 1: case 2: case 3: case 4:
        firstAbil = 0;
        secondAbil = ind + 1;
        break;

    case 5: case 6: case 7: case 8:
        firstAbil = 1;
        secondAbil = ind - 3;
        break;

    case 9: case 10: case 11:
        firstAbil = 2;
        secondAbil = ind - 6;
        break;

    case 12: case 13:
        firstAbil = 3;
        secondAbil = ind - 8;
        break;

    case 14:
        firstAbil = 4;
        secondAbil = 5;
        break;

    default:
        break;
    }

    // check if ss is real

    if (*(int*)((int)npc + 0x38 + firstAbil * 0x4) && *(int*)((int)npc + 0x38 + secondAbil * 0x4))
        canSkillBeLearned = true;

    return canSkillBeLearned;
}

int GetNpcSSkills(_Npc_* npc)
{   
    INT npcSecSkillsBits = *(INT*)((INT)npc + 0x120);
    INT npcBannedSecSkillsBits = *(INT*)((INT)npc + 0x124);

    npcSSVec.clear();
    npcSSVec.reserve(NPC_MAX_SKILLS);
    int mask = 1;
    INT8 npcSSarr[NPC_MAX_SKILLS] = {}; //init checking array

    for (int i = 0; i < NPC_MAX_SKILLS; i++)
    {
        npcSSarr[i] = 0;
        if (npcSecSkillsBits & mask) // if skill is already learned
            npcSSarr[i] = i *2 +1; // add skill frame to array

        else if (!(npcBannedSecSkillsBits & mask) && NPC_CalcSkillMayBe(npc, i)) // if skill isn't banned and can be achieved
            npcSSarr[i] = i * 2 +2;// add skill frame to array

        if (npcSSarr[i]) //if any skill is stored
            npcSSVec.emplace_back(npcSSarr[i]); // add its into main Vector

        mask <<= 1; // increase bit mask
    }

    npcSSVec.shrink_to_fit();
    // sort array - first odd, next even
    std::sort(std::begin(npcSSVec), std::end(npcSSVec), [](INT8 a, INT8 b) ->bool
        {
        //   if (a % 2 == b % 2)
            //    return b > a;
          // else
            return (a % 2 > b % 2);
        });
 

    return 0;
}

int __fastcall WogNPC_BTTNS_Proc(H3Msg* msg) // mouse handle foo
{
    H3BaseDlg* dlg = msg->GetDlg();

    if (msg->subtype == 13)
    {
        int sign = msg->itemId == NPC_DLG_LEFT_BTTN_ID ? -1 : 1; // clicking left/right arrow
        SetNewSecSkillsFrames(dlg, sign);
    }

    return 0;
}


int __stdcall Before_WndNPC_DLG(LoHook* h, HookContext* c) //before dlg run
{

    _DlgNPC_* npcDlg = o_dlgNPC;
    _Npc_* npc = (_Npc_*)o_dlgNPC->DlgTop;

    HDDlg* dlg = (HDDlg*)c->esi;
    
    GetNpcSSkills(npc); // place new skills into vector

    if (npcSSVec.size() > 6 && dlg) // if need to display >6 standard SSkills
    {
        int yPos = dlg->GetDef(60)->GetY() + 3; // get current any SS frame y pos
        int xLeftPos = dlg->GetDef(60)->GetX() - 25; // get current first SS frame x pos
        int xRighttPos = dlg->GetDef(65)->GetX() + 55 ; // get current last SS frame x pos

        H3DlgCustomButton* leftBttn = H3DlgCustomButton::Create(xLeftPos, yPos, NPC_DLG_LEFT_BTTN_ID, "hsbtns3.def", WogNPC_BTTNS_Proc, 0, 1);
        leftBttn->AddHotkey(eVKey::H3VK_LEFT); // adding 2 HK for lazy daemon
        leftBttn->AddHotkey(eVKey::H3VK_A);

        H3DlgCustomButton* rightBttn = H3DlgCustomButton::Create(xRighttPos, yPos, NPC_DLG_RIGHT_BTTN_ID, "hsbtns5.def", WogNPC_BTTNS_Proc, 0, 1);
        rightBttn->AddHotkey(eVKey::H3VK_RIGHT);// adding 2 HK for lazy daemon
        rightBttn->AddHotkey(eVKey::H3VK_D);
        // leftBttn->Cast<H3DlgCustomButtonVec*>;
        // H3DlgCustomButtonVec* testVecBttn = reinterpret_cast<H3DlgCustomButtonVec*>(leftBttn);

        dlg->AddItem(leftBttn);
        leftBttn->Disable(); // add and deactivate left bttn
        dlg->AddItem(rightBttn);  //add right bttn
        SetNewSecSkillsFrames(dlg); //reorder SS in dlg

    }

    return EXEC_DEFAULT;
}



_LHF_(HooksInit)
{

    //  _PI->WriteLoHook(0x4F9D79, OnHeroLvlUpDlgShow);

    H3DLL wndPlugin = h3::H3DLL::H3DLL("wog native dialogs.era");
    int pluginHookAddress = wndPlugin.NeedleSearch<3>({0x3D,0x68,0x02 }, 15);

    if (pluginHookAddress)
    {
        _PI->WriteLoHook(pluginHookAddress, Before_WndNPC_DLG);
        SSS_CreateResources(npc_abils, "dlg_npc3.def");
        _PI->WriteLoHook(0x5F3EA0, Dlg_CreatureInfo_Battle_AfterSettingText);
        _PI->WriteLoHook(0x5F51F8, Dlg_CreatureInfo_HintProc);
      
    }




   // _PI->WriteDword(0x5F3728 + 1, 350); //dlg height
   // _PI->WriteDword(0x5F3CE4 + 1, 324); // dlg hint bar pos
  //  _PI->WriteLoHook(0x5F3CB1, Dlg_CreatureInfo_Battle_BeforeSettingText);

    return EXEC_DEFAULT;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    static _bool_ plugin_On = 0;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:


            //if (ul_reason_for_call == DLL_PROCESS_ATTACH)
            if (!plugin_On)

            {
                plugin_On = 1;
                Era::ConnectEra();


                globalPatcher = GetPatcher();

                _PI = globalPatcher->CreateInstance("NPC_SSS.daemon.plugin");

                _PI->WriteLoHook(0x4EEAF2, HooksInit);


            }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

