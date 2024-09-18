#include "pch.h"
#include "DlgStyle.h"

std::vector<DlgStyle> DlgStyle::styles;

DlgStyle::DlgStyle(const char* pcxName, const RECT rect, const UINT16 maxRows, const LPCSTR f)
	: WIDGET_WIDTH(rect.right - rect.left), WIDGET_HEIGHT(rect.bottom - rect.top), MAXIMUM_ROWS(maxRows), fontName(f)
{

	auto pcx = H3LoadedPcx::Load(pcxName);

	localeBackgroundLoadedPcx = H3LoadedPcx::Create(h3_NullString, WIDGET_WIDTH, WIDGET_HEIGHT);
	pcx->DrawToPcx(rect.left, rect.top, rect.left + WIDGET_WIDTH, rect.top + WIDGET_HEIGHT, localeBackgroundLoadedPcx);

	pcx->Dereference();
	P_CombatManager->RefreshCreatures();
}

bool DlgStyle::CreateAssets(bool forceRecreate)
{
	if (forceRecreate)
		styles.clear();

	if (styles.empty())
	{
		styles.emplace_back(DlgStyle("comopbck.pcx", { 245, 253, 460, 276 }, 7, h3::NH3Dlg::Text::MEDIUM));
		styles.emplace_back(DlgStyle("ADOPYPNL.PCX", { 4, 1, 102, 19 }, 25, h3::NH3Dlg::Text::MEDIUM));
		styles.emplace_back(DlgStyle("ADVOPTBK.PCX", { 251, 549, 390, 569 }, 25, h3::NH3Dlg::Text::SMALL));
		text.Load();
	}
	return !styles.empty();
}

DlgStyle::~DlgStyle()
{
}


DlgStyle::StyleText DlgStyle::text;

void DlgStyle::StyleText::Load()
{
	displayNameFormat = EraJS::read("era.locale.dlg.buttonName");

}

