#pragma once

#include <unordered_map>

#include "framework.h"

#include "DlgPanels.h"
enum eDlgCallSource : INT
{
    UNKNOWN = -1,
    MAIN_MENU,
    ADV_MAP,
    COMBAT,
    TOWN_SCREEN,
    HERO_SCREEN,
    SWAP_MGR
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
    static constexpr int DLG_CAPTION_BUTTON_TOP_MARGIN = 18;
    static constexpr int DLG_CAPTION_BUTTON_HEIGHT = 40;
    static constexpr int DLG_TOPSETTINGS_MARGIN = DLG_CAPTION_BUTTON_TOP_MARGIN + DLG_CAPTION_BUTTON_HEIGHT + 4;

    static constexpr int DLG_RIGHT_PART_X_MARGIN = DLG_WIDTH - ISetting::WIDTH - 33;
    static constexpr int DLG_LEFT_PART_X_MARGIN = 25;

    static constexpr float SETTINGS_VERSION = .1f;

  public:
    static constexpr LPCSTR PAGE_CAPTION_DEF_NAME = "GSPsys1.def";
    static constexpr LPCSTR SINGLE_BUTTON = "GSPsys0.def";

    static constexpr LPCSTR MAIN_MENU_WIDGET_UUID = "rmg_main_menu_widget";
    struct RegistredERMButtonInfo
    {
        std::string nameKey;
        std::string descriptionKey;
        int ermFunctionId;
    };
    static std::unordered_map<std::string, RegistredERMButtonInfo> registredButtons;
    struct SettingsPage
    {
        H3DlgCaptionButton *captionBttn = nullptr;
        const char *name;
        UINT id;
        BOOL isVisible = false;
        UINT firstItemId = 0;

        H3Vector<H3DlgItem *> items;
        H3Vector<ISetting *> settings;
        H3Vector<H3DlgPcx16 *> createdDlgTitles;
        //  H3LoadedPcx16 *background = nullptr;
        std::unordered_map<int, ISetting *> settingsByItemId;

        SettingsPage(H3DlgCaptionButton *captionBttn) : captionBttn(captionBttn)
        {
            if (!captionBttn)
                return;
            name = captionBttn->GetText();
            id = captionBttn->GetID();
            captionBttn->SetClickFrame(1);
            firstItemId = id * 100 + 100;
            return;

            // if (background = H3LoadedPcx16::Create(DLG_WIDTH, DLG_HEIGHT))
            //{
            //     const int playerColor = P_Game->GetPlayerID();
            //     background->BackgroundRegion(0, 0, DLG_WIDTH, DLG_HEIGHT, false);
            //     background->FrameRegion(0, 0, DLG_WIDTH, DLG_HEIGHT, false, playerColor, false);
            // }
        }
        virtual ~SettingsPage()
        {
            for (auto &dlgPcx16 : createdDlgTitles)
            {
                if (auto pcx16 = dlgPcx16->GetPcx())
                {
                    pcx16->Destroy();
                    dlgPcx16->SetPcx(nullptr);
                }
            }
            for (auto &setting : settings)
                delete setting;
            // if (background)
            //     background->Destroy();
        }

      public:
        void AddSetting(ISetting *setting)
        {
            settings += setting;
            if (auto dlgPcx16 = setting->titleItem)
                createdDlgTitles += dlgPcx16;

            if (setting->firstClickableItemId > 0)
                for (int i = setting->firstClickableItemId; i <= setting->lastClickableItemId; ++i)
                    settingsByItemId[i] = setting;
        }

        H3DlgPcx16 *CreateTitle(int x, int &y, LPCSTR displayedText)
        {
            auto dlgPcx16 = ISetting::CreateTitle(x, y, displayedText, items);
            if (dlgPcx16)
                createdDlgTitles += dlgPcx16;
            return dlgPcx16;
        }

        template <class T, class Info> T *CreateSetting(const Info &info)
        {
            T *setting = T::Create(info, items);
            if (setting)
            {
                AddSetting(setting);
            }
            return setting;
        }

        void SetVisible(const BOOL state)
        {
            if (state == isVisible)
                return;

            isVisible = state;

            for (auto &it : items)
            {
                state ? it->ShowActivate() : it->HideDeactivate();
            }
            // deactivate bttn click
            captionBttn->SendCommand(6 - (state), 4096);

            for (auto &it : settings)
            {
                it->SetVisible(state);
            }
        }

        BOOL ProcessMessage(H3Msg &msg) noexcept
        {
            auto settingIt = settingsByItemId.find(msg.itemId);
            if (settingIt != settingsByItemId.end())
            {
                settingIt->second->ProcessMessage(msg);
            }
            return 1;
        }
    };

  protected:
    BOOL isInCombat = false;
    BOOL settingsChanged = false;
    BOOL quickCombatSettingState = OriginalConfig::Get().quickCombat;

    // static constexpr const char* m_iniPath = "Runtime/RMG_CustomizeObjectsProperties.ini";
    eDlgCallSource dlgCallSource = UNKNOWN;
    SettingsPage *m_currentPage = nullptr;
    H3Vector<SettingsPage *> m_pages;

  public:
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

  private:
    void CreateGameControlButtons() noexcept;
    void CreateDlgPages() noexcept;
    void CreateOtherSettingsPanel(SettingsPage *page, const int x, const int y, int &itemId) noexcept;

    void InitDlgPages()
    {
        for (auto &page : m_pages)
        {
            for (auto &it : page->items)
            {
                it->HideDeactivate();
                AddItem(it);
            }
        }
    }
    void SetActivePage(const UINT pageId, const BOOL redraw)
    {
        if (m_pages.Size() <= pageId)
            return;
        auto &page = m_pages[pageId];
        if (m_currentPage != page)
        {
            if (m_currentPage)
                m_currentPage->SetVisible(FALSE);

            page->SetVisible(TRUE);
            int yOffset = DLG_TOPSETTINGS_MARGIN;
            // if (page->background)
            //{
            //     page->background->DrawToPcx16(0, yOffset, FALSE, background, 0, yOffset);
            //     libc::memcpy(background->buffer, page->background->buffer, background->buffSize);
            // }
            m_currentPage = page;
            if (redraw)
                Redraw();
        }
    }

    BOOL ReadIniDlgSettings() noexcept;
    BOOL WriteIniDlgSettings() const noexcept;

  public:
    inline INT32 ResultItemId() const noexcept
    {
        return resultItemId;
    }
    static _ERH_(OnGameLeave)
    {
        registredButtons.clear();
    }
    static void SetPatches(PatcherInstance *_pi)
    {
        _REH_(OnGameLeave);
    }

    // hooks
  private:
    static void __stdcall CallWogOptionsDlg();
    static void __stdcall CallSelectLanguageDlg();

    static void AfterDlgClose();
};
