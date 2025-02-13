#pragma once
using namespace std;
class LocaleManager
{
  private:
    static constexpr LPCSTR m_localeFormat = "era.locale.name.%s";
    static constexpr LPCSTR defaultLocaleNames = ",en,ru,cn,ua,pl,kr";
    static constexpr LPCSTR INI_FILE_NAME = "heroes3.ini";
    static constexpr LPCSTR INI_KEY_NAME = "Language";
    static constexpr LPCSTR INI_SECTION_NAME = "Era";
    static constexpr LPCSTR JSON_KEY = "era.locale.list";

    static H3String m_displayedName;

  private:
    Locale *m_current;       // Locale;
    const Locale *m_seleted; // Locale;
    // Locale* m_default;
    std::vector<Locale *> m_locales;

  public:
    LocaleManager();
    virtual ~LocaleManager();

  private:
    const std::vector<Locale *>::const_iterator FindLocale(const char *other) const noexcept;

  public:
    LPCSTR LocaleFormat() const noexcept;

    const Locale *LocaleAt(int id) const noexcept;

    UINT32 GetCount() const noexcept;
    BOOL SetForUser(const Locale *locale) const;
    void SetSelected(const Locale *locale) noexcept;
    const Locale *GetCurrent() const noexcept;
    const Locale *GetSelected() const noexcept;

  public:
    static LPCSTR GetDisplayedName();

    static std::string ReadLocaleFromIni();
    // const Locale* DefaultLocale() const noexcept;

    // String Get
};
