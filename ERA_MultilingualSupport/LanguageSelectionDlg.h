#pragma once
class LanguageSelectionDlg : public H3Dlg
{

    const DlgStyle *m_style = nullptr;
    LocaleManager *localeManager;

    UINT16 firstLocaleItemId = -1;
    UINT16 lastLocaleItemId = -1;
    UINT localesToDraw = 0;
    H3DlgPcx16 *exportDlgPcx = nullptr;
    H3DlgFrame *selectionFrame = nullptr;

    struct DlgText : public IPluginText
    {
        H3String localeHasNoDescriptionFormat;

        const char *questionformat = nullptr;
        const char *sameLocaleFormat = nullptr;
        void Load() noexcept override;
    } dlgText;

  private:
    void CreateDlgItems();
    virtual BOOL OnMouseHover(H3DlgItem *it) override;
    virtual BOOL DialogProc(H3Msg &msg) override;

  public:
    LanguageSelectionDlg(const int x, const int y, const int width, const int height, const DlgStyle *style,
                         LocaleManager *handler);

    void RedrawLocales(UINT16 firstItemId = 0) noexcept;
    void HideFrame() const noexcept;
    void PlaceFrameAtWidget(const H3DlgTextPcxLocale *it) const noexcept;
    const LocaleManager *Handler() const noexcept;
    const DlgStyle *Style() const noexcept;
    virtual ~LanguageSelectionDlg();

    static void Init();
};
