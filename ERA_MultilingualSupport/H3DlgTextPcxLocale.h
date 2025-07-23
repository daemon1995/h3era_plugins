#pragma once
class DlgStyle;
class Locale;
class H3DlgPcx16Locale : public H3DlgPcx16
{

    const Locale *m_locale = nullptr;
    BOOL isBlueBack = false;
    H3String text;
    H3Font *font = nullptr;
    eTextAlignment align = eTextAlignment::MIDDLE_CENTER;

  public:
    void SetLocale(const Locale *locale);
    const Locale *GetLocale() const noexcept;
    const H3String &GetText() const noexcept;

  public:
    static H3DlgPcx16Locale *Create(const INT32 x, const INT32 y, const DlgStyle &style, const Locale *locale,
                                    H3Font *font, INT32 id, eTextAlignment align = eTextAlignment::MIDDLE_CENTER);
};
