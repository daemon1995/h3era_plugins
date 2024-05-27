#pragma once
using namespace std;
class LocaleHandler
{
private:


	Locale* m_current;// Locale;
	const Locale* m_seleted; //Locale;
	//Locale* m_default;
	const char* m_localeFormat = "era.locale.dlg.%s";
	static H3String m_displayedName;
	std::vector<Locale*> m_locales;

private:

	const std::vector<Locale*>::const_iterator FindLocale(const char* other) const noexcept;

public:

	LocaleHandler();

	static std::string ReadLocaleFromIni(const char* iniPath);
	static const char* GetDisplayedName();
	const char* const LocaleFormat() const noexcept;

	const Locale* LocaleAt(int id) const noexcept;

	const UINT32 GetCount() const noexcept;
	BOOL SetForUser(const Locale* locale) const;
	void SetSelected(const Locale* locale) noexcept;
	const Locale* GetCurrent() const noexcept;
	const Locale* GetSelected() const noexcept;
	//const Locale* DefaultLocale() const noexcept;
	virtual ~LocaleHandler();


	//String Get
};

