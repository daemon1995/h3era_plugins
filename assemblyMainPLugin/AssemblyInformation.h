#pragma once
#include "pch.h"
#include <atomic>

class AssemblyInformation :
    public IGamePatch
{

	static const char* m_iniPath;

	struct PluginText:public IPluginText
	{


		void Load() override;
	} m_text;

	struct Version
	{
		int x =0;
		int y =0;
		UINT16 itemId = -1;
		UINT characterLength = 7;
		const char* format = 0;
		H3String version = h3_NullString;
		H3String text =h3_NullString;

		bool customText = false;
		bool show = false;
		bool alwaysDraw = false;

		virtual void GetVersion() noexcept;
		virtual void GetJsonData(const char*jsonSubKey);

		H3DlgText* dlgItem = nullptr;
		H3DlgText* AddToDlg(H3BaseDlg* dlg) noexcept;
		virtual void AdjustItemText() noexcept;

	}  m_eraVersion;


	struct LocalVersion :public Version
	{
		bool readRegistry = false;
		bool customVersion = false;

		void GetJsonData(const char* jsonSubKey) override;
		void GetVersion() noexcept override;

		bool ReadRgistry(const char* registryKey);

		bool remoteVersionIsHigher = false;
		virtual void AdjustItemText() noexcept final override;


	} m_localVersion;

	struct RemoteVersion :public Version
	{
		H3String remoteFileUrl;
		void GetVersion() noexcept override;
		void GetJsonData(const char* jsonSubKey) override;
		std::atomic<bool> workDone;

	} m_remoteVersion;

	Version* versions[3];

	void CreateDlgItems(H3BaseDlg* dlg);
	AssemblyInformation(PatcherInstance* _pi);

	static int __stdcall DlgMainMenu_Proc(HiHook* h, H3Msg* msg);
	static void __stdcall OnAfterReloadLanguageData(Era::TEvent* e);
	static int __stdcall DlgMainMenu_Campaign_Run(HiHook* h, H3BaseDlg* dlg);
	static int __stdcall DlgMainMenu_NewLoad_Create(HiHook* h, H3BaseDlg* dlg, const int val);
	static int __stdcall DlgMainMenu_Create(HiHook* h, H3BaseDlg* dlg);

	void CreatePatches()  noexcept override;
	
public:
	static AssemblyInformation& Get();
	static const char* GetAssemblyVesrion();
	void LoadDataFromJson();


};

