#pragma once
#include <atomic>

class AssemblyInformation : public IGamePatch
{

    static const char *ASSEMBLY_INI_FILE;
    static const char *BASE_JSON_KEY;

    struct PluginText : public IPluginText
    {

        void Load() override;
    } m_text;

    struct Version
    {
        int x = 0;
        int y = 0;
        // UINT16 itemId = -1;
        UINT characterLength = 7;

        const char *fontName = 0;
        const char *format = 0;

        H3String version = h3_NullString;
        H3String text = h3_NullString;

        H3String shellExecutePath = h3_NullString;

        BOOL customText = false;
        BOOL show = false;
        BOOL alwaysDraw = false;

        H3DlgText *dlgItem = nullptr;

      public:
        virtual void GetJsonData(const char *jsonSubKey);
        virtual void GetVersion() noexcept;
        virtual void AdjustItemText() noexcept;

      public:
        void ClickProcedure() const noexcept;
        H3DlgText *AddToDlg(H3BaseDlg *dlg) noexcept;

    } m_eraVersion;
    BOOL alwaysDraw = false;

    struct LocalVersion : public Version
    {
        BOOL readRegistry = false;
        BOOL customVersion = false;
        BOOL remoteVersionIsHigher = false;

      public:
        virtual void GetJsonData(const char *jsonSubKey) final override;
        virtual void GetVersion() noexcept final override;
        virtual void AdjustItemText() noexcept final override;

      public:
        BOOL ReadRegistry(const char *registryKey);

    } m_localVersion;

    struct RemoteVersion : public Version
    {
        std::atomic<bool> workDone;

      public:
        virtual void GetJsonData(const char *jsonSubKey) final override;
        virtual void GetVersion() noexcept final override;
        // virtual void ClickProcedure() noexcept final override;

    } m_remoteVersion;

    Version *versions[3];

  private:
    AssemblyInformation(PatcherInstance *_pi);

  public:
    virtual void CreatePatches() noexcept final override;

  public:
    static void __stdcall OnAfterReloadLanguageData(Era::TEvent *e);

    static int __stdcall DlgMainMenu_Proc(HiHook *h, H3Msg *msg);
    static int __stdcall DlgMainMenu_Campaign_Run(HiHook *h, H3BaseDlg *dlg);
    static int __stdcall DlgMainMenu_NewLoad_Create(HiHook *h, H3BaseDlg *dlg, const int val);
    static int __stdcall DlgMainMenu_Create(HiHook *h, H3BaseDlg *dlg);

  private:
    void LoadDataFromJson();

  public:
    void CreateDlgItems(H3BaseDlg *dlg);

  public:
    static AssemblyInformation &Get();
    static const char *GetAssemblyVesrion();
    void CheckOnlineVersion();
    const BOOL CompareVersions();
};
