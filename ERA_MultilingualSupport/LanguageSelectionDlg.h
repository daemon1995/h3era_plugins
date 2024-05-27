#pragma once
class LanguageSelectionDlg :
	public H3Dlg
{

	const DlgStyle* m_style;
	LocaleHandler* m_localeHandler;

	UINT m_LOCALES_TO_DRAW{};

	BOOL DialogProc(H3Msg& msg) override;

	struct DlgText :public IPluginText
	{
		H3String localeHasNoDescriptionFormat;

		const char* questionformat = nullptr;
		const char* sameLocaleFormat = nullptr;
		void Load() noexcept override;
	} m_text;


private:
	void CreateDlgItems();
	BOOL OnMouseHover(H3DlgItem* it) override;
	H3DlgFrame* m_selectionFrame;



public:

	void RedrawLocales(UINT32 firstItemId = 0)  noexcept;
	LanguageSelectionDlg(const int width, const int height, const int x, const int y, const DlgStyle* style, LocaleHandler* handler);
	void HideFrame() const noexcept;
	void PlaceFrameAtWidget(const H3DlgTextPcxLocale* it) const noexcept;
	const LocaleHandler* Handler() const noexcept;
	const DlgStyle* Style() const noexcept;
	virtual ~LanguageSelectionDlg();

	static _LHF_(MyClassLoHook);
	static void Init();

};

