#include "pch.h"
#include "LocaleHander.h"
#include <sstream>
constexpr const char* ini = "";

constexpr LPCSTR defaultLocaleNames[4] = { "ru","en","cn","ua" };//, "kr", ""}
LocaleHandler::LocaleHandler()
	:m_current(nullptr), m_seleted(nullptr)//,m_default(nullptr)
{



	m_locales.clear();
	// 1. Read if default locale exists
	//;
	//m_locales.comp = &LocaleComparator;
	m_locales.reserve(25);
	//std::unordered_map<int, Locale*> tempSet;
	int counter = 0;// {};

	std::string str = ReadLocaleFromIni("default heroes3.ini");
	if (!str.empty())
	{
		const char* format = EraJS::read("era.locale.dlg.default");
		//H3String format = jsonKey
		const char* localeName = str.c_str();
		sprintf(h3_TextBuffer, format, localeName);
		//m_default = new Locale(localeName, h3_TextBuffer);
	//	tempSet.insert({ counter++,m_default });

	//	m_locales.emplace_back(m_default);
	}



	// 3. Read list of available locales

	std::stringstream ss(EraJS::read("era.locale.list"));
	while (ss.good() && counter++ < 512)
	{
		std::string substr;
		getline(ss, substr, ',');
		if (!substr.empty())
		{
			//const char* localeName = substr.c_str();
			const char* const localeName = substr.c_str();
			if (!LocaleExists(localeName))
			{

				sprintf(h3_TextBuffer, "era.locale.dlg.%s", localeName);
				Locale* locale = new Locale(localeName, EraJS::read(h3_TextBuffer));
				//tempSet.insert({ counter++,locale });
				m_locales.emplace_back(locale);

			}

		}
	}
	str = ReadLocaleFromIni("heroes3.ini");
	if (!str.empty())
	{
		sprintf(h3_TextBuffer, "era.locale.dlg.%s", str.c_str());

		m_current = new Locale(str.c_str(), EraJS::read(h3_TextBuffer));
	}

	m_locales.shrink_to_fit();

}

const Locale* LocaleHandler::LocaleAt(int id) const noexcept
{
	return m_locales.at(id);
}



const BOOL LocaleHandler::LocaleExists(const char* other) const noexcept
{

	bool compareResult = std::find_if(m_locales.begin(), m_locales.end(), [&](const Locale* locale)->bool
		{
			return !_strcmpi(locale->name.c_str(), other);
		}) != m_locales.end();

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

char* LocaleHandler::ReadLocaleFromIni(const char* iniPath)
{

	sprintf(h3_TextBuffer, "%d", 0); // set default buffer
	if (Era::ReadStrFromIni("Language", "Era", iniPath, h3_TextBuffer))
		return h3_TextBuffer;
	return 0;
}

const Locale* LocaleHandler::CurrentLocale() const noexcept
{
	return m_current;
}

const Locale* LocaleHandler::SelectedLocale() const noexcept
{
	return m_seleted;
}

void LocaleHandler::SetSelected(const Locale* locale) noexcept
{
	m_seleted = locale;
}
//const Locale* LocaleHandler::DefaultLocale() const noexcept
//{
//	return m_default;
//}

void LocaleHandler::GetCurrentLocale()
{

}
const UINT32 LocaleHandler::GetCount() const noexcept
{
	return m_locales.size();
}
LocaleHandler& LocaleHandler::Get() noexcept
{
	static LocaleHandler instance;
	return instance;
	// TODO: insert return statement here
}
LocaleHandler::~LocaleHandler()
{

	if (m_current)
	{
		delete m_current;
		m_current = nullptr;
	}
	if (m_seleted)
	{
		delete m_seleted;
		m_seleted = nullptr;
	}


}