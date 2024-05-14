#pragma once
class H3DlgTextPcxLocale :
    public H3DlgTextPcx
{
    const Locale* m_locale;

public:


   static H3DlgTextPcxLocale* Create(INT32 x, INT32 y, INT32 width, INT32 height,
       const Locale* locale, LPCSTR fontName, H3LoadedPcx* pcx, INT32 color, INT32 id = 0,
       INT32 align = eTextAlignment::MIDDLE_CENTER);

   void SetPcx(H3LoadedPcx* pcx);
   void SetLocale(const Locale* locale);
   const Locale* GetLocale() const noexcept;
};

