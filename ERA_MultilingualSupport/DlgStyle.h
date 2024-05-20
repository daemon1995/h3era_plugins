#pragma once
class DlgStyle
{


public:
	const UINT16 WIDGET_WIDTH;
	const UINT16 WIDGET_HEIGHT;
	const UINT16 MAXIMUM_ROWS;


	const char* dlgCallButtonName = nullptr;
	const char* localeBackgroundPcxName;
	H3LoadedPcx* localeBackgroundLoadedPcx = nullptr;

public:

	DlgStyle(const char* sourcePcxName, const RECT rect, const UINT16 maxRows, const char* defNam);
	static bool CreateAssets(bool forceRecreate=false);
	~DlgStyle();

	static std::vector<DlgStyle> styles;
	static DlgStyle* Create(const char* pcxName, const RECT rect, const UINT16 maxRow,const char* defName);
};

