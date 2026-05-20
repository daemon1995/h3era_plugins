#include "LocaleManager.h"

#include <unordered_set>

H3String LocaleManager::m_displayedName{};
static BOOL PathIsValid(const std::string &path)
{
    // Check if the path is not empty and does not contain invalid characters
    if (path.empty() || path.find_first_of("<>:\"/\\|?*") != std::string::npos)
        return false;
    // Check if the path exists
    return true; // GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

LocaleManager::LocaleManager() : m_current(nullptr), m_seleted(nullptr)
{

    locales.clear();

    //  Read list of available locales
    bool readSuccess = false;

    // create set of default locales

    constexpr size_t languagesCount = std::size(iso_639_1_languages);
    std::vector<std::string> iso_639_1_languagesVector(languagesCount);

    for (size_t i = 0; i < languagesCount; i++)
    {
        iso_639_1_languagesVector[i] = iso_639_1_languages[i];
    }

    std::unordered_set<std::string> iso_639_1_languagesSet(iso_639_1_languagesVector.begin(),
                                                           iso_639_1_languagesVector.end());

    // check if that locale has alternative name and replace it in set

    for (size_t i = 0; i < languagesCount; i++)
    {

        auto defaultLocaleName = iso_639_1_languages[i];

        LPCSTR alternativeName =
            EraJS::read(H3String::Format(format::alternative, defaultLocaleName).String(), readSuccess);

        if (readSuccess)
        {
            std::string alternativeNameStr(alternativeName);
            if (PathIsValid(alternativeNameStr))
            {
                if (iso_639_1_languagesSet.insert(alternativeName).second)
                {
                    iso_639_1_languagesSet.erase(defaultLocaleName);
                    iso_639_1_languagesVector[i] = alternativeName;
                }
            }
            else
            {
                libc::sprintf(h3_TextBuffer, EraJS::read(format::error::alternative), defaultLocaleName,
                              alternativeName);
                Era::ShowMessage(h3_TextBuffer);
            }
        }
    }

    // add default locales if they have defined names from json
    for (size_t i = 0; i < languagesCount; i++)
    {

        const auto &defaultLocaleName = iso_639_1_languagesVector[i];

        LPCSTR langName = EraJS::read(H3String::Format(format::name, defaultLocaleName.c_str()).String(), readSuccess);

        if (readSuccess && libc::strcmp(langName, h3_NullString))
        {
            int codepage =
                EraJS::readInt(H3String::Format(format::codepage, defaultLocaleName.c_str()).String(), readSuccess);
            if (!readSuccess)
            {
                codepage = iso_639_1_codepages[i];
            }

            locales.emplace_back(Locale(defaultLocaleName.c_str(), langName, codepage));
        }
    }

    // get current locale from ini and check if it is in vector, otherwise create new locale and add into vector

    const std::string currentGameLocale = ReadLocaleFromIni();
    if (!currentGameLocale.empty()) // if ther is ini entry
    {
        const char *localeName = currentGameLocale.c_str();
        auto currentLocalePtr = FindLocale(localeName);
        // check if added in vector
        if (currentLocalePtr != locales.end())
        {
            m_current = &*currentLocalePtr;
        }
        else
        {
            for (size_t i = 0; i < languagesCount; i++)
            {
                if (libc::strcmpi(localeName, iso_639_1_languagesVector[i].c_str()) == 0)
                {
                    localeName = iso_639_1_languagesVector[i].c_str();

                    int codepage = EraJS::readInt(H3String::Format(format::codepage, iso_639_1_languages[i]).String(),
                                                  readSuccess);
                    if (!readSuccess)
                    {
                        codepage = iso_639_1_codepages[i];
                    }

                    Locale current(localeName,
                                   EraJS::read(H3String::Format(format::name, localeName).String(), readSuccess),
                                   iso_639_1_codepages[i]);

                    break;
                }
            }
            // otherwise create new locale
            Locale current(localeName, EraJS::read(H3String::Format(format::name, localeName).String(), readSuccess),
                           ANSI);
            current.hasDescription = readSuccess && !current.displayedName.empty();
            // and add into vector
            locales.emplace_back(current);
            m_current = &locales.back();
        }
    }
}

const Locale &LocaleManager::LocaleAt(const int id) const noexcept
{
    return locales.at(id);
}
const Locale &LocaleManager::operator[](const int id) const noexcept
{
    return locales.at(id);
}

const std::vector<Locale>::const_iterator LocaleManager::FindLocale(const char *other) const noexcept
{

    auto compareResult = std::find_if(locales.begin(), locales.end(), [&](const Locale &locale) -> bool {
        return !libc::strcmpi(locale.name.c_str(), other);
    });

    return compareResult;
}

BOOL LocaleManager::SetForUser(const Locale *locale) const
{

    const auto localeToSet = locale->name.c_str();
    const DWORD codePage = locale->codePage;

    Era::SetLanguage(localeToSet);
    Era::SetCodePage(codePage);
    Era::ReloadLanguageData();

    Era::WriteStrToIni(INI_LANGUAGE_KEY_NAME, localeToSet, INI_SECTION_NAME, INI_FILE_NAME);
    Era::WriteStrToIni(INI_CODEPAGE_KEY_NAME, std::to_string(codePage).c_str(), INI_SECTION_NAME, INI_FILE_NAME);
    Era::SaveIni(INI_FILE_NAME);
    return 0; // Era::SetLanguage(locale->name);
}

std::string LocaleManager::ReadLocaleFromIni()
{
    std::string result;
    if (Era::era_str eraLocale = Era::GetLanguage())
    {
        result = eraLocale;
        Era::MemFree(eraLocale);
        return result;
    }

    char buff[16];
    libc::sprintf(buff, "%d", 0); // set default buffer
    Era::ReadStrFromIni(INI_LANGUAGE_KEY_NAME, INI_SECTION_NAME, INI_FILE_NAME, buff);
    result = buff;
    return result;
}

const Locale *LocaleManager::GetCurrent() const noexcept
{
    return m_current;
}

const Locale *LocaleManager::GetSelected() const noexcept
{
    return m_seleted;
}

void LocaleManager::SetSelected(const Locale *locale) noexcept
{
    m_seleted = locale;
}

LPCSTR LocaleManager::GetDisplayedName()
{
    char buff[256];
    libc::sprintf(buff, EraJS::read("era.locale.dlg.buttonName"), ReadLocaleFromIni().c_str());
    m_displayedName = buff;
    return m_displayedName.String();
}

UINT32 LocaleManager::GetCount() const noexcept
{
    return locales.size();
}

LocaleManager::~LocaleManager()
{
    locales.clear();
}
