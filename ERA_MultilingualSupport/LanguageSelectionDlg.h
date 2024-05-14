#pragma once
class LanguageSelectionDlg :
    public H3Dlg
{

    LocaleHandler* m_localeHandler;
    BOOL DialogProc(H3Msg& msg) override;

    void CreateDlgItems();
    static H3LoadedPcx* m_widgetBackground;
    BOOL OnMouseHover(H3DlgItem* it) override;
    H3DlgFrame* m_selectionFrame;

public:

    LanguageSelectionDlg(int width, int height, int x = -1, int y = -1, BOOL statusBar = false, BOOL makeBackground = false);
    LanguageSelectionDlg(const H3DlgItem* calledItem);
    const void HideFrame() const noexcept;
    void PlaceFrameAtWidget(const H3DlgTextPcxLocale* it) const noexcept;
    const LocaleHandler* Handler() const noexcept;
    virtual ~LanguageSelectionDlg();

    static void CreateAssets();
    static void Init();

};

