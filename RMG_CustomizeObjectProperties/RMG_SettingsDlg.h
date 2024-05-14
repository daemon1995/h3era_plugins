#pragma once
#include <vector>
#include <set>
class RMG_Settings;
class RMG_ObjectsSettings;




class MapObject
{


public:

	H3LoadedPcx16* enabledObjectsPcx;
	H3LoadedPcx16* disabledObjectsPcx;
	BOOL enabled;
	INT32 mapLimit;
	INT32 zoneLimit;
	const H3ObjectAttributes& attributes;
	MapObject(const H3ObjectAttributes& attributes);
	~MapObject();
	LPCSTR GetObjectName() const;

};

class RMG_SettingsDlg : public H3Dlg
{




	//std::vector<H3CreatureBankSetup> &banks;
	
	RMG_Settings* m_settings;
	const char* m_iniPath ="Runtime/RMG_CustomizeObjectsProperties.ini";

	RMG_Settings* ReadIniSettings();
	bool WriteIniSettings();

	BOOL DialogProc(H3Msg& msg) override;
	VOID OnOK() override;
	VOID OnCancel() override;

	H3Vector<H3ObjectAttributes> *m_objectsList;
	H3Vector<H3ObjectAttributes> m_banks;



private:
	void GetCreatureBanks();
private:

	struct DlgPage;

	struct DlgText
	{

	};
	struct MapObject_DlgPanel
	{
		int x;
		int y;
		static UINT id;
		
		BOOL visible;
		DlgPage* parentPage;
		MapObject* data = nullptr;

		H3DlgPcx16* pictureItem =nullptr;
		H3DlgPcx16* backDlgPcx = nullptr;
		H3DlgText* objectNameItem = nullptr;


		// dlg control for user input
		H3DlgText* mapLimitTitle = nullptr;
		H3DlgEdit* mapLimitEdit = nullptr;

		H3DlgText* zoneLimitTitle = nullptr;
		H3DlgEdit* zoneLimitEdit = nullptr;
		H3Vector<H3DlgItem*> items;


		MapObject_DlgPanel(const int x, const int y,  DlgPage* parent);
		virtual ~MapObject_DlgPanel();

		void HideDeactivate();
		void ShowActivate();

		void SetData( MapObject* mapObject);
		void Refresh();
	};
	
	struct DlgPage
	{
		const char* name;
		std::vector<int> pageItemsIds;
		bool visible;



		H3DlgScrollbar* scrollBar;
		DlgPage(const char* name);
		//H3Vector<H3LoadedPcx16> resizedPcx16;

		~DlgPage();
		virtual void SetVisible(bool state) =0;
		//	H3DlgScrollbar* CreateScrollBar();
		virtual void FillObjects(int firstItemId = 0) = 0;
		virtual void SaveData() = 0;
		virtual BOOL Proc(H3Msg& msg) = 0;
		static RMG_SettingsDlg* dlg;
		//~DlgPage();
	};
	struct MapObjects_DlgPage :public DlgPage
	{

		MapObjects_DlgPage(const char* name, const H3Vector<H3ObjectAttributes>& data);
		//H3Vector<H3LoadedPcx16> resizedPcx16;

		virtual ~MapObjects_DlgPage();

		virtual void FillObjects(int firstItemId =0) override;
		virtual BOOL Proc(H3Msg& msg) override;
		void CreateDlgPanels();
		//	H3DlgScrollbar* CreateScrollBar();
		virtual void SetVisible(bool state);
		virtual void SaveData();

		std::vector<MapObject_DlgPanel*> dlgPanels;

		std::vector<MapObject*> objectParameters;


		const H3Vector<H3ObjectAttributes>& h3ObjectAttributes;

	};


	struct CreatureBanks_DlgPage :public MapObjects_DlgPage
	{
		CreatureBanks_DlgPage(const char* name, const H3Vector<H3ObjectAttributes>& data);
		static void __fastcall CreatureBankPage_ScrollBarProc(INT32 tick, H3BaseDlg* dlg);

	//	virtual ~CreatureBanks_DlgPage();
		//bool SaveSettings();

	};

	BOOL OnCreate() override;
	DlgPage* m_currentPage;

	std::vector<DlgPage*> m_pages;
	int m_lastPageId;

private:
	void SetActivePage(DlgPage* page) noexcept;
private:

	static _LHF_(H3SelectScenarioDialog_HideRandomMapsSettings);
	static _LHF_(H3SelectScenarioDialog_ShowRandomMapsSettings);
	static void __stdcall NewScenarioDlg_Create(HiHook* hook, H3SelectScenarioDialog* dlg, H3Msg* msg);
	//static void __stdcall Dlg_SelectScenario_Proc(HiHook* hook, H3Msg* msg);
	static _LHF_(Dlg_SelectScenario_Proc);

	~RMG_SettingsDlg();

public:
	RMG_SettingsDlg(int width, int height, int x, int y);

	static void SetPatches(PatcherInstance* _pi);
};

