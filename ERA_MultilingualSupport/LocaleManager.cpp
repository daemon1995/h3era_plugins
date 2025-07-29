#include "pch.h"
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

    std::unordered_set<std::string> iso_639_1_languagesSet;
    // create set of default locales
    for (const auto &defaultLocaleName : iso_639_1_languages)
    {
        iso_639_1_languagesSet.insert(defaultLocaleName);
    }

    // check if that locale has alternative name and replace it in set
    for (const auto &defaultLocaleName : iso_639_1_languages)
    {
        LPCSTR alternativeName =
            EraJS::read(H3String::Format(format::alternative, defaultLocaleName).String(), readSuccess);

        if (readSuccess)
        {
            std::string alternativeNameStr(alternativeName);
            if (PathIsValid(alternativeNameStr))
            {
                iso_639_1_languagesSet.erase(defaultLocaleName);
                iso_639_1_languagesSet.insert(alternativeName);
            }
            else
            {
                libc::sprintf(h3_TextBuffer, EraJS::read(error::alternative), defaultLocaleName, alternativeName);
                Era::ShowMessage(h3_TextBuffer);
            }
        }
    }

    // add default locales if they have defined names from json
    for (auto &i : iso_639_1_languagesSet)
    {
        LPCSTR langName = EraJS::read(H3String::Format(format::name, i.c_str()).String(), readSuccess);

        if (readSuccess && libc::strcmp(langName, h3_NullString))
        {
            locales.emplace_back(Locale(i.c_str(), langName));
        }
    }

    const std::string str = ReadLocaleFromIni();
    if (!str.empty()) // if ther is ini entry
    {
        const char *localeName = str.c_str();
        auto currentLocalePtr = FindLocale(localeName);
        // check if added in vector
        if (currentLocalePtr != locales.end())
        {
            m_current = &*currentLocalePtr;
        }
        else
        {
            // otherwise create new locale
            Locale current(localeName, EraJS::read(H3String::Format(format::name, localeName).String(), readSuccess));
            current.hasDescription = readSuccess && !current.displayedName.empty();
            // and add into vector
            locales.emplace_back(current);
            m_current = &locales.back();
        }
    }
}

LPCSTR LocaleManager::LocaleFormat() const noexcept
{
    return format::name;
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
    Era::SetLanguage(locale->name.c_str());
    Era::ReloadLanguageData();

    Era::WriteStrToIni(INI_KEY_NAME, locale->name.c_str(), INI_SECTION_NAME, INI_FILE_NAME);
    Era::SaveIni(INI_FILE_NAME);
    return 0; // Era::SetLanguage(locale->name);
}

std::string LocaleManager::ReadLocaleFromIni()
{
    char buff[16];
    libc::sprintf(buff, "%d", 0); // set default buffer
    Era::ReadStrFromIni(INI_KEY_NAME, INI_SECTION_NAME, INI_FILE_NAME, buff);
    return std::string(buff);
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
