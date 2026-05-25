#pragma once
#include "framework.h"

#include "DlgPanels.h"
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

enum ePageItemId : INT
{
    PAGE_ITEM_GENERAL = 1,
    PAGE_ITEM_ADV_MAP,
    PAGE_ITEM_COMBAT,
    PAGE_ITEM_ERA_MODS,
};
enum ePageIndex : INT
{
    PAGE_INDEX_GENERAL,
    PAGE_INDEX_ADV_MAP,
    PAGE_INDEX_COMBAT,
    PAGE_INDEX_ERA_MODS,
};

class SystemOptionsDlg : public H3Dlg
{

  public:
    static constexpr int DLG_WIDTH = 481;
    static constexpr int DLG_HEIGHT = 487;
    static constexpr int DLG_MARGIN = 16;
    static constexpr int DLG_ITEM_VMARGIN = 25;

    static constexpr float SETTINGS_VERSION = .1f;

  public:
  public:
    static constexpr LPCSTR BIG_BUTTON = "emenubig.def";

    static constexpr LPCSTR MAIN_MENU_WIDGET_UUID = "rmg_main_menu_widget";

    // std::vector<H3CreatureBankSetup> &creatureBanks;

    struct ISettingsPage
    {

        const char *name;
        UINT id;
        BOOL isVisible = false;
        UINT firstItemCount = 0;
        H3DlgCaptionButton *captionBttn = nullptr;

        H3Vector<H3DlgItem *> items;
        H3Vector<ISetting *> settings;

        ISettingsPage(H3DlgCaptionButton *captionBttn) : captionBttn(captionBttn)
        {
            name = captionBttn->GetText();
            captionBttn->SetClickFrame(1);
        }
        virtual ~ISettingsPage()
        {
            for (auto &setting : settings)
            {
                delete setting;
            }
        }

      public:
        virtual void SetVisible(const BOOL state)
        {
            if (state == isVisible)
                return;
            isVisible = state;
            for (auto &it : items)
            {
                state ? it->ShowActivate() : it->HideDeactivate();
            }
            for (auto &it : settings)
            {
                it->SetVisible(state);
            }
        }
        // virtual void SetDefault();
        virtual BOOL Proc(H3Msg &msg)
        {
            return TRUE;
        }
    };

    struct GeneralSettingsPage : public ISettingsPage
    {
        GeneralSettingsPage(H3DlgCaptionButton *captionbttn) : ISettingsPage(captionbttn)
        {
        }
        virtual ~GeneralSettingsPage()
        {
        }

      public:
        // virtual BOOL Proc(H3Msg &msg) override;

      public:
        static GeneralSettingsPage *Create(H3DlgCaptionButton *captionbttn, H3BaseDlg *dlg);
    };
    struct AdventureMapSettingsPage : public ISettingsPage
    {
        AdventureMapSettingsPage(H3DlgCaptionButton *captionbttn) : ISettingsPage(captionbttn)
        {
        }
        virtual ~AdventureMapSettingsPage()
        {
        }

      public:
        //  virtual BOOL Proc(H3Msg &msg) override;

      public:
        static AdventureMapSettingsPage *Create(H3DlgCaptionButton *captionbttn, H3BaseDlg *dlg);
    };
    struct CombatSettingsPage : public ISettingsPage
    {
        CombatSettingsPage(H3DlgCaptionButton *captionbttn) : ISettingsPage(captionbttn)
        {
        }
        virtual ~CombatSettingsPage()
        {
        }

      public:
        // virtual void SaveData();
        // virtual void SetDefault() override;
        // virtual BOOL Proc(H3Msg &msg) override;

      public:
        static CombatSettingsPage *Create(H3DlgCaptionButton *captionbttn, H3BaseDlg *dlg);
    };

  protected:
    BOOL isInCombat = false;
    BOOL settingsChanged = false;
    BOOL quickCombatSettingState = IntAt(0x6987CC);

    // static constexpr const char* m_iniPath = "Runtime/RMG_CustomizeObjectsProperties.ini";
    eDlgCallSource dlgCallSource = UNKNOWN;
    UINT m_lastPageId = 0;

    ISettingsPage *m_currentPage = nullptr;

    H3Vector<ISettingsPage *> m_pages;
    H3Vector<H3DlgCaptionButton *> captionButtons;

  public:
    static SystemOptionsDlg *instance;

  protected:
    void InitDlgPages()
    {
        for (auto &page : m_pages)
        {
            for (auto &it : page->items)
            {
                it->HideDeactivate();
                AddItem(it, page);
            }
        }
    }
    void SetActivePage(const UINT pageId, const BOOL redraw)
    {
        auto &page = m_pages[pageId];
        if (m_currentPage != page)
        {
            if (m_currentPage)
                m_currentPage->SetVisible(FALSE);

            page->SetVisible(TRUE);
            m_currentPage = page;
            if (redraw)
                Redraw();
        }
    }

    // ctors
  public:
    SystemOptionsDlg(int width, int height, int x, int y);
    SystemOptionsDlg() : SystemOptionsDlg(DLG_WIDTH, DLG_HEIGHT, -1, -1) {};
    virtual ~SystemOptionsDlg();

    // virtual methods
  private:
    virtual BOOL OnCreate() override;
    virtual BOOL DialogProc(H3Msg &msg) override;
    virtual BOOL OnLeftClick(INT itemId, H3Msg &msg) override;
    virtual VOID OnOK() override;
    virtual VOID OnCancel() override;

  private:
    void AddItem(H3DlgItem *item, ISettingsPage *page) noexcept
    {
        H3Dlg::AddItem(item);
        // if (page)
        //    page->items += item;
    }
    void CreateCaptionButtons() noexcept;

    void CreateGameControlButtons() noexcept;
    BOOL ReadIniDlgSettings() noexcept;
    BOOL WriteIniDlgSettings() const noexcept;

    BOOL SetActivePage(ISettingsPage *page) noexcept;
    BOOL SaveRMGObjectsInfo(const BOOL saveIni = true) const noexcept;

    VOID OnHelp() const noexcept;

    BOOL RemoveEditsFocus(const BOOL save) const noexcept;

    // hooks
  private:
    static _ERH_(OnAfterReloadLanguageData);

  public:
    static void AfterDlgClose();
    static void SetPatches(PatcherInstance *_pi);
};
