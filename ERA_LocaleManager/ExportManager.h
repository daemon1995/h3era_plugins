#pragma once
#include "framework.h"
#ifdef CREATE_TEXT_JSON_EXPORTS
#include "../headers/Config/ExportClient.h"

class ExportManager
{
  public:
    static BOOL WriteJsonFile(const std::string &filePath, const std::string &utf8Json);
    static std::vector<configExport::Category> Collect();
};

class ExportDlg : public H3Dlg
{
  public:
    static constexpr LPCSTR BUTTON_NAME = "era.locale.dlg.export.name";
    explicit ExportDlg(unsigned mode = 0);
    BOOL DialogProc(H3Msg &msg) override;
    VOID OnOK() override;

  private:
    static constexpr int PADDING = 20;
    static constexpr int FIRST_CHECKBOX_ID = 100;
    static constexpr int MERGE_OPTION_ID = 60000;
    static constexpr int IGNORE_DEFAULTS_OPTION_ID = 60001;
    struct Layout
    {
        int width, height, labelWidth, columnWidth, rowHeight, headerHeight, noteHeight, visibleRows;
        std::array<int, 2> optionHeights;
    };
    struct Row
    {
        configExport::Selection selection;
        std::array<H3DlgDef *, 4> checkboxes{};
        std::array<H3DlgText *, 4> counts{};
        std::array<bool, 4> enabled{};
        H3DlgText *label = nullptr;
    };
    std::vector<configExport::Category> categories;
    std::vector<Row> rows;
    Layout layout;
    configExport::Options options;
    unsigned mode = 0;
    unsigned page = 0;
    std::array<H3DlgDef *, 2> optionCheckboxes{};

    ExportDlg(std::vector<configExport::Category> categories, unsigned mode);
    ExportDlg(const std::vector<configExport::Category> &categories, const Layout &layout, unsigned mode);
    static Layout Measure(const std::vector<configExport::Category> &categories);
    void CreateDlgItems();
    void RefreshRow(size_t index);
};
#endif
