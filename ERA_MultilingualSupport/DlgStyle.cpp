#include "pch.h"
#include "DlgStyle.h"

std::vector<DlgStyle> DlgStyle::styles;

DlgStyle::DlgStyle(const char* pcxName, const RECT rect, const UINT16 maxRows, const char* defName)
	:localeBackgroundPcxName(pcxName), WIDGET_WIDTH(rect.right - rect.left), WIDGET_HEIGHT(rect.bottom - rect.top), MAXIMUM_ROWS(maxRows), dlgCallButtonName(defName)
{

	auto pcx = H3LoadedPcx::Load(pcxName);

	localeBackgroundLoadedPcx = H3LoadedPcx::Create(h3_NullString, WIDGET_WIDTH, WIDGET_HEIGHT);
	pcx->DrawToPcx(rect.left, rect.top, rect.left + WIDGET_WIDTH, rect.top + WIDGET_HEIGHT, localeBackgroundLoadedPcx);

	pcx->Dereference();

}
//H3LoadedPcx* DlgStyle::Pcx() const noexcept
//{
//	return localeBackgroundLoadedPcx;
//}

bool DlgStyle::CreateAssets(bool forceRecreate)
{
	if (forceRecreate)
		styles.clear();

	if (styles.empty())
	{
		DlgStyle::Create("comopbck.pcx", { 245, 253,365,277 }, 15, "OVBUTN3.def");
		DlgStyle::Create("ADOPYPNL.PCX", { 4,1,102,19 }, 9, "OVBUTN3.def");

	}
	return !styles.empty();
}

DlgStyle::~DlgStyle()
{
}

DlgStyle* DlgStyle::Create(const char* pcxName, const RECT rect, const UINT16 maxRows, const char* defName)
{

	styles.emplace_back(DlgStyle{ pcxName, rect, maxRows,defName });

	return nullptr;
}
