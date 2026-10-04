#include "LocaleManager.h"

#include <unordered_set>

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
            EraJS::read(EraJS::FormatText(format::alternative, {defaultLocaleName}), readSuccess);

        if (readSuccess)
        {
            std::string alternativeNameStr(alternativeName);
            if (EraJS::LanguageNameIsValid(alternativeNameStr))
            {
                if (iso_639_1_languagesSet.insert(alternativeName).second)
                {
                    iso_639_1_languagesSet.erase(defaultLocaleName);
                    iso_639_1_languagesVector[i] = alternativeName;
                }
            }
            else
            {
                const auto message = EraJS::FormatText(EraJS::read(format::error::alternative),
                                                       {defaultLocaleName, alternativeName});
                Era::ShowMessage(message.c_str());
            }
        }
    }

    // add default locales if they have defined names from json
    for (size_t i = 0; i < languagesCount; i++)
    {

        const auto &defaultLocaleName = iso_639_1_languagesVector[i];

        LPCSTR langName = EraJS::read(EraJS::FormatText(format::name, {defaultLocaleName}), readSuccess);

        if (readSuccess && langName && *langName)
        {
            int codepage =
                EraJS::readInt(EraJS::FormatText(format::codepage, {defaultLocaleName}), readSuccess);
            if (!readSuccess || !IsValidCodePage(codepage))
                codepage = iso_639_1_codepages[i] ? iso_639_1_codepages[i] : Era::GetCodePage();

            locales.emplace_back(Locale(defaultLocaleName.c_str(), langName, codepage));
        }
    }

    // Find the active ERA language, or append it when its JSON description is absent.

    const std::string currentGameLocale = ReadCurrentLanguage();
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
            DWORD codepage = Era::GetCodePage();
            for (size_t i = 0; i < languagesCount; ++i)
            {
                if (_stricmp(localeName, iso_639_1_languagesVector[i].c_str()) == 0)
                {
                    localeName = iso_639_1_languagesVector[i].c_str();
                    const int configured = EraJS::readInt(EraJS::FormatText(format::codepage, {localeName}), readSuccess);
                    if (readSuccess && IsValidCodePage(configured)) codepage = configured;
                    else if (iso_639_1_codepages[i]) codepage = iso_639_1_codepages[i];
                    break;
                }
            }
            if (!IsValidCodePage(codepage)) codepage = ANSI;
            LPCSTR displayName = EraJS::read(EraJS::FormatText(format::name, {localeName}), readSuccess);
            Locale current(localeName, readSuccess ? displayName : localeName, codepage);
            current.hasDescription = readSuccess && !current.displayedName.empty();
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
        return !_stricmp(locale.name.c_str(), other);
    });

    return compareResult;
}

BOOL LocaleManager::SetForUser(const Locale *locale)
{
    if (!locale || !EraJS::LanguageNameIsValid(locale->name) || !IsValidCodePage(locale->codePage)) return FALSE;
    const std::string previousLanguage = ReadCurrentLanguage();
    const DWORD previousCodepage = Era::GetCodePage();
    if (!Era::SetLanguage(locale->name.c_str())) return FALSE;
    if (!Era::SetCodePage(locale->codePage))
    {
        Era::SetLanguage(previousLanguage.c_str());
        return FALSE;
    }
    Era::ReloadLanguageData();

    const BOOL saved =
        Era::WriteStrToIni(INI_LANGUAGE_KEY_NAME, locale->name.c_str(), INI_SECTION_NAME, INI_FILE_NAME) &&
        Era::WriteStrToIni(INI_CODEPAGE_KEY_NAME, std::to_string(locale->codePage).c_str(), INI_SECTION_NAME, INI_FILE_NAME) &&
        Era::SaveIni(INI_FILE_NAME);
    if (!saved)
    {
        Era::SetLanguage(previousLanguage.c_str());
        Era::SetCodePage(previousCodepage);
        Era::ReloadLanguageData();
        // Restore the pending INI values too, so a later SaveIni cannot commit a failed selection.
        Era::WriteStrToIni(INI_LANGUAGE_KEY_NAME, previousLanguage.c_str(), INI_SECTION_NAME, INI_FILE_NAME);
        Era::WriteStrToIni(INI_CODEPAGE_KEY_NAME, std::to_string(previousCodepage).c_str(), INI_SECTION_NAME, INI_FILE_NAME);
        Era::SaveIni(INI_FILE_NAME);
    }
    return saved;
}

std::string LocaleManager::ReadCurrentLanguage()
{
    if (Era::era_str language = Era::GetLanguage())
    {
        std::string result(language);
        Era::MemFree(language);
        return result;
    }
    char path[MAX_PATH], buffer[21] = {};
    const DWORD length = GetFullPathNameA(INI_FILE_NAME, sizeof(path), path, nullptr);
    if (length && length < sizeof(path))
        GetPrivateProfileStringA(INI_SECTION_NAME, INI_LANGUAGE_KEY_NAME, "", buffer, sizeof(buffer), path);
    return buffer;
}

std::string LocaleManager::GetButtonText()
{
    return EraJS::FormatText(EraJS::read("era.locale.dlg.buttonName"), {ReadCurrentLanguage()});
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

UINT32 LocaleManager::GetCount() const noexcept
{
    return locales.size();
}

LocaleManager::~LocaleManager()
{
    locales.clear();
}
