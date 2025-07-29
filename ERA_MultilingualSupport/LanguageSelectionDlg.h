#pragma once

struct DlgStyle
{
    enum STYLE_ID
    {
        BROWN_BACK = 0,
        BLUE_BACK
    };
    const UINT width;
    const UINT height;
    UINT maxRows;
    const BOOL createExportButton = true;
    const BOOL isBlueBack = false;
    LPCSTR pcxName = nullptr;
    H3String fontName;
    DlgStyle(const UINT width, const UINT height, const UINT maxRows, const BOOL createExportButton,
             const BOOL isBlueBack, LPCSTR fontName);
};

class LanguageSelectionDlg : public H3Dlg
{

  public:
    static constexpr LPCSTR UNIQUE_BUTTON_NAME = "locale_selection_button";

    static std::vector<DlgStyle> styles;

  private:
    const DlgStyle &style;
    H3FontLoader fontLoader{};
    LocaleManager *localeManager = nullptr;
    UINT16 firstLocaleItemId = -1;
    UINT16 lastLocaleItemId = -1;
    UINT localesToDraw = 0;
    H3DlgPcx16 *exportDlgPcx = nullptr;
    H3DlgFrame *selectionFrame = nullptr;
    std::vector<H3DlgPcx16 *> displayedLocales;
    struct DlgText
    {
        LPCSTR localeHasNoDescriptionFormat = nullptr;
        LPCSTR questionformat = nullptr;
        LPCSTR sameLocaleFormat = nullptr;
        void Load() noexcept;
    } dlgText;

  public:
    LanguageSelectionDlg(const int x, const int y, const int width, const int height, const DlgStyle &style,
                         LocaleManager *handler);
    virtual ~LanguageSelectionDlg();

  protected:
    virtual BOOL OnMouseHover(H3DlgItem *it) override;
    virtual BOOL DialogProc(H3Msg &msg) override;

  private:
    void CreateDlgItems();

    void RedrawLocales(UINT16 firstItemId = 0) noexcept;
    void HideFrame() const noexcept;
    void PlaceFrameAtWidget(const H3DlgPcx16Locale *it) const noexcept;

  public:
    static void __fastcall DlgSroll_Proc(INT32 tickId, H3BaseDlg *dlg);
    static bool CreateAssets(const BOOL forceRecreate = false);
    static void Init();
};
