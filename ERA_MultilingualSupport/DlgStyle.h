#pragma once
class DlgStyle
{


public:
	const UINT16 WIDGET_WIDTH;
	const UINT16 WIDGET_HEIGHT;
	const UINT16 MAXIMUM_ROWS;

	H3String fontName;
	const const char* dlgCallAssetPcxName = nullptr;
	static struct StyleText :public IPluginText
	{

		const char* displayNameFormat;
		void Load() override;
	} text;

	H3LoadedPcx* localeBackgroundLoadedPcx = nullptr;

public:

	DlgStyle(const char* sourcePcxName, const RECT rect, const UINT16 maxRows, const LPCSTR f);
	static bool CreateAssets(bool forceRecreate = false);
	~DlgStyle();

	static std::vector<DlgStyle> styles;
	//static DlgStyle* Create(const char* pcxName, const RECT rect, const UINT16 maxRow, const LPCSTR f);
};

