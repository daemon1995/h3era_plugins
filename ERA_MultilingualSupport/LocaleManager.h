#pragma once

namespace localeFormat
{
constexpr LPCSTR name = "era.locale.list.%s.name";
constexpr LPCSTR alternative = "era.locale.name.%s";
} // namespace localeFormat
class LocaleManager
{
  public:
    struct format
    {
        static constexpr LPCSTR name = "era.locale.list.%s.name";
        static constexpr LPCSTR alternative = "era.locale.list.%s.alternative";
    };
    struct error
    {
        static constexpr LPCSTR name = "era.locale.error.name";
        static constexpr LPCSTR alternative = "era.locale.error.alternative";
    };

  private:
    static constexpr const char *iso_639_1_languages[] = {
        "aa", "ab", "ae", "af", "ak", "am", "an", "ar", "as", "av", "ay", "az", "ba", "be", "bg", "bh", "bi",
        "bm", "bn", "bo", "br", "bs", "ca", "ce", "ch", "co", "cr", "cs", "cu", "cv", "cy", "da", "de", "dv",
        "dz", "ee", "el", "en", "eo", "es", "et", "eu", "fa", "ff", "fi", "fj", "fo", "fr", "fy", "ga", "gd",
        "gl", "gn", "gu", "gv", "ha", "he", "hi", "ho", "hr", "ht", "hu", "hy", "hz", "ia", "id", "ie", "ig",
        "ii", "ik", "io", "is", "it", "iu", "ja", "jv", "ka", "kg", "ki", "kj", "kk", "kl", "km", "kn", "ko",
        "kr", "ks", "ku", "kv", "kw", "ky", "la", "lb", "lg", "li", "ln", "lo", "lt", "lu", "lv", "mg", "mh",
        "mi", "mk", "ml", "mn", "mr", "ms", "mt", "my", "na", "nb", "nd", "ne", "ng", "nl", "nn", "no", "nr",
        "nv", "ny", "oc", "oj", "om", "or", "os", "pa", "pi", "pl", "ps", "pt", "qu", "rm", "rn", "ro", "ru",
        "rw", "sa", "sc", "sd", "se", "sg", "si", "sk", "sl", "sm", "sn", "so", "sq", "sr", "ss", "st", "su",
        "sv", "sw", "ta", "te", "tg", "th", "ti", "tk", "tl", "tn", "to", "tr", "ts", "tt", "tw", "ty", "ug",
        "uk", "ur", "uz", "ve", "vi", "vo", "wa", "wo", "xh", "yi", "yo", "za", "zh", "zu"};

    static constexpr LPCSTR defaultLocaleNames = ",en,ru,cn,ua,pl,ko";

    static constexpr LPCSTR INI_FILE_NAME = "heroes3.ini";
    static constexpr LPCSTR INI_KEY_NAME = "Language";
    static constexpr LPCSTR INI_SECTION_NAME = "Era";

    static H3String m_displayedName;

  private:
    const Locale *m_current = nullptr; // Locale;
    const Locale *m_seleted = nullptr; // Locale;
    // Locale* m_default;
    std::vector<Locale> locales;

  public:
    LocaleManager();
    virtual ~LocaleManager();

  private:
    const std::vector<Locale>::const_iterator FindLocale(const char *other) const noexcept;

  public:
    LPCSTR LocaleFormat() const noexcept;

    const Locale &LocaleAt(const int id) const noexcept;
    const Locale &operator[](const int id) const noexcept;
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
