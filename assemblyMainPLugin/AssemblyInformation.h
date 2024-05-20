#pragma once
#include "pch.h"
class AssemblyInformation :
    public IGamePatch
{




	struct Version
	{
		int x =0;
		int y =0;
		UINT16 id = -1;
		UINT characterLength = 7;
		const char* format = 0;
		H3String version {};
		H3String text{};

		bool customText = false;
		bool show = false;

		virtual void GetVersion();
		virtual void GetJsonData(const char*jsonSubKey);

		H3DlgText* dlgItem = nullptr;
		H3DlgText* AddToDlg(H3BaseDlg* dlg) noexcept;
		void SetItemText();

	}  m_eraVersion;


	struct LocalVersion :public Version
	{
		bool readRegistry = false;
		bool customVersion = false;

		void GetJsonData(const char* jsonSubKey) override;
		void GetVersion() override;

		bool ReadRgistry(const char* registryKey);


	}m_localVersion;


	struct RemoteVersion :public Version
	{
		const char* remoteFileUrl;
		void GetVersion() override;
		void GetJsonData(const char* jsonSubKey) override;

	} m_remoteVersion;


	AssemblyInformation(PatcherInstance* _pi);


	static _LHF_(DlgMainMenu_Create);
	void CreatePatches()  noexcept override;
	
public:
	static AssemblyInformation& Get();
	static const char* GetAssemblyVesrion();
	void LoadDataFromJson();


};

