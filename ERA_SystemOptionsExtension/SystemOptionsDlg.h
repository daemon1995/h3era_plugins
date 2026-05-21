#pragma once
#include "framework.h"
enum eDlgCallSource : INT
{
    UNKNOWN = -1,
    ADV_MAP = 0,
    MAIN_MENU = 1,
    COMBAT = 2,
    TOWN_SCREEN = 3,
    HERO_SCREEN = 4,
    SWAP_MGR = 5
};
class SystemOptionsDlg : public H3Dlg
{

    static constexpr float SETTINGS_VERSION = .1f;
    static DWORD userRandSeed;

  public:
    static constexpr LPCSTR MAIN_MENU_WIDGET_UUID = "rmg_main_menu_widget";
    static constexpr LPCSTR MAIN_MENU_JSON_KEY = "RMG.text.title";

    static constexpr LPCSTR DLG_TEXT_JSON_KEY = MAIN_MENU_JSON_KEY;
    static constexpr LPCSTR SETTINGS_INI_SECTION = "DlgSettings";
    static constexpr LPCSTR INI_ALWAYS_RANDOM = "alwaysRandom";
    static constexpr LPCSTR INI_FILE_PATH = "Runtime/RMG_CustomizeObjectsProperties.ini";
    // std::vector<H3CreatureBankSetup> &creatureBanks;

  private:
    struct Page;

    struct ObjectsPanel
    {
        int x;
        int y;
        static UINT id;

        BOOL visible = false;
        Page *parentPage = nullptr;

        H3DlgPcx16 *pictureItem = nullptr;
        H3DlgPcx16 *backgroundPcx = nullptr;
        H3DlgText *objectNameItem = nullptr;

        // dlg control for user input
        union {
            struct
            {
                H3DlgEdit *mapLimitEdit;
                H3DlgEdit *zoneLimitEdit;
                H3DlgEdit *valueEdit;
                H3DlgEdit *densityEdit;
            };
            H3DlgEdit *edits[4] = {};
        };

        H3DlgDef *enabledCheckBox = nullptr;
        H3DlgDefButton *defaultButton = nullptr;

        std::vector<H3DlgItem *> items;
        DWORD lastChangedPictureTime = 0;

      public:
        ObjectsPanel(const int x, const int y, Page *parent);
        virtual ~ObjectsPanel();

      public:
        void SetVisible(const BOOL state) noexcept;
        //	void ObjectInfoToPanelInfo() noexcept;
        void ObjectInfoToPanelInfo() noexcept;
        void PanelInfoToObjectInfo() noexcept;

        // returns true if any of edits was focused
        const BOOL UnfocusEdits(const BOOL saveChanges) noexcept;
    };

    struct Page
    {
        static SystemOptionsDlg *dlg;

        const char *name;
        UINT id;
        BOOL visible;
        UINT firstItemCount = 0;
        H3DlgCaptionButton *captionbttn = nullptr;
        H3DlgScrollbar *verticalScrollBar = nullptr;
        H3DlgScrollbar *horizontalScrollBar = nullptr;

        Page(H3DlgCaptionButton *captionbttn);
        virtual ~Page();

      public:
        virtual void FillObjects(int firstItemId = 0) = 0;
        virtual void SetVisible(const bool state) = 0;
        virtual void SaveData() = 0;
        virtual void SetRandom(const H3Msg &msg) = 0;
        virtual void SetDefault() = 0;
        virtual BOOL Proc(H3Msg &msg) = 0;
    };

    struct ObjectsPage : public Page
    {
        static constexpr DWORD REFRESH_RATE_FREQUENCY = 2500;

        struct PageHeader
        {

            BOOL visible = false;
            H3DlgPcx16 *backgroundPcx = nullptr;
            std::vector<H3DlgDef *> arrows;
            std::vector<H3DlgItem *> items;

            PageHeader(const int x, const int y, const int width, const int height, const int objectsNum);

            // PageHeader();
            void SetVisible(bool state);

        } *pageHeader = nullptr;

        struct
        {
            int type = -1;
            BOOL isReverse = false;
        } lastSorting;

        // std::vector<H3ObjectAttributes> displayedAttributes;
        BOOL ignoreSubtypes;

        std::vector<ObjectsPanel *> objectsPanels;

        ObjectsPage(H3DlgCaptionButton *captionbttn, const BOOL ignoreSubtypes = false);
        virtual ~ObjectsPage();

      protected:
        virtual void FillObjects(int firstItemId = 0) override;
        virtual void SaveData();
        virtual void SetRandom(const H3Msg &msg) override;
        virtual void SetDefault() override;
        virtual BOOL Proc(H3Msg &msg) override;
        virtual BOOL ShowObjectExtendedInfo(const ObjectsPanel *panel, const H3Msg &msg) const noexcept;

      protected:
        void SetVisible(const bool state);
        void CreateVerticalScrollBar();
        void CreateHorizontalScrollBar();

      public:
        void ResetSortingState();

      private:
        static void __fastcall VerticalScrollBarProc(INT32 tick, H3BaseDlg *dlg);
        static void __fastcall HorizontalScrollBarProc(INT32 tick, H3BaseDlg *dlg);
    };

    struct BanksPage : public ObjectsPage
    {
        BanksPage(H3DlgCaptionButton *captionbttn, const BOOL ignoreSubtypes = false);
        virtual ~BanksPage();

        virtual BOOL ShowObjectExtendedInfo(const ObjectsPanel *panel, const H3Msg &msg) const noexcept final override;

        // bool SaveSettings();
    };

    struct MiscPage : public ObjectsPage
    {
        // virtual void ShowObjectExtendedInfo(const ObjectsPanel* panel) const noexcept final override;

        MiscPage(H3DlgCaptionButton *captionbttn);
        virtual ~MiscPage();

        // static void __fastcall ObjectPage_ScrollBarProc(INT32 tick, H3BaseDlg* dlg);

        // bool SaveSettings();
    };

    struct DwellingsPage : public ObjectsPage
    {
        // virtual void ShowObjectExtendedInfo(const ObjectsPanel* panel) const noexcept final override;

        DwellingsPage(H3DlgCaptionButton *captionbttn);
        virtual ~DwellingsPage();

        // static void __fastcall ObjectPage_ScrollBarProc(INT32 tick, H3BaseDlg* dlg);

        // bool SaveSettings();
    };

  private:
    // static constexpr const char* m_iniPath = "Runtime/RMG_CustomizeObjectsProperties.ini";
    eDlgCallSource dlgCallSource = UNKNOWN;
    UINT m_lastPageId = 0;
    H3DlgPcx16 *headerPcx = nullptr;
    BOOL blockLettersInput = false;
    Page *m_currentPage;
    BOOL m_randomIsPressed = false;
    std::vector<Page *> m_pages;
    std::vector<H3DlgCaptionButton *> captionButtons;

  public:
    static SystemOptionsDlg *instance;

  private:
    static BOOL isDlgTextEditInput;
    static BOOL userHasAccessToDlg;
    static H3MainSetup *mainSetup; // used to get default objects info from zaobj or obje txt;

    // ctors
  public:
    SystemOptionsDlg(int width, int height, int x, int y);
    SystemOptionsDlg() : SystemOptionsDlg(400, 300, -1, -1) {};
    virtual ~SystemOptionsDlg();

    // virtual methods
  private:
    virtual BOOL OnCreate() override;
    virtual BOOL DialogProc(H3Msg &msg) override;
    virtual VOID OnOK() override;
    virtual VOID OnCancel() override;

  private:
    BOOL ReadIniDlgSettings() noexcept;
    BOOL WriteIniDlgSettings() const noexcept;

    BOOL SetActivePage(Page *page) noexcept;
    BOOL SaveRMGObjectsInfo(const BOOL saveIni = true) const noexcept;

    VOID OnHelp() const noexcept;

    BOOL RemoveEditsFocus(const BOOL save) const noexcept;

    // hooks
  private:
    static _LHF_(H3SelectScenarioDialog_HideRandomMapsSettings);
    static _LHF_(H3SelectScenarioDialog_ShowRandomMapsSettings);
    static _LHF_(H3SelectScenarioDialog_StartButtonClick);

    // static void __stdcall Dlg_SelectScenario_Proc(HiHook* hook, H3Msg* msg);
    static _LHF_(Dlg_SelectScenario_Proc);

    static void __stdcall NewScenarioDlg_Create(HiHook *hook, H3SelectScenarioDialog *dlg, const DWORD dlgCallType);
    static H3Msg *__stdcall H3DlgEdit__TranslateInputKey(HiHook *h, H3InputManager *inpt, H3Msg *msg);
    static int __fastcall RMGDlgOptionsButtonProc(void *msg);
    static _ERH_(OnAfterReloadLanguageData);

  public:
    static void SetPatches(PatcherInstance *_pi);
    static BOOL CreateObjectPrototypesLists(const H3Vector<H3RmgObjectGenerator *> *objectGenerators);
    static void CopyOriginalObjectDefsIntoPcx16();
    static DWORD GetUserRandSeedInput() noexcept;
};
