#include "pch.h"
#include "AssemblyInformation.h"

#include "webFunctions.h"

constexpr const char* BASE_JSON_KEY = "gem_plugin.main_menu";

const char* AssemblyInformation::m_iniPath = "ERA_Project.ini";
// @todo // add red color for different versions
AssemblyInformation::AssemblyInformation(PatcherInstance* _pi) :IGamePatch(_pi)
{


}
void AssemblyInformation::Version::GetVersion() noexcept
{
	//	version = Era::GetEraVersion();
}

void AssemblyInformation::Version::GetJsonData(const char* jsonSubKey)
{
	bool readSuccess = false;

	sprintf(h3_TextBuffer, "%s.%s.display_version", BASE_JSON_KEY, jsonSubKey);
	show = EraJS::readInt(h3_TextBuffer, readSuccess);

	if (show && readSuccess)
	{
		sprintf(h3_TextBuffer, "%s.%s.x", BASE_JSON_KEY, jsonSubKey);
		x = EraJS::readInt(h3_TextBuffer);

		sprintf(h3_TextBuffer, "%s.%s.y", BASE_JSON_KEY, jsonSubKey);
		y = EraJS::readInt(h3_TextBuffer);

		sprintf(h3_TextBuffer, "%s.%s.custom_text", BASE_JSON_KEY, jsonSubKey);
		text = EraJS::read(h3_TextBuffer, readSuccess);
		if (text.Empty())
			customText = false;

		sprintf(h3_TextBuffer, "%s.%s.format", BASE_JSON_KEY, jsonSubKey);
		format = EraJS::read(h3_TextBuffer);

		sprintf(h3_TextBuffer, "%s.%s.length_in_px", BASE_JSON_KEY, jsonSubKey);
		UINT lenFromJson = EraJS::readInt(h3_TextBuffer, readSuccess);
		if (readSuccess && lenFromJson > 0)
			characterLength = lenFromJson;

	}

}

H3DlgText* AssemblyInformation::Version::AddToDlg(H3BaseDlg* dlg)  noexcept
{
	const int itemWidth = text.Length() * characterLength;
	if (800 - x < itemWidth)
		x = 800 - itemWidth;
	if (600 - y < 16)
		y = 600 - 16;
	auto it = dlg->CreateText(x, y, itemWidth, 16, text.String(), h3::NH3Dlg::Text::MEDIUM, 7, itemId, eTextAlignment::HLEFT);
	//if (it)
	//	*reinterpret_cast<int*>(*reinterpret_cast<int*>(it) +0x44) = eTextColor::RED;
	return it;
}

void AssemblyInformation::Version::AdjustItemText() noexcept
{
	if (!customText)
	{
		sprintf(h3_TextBuffer, format, version.String());
		text = h3_TextBuffer;

	}

}

void AssemblyInformation::LocalVersion::AdjustItemText() noexcept
{
	Version::AdjustItemText();
	if (!customText && remoteVersionIsHigher)
	{

	}

}


#include <thread>
#include "webFunctions.h"
void AssemblyInformation::RemoteVersion::GetJsonData(const char* jsonSubKey)
{
	Version::GetJsonData(jsonSubKey);
	if (show && !customText)
	{
		Era::ReadStrFromIni("ReleaseURL", "GitHub", m_iniPath, h3_TextBuffer);
		remoteFileUrl = h3_TextBuffer;

		workDone = false;
		std::thread th(&AssemblyInformation::RemoteVersion::GetVersion, this);
		th.detach();
	}

}

void AssemblyInformation::RemoteVersion::GetVersion() noexcept
{
	// reset buffer
	sprintf(h3_TextBuffer, "%s", "");

	// create WideStringsPtr
	Era::ReadStrFromIni("API", "GitHub", m_iniPath, h3_TextBuffer);
	std::string narrowString = h3_TextBuffer;
	std::wstring api(narrowString.begin(), narrowString.end());

	Era::ReadStrFromIni("Host", "GitHub", m_iniPath, h3_TextBuffer);
	narrowString = h3_TextBuffer;
	std::wstring host(narrowString.begin(), narrowString.end());

	Era::ReadStrFromIni("Path", "GitHub", m_iniPath, h3_TextBuffer);
	narrowString = h3_TextBuffer;
	std::wstring path(narrowString.begin(), narrowString.end());

	version = "";
	if (!api.empty() && !host.empty() && !path.empty())
	{
		// make an HTTP request (thanks to Chat GPT)
		std::string requestResponce = PerformWinHTTPRequest(api.c_str(), host.c_str(), path.c_str());
		if (!requestResponce.empty())
		{
			// parse json (thanks to nlohmann)
			nlohmann::json j = nlohmann::json::parse(requestResponce, nullptr, false);
			// get object and check if it is correct
			auto& obj = j["tag_name"];
			if (!obj.is_null() && obj.is_string())
				version = obj.get<std::string>().c_str();
		}

	}
	workDone.store(true);


}
void AssemblyInformation::LocalVersion::GetJsonData(const char* jsonSubKey)
{
	Version::GetJsonData(jsonSubKey);
	bool readSuccess = false;

	sprintf(h3_TextBuffer, "%s.%s.custom_version", BASE_JSON_KEY, jsonSubKey);
	version = EraJS::read(h3_TextBuffer, readSuccess);
	if (readSuccess && !version.Empty())
		customVersion = true;

	sprintf(h3_TextBuffer, "%s.%s.read_registry", BASE_JSON_KEY, jsonSubKey);
	readRegistry = EraJS::readInt(h3_TextBuffer);
	if (!customText)
		GetVersion();

}

EXTERN_C __declspec(dllexport) void GetAssemblyGameVersion(char* buffer)
{
	sprintf(buffer, AssemblyInformation::GetAssemblyVesrion());
}

const char* AssemblyInformation::GetAssemblyVesrion()
{
	return AssemblyInformation::Get().m_localVersion.version.String();
}

void AssemblyInformation::LocalVersion::GetVersion()noexcept
{

	if (!customVersion)
	{
		Era::ReadStrFromIni("key", "Registry", m_iniPath, h3_TextBuffer);
		if (ReadRgistry(h3_TextBuffer))
		{

		}

		if (Era::ReadStrFromIni("Version", "Assembly", "ERA_Project.ini", h3_TextBuffer))
			version = h3_TextBuffer;
	}

}

bool AssemblyInformation::LocalVersion::ReadRgistry(const char* registryKey)
{
	HKEY hKey;

	LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, registryKey, 0, KEY_READ, &hKey);
	bool bExistsAndSuccess(lRes == ERROR_SUCCESS);

	if (bExistsAndSuccess)
	{
		CHAR szBuffer[64];
		DWORD dwBufferSize = sizeof(szBuffer);
		ULONG nError;
		nError = RegQueryValueExA(hKey, "version", 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
		if (ERROR_SUCCESS == nError)
			version = szBuffer;
		RegCloseKey(hKey);
		bExistsAndSuccess = !version.Empty();
	}

	return bExistsAndSuccess;

}


_LHF_(AssemblyInformation::DlgMainMenu_Create)
{


	H3BaseDlg* dlg = reinterpret_cast<H3BaseDlg*>(c->ecx);// -0x280); //edi - from IDA //changed to ecx cause i like ecx


	// hidw wnd version hint
	if (auto* it = dlg->GetH3DlgItem(545))
		it->Hide();

	Version* versions[3] = { &Get().m_eraVersion,&Get().m_localVersion,&Get().m_remoteVersion };
	for (const auto it : versions)
	{
		if (it->show)
		{
			it->AdjustItemText();
			it->dlgItem = it->AddToDlg(dlg);
		}
	}


	return EXEC_DEFAULT;
}
#include <ShellAPI.h>;

int __stdcall AssemblyInformation::DlgMainMenu_Proc(HiHook* h, H3Msg* msg)
{
	AssemblyInformation::RemoteVersion& remoteVersion = Get().m_remoteVersion;
	H3DlgText* it = remoteVersion.dlgItem;
	if (it && it->IsVisible())
	{
		if (remoteVersion.workDone.load())
		{

			remoteVersion.AdjustItemText();
			it->SetText(remoteVersion.text);
			it->SetWidth((remoteVersion.text.Length() + 1) * remoteVersion.characterLength);
			it->Draw();
			it->Refresh();

			////	AssemblyInformation::LocalVersion& localVersion = Get().m_localVersion;

			//	//if (localVersion.dlgItem&& remoteVersion.text.String() > localVersion.text.String())
			//	//{
			//	//	localVersion.AdjustItemText();

			//	//	localVersion.text = H3String::Format("{~r}%s}", localVersion.text.String());
			//	//	localVersion.dlgItem->SetText(remoteVersion.text);

			//	//	localVersion.dlgItem->Draw();
			//	//	localVersion.dlgItem->Refresh();
			//	//}
			//	//remoteVersion.workDone.store(false);
			//	
		}
		H3POINT mousePos = msg->GetCoords().GetCurrentCursorPosition();
		RECT rect = { it->GetAbsoluteX() ,it->GetAbsoluteY() ,it->GetAbsoluteX() + it->GetWidth(),it->GetAbsoluteY() + it->GetHeight() };

		if (rect.left <= mousePos.x && mousePos.x <= rect.right
			&& rect.top <= mousePos.y && mousePos.y <= rect.bottom)
		{
			P_MouseManager->SetCursor(41, 1);
			if (msg->IsLeftDown())
			{
				P_SoundManager->ClickSound();
				ShellExecuteA(NULL, "open", remoteVersion.remoteFileUrl.String(), NULL, NULL, SW_SHOWNORMAL);
			}

		}
		else
			P_MouseManager->SetCursor(0, 0);

	}
	else
	{
		h->Undo();
	}

	return FASTCALL_1(int, h->GetDefaultFunc(), msg);
}

void __stdcall AssemblyInformation::OnAfterReloadLanguageData(Era::TEvent* e)
{
	Get().LoadDataFromJson();
	Version* versions[3] = { &Get().m_eraVersion,&Get().m_localVersion,&Get().m_remoteVersion };
	for (const auto it : versions)
	{
		if (it && it->show)
		{
			it->AdjustItemText();
		}
	}
}



void AssemblyInformation::CreatePatches()  noexcept
{
	if (!m_isEnabled)
	{
		_PI->WriteLoHook(0x4EF259, DlgMainMenu_Create);
		_PI->WriteLoHook(0x4EF331, DlgMainMenu_Create);
		_PI->WriteLoHook(0x4EF668, DlgMainMenu_Create);
		_PI->WriteLoHook(0x4F0799, DlgMainMenu_Create); //goes from new game

		// move and resize iam00.def (next hero buttn)
		_PI->WriteByte(0x401A85 + 1, 32); // set width
		_PI->WriteDword(0x401A8C + 1, 679 + 32); // set y

		_PI->WriteHiHook(0x4FBDA0, THISCALL_, DlgMainMenu_Proc); // Main Main Menu Dlg Proc
	//	Era::RegisterHandler(OnAfterReloadLanguageData, "OnAfterReloadLanguageData");

		m_isEnabled = true;
	}
}


AssemblyInformation& AssemblyInformation::Get()
{
	static AssemblyInformation instance(_PI);
	return instance;

	// TODO: insert return statement here
}

void AssemblyInformation::LoadDataFromJson()
{
	m_eraVersion.GetJsonData("era_version");
	m_localVersion.GetJsonData("current_version");
	m_remoteVersion.GetJsonData("online_version");
	m_eraVersion.version = Era::GetEraVersion();
	if (m_eraVersion.show || m_localVersion.show || m_remoteVersion.show)
		CreatePatches();

	m_isInited = true;

}

void AssemblyInformation::PluginText::Load()
{
}
