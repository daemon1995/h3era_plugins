#pragma once
#include "framework.h"

namespace jsonFormat
{
constexpr LPCSTR UNIQUE_MOD_NAME = "era.%s.notification.name";
constexpr LPCSTR UNIQIUE_MOD_TEXT = "era.%s.notification.text";
constexpr LPCSTR UNIQUE_MOD_URL = "era.%s.notification.url";

constexpr LPCSTR ORDERED_MOD_NAME = "era.%s.notification.%d.name";
constexpr LPCSTR ORDERED_MOD_TEXT = "era.%s.notification.%d.text";
constexpr LPCSTR ORDERED_MOD_URL = "era.%s.notification.%d.url";
} // namespace jsonFormat
struct NotificationPanel
{
    static NotificationPanel *instance;
    static constexpr int FRAME_OFFSET = 2;
    // namespace assetss

    static constexpr LPCSTR PCX_BACK = "DlgBluBk.pcx";

    static constexpr LPCSTR NOTIFICATION_COUNTER_FORMAT = "%d / %d";
    static constexpr LPCSTR INI_FILE_NAME = "runtime/era/main_menu_notifications.ini";
    static constexpr LPCSTR INI_SECTION_NAME = "HIDDEN";

    static constexpr int SCROLL_BAR_WIDTH = 10;
    static constexpr int SCROLL_BAR_HEIGHT = 10;
    static constexpr int SCROLL_BAR_X = 5;
    static constexpr int SCROLL_BAR_Y = 5;

    struct ModInfo
    {
        static constexpr int NOTIFICATIONS_PER_MOD = 5;
        static constexpr int NONE_INDEX = -1;

        BOOL isVisible = false;
        BOOL isHiddenByUser = false;
        BOOL savedAsHiddenByUser = false;
        UINT displayedIndex = 0;
        const INT modIndex = NONE_INDEX;

        UINT hiddenByUserDescriptionHash = 0;
        UINT currentDescriptionHash = 0;

        H3DlgText *modNameDlgText = nullptr;
        H3DlgFrame *nameUnderline = nullptr;

        LPCSTR displayedText = nullptr;
        LPCSTR displayedName = nullptr;
        LPCSTR externalLink = nullptr;

        std::string modFolderName;
        H3DlgScrollableText *descriptionTextScrollBar = nullptr;

        // H3DlgText *author = nullptr;
        //   H3DlgText *description = nullptr;
        std::vector<H3DlgItem *> items;

      public:
        ModInfo(LPCSTR description, LPCSTR folderName, const int index = NONE_INDEX);

      public:
        BOOL MarkAsHiddenByUser() noexcept;
        BOOL ReloadDescription() noexcept;
    };
    INT x = 0;
    INT y = 0;
    INT width = 0;
    INT height = 0;
    BOOL isVisible = false;
    H3BaseDlg *parentDlg = nullptr;

    H3DlgPcx16 *parrentCaller = nullptr;
    H3LoadedPcx16 *backupScreen = nullptr;

    // personal dlg items
    struct RuntimeDlgPcx16
    {
        H3DlgPcx16 *item = nullptr;
        H3LoadedPcx16 *pcx = nullptr;
    };

    struct Runtimes
    {
        RuntimeDlgPcx16 panelBackground;
        RuntimeDlgPcx16 modBackground;
        RuntimeDlgPcx16 hideMod;
        RuntimeDlgPcx16 hideAll;

        RuntimeDlgPcx16 *asArray[4] = {&panelBackground, &modBackground, &hideMod, &hideAll};
    } runtimes;

    H3DlgDefButton *nextModButton = nullptr;
    H3DlgDefButton *previousModButton = nullptr;

    H3DlgCaptionButton *hideOneButton = nullptr;
    H3DlgCaptionButton *hideAllButton = nullptr;
    H3DlgTextPcx *notificationsCounter = nullptr;
    H3DlgText *panelTitle = nullptr;

    std::vector<ModInfo> modInfos;
    // mod info data
    ModInfo *currentModInfo = nullptr;
    UINT currentModInfoIndex = 0;
    UINT notificationsTotal = 0;
    UINT notificationsVisible = 0;
    // H3Vector<H3DlgItem*> personalItems;
    H3Vector<H3DlgItem *> items;

  private:
    NotificationPanel(H3BaseDlg *parrent, const int x = 1, const int y = 1, const int width = 400,
                      const int height = 600) noexcept;

  public:
    ~NotificationPanel() noexcept;

  private:
    H3DlgItem *AddItem(H3DlgItem *item, const bool isCommon = false) noexcept;

    void CreateModInfoList(std::vector<std::string> &modList) noexcept;
    void CreateModAreaFrame(const UINT x, const UINT y, const UINT width, const UINT height) noexcept;
    void CreateModDlgItems(H3BaseDlg *dlg, ModInfo &modInfo, H3DlgPcx16 *modBackground) noexcept;

    void UpdateVisibleNotificationsList() noexcept;
    ModInfo *GetModInfoFromVisible(const UINT index) noexcept;
    void SwitchModInfo(const int step) noexcept;
    void SetModVisible(ModInfo &modInfo, const BOOL isVisible) noexcept;
    void Retranslate(const BOOL redraw = false) noexcept;

  public:
    void SetVisible(const BOOL isVisible, const BOOL activateAllNotifications = false) noexcept;
    BOOL ProcessPanel(H3Msg *msg, const BOOL forceRedraw = false) noexcept;
    void ReloadLanguageData() noexcept;
    static NotificationPanel *Init(H3BaseDlg *parrent, const int x = 1, const int y = 1, const int width = 400,
                                   const int height = 600) noexcept;
};
