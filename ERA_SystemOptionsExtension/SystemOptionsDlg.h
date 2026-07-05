#pragma once

#include <stack>
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
    // PAGE_ITEM_ERA_MODS,
    PAGE_ITEM_TOTAL_COUNT,
};

enum ePageIndex : INT
{
    PAGE_INDEX_GENERAL,
    PAGE_INDEX_ADV_MAP,
    PAGE_INDEX_COMBAT,
    PAGE_INDEX_ERA_MODS,
};
struct RegisteredButtonInfo
{
    std::string nameKey;
    std::string descriptionKey;
    // union {
    int ermFunctionId = 0;
    void (*callback)();
};

struct ExternalButtonsManager
{
  private:
    static ExternalButtonsManager ermInfo;
    static ExternalButtonsManager pluginsInfo;

  protected:
    std::vector<RegisteredButtonInfo> registeredErmButtonsVec;
    std::unordered_map<std::string, size_t> nameToIndexMap;
    std::stack<size_t> freedIndices;

  public:
    BOOL RegisterButton(LPCSTR tag, const RegisteredButtonInfo &info);
    BOOL UnregisterButton(LPCSTR tag);
    size_t Size() const
    {
        return nameToIndexMap.size();
    }
    const std::vector<RegisteredButtonInfo> &Data() const
    {
        return registeredErmButtonsVec;
    }
    VOID Clear()
    {
        registeredErmButtonsVec.clear();
        nameToIndexMap.clear();
        size_t size = freedIndices.size();
        for (size_t i = 0; i < size; i++)
            freedIndices.pop();
    }

  public:
    static ExternalButtonsManager &GetErmInfo()
    {
        return ermInfo;
    }
    static ExternalButtonsManager &GetPluginsInfo()
    {
        return pluginsInfo;
    }
};

class SystemOptionsDlg : public H3Dlg
{
  public:
    static constexpr int DLG_WIDTH = 481;
    static constexpr int DLG_HEIGHT = 487;
    static constexpr int DLG_CAPTION_BUTTON_TOP_MARGIN = 18;
    static constexpr int DLG_CAPTION_BUTTON_HEIGHT = 40;
    static constexpr int DLG_TOP_SETTINGS_MARGIN = DLG_CAPTION_BUTTON_TOP_MARGIN + DLG_CAPTION_BUTTON_HEIGHT + 4;

    static constexpr int DLG_X_MARGIN = 28;
    static constexpr int DLG_LEFT_PART_X_MARGIN = DLG_X_MARGIN;
    static constexpr int DLG_RIGHT_PART_X_MARGIN = DLG_WIDTH - ISetting::WIDTH - DLG_X_MARGIN;

    static constexpr float SETTINGS_VERSION = .1f;

  public:
    static constexpr LPCSTR PAGE_CAPTION_DEF_NAME = "GSPsys1.def";
    static constexpr LPCSTR SINGLE_BUTTON = "GSPsys0.def";

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
        std::unordered_map<int, ISetting *> settingsByItemId;

      public:
        SettingsPage(H3DlgCaptionButton *captionBttn) : captionBttn(captionBttn)
        {
            if (!captionBttn)
                return;
            name = captionBttn->GetText();
            id = captionBttn->GetID();
            captionBttn->SetClickFrame(1);
            firstItemId = id * 100 + 100;
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

  public:
  protected:
    BOOL isInCombat = false;
    BOOL settingsChanged = false;
    const BOOL quickCombatSettingState = OriginalConfig::Get().quickCombat;
    eDlgCallSource dlgCallSource = UNKNOWN;
    SettingsPage *m_currentPage = nullptr;
    std::vector<SettingsPage *> m_pages;
    std::vector<const RegisteredButtonInfo *> sortedButtonsInfo;
    std::vector<CaptionButtonSetting *> callbackButtons;
    H3DlgScrollbar *scrollBar = nullptr;
    UINT currentTopErmButtonIdx = 0;

  protected:
    static SystemOptionsDlg *instance;

  public:
    // ctors
    SystemOptionsDlg(int width, int height, int x, int y);
    SystemOptionsDlg() : SystemOptionsDlg(DLG_WIDTH, DLG_HEIGHT, -1, -1) {};
    virtual ~SystemOptionsDlg();

  private:
    // virtual methods
    virtual BOOL OnCreate() override;
    virtual BOOL DialogProc(H3Msg &msg) override;
    virtual BOOL OnLeftClick(INT itemId, H3Msg &msg) override;

  private:
    void CreateGameControlButtons() noexcept;
    void CreateDlgPages() noexcept;
    void CreateImportedSettingsPanel(SettingsPage *page, const int x, const int y, int &itemId) noexcept;

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
        if (m_pages.size() <= pageId)
            return;
        auto &page = m_pages[pageId];
        if (m_currentPage != page)
        {
            if (m_currentPage)
                m_currentPage->SetVisible(FALSE);

            page->SetVisible(TRUE);
            int yOffset = DLG_TOP_SETTINGS_MARGIN;
            m_currentPage = page;
            if (redraw)
                Redraw();
        }
    }

    VOID AssignErmButtons(const int firstItemId, const BOOL redraw) noexcept
    {
        currentTopErmButtonIdx = firstItemId;
        const size_t length = callbackButtons.size();
        for (size_t i = 0; i < length; i++)
        {
            const size_t id = firstItemId + i;
            const auto &info = sortedButtonsInfo[id];
            LPCSTR namePtr = info->nameKey.empty() ? h3_NullString : EraJS::read(info->nameKey);
            LPCSTR descriptionPtr = info->descriptionKey.empty() ? h3_NullString : EraJS::read(info->descriptionKey);

            auto button = callbackButtons[i]->captionButton;
            button->SetText(namePtr);
            button->SetRightClickHint(descriptionPtr);
            if (const int ermFunctionId = info->ermFunctionId)
            {
                callbackButtons[i]->SetOnChange([ermFunctionId](ISetting *) { Era::FireErmEvent(ermFunctionId); });
            }
            else if (const auto &function = info->callback)
            {
                callbackButtons[i]->SetOnChange([function](ISetting *) { function(); });
            }
            if (redraw)
            {
                button->Draw();
                button->Refresh();
            }
        }
    }

  private:
    static void CallWogOptionsDlg();
    static void __stdcall CallErmFunction(const int ermFunctionId)
    {
        Era::FireErmEvent(ermFunctionId);
    }
    static VOID __fastcall ScrollBarProc(INT32 itemId, H3BaseDlg *_dlg)
    {
        auto dlg = dynamic_cast<SystemOptionsDlg *>(_dlg);
        if (itemId != dlg->currentTopErmButtonIdx)
        {
            dlg->AssignErmButtons(itemId, TRUE);
            auto scrollBar = dlg->scrollBar;
            scrollBar->Draw();
            scrollBar->Refresh();
        }
    }

  public:
    inline Era::EGameMenuTarget ResultItemId() const noexcept
    {
        return Era::EGameMenuTarget(resultItemId);
    }
    //  hooks
    static void SetPatches(PatcherInstance *_pi);
};
