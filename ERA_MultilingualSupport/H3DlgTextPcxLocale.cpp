#include "pch.h"
#include "H3DlgTextPcxLocale.h"


void H3DlgTextPcxLocale::SetPcx(H3LoadedPcx* pcx)
{
	loadedPcx = pcx;
}
void H3DlgTextPcxLocale::SetLocale(const Locale* locale)
{
	m_locale = locale;
	SetText(m_locale->displayedName.c_str());
}

const Locale* H3DlgTextPcxLocale::GetLocale() const noexcept
{
	return m_locale;
}

H3DlgTextPcxLocale* H3DlgTextPcxLocale::Create(INT32 x, INT32 y, INT32 width, INT32 height,
	const Locale* locale, LPCSTR fontName, H3LoadedPcx* pcx, INT32 color, INT32 id, INT32 align)
{
	H3DlgTextPcxLocale* t = H3ObjectAllocator<H3DlgTextPcxLocale>().allocate(1);
	if (t)
	{
		// set nop for loading pcx to skip default proc

		auto* blockLoadingPcx = _PI->WriteHexPatch(0x5BCBD0, "90 90 90 90 90");

		THISCALL_12(H3DlgTextPcxLocale*, 0x5BCB70, t, x, y, width, height, 0, fontName,
			nullptr, color, id, align, 8);
		//restore orginal code;
		blockLoadingPcx->Undo();
		blockLoadingPcx->Destroy();
		if (locale)
			t->SetLocale(locale);


		t->loadedPcx = pcx;
	}
	return t;
}
