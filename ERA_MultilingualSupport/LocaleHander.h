#pragma once
using namespace std;
class LocaleHandler
{
private:

	Locale* m_current;// Locale;
	const Locale* m_seleted; //Locale;
	//Locale* m_default;

	std::vector<Locale*> m_locales;

private:
//	void GetDefaultLocale();
	void GetCurrentLocale();
	char* ReadLocaleFromIni(const char* iniPath);


public:

	const BOOL LocaleExists(const char* other) const noexcept;
	const Locale* LocaleAt(int id) const noexcept;

	LocaleHandler();
	const UINT32 GetCount() const noexcept;
	BOOL ChangeLocale(const Locale* locale) const;
	void SetSelected(const Locale* locale) noexcept;
	const Locale* CurrentLocale() const noexcept;
	const Locale* SelectedLocale() const noexcept;
	//const Locale* DefaultLocale() const noexcept;
	virtual ~LocaleHandler();


	//String Get
};

