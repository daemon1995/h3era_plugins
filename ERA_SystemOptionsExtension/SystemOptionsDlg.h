#pragma once

#include <unordered_map>

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

struct CallBackInfo
{
    H3DlgCaptionButton *bttn = nullptr;
    void *callbackFuncion;
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
typedef void(__stdcall *CallLocaleSelectionDlg_t)(const int x, const int y, const int style);
typedef const char *(__stdcall *GetDisplayedName_t)();
class SystemOptionsDlg : public H3Dlg
{
  public:
    static constexpr int DLG_WIDTH = 481;
    static constexpr int DLG_HEIGHT = 487;
    static constexpr int DLG_CAPTION_BUTTON_TOP_MARGIN = 16;
    static constexpr int DLG_TOPSETTINGS_MARGIN = DLG_CAPTION_BUTTON_TOP_MARGIN + 44;

    static constexpr int DLG_RIGHT_PART_X_MARGIN = DLG_WIDTH - ISetting::WIDTH - 33;
    static constexpr int DLG_LEFT_PART_X_MARGIN = 25;

    static constexpr float SETTINGS_VERSION = .1f;

  public:
    static constexpr LPCSTR BIG_BUTTON = "GSPsys1.def";
    static constexpr LPCSTR SINGLE_BUTTON = "GSPsys0.def";

    static constexpr LPCSTR MAIN_MENU_WIDGET_UUID = "rmg_main_menu_widget";

    // std::vector<H3CreatureBankSetup> &creatureBanks;

    struct ISettingsPage
    {
        const char *name;
        UINT id;
        BOOL isVisible = false;
        UINT firstItemId = 0;
        H3DlgCaptionButton *captionBttn = nullptr;

        H3Vector<H3DlgItem *> items;
        H3Vector<ISetting *> settings;
        std::unordered_map<int, ISetting *> settingsByItemId;

        ISettingsPage(H3DlgCaptionButton *captionBttn) : captionBttn(captionBttn)
        {
            name = captionBttn->GetText();
            id = captionBttn->GetID();
            captionBttn->SetClickFrame(1);
            firstItemId = id * 100 + 100;
        }
        virtual ~ISettingsPage()
        {
            for (auto &setting : settings)
                delete setting;
        }

      public:
        void AddSetting(ISetting *setting)
        {
            settings += setting;
            if (setting->firstClickableItemId > 0)
                for (int i = setting->firstClickableItemId; i <= setting->lastClickableItemId; ++i)
                    settingsByItemId[i] = setting;
        }
        virtual void SetVisible(const BOOL state)
        {
            if (state == isVisible)
                return;

            isVisible = state;

            for (auto &it : items)
            {
                state ? it->ShowActivate() : it->HideDeactivate();
            }
            // deactivate bttn click
            // captionBttn->SendCommand(6 - (state), 4096);

            for (auto &it : settings)
            {
                it->SetVisible(state);
            }
        }

        BOOL ProcessMessage(H3Msg &msg)
        {
            auto settingIt = settingsByItemId.find(msg.itemId);
            if (settingIt != settingsByItemId.end())
            {
                return settingIt->second->ProcessMessage(msg);
            }

            return FALSE;
        }
    };

    struct GeneralSettingsPage : public ISettingsPage
    {
        GeneralSettingsPage(H3DlgCaptionButton *captionbttn) : ISettingsPage(captionbttn) {};
        virtual ~GeneralSettingsPage() {};

      public:
        static GeneralSettingsPage *Create(H3DlgCaptionButton *captionbttn, H3BaseDlg *dlg);
    };
    struct AdventureMapSettingsPage : public ISettingsPage
    {
        AdventureMapSettingsPage(H3DlgCaptionButton *captionbttn) : ISettingsPage(captionbttn) {};
        virtual ~AdventureMapSettingsPage() {};

      public:
        static AdventureMapSettingsPage *Create(H3DlgCaptionButton *captionbttn, H3BaseDlg *dlg);
    };
    struct CombatSettingsPage : public ISettingsPage
    {
        CombatSettingsPage(H3DlgCaptionButton *captionbttn) : ISettingsPage(captionbttn) {};
        virtual ~CombatSettingsPage() {};

      public:
        static CombatSettingsPage *Create(H3DlgCaptionButton *captionbttn, H3BaseDlg *dlg);
    };

  protected:
    BOOL isInCombat = false;
    BOOL settingsChanged = false;
    BOOL quickCombatSettingState = IntAt(0x6987CC);

    // static constexpr const char* m_iniPath = "Runtime/RMG_CustomizeObjectsProperties.ini";
    eDlgCallSource dlgCallSource = UNKNOWN;
    ISettingsPage *m_currentPage = nullptr;
    H3Vector<ISettingsPage *> m_pages;
    H3Vector<H3DlgCaptionButton *> captionButtons;

  public:
    static struct LanguageDlgCallInfo
    {
        HMODULE hModule = nullptr;
        CallLocaleSelectionDlg_t callLocaleSelectionDlg = nullptr;
        GetDisplayedName_t getDisplayedName = nullptr;
        H3DlgCaptionButton *currentLanguageText = nullptr;
    } languageDlgInfo;
    static struct HealthBarDlgCallInfo
    {
        void(__stdcall *callHealthBarDlg)() = nullptr;
        H3DlgDef *affectedCheckbox = nullptr;
        DWORD healthBarValuePtr = 0;
        INT *dlgValuePtr = nullptr;
        H3DlgCaptionButton *captionBttn = nullptr;

    } healthBarDlgInfo;

    static SystemOptionsDlg *instance;

  protected:
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
    void CreateGameControlButtons() noexcept;
    void CreateDlgPages() noexcept;
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
    void AddItem(H3DlgItem *item, ISettingsPage *page) noexcept
    {
        H3Dlg::AddItem(item);
        // if (page)
        //    page->items += item;
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

    BOOL ReadIniDlgSettings() noexcept;
    BOOL WriteIniDlgSettings() const noexcept;

    // hooks
  public:
    static void __stdcall CallWogOptionsDlg();
    static void __stdcall CallSelectLanguageDlg();
    static void __stdcall CallHealthBarDlg();
    static void AfterDlgClose();
    static void SetPatches(PatcherInstance *_pi);
};
