#pragma once
class DlgStyle
{

  public:
    enum STYLE_ID
    {
        BROWN_BACK = 0,
        BLUE_BACK
    };
    const UINT width;
    const UINT height;
    const UINT maxRows;
    const BOOL createExportButton = true;
    LPCSTR pcxName = nullptr;

    H3String fontName;
    const char *const dlgCallAssetPcxName = nullptr;
    static struct StyleText : public IPluginText
    {

        const char *displayNameFormat;
        void Load() override;
    } text;

  public:
    DlgStyle(const UINT width, const UINT height, const UINT16 maxRows, const char *sourcePcxName, const LPCSTR f,
             const BOOL createExport);
    static bool CreateAssets(bool forceRecreate = false);
    ~DlgStyle();

    static std::vector<DlgStyle> styles;
    // static DlgStyle* Create(const char* pcxName, const RECT rect, const UINT16 maxRow, const LPCSTR f);
};
