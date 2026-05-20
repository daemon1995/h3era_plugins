#pragma once

#include "framework.h"

namespace localeFormat
{
constexpr LPCSTR name = "era.locale.list.%s.name";
constexpr LPCSTR codepage = "era.locale.list.%s.codepage";
constexpr LPCSTR alternative = "era.locale.name.%s";
} // namespace localeFormat
class LocaleManager
{
  public:
    enum CodePage : DWORD
    {
        NO_ANSI = 0,
        Thai = 874,
        Korean = 949,
        Japanese = 932,
        ChineseSimplified = 936,
        Vietnamese = 1258,
        ANSI = 1252,
        Arabic = 1256,
        Baltic = 1257,
        CentralEuropean = 1250,
        Cyrillic = 1251,
        Greek = 1253,
        Hebrew = 1255,
        Turkish = 1254,
    };
    struct format
    {
        static constexpr LPCSTR name = "era.locale.list.%s.name";
        static constexpr LPCSTR codepage = "era.locale.list.%s.codepage";
        static constexpr LPCSTR alternative = "era.locale.list.%s.alternative";

        struct error
        {
            static constexpr LPCSTR name = "era.locale.error.name";
            static constexpr LPCSTR alternative = "era.locale.error.alternative";
        };
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

    static constexpr unsigned int iso_639_1_codepages[] = {
        ANSI,              // aa (Afar) -> Latin 1
        Cyrillic,          // ab (Abkhazian) -> Cyrillic
        ANSI,              // ae (Avestan) -> Latin 1
        ANSI,              // af (Afrikaans) -> Latin 1
        ANSI,              // ak (Akan) -> Latin 1
        NO_ANSI,           // am (Amharic) -> No ANSI (Unicode only)
        ANSI,              // an (Aragonese) -> Latin 1
        Arabic,            // ar (Arabic) -> Arabic
        NO_ANSI,           // as (Assamese) -> No ANSI (Unicode only)
        Cyrillic,          // av (Avaric) -> Cyrillic
        ANSI,              // ay (Aymara) -> Latin 1
        Turkish,           // az (Azerbaijani) -> Turkish (or 1251 if Cyrillic)
        Cyrillic,          // ba (Bashkir) -> Cyrillic
        Cyrillic,          // be (Belarusian) -> Cyrillic
        Cyrillic,          // bg (Bulgarian) -> Cyrillic
        NO_ANSI,           // bh (Bihari languages) -> No ANSI (Unicode only)
        ANSI,              // bi (Bislama) -> Latin 1
        ANSI,              // bm (Bambara) -> Latin 1
        NO_ANSI,           // bn (Bengali) -> No ANSI (Unicode only)
        NO_ANSI,           // bo (Tibetan) -> No ANSI (Unicode only)
        ANSI,              // br (Breton) -> Latin 1
        CentralEuropean,   // bs (Bosnian) -> Latin 2 (Central European)
        ANSI,              // ca (Catalan) -> Latin 1
        Cyrillic,          // ce (Chechen) -> Cyrillic
        ANSI,              // ch (Chamorro) -> Latin 1
        ANSI,              // co (Corsican) -> Latin 1
        ANSI,              // cr (Cree) -> Latin 1
        CentralEuropean,   // cs (Czech) -> Latin 2 (Central European)
        Cyrillic,          // cu (Church Slavic) -> Cyrillic
        Cyrillic,          // cv (Chuvash) -> Cyrillic
        ANSI,              // cy (Welsh) -> Latin 1
        ANSI,              // da (Danish) -> Latin 1
        ANSI,              // de (German) -> Latin 1
        Arabic,            // dv (Dhivehi) -> Arabic
        NO_ANSI,           // dz (Dzongkha) -> No ANSI (Unicode only)
        ANSI,              // ee (Ewe) -> Latin 1
        Greek,             // el (Greek) -> Greek
        ANSI,              // en (English) -> Latin 1
        ANSI,              // eo (Esperanto) -> Latin 1 (with approximations)
        ANSI,              // es (Spanish) -> Latin 1
        Baltic,            // et (Estonian) -> Baltic
        ANSI,              // eu (Basque) -> Latin 1
        Arabic,            // fa (Persian) -> Arabic
        ANSI,              // ff (Fulah) -> Latin 1
        ANSI,              // fi (Finnish) -> Latin 1
        ANSI,              // fj (Fijian) -> Latin 1
        ANSI,              // fo (Faroese) -> Latin 1
        ANSI,              // fr (French) -> Latin 1
        ANSI,              // fy (Western Frisian) -> Latin 1
        ANSI,              // ga (Irish) -> Latin 1
        ANSI,              // gd (Scottish Gaelic) -> Latin 1
        ANSI,              // gl (Galician) -> Latin 1
        ANSI,              // gn (Guarani) -> Latin 1
        NO_ANSI,           // gu (Gujarati) -> No ANSI (Unicode only)
        ANSI,              // gv (Manx) -> Latin 1
        ANSI,              // ha (Hausa) -> Latin 1
        Hebrew,            // he (Hebrew) -> Hebrew
        NO_ANSI,           // hi (Hindi) -> No ANSI (Unicode only)
        ANSI,              // ho (Hiri Motu) -> Latin 1
        CentralEuropean,   // hr (Croatian) -> Latin 2 (Central European)
        ANSI,              // ht (Haitian) -> Latin 1
        CentralEuropean,   // hu (Hungarian) -> Latin 2 (Central European)
        NO_ANSI,           // hy (Armenian) -> No ANSI (Unicode only)
        ANSI,              // hz (Herero) -> Latin 1
        ANSI,              // ia (Interlingua) -> Latin 1
        ANSI,              // id (Indonesian) -> Latin 1
        ANSI,              // ie (Interlingue) -> Latin 1
        ANSI,              // ig (Igbo) -> Latin 1
        NO_ANSI,           // ii (Sichuan Yi) -> No ANSI (Unicode only)
        ANSI,              // ik (Inupiaq) -> Latin 1
        ANSI,              // io (Ido) -> Latin 1
        ANSI,              // is (Icelandic) -> Latin 1
        ANSI,              // it (Italian) -> Latin 1
        ANSI,              // iu (Inuktitut) -> Latin 1 (Latin script version)
        Japanese,          // ja (Japanese) -> Shift-JIS
        ANSI,              // jv (Javanese) -> Latin 1
        NO_ANSI,           // ka (Georgian) -> No ANSI (Unicode only)
        ANSI,              // kg (Kongo) -> Latin 1
        ANSI,              // ki (Kikuyu) -> Latin 1
        ANSI,              // kj (Kwanyama) -> Latin 1
        Cyrillic,          // kk (Kazakh) -> Cyrillic
        ANSI,              // kl (Kalaallisut) -> Latin 1
        NO_ANSI,           // km (Khmer) -> No ANSI (Unicode only)
        NO_ANSI,           // kn (Kannada) -> No ANSI (Unicode only)
        Korean,            // ko (Korean) -> Unified Hangul Code (EUC-KR)
        ANSI,              // kr (Kanuri) -> Latin 1
        NO_ANSI,           // ks (Kashmiri) -> No ANSI (Unicode only)
        Turkish,           // ku (Kurdish) -> Turkish (Latin script version)
        Cyrillic,          // kv (Komi) -> Cyrillic
        ANSI,              // kw (Cornish) -> Latin 1
        Cyrillic,          // ky (Kirghiz) -> Cyrillic
        ANSI,              // la (Latin) -> Latin 1
        ANSI,              // lb (Luxembourgish) -> Latin 1
        ANSI,              // lg (Ganda) -> Latin 1
        ANSI,              // li (Limburgish) -> Latin 1
        ANSI,              // ln (Lingala) -> Latin 1
        NO_ANSI,           // lo (Lao) -> No ANSI (Unicode only)
        Baltic,            // lt (Lithuanian) -> Baltic
        ANSI,              // lu (Luba-Katanga) -> Latin 1
        Baltic,            // lv (Latvian) -> Baltic
        ANSI,              // mg (Malagasy) -> Latin 1
        ANSI,              // mh (Marshallese) -> Latin 1
        ANSI,              // mi (Maori) -> Latin 1
        Cyrillic,          // mk (Macedonian) -> Cyrillic
        NO_ANSI,           // ml (Malayalam) -> No ANSI (Unicode only)
        Cyrillic,          // mn (Mongolian) -> Cyrillic
        NO_ANSI,           // mr (Marathi) -> No ANSI (Unicode only)
        ANSI,              // ms (Malay) -> Latin 1
        ANSI,              // mt (Maltese) -> Latin 1
        NO_ANSI,           // my (Burmese) -> No ANSI (Unicode only)
        ANSI,              // na (Nauru) -> Latin 1
        ANSI,              // nb (Norwegian Bokmål) -> Latin 1
        ANSI,              // nd (North Ndebele) -> Latin 1
        NO_ANSI,           // ne (Nepali) -> No ANSI (Unicode only)
        ANSI,              // ng (Ndonga) -> Latin 1
        ANSI,              // nl (Dutch) -> Latin 1
        ANSI,              // nn (Norwegian Nynorsk) -> Latin 1
        ANSI,              // no (Norwegian) -> Latin 1
        ANSI,              // nr (South Ndebele) -> Latin 1
        ANSI,              // nv (Navajo) -> Latin 1
        ANSI,              // ny (Chichewa) -> Latin 1
        ANSI,              // oc (Occitan) -> Latin 1
        ANSI,              // oj (Ojibwa) -> Latin 1
        ANSI,              // om (Oromo) -> Latin 1
        NO_ANSI,           // or (Oriya) -> No ANSI (Unicode only)
        Cyrillic,          // os (Ossetian) -> Cyrillic
        NO_ANSI,           // pa (Punjabi) -> No ANSI (Unicode only)
        ANSI,              // pi (Pali) -> Latin 1
        CentralEuropean,   // pl (Polish) -> Latin 2 (Central European)
        Arabic,            // ps (Pashto) -> Arabic
        ANSI,              // pt (Portuguese) -> Latin 1
        ANSI,              // qu (Quechua) -> Latin 1
        ANSI,              // rm (Romansh) -> Latin 1
        ANSI,              // rn (Rundi) -> Latin 1
        CentralEuropean,   // ro (Romanian) -> Latin 2 (Central European)
        Cyrillic,          // ru (Russian) -> Cyrillic
        ANSI,              // rw (Kinyarwanda) -> Latin 1
        NO_ANSI,           // sa (Sanskrit) -> No ANSI (Unicode only)
        ANSI,              // sc (Sardinian) -> Latin 1
        Arabic,            // sd (Sindhi) -> Arabic
        ANSI,              // se (Northern Sami) -> Latin 1
        ANSI,              // sg (Sango) -> Latin 1
        NO_ANSI,           // si (Sinhala) -> No ANSI (Unicode only)
        CentralEuropean,   // sk (Slovak) -> Latin 2 (Central European)
        CentralEuropean,   // sl (Slovenian) -> Latin 2 (Central European)
        ANSI,              // sm (Samoan) -> Latin 1
        ANSI,              // sn (Shona) -> Latin 1
        ANSI,              // so (Somali) -> Latin 1
        CentralEuropean,   // sq (Albanian) -> Latin 2 (Central European)
        Cyrillic,          // sr (Serbian) -> Cyrillic (или CentralEuropean, если используется латиница)
        ANSI,              // ss (Swati) -> Latin 1
        ANSI,              // st (Southern Sotho) -> Latin 1
        ANSI,              // su (Sundanese) -> Latin 1
        ANSI,              // sv (Swedish) -> Latin 1
        ANSI,              // sw (Swahili) -> Latin 1
        NO_ANSI,           // ta (Tamil) -> No ANSI (Unicode only)
        NO_ANSI,           // te (Telugu) -> No ANSI (Unicode only)
        Cyrillic,          // tg (Tajik) -> Cyrillic
        Thai,              // th (Thai) -> Thai
        ANSI,              // ti (Tigrinya) -> Latin 1
        Turkish,           // tk (Turkmen) -> Turkish
        ANSI,              // tl (Tagalog) -> Latin 1
        ANSI,              // tn (Tswana) -> Latin 1
        ANSI,              // to (Tonga) -> Latin 1
        Turkish,           // tr (Turkish) -> Turkish
        ANSI,              // ts (Tsonga) -> Latin 1
        Cyrillic,          // tt (Tatar) -> Cyrillic
        ANSI,              // tw (Twi) -> Latin 1
        ANSI,              // ty (Tahitian) -> Latin 1
        Arabic,            // ug (Uighur) -> Arabic
        Cyrillic,          // uk (Ukrainian) -> Cyrillic
        Arabic,            // ur (Urdu) -> Arabic
        Turkish,           // uz (Uzbek) -> Turkish (или 1251, если используется кириллица)
        ANSI,              // ve (Venda) -> Latin 1
        Vietnamese,        // vi (Vietnamese) -> Vietnamese
        ANSI,              // vo (Volapük) -> Latin 1
        ANSI,              // wa (Walloon) -> Latin 1
        ANSI,              // wo (Wolof) -> Latin 1
        ANSI,              // xh (Xhosa) -> Latin 1
        Hebrew,            // yi (Yiddish) -> Hebrew
        ANSI,              // yo (Yoruba) -> Latin 1
        ChineseSimplified, // za (Zhuang) -> Выделен под GBK (близко к Китаю)
        ChineseSimplified, // zh (Chinese) -> Simplified Chinese (GBK / GB2312)
        ANSI               // zu (Zulu) -> Latin 1
    };

    static constexpr LPCSTR INI_FILE_NAME = "heroes3.ini";
    static constexpr LPCSTR INI_LANGUAGE_KEY_NAME = "Language";
    static constexpr LPCSTR INI_CODEPAGE_KEY_NAME = "CodePage";
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
