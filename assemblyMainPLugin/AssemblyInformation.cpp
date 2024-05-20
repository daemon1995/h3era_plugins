#include "pch.h"
#include "AssemblyInformation.h"

#include "webFunctions.h"

constexpr const char* BASE_JSON_KEY = "gem_plugin.main_menu";



AssemblyInformation::AssemblyInformation(PatcherInstance* _pi) :IGamePatch(_pi)
{


}
void AssemblyInformation::Version::GetVersion()
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

	return dlg->CreateText(x, y, itemWidth, 16, text.String(), h3::NH3Dlg::Text::MEDIUM, 7, id);
}

void AssemblyInformation::Version::SetItemText()
{
	if (!customText)
	{
		sprintf(h3_TextBuffer, format, version.String());
		text = h3_TextBuffer;
	}
	
}


#include <thread>
#include "webFunctions.h"
void AssemblyInformation::RemoteVersion::GetJsonData(const char* jsonSubKey)
{
	Version::GetJsonData(jsonSubKey);
	sprintf(h3_TextBuffer, "%s.%s.remote_file", BASE_JSON_KEY, jsonSubKey);
	bool readSuccess = false;
	remoteFileUrl = EraJS::read(h3_TextBuffer, readSuccess);

	if (!customText && readSuccess && remoteFileUrl)
	{
		std::thread th(&AssemblyInformation::RemoteVersion::GetVersion, this);
		th.detach();
	}

}
void AssemblyInformation::RemoteVersion::GetVersion()
{
	// USES_CONVERSION_EX;
	BOOL clearCa = DeleteUrlCacheEntryA(remoteFileUrl);


	ComInit init;
	// use CComPtr so you don't have to manually call Release()
	CComPtr<IStream> pStream;
	// Open the HTTP request.
	HRESULT hr = URLOpenBlockingStreamA(nullptr, remoteFileUrl, &pStream, 0, nullptr);
	if (FAILED(hr))
		version = "N/A";
	else
	{
		char buffer[64] = "";
		do
		{
			DWORD bytesRead = 0;
			hr = pStream->Read(buffer, sizeof(buffer), &bytesRead);

			if (bytesRead > 0)
			{
				version = buffer;
				return;
			}

		} while (SUCCEEDED(hr) && hr != S_FALSE);

		if (FAILED(hr))
			version = "N/A";

	}
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

void AssemblyInformation::LocalVersion::GetVersion()
{

	if (!customVersion)
	{
		if (ReadRgistry("SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\HoMM 3 ERA\\"))
		{

		}

		if (Era::ReadStrFromIni("Version", "Assembly", "default heroes3.ini", h3_TextBuffer))
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
		bExistsAndSuccess = !text.Empty();
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
			it->SetItemText();
			it->dlgItem = it->AddToDlg(dlg);
		}
	}




	return EXEC_DEFAULT;
}




void AssemblyInformation::CreatePatches()  noexcept
{
	if (!m_isEnabled)
	{
		_PI->WriteLoHook(0x4EF252, DlgMainMenu_Create);
		_PI->WriteLoHook(0x4EF32A, DlgMainMenu_Create);
		_PI->WriteLoHook(0x4EF665, DlgMainMenu_Create);
		_PI->WriteLoHook(0x4F0799, DlgMainMenu_Create); //goes from new game
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
