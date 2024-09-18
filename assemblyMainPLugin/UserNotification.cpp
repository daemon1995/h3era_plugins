#include <experimental/filesystem>

#include "framework.h"

UserNotification::UserNotification(PatcherInstance* _pi) :IGamePatch(_pi)
{

	if (Era::ReadStrFromIni("datFileMissing", "UserNotifications", "Runtime/game_enhancement_mod.ini", h3_TextBuffer))
		m_ignore = atoi(h3_TextBuffer);

	CreatePatches();
	m_text.Load();
}




namespace fs = std::experimental::filesystem;
BOOL Game__CheckWogOptionsFileExists()
{
	BOOL result = false;
	H3String settingsfileName;
	H3String settingsfilePath;


	if (result = Era::ReadStrFromIni("Options_File_Name", "WoGification", "WoG.ini", h3_TextBuffer))
	{
		settingsfileName = h3_TextBuffer;

		if (result = Era::ReadStrFromIni("Options_File_Path", "WoGification", "WoG.ini", h3_TextBuffer))
		{
			settingsfilePath = h3_TextBuffer;
			result = !settingsfileName.Empty() && !settingsfilePath.Empty();
		}
	}

	if (result)
	{
		settingsfilePath.Append(settingsfileName);
		fs::path path(settingsfilePath.String());
		result = fs::exists(path) && !fs::is_directory(path);

	}

	return result;
}



_LHF_(UserNotification::H3SelectScenarioDialog__StartButtonClick)
{



	auto dlg = reinterpret_cast<H3SelectScenarioDialog*>(c->ebx);
	if (!Get().m_ignore)
	{
		if (dlg && !dlg->isCampaignMaybe && !dlg->isLoadingMaybe)
		{
			if (!Game__CheckWogOptionsFileExists())
			{
				bool hideNotification = !H3Messagebox::Choice(Get().m_text.startGameWithoutOptionsFile);
				if (hideNotification)
				{
					Get().m_ignore = true;
					Era::WriteStrToIni("datFileMissing", "1", "UserNotifications", "Runtime/game_enhancement_mod.ini");
					Era::SaveIni("Runtime/game_enhancement_mod.ini");
				}
			}
		}
	}
	else
		h->Undo();



	return EXEC_DEFAULT;

}

void __stdcall UserNotification::OnAfterReloadLanguageData(Era::TEvent* e)
{
	Get().m_text.Load();

}

void __stdcall UserNotification::AdvMgrDlg_RedrawLog(HiHook* h, H3AdventureMgrDlg* dlg, bool redrawLog)
{
	THISCALL_2(void, h->GetDefaultFunc(), dlg, redrawLog);
	auto* updateInfo = &Get().m_updateInfo;

	if (updateInfo->m_needToCheck)
	{
		//Era::ExecErmCmd("IF:L^It's time^");

		const DWORD timeDifference = GetTime() - updateInfo->m_lastCheckTime;
		// check if time to update

		if (timeDifference >= updateInfo->m_chechInterval)
		{

			updateInfo->m_lastCheckTime += timeDifference;
			AssemblyInformation::Get().CheckOnlineVersion();

		}
		if (timeDifference >= 5000
			&& AssemblyInformation::Get().CompareVersions())
		{

			//	Era::ExecErmCmd("IF:L^ThereIsNewer version^");

		}

	}

}
#include <chrono>
char __stdcall UserNotification::AdvMgr_DrawMap(HiHook* h, H3AdventureManager* advMan, const __int16 mapX, const __int16 mapY, const __int16 mapZ, const int hotSeat, char updateInfoPanel)
{
	// call default function - we don't need to affect it
	char result = THISCALL_6(char, h->GetDefaultFunc(), advMan, mapX, mapY, mapY, hotSeat, updateInfoPanel);




	return result;

}

void UserNotification::PluginText::Load() noexcept
{
	startGameWithoutOptionsFile = EraJS::read("gem_plugin.user_notification.no_dat_settings");
}


void __stdcall OnDrawActionPlayCastSpell(HiHook* h, H3CombatManager* mgr, eSpell spellId, int pos, int casterKind, int pos2, int skilLevel, int spellPower)
{

	//THISCALL_7(void, h->GetDefaultFunc(), mgr, spellId, pos, casterKind, pos2, skilLevel, skilLevel);
}

UserNotification& UserNotification::Get()
{
	static UserNotification instance(_PI);
	return instance;
}
void UserNotification::CreatePatches() noexcept
{
	if (!m_isEnabled)
	{
		_pi->WriteLoHook(0x58698B, H3SelectScenarioDialog__StartButtonClick);
		Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");

		// set notification for incoming version check
		//_pi->WriteHiHook(0x40F350, THISCALL_, AdvMgr_DrawMap);
		// 
#ifdef _DEBUG
		_pi->WriteHiHook(0x40F7AD, THISCALL_, AdvMgrDlg_RedrawLog);
#endif // _DEBUG


		m_isEnabled = true;
	}


}

