// dllmain.cpp : Определяет точку входа для приложения DLL.

//#include "resizing_algorithms.cpp"
#include "header.h"
#include <thread>


//void cleanCache(string fileLink);


using namespace Era;

Patcher* globalPatcher;
PatcherInstance* _PI;


//_Def_* def = o_LoadDef("dlg_npc3.def");
////def.dr
//_Pcx16_* pcx = o_LoadPcx16("dfault24.pcx");
//
//_Pcx16_* _new = _Pcx16_::CreateNew((char*)"lol_lol.pcx", pcx->width / 2, pcx->height / 2);
//
//_new->DrawPcx16ResizedBicubic(pcx, pcx->width, pcx->height, 0, 0, _new->width, _new->height);
//_DlgStaticPcx16_* dlg_pcx16 = _DlgStaticPcx16_::Create(400, 200, _new->width, _new->width, 234, nullptr, 0);
//_new->Sharpen(0.50);
//dlg_pcx16->pcx16 = _new;
//dlg->AddItem(dlg_pcx16);



void Debug(int a = 1)
{
	y[80] = a;
	ExecErmCmd("IF:L^%Y80^");
}


bool MMstrings[3] = { 0,0,0 };
std::string onlineVersion = "";
std::string localVersion = "";

char* GetEraJSON(const char* json_string_name) {
	return tr(json_string_name);
}
int __stdcall GameStart(LoHook* h, HookContext* c)
{
	h->Undo();

	if (std::atoi(GetEraJSON("gem_plugin.main_menu.current_version.display_version")) == 1)
	{
		MMstrings[0] = true;
		if (std::atoi(GetEraJSON("gem_plugin.main_menu.current_version.read_registry")) == 1)
			localVersion = getGameFromRegistry();
	}

	if (std::atoi(GetEraJSON("gem_plugin.main_menu.online_version.display_version")) == 1)
	{
		MMstrings[1] = true;
		if (std::atoi(GetEraJSON("gem_plugin.main_menu.online_version.check_online")) == 1)
		{
			string myVer = GetEraJSON("gem_plugin.main_menu.online_version.remote_file");
			try
			{
				std::thread version_check(GetOnlineVersion, myVer, std::ref(onlineVersion)); // get online version in other thread
				version_check.detach();
			}
			catch (const std::exception& e)
			{
				printf(e.what());
			}
		}
	}

	if (std::atoi(GetEraJSON("gem_plugin.main_menu.era_version.display_version")) == 1)
		MMstrings[2] = true;

	return EXEC_DEFAULT;
}
//int __stdcall Dlg_OnMouseMove(HiHook* hook, H3Dlg* dlg, int Xabs, int Yasb)

void Dlg_MainMenu_CreateText(_Dlg_* dlg, int x, int y, int length, string myText, int id)
{

	if (800 - x < length)
		x = 800 - length;
	if (600 - y < 16)
		y = 600 - 16;

	//dlg->AddItem(_DlgStaticText_::Create(0, 0, 1, 1, (char*)myText.c_str(), (char*)"medfont2.fnt", 7, id, 0, 0));

	dlg->AddItem(_DlgStaticText_::Create(x, y, length, 16, (char*)myText.c_str(), (char*)"medfont2.fnt", 7, id, ALIGN_H_CENTER, 0));
}
void Dlg_MainMenu_Info(_Dlg_* dlg)
{

	if (MMstrings[0])
	{
		string jsonString = "gem_plugin.main_menu.current_version";

		string c_version;
		if (localVersion.size())
			c_version = localVersion;
		else
			c_version = dllText::PLUGIN_VERSION;
		if (std::atoi(GetEraJSON((jsonString + ".custom_version").c_str())))
		{
			c_version = GetEraJSON((jsonString + ".custom_version").c_str());

			while (c_version.length() < 4)
				c_version += "0";
		}

		int xPos = std::atoi(GetEraJSON((jsonString + ".x").c_str()));
		int yPos = std::atoi(GetEraJSON((jsonString + ".y").c_str()));
		int id = 550;
		//c_version = GetEraJSON("gem_plugin.version");
		int charLength = 8;

		std::string textLine = "ASSEMBLY v" + c_version.erase(4);
		std::string checkLine = GetEraJSON((jsonString + ".custom_text").c_str());

		if (checkLine.length())
			textLine = checkLine;

		int stringLength = std::atoi(GetEraJSON((jsonString + ".length_in_px").c_str()));

		if (stringLength <= 0)
			stringLength = textLine.length() * charLength;

		Dlg_MainMenu_CreateText(dlg, xPos, yPos, stringLength, textLine, id);
	}

	if (MMstrings[1])
	{
		string jsonString = "gem_plugin.main_menu.online_version";
		std::string textLine = "";
		if (onlineVersion > "1")
			textLine = "(latest v" + onlineVersion.erase(4) + ")";
		else
			textLine = "(latest \"N/A\")";


		int xPos = std::atoi(GetEraJSON((jsonString + ".x").c_str()));
		int yPos = std::atoi(GetEraJSON((jsonString + ".y").c_str()));
		int id = 551;
		//c_version = GetEraJSON("gem_plugin.version");
		int charLength = 7;

		std::string checkLine = GetEraJSON((jsonString + ".custom_text").c_str());
		if (checkLine.length())
			textLine = checkLine;

		int stringLength = std::atoi(GetEraJSON((jsonString + ".length_in_px").c_str()));

		if (stringLength <= 0)
			stringLength = textLine.length() * charLength;

		Dlg_MainMenu_CreateText(dlg, xPos, yPos, stringLength, textLine, id);

	}

	if (onlineVersion.size() && localVersion.size() && onlineVersion > "1" && onlineVersion > localVersion)
	{
		//string newVersionMsg = "New version " + onlineVersion.erase(4) + " is available\n\nDo You wanna update?";
		// CALL_12(void, __fastcall, 0x4F6C00, std::to_string(msg->type).c_str(), 1, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
   //     int result = MessageBoxA(NULL, newVersionMsg.c_str(), "", MB_OKCANCEL);
	 //   if (result == MBX_OK)
		{
			//CALL_12(void, __fastcall, 0x4F6C00, newVersionMsg.c_str(), 2, -1, -1, -1, 0, -1, 0, -1, 0, -1, 0);
			// CreateProcessA()
	  //      CALL_1(char, __thiscall, 0x4F3D20, 0);


		}
		// ShowMessage(newVersionMsg.c_str());
		//  msg->type
	}
	//_DlgCallbackButton_ * btn = _DlgCallbackButton_::Create()
	_DlgItem_* wndText = dlg->GetItem(545);
	if (wndText)
		wndText->Hide();

	if (MMstrings[2])
	{
		string jsonString = "gem_plugin.main_menu.era_version";

		int xPos = std::atoi(GetEraJSON((jsonString + ".x").c_str()));
		int yPos = std::atoi(GetEraJSON((jsonString + ".y").c_str()));
		int id = 545;
		//c_version = GetEraJSON("gem_plugin.version");
		int charLength = 7;

		string textLine = "HoMM3 ERA " + (string)GetEraVersion();

		std::string checkLine = GetEraJSON((jsonString + ".custom_text").c_str());
		if (checkLine.length())
			textLine = checkLine;

		int stringLength = std::atoi(GetEraJSON((jsonString + ".length_in_px").c_str()));

		if (stringLength <= 0)
			stringLength = textLine.length() * charLength;

		Dlg_MainMenu_CreateText(dlg, xPos, yPos, stringLength, textLine, id);
	}
}


int __stdcall gem_Dlg_MainMenu_Create(LoHook* hook, HookContext* c) //at the and of the Create function
{
	_Dlg_* dlg = (_Dlg_*)c->ecx;// -0x280); //edi - from IDA //changed to ecx cause i like ecx
	Dlg_MainMenu_Info(dlg);
	return EXEC_DEFAULT;
}



void HooksInit()
{

	//Dlg's
 //   _PI->WriteLoHook(0x4FBD71, gem_Dlg_MainMenu_Create);
  //  _PI->WriteLoHook(0x4EF32A, gem_Dlg_MainMenu_NewGame);
  //  _PI->WriteLoHook(0x4EF665, gem_Dlg_MainMenu_LoadGame);
 //   _PI->WriteLoHook(0x4F0799, gem_Dlg_MainMenu_CampaignGame); //goes from new game
	_PI->WriteLoHook(0x4EF252, gem_Dlg_MainMenu_Create);
	_PI->WriteLoHook(0x4EF32A, gem_Dlg_MainMenu_Create);
	_PI->WriteLoHook(0x4EF665, gem_Dlg_MainMenu_Create);
	_PI->WriteLoHook(0x4F0799, gem_Dlg_MainMenu_Create); //goes from new game



	//_PI->WriteDword(0x5F3E46 +1, 0x0C0 - 64);


   //_PI->WriteLoHook(0x471FF9, NewIntBttnPos_BattleDlg_Create);
   //_PI->WriteLoHook(0x5E7F76, OnOpenMarketWindowDlgLow);

	_PI->WriteLoHook(0x597870, GameStart);
	return;

}
#ifdef o_BPP 

#endif // !o_BPP 


BOOL APIENTRY DllMain(HMODULE hModule,
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
			ConnectEra();
			//RegisterHandler(OnBattleReplay, "OnBattleReplay");

			globalPatcher = GetPatcher();
			_PI = globalPatcher->CreateInstance((char*)"ERA.assembly.plugin");
			HooksInit();


		}
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

