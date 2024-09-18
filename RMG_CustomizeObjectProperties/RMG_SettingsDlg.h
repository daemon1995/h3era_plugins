#pragma once
struct RMG_Settings;
namespace rmgdlg
{

	/// <summary>
	/// FOR the PANDORA BOXES Create Spoilers:
	/// 
	/// </summary>

	class RMGObject
	{

	public:
		// displayed/edited rmgObjInfo
		RMGObjectInfo objectInfo;
		// properties of the real object - dont change
		const H3ObjectAttributes* attributes;
		// displayd resized pcx16 - should be deleted before dlg close
		H3LoadedPcx16* objectPcx = nullptr;

		struct Graphics
		{
			const H3ObjectAttributes* attributes;
			H3LoadedPcx16* objectPcx = nullptr;

		};

		//std::vector<Graphics> objectPictures;
		//UINT lastDrawnFrame = NULL;

	private:
		RMGObject();
	public:

		//	Object(const H3ObjectAttributes& attributes, const RMGObjectInfo& objectInfo);
		//Object(const H3ObjectAttributes& attributes);
		RMGObject(const H3ObjectAttributes& attributes, H3LoadedPcx16* objectPcx = nullptr);
		//Object(std::pair < H3ObjectAttributes, H3LoadedPcx16*> info);
		//~Object();
	};

	class RMG_SettingsDlg : public H3Dlg
	{

		//std::vector<H3CreatureBankSetup> &creatureBanks;


	private:

		struct Page;

		struct ObjectsPanel
		{
			int x;
			int y;
			static UINT id;

			BOOL visible = false;
			Page* parentPage = nullptr;
			RMGObject* rmgObject = nullptr;

			H3DlgPcx16* pictureItem = nullptr;
			H3DlgPcx16* backgroundPcx = nullptr;
			H3DlgText* objectNameItem = nullptr;


			// dlg control for user input
			union
			{
				struct
				{
					H3DlgEdit* mapLimitEdit;
					H3DlgEdit* zoneLimitEdit;
					H3DlgEdit* valueEdit;
					H3DlgEdit* densityEdit;
				};
				H3DlgEdit* edits[4] = {};
			};



			H3DlgDef* enabledCheckBox = nullptr;

			std::vector<H3DlgItem*> items;


			ObjectsPanel(const int x, const int y, Page* parent);
			virtual ~ObjectsPanel();

		public:
			void SetVisible(const BOOL state) noexcept;
			void SetObject(RMGObject* mapObject) noexcept;
			//	void ObjectInfoToPanelInfo() noexcept;
			void ObjectInfoToPanelInfo() noexcept;
			void PanelInfoToObjectInfo() noexcept;

			// returns true if any of edits was focused
			const BOOL UnfocusEdits(const BOOL saveChanges) noexcept;

		};

		struct Page
		{
			const char* name;
			UINT id;
			BOOL visible;
			UINT firstItemCount = 0;
			H3DlgCaptionButton* captionbttn = nullptr;
			H3DlgScrollbar* verticalScrollBar = nullptr;
			H3DlgScrollbar* horizontalScrollBar = nullptr;




			static RMG_SettingsDlg* dlg;

			Page(H3DlgCaptionButton* captionbttn);
			virtual ~Page();

		public:
			virtual void FillObjects(int firstItemId = 0) = 0;
			virtual void SetVisible(const bool state) = 0;
			virtual void SaveData() = 0;
			virtual void SetRandom(const H3Msg& msg) = 0;
			virtual void SetDefault() = 0;
			virtual BOOL Proc(H3Msg& msg) = 0;
		};

		struct ObjectsPage :public Page
		{
			struct PageHeader
			{

				BOOL visible = false;
				H3DlgPcx16* backgroundPcx = nullptr;
				std::vector<H3DlgDef*> arrows;
				std::vector<H3DlgItem*> items;

				PageHeader(const int x, const int y, const int width, const int height, const int objectsNum);

				//PageHeader();
				void SetVisible(bool state);

			}*pageHeader = nullptr;

			struct
			{
				int type = -1;
				BOOL isReverse = false;
			} lastSorting;


			// used to have copies 
			const std::vector<std::pair<H3ObjectAttributes, H3LoadedPcx16*>>* objectAttrbts;
			//std::vector<H3ObjectAttributes> displayedAttributes;
			BOOL ignoreSubtypes;


			std::vector<ObjectsPanel*> dlgPanels;
			std::vector<RMGObject> RMGObjects;

			ObjectsPage(H3DlgCaptionButton* captionbttn, const std::vector<std::pair<H3ObjectAttributes, H3LoadedPcx16*>>& attributes, const BOOL ignoreSubtypes = false);
			virtual ~ObjectsPage();

		protected:

			virtual void FillObjects(int firstItemId = 0) override;
			virtual void SaveData();
			virtual void SetRandom(const H3Msg& msg) override;
			virtual void SetDefault() override;
			virtual BOOL Proc(H3Msg& msg) override;
			virtual BOOL ShowObjectExtendedInfo(const ObjectsPanel* panel, const H3Msg& msg) const noexcept;

		protected:
			void SetVisible(const bool state);
			void CreateVerticalScrollBar();
			void CreateHorizontalScrollBar();

		public:
			void ResetSortingState();


		private:
			static void __fastcall VerticalScrollBarProc(INT32 tick, H3BaseDlg* dlg);
			static void __fastcall HorizontalScrollBarProc(INT32 tick, H3BaseDlg* dlg);

		};


		struct BanksPage :public ObjectsPage
		{
			BanksPage(H3DlgCaptionButton* captionbttn, const std::vector<std::pair<H3ObjectAttributes, H3LoadedPcx16*>>& data, const BOOL ignoreSubtypes = false);
			virtual ~BanksPage();

			virtual BOOL ShowObjectExtendedInfo(const ObjectsPanel* panel, const H3Msg& msg) const noexcept final override;

			//bool SaveSettings();

		};

		struct MiscPage :public ObjectsPage
		{
			//virtual void ShowObjectExtendedInfo(const ObjectsPanel* panel) const noexcept final override;

			MiscPage(H3DlgCaptionButton* captionbttn, const std::vector<std::pair<H3ObjectAttributes, H3LoadedPcx16*>
			>& data);
			virtual ~MiscPage();

			//static void __fastcall ObjectPage_ScrollBarProc(INT32 tick, H3BaseDlg* dlg);

			//bool SaveSettings();

		};

		struct DwellingsPage :public ObjectsPage
		{
			//virtual void ShowObjectExtendedInfo(const ObjectsPanel* panel) const noexcept final override;

			DwellingsPage(H3DlgCaptionButton* captionbttn, const std::vector< std::pair<H3ObjectAttributes, H3LoadedPcx16*>>& data);
			//virtual ~DwellingsPage();

			//static void __fastcall ObjectPage_ScrollBarProc(INT32 tick, H3BaseDlg* dlg);

			//bool SaveSettings();

		};

	private:

		RMG_Settings* m_settings;
		//static constexpr const char* m_iniPath = "Runtime/RMG_CustomizeObjectsProperties.ini";




		UINT m_lastPageId = 0;
		H3DlgPcx16* headerPcx = nullptr;
		BOOL blockLettersInput = false;
		Page* m_currentPage;
		BOOL m_randomIsPressed = false;
		std::vector<Page*> m_pages;
		std::vector<H3DlgCaptionButton*> captionButtons;

	private:

		static RMG_SettingsDlg* instance;
		static std::vector<std::pair<H3ObjectAttributes, H3LoadedPcx16*>> m_banks;
		static std::vector<std::pair<H3ObjectAttributes, H3LoadedPcx16*>> m_commonObjects;
		static std::vector<std::pair<H3ObjectAttributes, H3LoadedPcx16*>> m_dwellings;
		static std::vector<std::pair<H3ObjectAttributes, H3LoadedPcx16*>> m_wogObjects;
		static const std::vector<std::vector<std::pair<H3ObjectAttributes, H3LoadedPcx16*>>*> m_objectAttributes;

	public:
		static constexpr const char* dlgIniPath = "Runtime/RMG_CustomizeObjectsProperties.ini";


		// ctors
	public:
		RMG_SettingsDlg(int width, int height, int x, int y);

		// virtual methods
	private:
		virtual BOOL OnCreate() override;
		virtual BOOL DialogProc(H3Msg& msg) override;
		virtual VOID OnOK() override;
		virtual VOID OnCancel() override;
		virtual ~RMG_SettingsDlg();

	private:
		const BOOL ReadIniDlgSettings() noexcept;
		const BOOL WriteIniDlgSettings() const noexcept;

		BOOL SetActivePage(Page* page) noexcept;
		BOOL SaveObjects(const BOOL saveIni = true);

		VOID OnHelp();

		const BOOL RemoveEditsFocus(const BOOL save) const noexcept;



		// hooks
	private:
		static _LHF_(H3SelectScenarioDialog_HideRandomMapsSettings);
		static _LHF_(H3SelectScenarioDialog_ShowRandomMapsSettings);

		//static void __stdcall Dlg_SelectScenario_Proc(HiHook* hook, H3Msg* msg);
		static _LHF_(Dlg_SelectScenario_Proc);

		static void __stdcall NewScenarioDlg_Create(HiHook* hook, H3SelectScenarioDialog* dlg, const DWORD dlgCallType);
		static H3Msg* __stdcall H3DlgEdit__TranslateInputKey(HiHook* h, H3InputManager* inpt, H3Msg* msg);

	public:
		static void SetPatches(PatcherInstance* _pi);
		static const  std::vector<std::vector<std::pair<H3ObjectAttributes, H3LoadedPcx16*>>*>& GetObjectAttributes() noexcept;


	};

}