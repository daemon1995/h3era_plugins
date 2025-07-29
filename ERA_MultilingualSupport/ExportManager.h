#pragma once
#include <Windows.h>
#include <string>
#include <vector>

class ExportDlg : public H3Dlg
{
  public:
    static constexpr LPCSTR BUTTON_NAME = "era.locale.dlg.export.name";

  private:
    static constexpr LPCSTR PANEL_NAME_FORMAT = "era.locale.dlg.export.items.%d";

    static constexpr int PANEL_TEXT_WIDTH = 200;
    static constexpr int CHECKBOX_PADDING = 200;
    static constexpr int PANELS_PADDING = 30;
    struct SelectionPanel
    {

        H3DlgText *panelText = nullptr;
        H3DlgDef *originalDataCheckBox = nullptr;
        H3DlgDef *additionalDataCheckBox = nullptr;
        LPCSTR exportPath = nullptr;
        BOOL (*exportFunction)(LPCSTR, const BOOL, const BOOL) = nullptr;
        // SelectionPanel(const int x, const int y, LPCSTR text, void *(*exportFunc)(LPCSTR, const BOOL));
    };

    std::vector<SelectionPanel *> selectionPanels;
    SelectionPanel *CreateSelectionPanel(const int x, const int y, LPCSTR text, const int startId, LPCSTR path,
                                         BOOL (*exportFunc)(LPCSTR, const BOOL, const BOOL));

  public:
    ExportDlg(const int width, const int height, const int x, const int y, const DlgStyle &style);
    virtual ~ExportDlg();

    virtual BOOL DialogProc(H3Msg &msg) override;
    virtual VOID OnOK() override;

  private:
    void CreateDlgItems();
};
class ExportManager
{
    static constexpr LPCSTR MAX_ID_JSON = "era.dlg.maximumId.";

  public:
    struct MonsterInfo
    {
        static constexpr LPCSTR DEFAULT_PATH = "CreaturesText.json";
    };
    struct ArtifactInfo
    {
        static constexpr LPCSTR DEFAULT_PATH = "ArtifactText.json";
    };
    struct ObjectInfo
    {
        static constexpr LPCSTR DEFAULT_PATH = "ObjectText.json";
    };
    struct CreatureBankInfo
    {
        static constexpr LPCSTR DEFAULT_PATH = "CreatureBankText.json";
    };
    struct TownBuildingInfo
    {
        static constexpr LPCSTR DEFAULT_PATH = "TownText.json";
    };
    struct HeroInfo
    {
        static constexpr LPCSTR DEFAULT_PATH = "HeroText.json";
    };

    static std::string LPCSTR_to_wstring(LPCSTR ansi_str);
    static INT GetMaxOriginalId(LPCSTR keySubstring, const int defaultValue);
    static BOOL WriteJsonFile(const std::string &filePath, nlohmann::json &j);
    static BOOL WriteJsonFile(const std::string &filePath, nlohmann::ordered_json &j);
    static BOOL CreateMonstersJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData);
    static BOOL CreateArtifactsJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData);
    static BOOL CreateObjectsJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData);
    static BOOL CreateCreatureBanksJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData);
    static BOOL CreateTownBuildingsJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData);
    static BOOL CreateHeroesJson(LPCSTR filePath, const BOOL originalData, const BOOL additionalData);

    //  static BOOL ExportAllToJson(const BOOL originalData, const BOOL additionalData);
};
