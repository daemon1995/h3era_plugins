#include "pch.h"

H3LoadedPcx16 *H3DlgPcx16Locale::backgroundPcx = nullptr;
void H3DlgPcx16Locale::SetLocale(const Locale *locale)
{
    m_locale = locale;
    LPCSTR localName = m_locale ? m_locale->displayedName.c_str() : h3_NullString;
    //    SetText(localName);

    if (auto pcx = loadedPcx16)
    {
        const UINT width = pcx->width;
        const UINT height = pcx->height;

        loadedPcx16->CopyRegion(backgroundPcx, 0, 0);
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
                                           H3Font *font, INT32 id, eTextAlignment align)
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

void H3DlgPcx16Locale::CreateBackgroundPcx(const DlgStyle &style)
{

    if (!backgroundPcx)
    {
        backgroundPcx = H3LoadedPcx16::Create(style.width, style.height);
        if (backgroundPcx)
        {
            backgroundPcx->BackgroundRegion(0, 0, style.width, style.height, style.isBlueBack);
            backgroundPcx->DarkenArea(0, 0, style.width, style.height, 50);
            backgroundPcx->BevelArea(1, 1, style.width - 2, style.height - 2);
        }
    }
}

void H3DlgPcx16Locale::DeleteBackgroundPcx() noexcept
{
    if (backgroundPcx)
    {
        backgroundPcx->Destroy();
        backgroundPcx = nullptr;
    }
}
