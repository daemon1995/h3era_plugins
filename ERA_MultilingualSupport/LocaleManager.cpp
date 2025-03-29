#include "pch.h"
#include <sstream>

H3String LocaleManager::m_displayedName{};

LocaleManager::LocaleManager() : m_current(nullptr), m_seleted(nullptr)
{

    m_locales.clear();

    m_locales.reserve(25);

    //  Read list of available locales
    bool readSuccess = false;

    std::string localeList = EraJS::read(JSON_KEY, readSuccess);
    if (readSuccess)
        localeList.append(defaultLocaleNames);
    else
        localeList = defaultLocaleNames;

    std::stringstream ss(localeList);

    int counter = 0; // {};

    while (ss.good() && counter++ < 512)
    {
        std::string substr;
        getline(ss, substr, ',');
        if (!substr.empty())
        {
            const char *const localeName = substr.c_str();

            if (FindLocale(localeName) == m_locales.cend())
            {

                sprintf(h3_TextBuffer, m_localeFormat, localeName);
                readSuccess = false;
                std::string localeNameStr = EraJS::read(h3_TextBuffer, readSuccess);
                localeNameStr.append(" - ").append(localeName);
                Locale *locale = new Locale(localeName, localeNameStr.c_str());
                locale->hasDescription = readSuccess && !locale->displayedName.empty();
                m_locales.emplace_back(locale);
            }
        }
    }
    // find current locale

    std::string str = ReadLocaleFromIni();
    if (!str.empty()) // if ther is ini entry
    {
        const char *localeName = str.c_str();
        auto currentLocalePtr = FindLocale(localeName);
        // check if added in vector
        if (currentLocalePtr != m_locales.end())
        {
            m_current = *currentLocalePtr;
        }
        else
        {
            // otherwise create new locale
            // sprintf(h3_TextBuffer, m_localeFormat, localeName);
            readSuccess = false;
            m_current =
                new Locale(localeName, EraJS::read(H3String::Format(m_localeFormat, localeName).String(), readSuccess));
            m_current->hasDescription = readSuccess && !m_current->displayedName.empty();
            // and add into vector
            m_locales.emplace_back(m_current);
        }
    }

    m_locales.shrink_to_fit();
}

LPCSTR LocaleManager::LocaleFormat() const noexcept
{
    return m_localeFormat;
}

const Locale *LocaleManager::LocaleAt(int id) const noexcept
{
    return m_locales.at(id);
}

const std::vector<Locale *>::const_iterator LocaleManager::FindLocale(const char *other) const noexcept
{

    auto compareResult = std::find_if(m_locales.begin(), m_locales.end(), [&](const Locale *locale) -> bool {
        return !_strcmpi(locale->name.c_str(), other);
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
    sprintf(h3_TextBuffer, "%d", 0); // set default buffer
    Era::ReadStrFromIni(INI_KEY_NAME, INI_SECTION_NAME, INI_FILE_NAME, h3_TextBuffer);
    return std::string(h3_TextBuffer);
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
    DlgStyle::text.Load();
    sprintf(h3_TextBuffer, DlgStyle::text.displayNameFormat, ReadLocaleFromIni().c_str());
    m_displayedName = h3_TextBuffer;
    return m_displayedName.String();
}

UINT32 LocaleManager::GetCount() const noexcept
{
    return m_locales.size();
}

LocaleManager::~LocaleManager()
{
    for (auto &locale : m_locales)
    {
        if (locale)
        {
            delete locale;
            locale = nullptr;
        }
    }
    m_locales.clear();
}
