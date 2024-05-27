#include "pch.h"
#include "LocaleHander.h"
#include <sstream>

constexpr LPCSTR defaultLocaleNames = ",ru,en,cn,ua,pl,kr";
H3String LocaleHandler::m_displayedName{};

LocaleHandler::LocaleHandler()
	:m_current(nullptr), m_seleted(nullptr)//,m_default(nullptr)
{

	m_locales.clear();

	m_locales.reserve(25);

	//  Read list of available locales
	bool readSuccess = false;

	std::string localeList = EraJS::read("era.locale.list", readSuccess);
	if (readSuccess)
		localeList.append(defaultLocaleNames);
	else
		localeList = defaultLocaleNames;

	std::stringstream ss(localeList);

	int counter = 0;// {};

	while (ss.good() && counter++ < 512)
	{
		std::string substr;
		getline(ss, substr, ',');
		if (!substr.empty())
		{
			const char* const localeName = substr.c_str();

			if (FindLocale(localeName) == m_locales.cend())
			{

				sprintf(h3_TextBuffer, m_localeFormat, localeName);
				readSuccess = false;
				Locale* locale = new Locale(localeName, EraJS::read(h3_TextBuffer, readSuccess));
				locale->hasDescription = readSuccess && !locale->displayedName.empty();
				m_locales.emplace_back(locale);

			}

		}
	}
	// find current locale

	std::string str = ReadLocaleFromIni("heroes3.ini");
	if (!str.empty()) // if ther is ini entry
	{
		const char* localeName = str.c_str();
		auto currentLocalePtr = FindLocale(localeName);
		// check if added in vector
		if (currentLocalePtr != m_locales.end())
		{
			m_current = *currentLocalePtr;

		}
		else
		{
			// otherwise create new locale
			//sprintf(h3_TextBuffer, m_localeFormat, localeName);
			readSuccess = false;
			m_current = new Locale(localeName, EraJS::read(H3String::Format(m_localeFormat, localeName).String(), readSuccess));
			m_current->hasDescription = readSuccess && !m_current->displayedName.empty();
			// and add into vector
			m_locales.emplace_back(m_current);
		}

	}

	m_locales.shrink_to_fit();

}

const char* const LocaleHandler::LocaleFormat() const noexcept
{
	return m_localeFormat;
}

const Locale* LocaleHandler::LocaleAt(int id) const noexcept
{
	return m_locales.at(id);
}



const std::vector<Locale*>::const_iterator LocaleHandler::FindLocale(const char* other) const noexcept
{

	auto compareResult = std::find_if(m_locales.begin(), m_locales.end(), [&](const Locale* locale)->bool
		{
			return !_strcmpi(locale->name.c_str(), other);
		});

	return compareResult;
}

BOOL LocaleHandler::SetForUser(const Locale* locale) const
{
	Era::SetLanguage(locale->name.c_str());
	Era::ReloadLanguageData();

	Era::WriteStrToIni("Language", locale->name.c_str(), "Era", "heroes3.ini");
	Era::SaveIni("heroes3.ini");
	return 0;// Era::SetLanguage(locale->name);
}

std::string LocaleHandler::ReadLocaleFromIni(const char* iniPath)
{
	sprintf(h3_TextBuffer, "%d", 0); // set default buffer
	Era::ReadStrFromIni("Language", "Era", iniPath, h3_TextBuffer);
	return std::string(h3_TextBuffer);
}



const Locale* LocaleHandler::GetCurrent() const noexcept { return m_current; }

const Locale* LocaleHandler::GetSelected() const noexcept { return m_seleted; }

void LocaleHandler::SetSelected(const Locale* locale) noexcept { m_seleted = locale; }

const char* LocaleHandler::GetDisplayedName()
{
	DlgStyle::text.Load();
	sprintf(h3_TextBuffer, DlgStyle::text.displayNameFormat, ReadLocaleFromIni("heroes3.ini").c_str());
	m_displayedName = h3_TextBuffer;
	return m_displayedName.String();
}

const UINT32 LocaleHandler::GetCount() const noexcept { return m_locales.size(); }

LocaleHandler::~LocaleHandler()
{
	m_locales.clear();

}