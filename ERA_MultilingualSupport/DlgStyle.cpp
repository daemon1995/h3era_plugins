#include "pch.h"
#include "DlgStyle.h"

std::vector<DlgStyle> DlgStyle::styles;

DlgStyle::DlgStyle(const char* pcxName, const RECT rect, const UINT16 maxRows)
	: WIDGET_WIDTH(rect.right - rect.left), WIDGET_HEIGHT(rect.bottom - rect.top), MAXIMUM_ROWS(maxRows)
{

	auto pcx = H3LoadedPcx::Load(pcxName);

	localeBackgroundLoadedPcx = H3LoadedPcx::Create(h3_NullString, WIDGET_WIDTH, WIDGET_HEIGHT);
	pcx->DrawToPcx(rect.left, rect.top, rect.left + WIDGET_WIDTH, rect.top + WIDGET_HEIGHT, localeBackgroundLoadedPcx);

	pcx->Dereference();

}

bool DlgStyle::CreateAssets(bool forceRecreate)
{
	if (forceRecreate)
		styles.clear();

	if (styles.empty())
	{
		Create("comopbck.pcx", { 245, 253, 460, 277 }, 7);
		Create("ADOPYPNL.PCX", { 4, 1, 102, 19 }, 25);
		Create("ADVOPTBK.PCX", { 251, 549, 390, 568 }, 25);
		text.Load();
	}
	return !styles.empty();
}

DlgStyle::~DlgStyle()
{
}

DlgStyle* DlgStyle::Create(const char* pcxName, const RECT rect, const UINT16 maxRows)
{

	styles.emplace_back(DlgStyle{ pcxName, rect, maxRows });

	return nullptr;
}
DlgStyle::StyleText DlgStyle::text;

void DlgStyle::StyleText::Load()
{
	displayNameFormat = EraJS::read("era.locale.dlg.buttonName");

}

