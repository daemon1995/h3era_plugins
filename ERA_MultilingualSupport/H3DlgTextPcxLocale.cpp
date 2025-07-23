#include "pch.h"

void H3DlgPcx16Locale::SetLocale(const Locale *locale)
{
    m_locale = locale;
    LPCSTR localName = m_locale ? m_locale->displayedName.c_str() : h3_NullString;
    //    SetText(localName);

    if (auto pcx = loadedPcx16)
    {
        const UINT width = pcx->width;
        const UINT height = pcx->height;

        pcx->BackgroundRegion(0, 0, width, height, isBlueBack);
        pcx->DarkenArea(0, 0, width, height, 50);
        pcx->BevelArea(1, 1, width - 2, height - 2);
        if (font)
        {
            font->TextDraw(pcx, localName, 0, 0, width, height);
        }
    }
}

const Locale *H3DlgPcx16Locale::GetLocale() const noexcept
{
    return m_locale;
}

H3DlgPcx16Locale *H3DlgPcx16Locale::Create(const INT32 x, const INT32 y, const DlgStyle &style, const Locale *locale,
                                           H3Font *font,  INT32 id, eTextAlignment align)
{
    H3DlgPcx16Locale *t = H3ObjectAllocator<H3DlgPcx16Locale>().allocate(1);
    if (t)
    {
        THISCALL_8(H3DlgPcx16Locale *, 0x450340, t, x, y, style.width, style.height, id, nullptr, 0x800);
        H3LoadedPcx16 *pcx = H3LoadedPcx16::Create(style.width, style.height);
        t->isBlueBack = style.isBlueBack;
        t->SetPcx(pcx);
        t->font = font;
        t->align = align;
        t->SetLocale(locale);
    }
    return t;
}
