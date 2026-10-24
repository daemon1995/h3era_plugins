#pragma once
#include <future>

class AssemblyInformation : public IGamePatch
{

    static constexpr LPCSTR ASSEMBLY_INI_FILE = "ERA_Project.ini";
    static constexpr LPCSTR BASE_JSON_KEY = "gem_plugin.main_menu";
    static constexpr LPCSTR ASSEMBLY_SETTINGS_INI = "Runtime/Era/assembly_settings.ini";
    static constexpr LPCSTR ASSEMBLY_SETTINGS_SECTION = "Assembly";

    static AssemblyInformation *instance;
    struct PluginText : public IPluginText
    {

        void Load() override;
    } m_text;

    struct Version
    {
        int x = 0;
        int y = 0;
        int fixedWidth = 0;
        int fixedHeight = 0;
        const char *fontName = NH3Dlg::Text::SMALL;
        const char *format = 0;
        eTextColor textColor = eTextColor::HIGHLIGHT;

        H3String version = h3_NullString;
        H3String text = h3_NullString;

        H3String shellExecutePath = h3_NullString;

        BOOL customText = false;
        BOOL show = false;
        BOOL alwaysDraw = false;

        H3DlgPcx16 *dlgItem = nullptr;
        H3LoadedPcx16 *labelPcx = nullptr;
        H3BaseDlg *parentDlg = nullptr;

      public:
        virtual void GetJsonData(const char *jsonSubKey);
        virtual void GetVersion() noexcept;
        virtual void AdjustItemText() noexcept;

      public:
        void ClickProcedure() const noexcept;
        H3DlgPcx16 *AddToDlg(H3BaseDlg *dlg) noexcept;
        bool UpdateImage(bool drawText = true) noexcept;
        void ReleaseImage() noexcept;

    } m_eraVersion;
    Version m_assemblyTitle;

    struct LocalVersion : public Version
    {
        BOOL customVersion = false;
        BOOL remoteVersionIsHigher = false;

      public:
        virtual void GetJsonData(const char *jsonSubKey) final override;
        virtual void GetVersion() noexcept final override;
        virtual void AdjustItemText() noexcept final override;

    } m_localVersion;

    struct RemoteVersion : public Version
    {
        static constexpr LPCSTR LAST_VERSION_INI_KEY = "LastVersionReceived";
        static constexpr LPCSTR LAST_TIME_CHECKED_INI_KEY = "LastRemoteCheckTime";
        // Only the game thread accesses the version, INI cache and display flag.
        std::future<std::string> request;
        bool workDone = false;

      public:
        virtual void GetJsonData(const char *jsonSubKey) final override;
        virtual void GetVersion() noexcept final override;
        void ApplyCompletedRequest();
        // virtual void ClickProcedure() noexcept final override;

    } m_remoteVersion;

    Version *versions[3];
    BOOL isVisible = false;
    DWORD versionBlinkLastUpdate = 0;
    BOOL versionBlinkVisible = true;

  private:
    AssemblyInformation(PatcherInstance *_pi);

  public:
    virtual void CreatePatches() noexcept final override;

  public:
    static int __stdcall DlgMainMenu_Create(HiHook *h, H3BaseDlg *dlg);
    static int __stdcall DlgMainMenu_Proc(HiHook *h, H3Msg *msg);
    static int __stdcall DlgMainMenu_Dtor(HiHook *h, H3BaseDlg *dlg);

    // static int __stdcall DlgMainMenu_Campaign_Run(HiHook *h, H3BaseDlg *dlg);
    //  static int __stdcall DlgMainMenu_NewLoad_Create(HiHook *h, H3BaseDlg *dlg, const int val);

  private:
    void LoadDataFromJson();
    void UpdateVersionBlink() noexcept;
    void PrepareAssemblyLabels(H3BaseDlg *dlg) noexcept;
    void LoadAssemblyLabelText();
    static void __stdcall OnAfterReloadLanguageData(Era::TEvent *e);

  public:
    void CreateDlgItems(H3BaseDlg *dlg);

  public:
    static AssemblyInformation &Get();
    static const char *GetAssemblyVesrion();
    void CheckOnlineVersion();
    const BOOL CompareVersions();
};
